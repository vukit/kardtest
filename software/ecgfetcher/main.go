package main

import (
	"encoding/binary"
	"fmt"
	"math"
	"os"
)

const (
	TargetFS   = 1024.0
	OriginalFS = 1000.0
	AdcMax     = 4095.0
	// 1 мВ из базы -> 0.12В на ЦАП -> 1 мВ после делителя
	// (4095 / 1.2) * 0.12 = 409.5
	LsbPerMv   = 409.5  
	Offset     = 2048.0 // Ваша изолиния (0.6В на ЦАП -> 0 мВ на выходе)
	Gain       = 2000.0 // База PhysioNet
	Duration   = 10.0
	FadeS      = 0.05   // 50 мс для плавного зацикливания
)

func main() {
	files := [][]string{
		{"./physionet.org/s0306lre.dat", "./data/normal.bin"},
		{"./physionet.org/s0010_re.dat", "./data/infarct.bin"},
		{"./physionet.org/s0338lre.dat", "./data/arrhythmia.bin"},
	}

	for _, f := range files {
		fmt.Printf("Обработка %s -> %s\n", f[0], f[1])
		if err := process(f[0], f[1]); err != nil {
			fmt.Printf("Ошибка: %v\n", err)
		}
	}
}

func process(inputPath, outputPath string) error {
	data, err := os.ReadFile(inputPath)
	if err != nil { return err }

	samplesCount := len(data) / 32
	targetCount := int(Duration * TargetFS)
	fadeSamples := int(math.Round(FadeS * TargetFS))

	// Буфер для хранения 9 каналов (используем срез срезов)
	tempBuffer := make([][9]float64, targetCount)

	for i := 0; i < targetCount; i++ {
		t := float64(i) / TargetFS
		idx := t * OriginalFS
		i0 := int(math.Floor(idx))
		i1 := i0 + 1
		if i1 >= samplesCount { break }
		frac := idx - float64(i0)

		v0, v1 := getChannels(data, i0), getChannels(data, i1)

		// ИСПРАВЛЕНО: Обращаемся к индексам [0] и [1] для I и II
		I := (v0[0] + frac*(v1[0]-v0[0])) / Gain
		II := (v0[1] + frac*(v1[1]-v0[1])) / Gain

		R := -(I + II) / 3.0
		L := (2.0*I - II) / 3.0
		F := (2.0*II - I) / 3.0

		// Порядок: C6..C1 (индексы 11..6), F, L, R
		for j := 0; j < 6; j++ {
			// ИСПРАВЛЕНО: Интерполяция для каждого грудного отведения отдельно
			idxV := 11 - j
			tempBuffer[i][j] = (v0[idxV] + frac*(v1[idxV]-v0[idxV])) / Gain
		}
		tempBuffer[i][6] = F
		tempBuffer[i][7] = L
		tempBuffer[i][8] = R
	}

	// Плавная склейка начала и конца
	for i := 0; i < fadeSamples; i++ {
		alpha := float64(i) / float64(fadeSamples)
		for ch := 0; ch < 9; ch++ {
			startVal := tempBuffer[i][ch]
			endVal := tempBuffer[targetCount-fadeSamples+i][ch]
			tempBuffer[i][ch] = endVal*(1.0-alpha) + startVal*alpha
		}
	}

	out, _ := os.Create(outputPath)
	defer out.Close()

	for i := 0; i < targetCount-fadeSamples; i++ {
		for ch := 0; ch < 9; ch++ {
			adcValRaw := (tempBuffer[i][ch] * LsbPerMv) + Offset
			if adcValRaw > 4095 { adcValRaw = 4095 }
			if adcValRaw < 0 { adcValRaw = 0 }
			binary.Write(out, binary.BigEndian, uint16(math.Round(adcValRaw)))
		}
	}
	return nil
}

func getChannels(data []byte, idx int) []float64 {
	offset := idx * 32
	res := make([]float64, 12)
	for j := 0; j < 12; j++ {
		start := offset + j*2
		res[j] = float64(int16(binary.LittleEndian.Uint16(data[start : start+2])))
	}
	return res
}
