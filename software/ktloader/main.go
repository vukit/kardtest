package main

import (
	"bufio"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"math"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/snksoft/crc"
	"go.bug.st/serial"
)

const (
	flashBinFile        = "flash.bin"
	metaFile            = "meta.json"
	flashMaxPages       = 8192
	flashPageSize       = 256
	taskInfoSize        = 38
	activeChannelsCount = 9
	uartBaudRate        = 57600
	ackValue            = 0xAA
	startSATPage        = 8
	lutSize             = 1024
	halfLutSize         = lutSize / 2
	dacHalfMAX          = 2047.5
)

type Meta struct {
	Title string `json:"title"`
	File  string `json:"file"`
}

func main() {
	log.SetFlags(log.LstdFlags | log.Lshortfile)

	start := time.Now()

	portName := os.Args[1] // /dev/ttyUSB0
	dataPath := os.Args[2] // ./data

	fileName := dataPath + "/" + flashBinFile

	err := createFile(dataPath)
	if err != nil {
		log.Printf("%s", err)
		return
	}

	defer os.Remove(fileName)

	fileInfo, err := os.Stat(fileName)
	if err != nil {
		log.Printf("%s", err)
		return
	}
	fileSize := fileInfo.Size()

	flashPages := fileSize / flashPageSize
	if flashPages > flashMaxPages {
		log.Printf("File too big: max %d bytes", flashMaxPages*flashPageSize)
		return
	}

	bytesOnLastPage := uint16(fileSize - flashPages*flashPageSize)
	if bytesOnLastPage > 0 {
		flashPages++
	} else {
		bytesOnLastPage = flashPageSize
	}

	mode := &serial.Mode{
		BaudRate: uartBaudRate,
		InitialStatusBits: &serial.ModemOutputBits{
			RTS: true,
			DTR: false,
		},
	}

	port, err := serial.Open(portName, mode)
	if err != nil {
		log.Printf("Failed to open tty port %s: %s", portName, err)
		return
	}
	defer port.Close()

	// Посылаем количество страниц
	_, err = port.Write([]byte{byte((flashPages & 0xff00) >> 8), byte(flashPages & 0x00ff)})
	if err != nil {
		log.Printf("Error write into tty port %s: %s", portName, err)
		return
	}

	// Посылаем количество байт на последней странице
	_, err = port.Write([]byte{byte((bytesOnLastPage & 0xff00) >> 8), byte(bytesOnLastPage & 0x00ff)})
	if err != nil {
		log.Printf("Error write into tty port %s: %s", portName, err)
		return
	}

	fd, err := os.Open(fileName)
	if err != nil {
		log.Fatal(err)
	}
	defer fd.Close()

	br := bufio.NewReader(fd)

	hash := crc.NewHash(crc.XMODEM)

	var crcValue uint16

	allBytes := 0
	bytesPerPage := 0
	page := int64(1)
	for {

		b, err := br.ReadByte()

		if err != nil && !errors.Is(err, io.EOF) {
			fmt.Println(err)
			break
		}

		if err == nil {
			// Посылаем байт данных
			n, err := port.Write([]byte{b})
			if err != nil {
				log.Printf("Error write into tty port %s: %s", portName, err)
				return
			}
			hash.Update([]byte{b})
			allBytes += n
			fmt.Print(".")

			bytesPerPage++
			if uint16(bytesPerPage) == bytesOnLastPage && page == flashPages {
				if !waitACK(port) {
					break
				}
			}
			if bytesPerPage == flashPageSize && page < flashPages {
				if !waitACK(port) {
					break
				}
				page++
				bytesPerPage = 0
			}

		} else {
			// Посылаем CRC
			crcValue = uint16(hash.CRC())
			_, err = port.Write([]byte{byte((crcValue & 0xff00) >> 8), byte(crcValue & 0x00ff)})
			if err != nil {
				log.Printf("Error write into tty port %s: %s", portName, err)
				return
			}
			break
		}
	}

	log.Printf("\nSend %d bytes, crc is %d, time %v\n", allBytes, crcValue, time.Since(start))
}

// waitACK возвращает успех если кардтест подтвердил приём данных
func waitACK(port serial.Port) bool {

	port.SetReadTimeout(5 * time.Second)

	buff := make([]byte, 1)

	for {
		n, err := port.Read(buff)
		if err != nil {
			log.Printf("%s", err)
			return false
		}
		if n == 0 {
			fmt.Print("TO")
			return false
		}
		if buff[0] == ackValue {
			fmt.Print("ACK")
			return true
		}
	}
}

// createFile формирует файл с данными для записи во flash кардтеста
func createFile(dataPath string) error {
	fd, err := os.Create(dataPath + "/" + flashBinFile)
	if err != nil {
		return err
	}

	defer fd.Close()

	err = writeSineLUT(fd)
	if err != nil {
		return err
	}

	err = writeTaskInfo(fd, dataPath)
	if err != nil {
		return err
	}

	err = writeSignals(fd, dataPath)
	if err != nil {
		return err
	}

	return nil
}

// writeSineLUT записывает таблицу значений синуса для DDS.
func writeSineLUT(fd *os.File) error {
	for i := range lutSize {
		value := uint16(math.Round(dacHalfMAX + dacHalfMAX*math.Sin(2.0*math.Pi*float64(i)/float64(lutSize))))

		_, err := fd.Write([]byte{byte(value >> 8), byte(value)})
		if err != nil {
			return err
		}
	}

	return nil
}

/*
1 байт - количество задач N
38*N байт - размер описателей задач
Описатель сигнала:
32 байта - название сигнала
2 байта - начальная страница данных сигналов
1 байт - начальный байт данных сигналов
2 байта - конечная страница данных сигналов
1 байт - конечный байт данных сигналов
Итого 32 + 2 + 1 + 2 + 1 = 38 байта
*/

// writeTaskInfo записывает мета-информацию о задачах
func writeTaskInfo(fd *os.File, dataPath string) error {
	metafd, err := os.Open(dataPath + "/" + metaFile)
	if err != nil {
		return err
	}
	defer metafd.Close()

	var metas []Meta
	decoder := json.NewDecoder(metafd)
	err = decoder.Decode(&metas)
	if err != nil {
		return fmt.Errorf("failed to decode JSON: %s", err)
	}

	allMetas := len(metas)

	if allMetas > 254 { // 255 во flash воспринимаем как пустой байт, состояние очищенной памяти  (количество сигналов ноль)
		return fmt.Errorf("the number of files is greater than 254")
	}

	_, err = fd.Write([]byte{byte(allMetas)})
	if err != nil {
		return err
	}

	allMetasBytes := 1 + taskInfoSize*allMetas

	startDataPage := startSATPage + uint16(allMetasBytes/flashPageSize)
	startDataByte := uint8(allMetasBytes % flashPageSize)

	for _, meta := range metas {

		_, err = fd.Write([]byte(convertTitle(meta.Title)))
		if err != nil {
			return err
		}

		fileInfo, err := os.Stat(dataPath + "/" + meta.File)
		if err != nil {
			return err
		}
		fileSize := fileInfo.Size()

		if fileSize%int64(2*activeChannelsCount) != 0 {
			return fmt.Errorf("invalid file size for %s", meta.Title)
		}

		allWholePages := uint16(fileSize / flashPageSize)
		bytesRemainder := uint8(fileSize - int64(flashPageSize*allWholePages) - 256 + int64(startDataByte))
		finishDataPage := startDataPage + allWholePages
		finishDataByte := bytesRemainder - 1

		_, err = fd.Write([]byte{
			byte(startDataPage >> 8), byte(startDataPage),
			startDataByte,
			byte(finishDataPage >> 8), byte(finishDataPage),
			finishDataByte,
		})
		if err != nil {
			return err
		}

		if finishDataByte == 255 {
			startDataPage = finishDataPage + 1
			startDataByte = 0
		} else {
			startDataPage = finishDataPage
			startDataByte = finishDataByte + 1
		}
	}

	return nil
}

/*
Остальное пространство после SAT под 12 битные отсчёты сигналов (на каждый по 16 бит)
*/

// writeSignals записывает значения отсчетов сигналов
func writeSignals(fd *os.File, dataPath string) error {
	metafd, err := os.Open(dataPath + "/" + metaFile)
	if err != nil {
		return err
	}
	defer metafd.Close()

	var metas []Meta
	decoder := json.NewDecoder(metafd)
	err = decoder.Decode(&metas)
	if err != nil {
		return err
	}

	for _, meta := range metas {
		datafd, err := os.Open(dataPath + "/" + meta.File)
		if err != nil {
			return err
		}
		for {
			var data [2]byte

			err = binary.Read(datafd, binary.BigEndian, &data)
			if err != nil {
				if err == io.EOF {
					break
				}
				return err
			}

			_, err = fd.Write(data[:])
			if err != nil {
				return err
			}

		}

		datafd.Close()
	}

	return nil
}

var russianCodePage = map[rune]byte{
	'А': 0x41, 'Б': 0xA0, 'В': 0x42, 'Г': 0xA1, 'Д': 0xE0, 'Е': 0x45,
	'Ё': 0xA2, 'Ж': 0xA3, 'З': 0xA4, 'И': 0xA5, 'Й': 0xA6, 'К': 0x4B,
	'Л': 0xA7, 'М': 0x4D, 'Н': 0x48, 'О': 0x4F, 'П': 0xA8, 'Р': 0x50,
	'С': 0x43, 'Т': 0x54, 'У': 0xA9, 'Ф': 0xAA, 'Х': 0x58, 'Ц': 0xE1,
	'Ч': 0xAB, 'Ш': 0xAC, 'Щ': 0xE2, 'Ъ': 0xAD, 'Ы': 0xAE, 'Ь': 0x62,
	'Э': 0xAF, 'Ю': 0xB0, 'Я': 0xB1, 'а': 0x61, 'б': 0xB2, 'в': 0xB3,
	'г': 0xB4, 'д': 0xE3, 'е': 0x65, 'ё': 0xB5, 'ж': 0xB6, 'з': 0xB7,
	'и': 0xB8, 'й': 0xB9, 'к': 0xBA, 'л': 0xBB, 'м': 0xBC, 'н': 0xBD,
	'о': 0x6F, 'п': 0xBE, 'р': 0x70, 'с': 0x63, 'т': 0xBF, 'у': 0x79,
	'ф': 0xE4, 'х': 0x78, 'ц': 0xE5, 'ч': 0xC0, 'ш': 0xC1, 'щ': 0xE6,
	'ъ': 0xC2, 'ы': 0xC3, 'ь': 0xC4, 'э': 0xC5, 'ю': 0xC6, 'я': 0xC7,
}

// convertTitle преобразует строку в кодировку LCD
func convertTitle(title string) []byte {
	result := make([]byte, 32)

	for i := range 32 {
		result[i] = ' '
	}

	i := 0
	for _, rune := range title {
		code, ok := russianCodePage[rune]
		if ok {
			result[i] = code
		} else {
			result[i] = byte(rune)
		}
		i++
		if i >= 32 {
			break
		}
	}

	return result
}

// getChannels преобразует строковое HEX значение поля channels в uint16
func getChannels(channels string) (uint16, error) {
	channels = strings.ReplaceAll(channels, "0x", "")

	result, err := strconv.ParseUint(channels, 16, 16)

	if err != nil {
		return 0, err
	} else {
		return uint16(result), nil
	}
}

// getActiveChannelsCount возвращает количестов установленных разрядов в channels
func getActiveChannelsCount(channels uint16) uint8 {
	count := uint8(0)

	for channels > 0 {
		channels = channels & (channels - 1)
		count = count + 1
	}

	return count
}
