package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"math"
	"os"

	"gonum.org/v1/plot"
	"gonum.org/v1/plot/plotter"
	"gonum.org/v1/plot/vg"
	"gonum.org/v1/plot/vg/draw"
	"gonum.org/v1/plot/vg/vgimg"
)

func main() {
	// Константы как переменные для корректного приведения типов
	var fs float64 = 1024
	var bpm float64 = 80 // 80 BPM дает ровно 768 отсчетов на удар (1024 * 60 / 80)

	// Явное вычисление количества отсчетов
	samplesPerBeat := int(fs * (60.0 / bpm))
	numBeats := 10
	totalSamples := samplesPerBeat * numBeats

	f, err := os.Create("ecg_smooth.bin")
	if err != nil {
		panic(err)
	}
	defer f.Close()

	channels := []string{"C6", "C5", "C4", "C3", "C2", "C1", "F", "L", "R"}
	offset := 2048.0
	amplitude := 1000.0

	for i := 0; i < totalSamples; i++ {
		// Текущая фаза удара (от 0.0 до 1.0)
		phase := float64(i%samplesPerBeat) / float64(samplesPerBeat)

		val := generateECG(phase)

		for _, ch := range channels {
			gain := 1.0
			if ch == "F" || ch == "L" || ch == "R" {
				gain = 0.7
			}

			sample := uint16(val*amplitude*gain + offset)

			// Запись BIG ENDIAN
			binary.Write(f, binary.BigEndian, sample)
		}
	}
	fmt.Printf("Файл создан: %d ударов по %d отсчетов. Итого: %d\n", numBeats, samplesPerBeat, totalSamples)

	// Строим графики сигналов для всех 9-ти каналов
	fileName := "ecg_smooth.bin"
	file, err := os.Open(fileName)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	const numChannels = 9
	channelNames := []string{"C6", "C5", "C4", "C3", "C2", "C1", "F", "L", "R"}
	
	allData := make([]plotter.XYs, numChannels)
	count := 0

	for {
		frame := make([]uint16, numChannels)
		err := binary.Read(file, binary.BigEndian, &frame)
		if err == io.EOF {
			break
		}
		if err != nil {
			panic(err)
		}

		for i := 0; i < numChannels; i++ {
			allData[i] = append(allData[i], plotter.XY{X: float64(count), Y: float64(frame[i])})
		}
		count++
		if count > 1536 { // 1.5 секунды записи
			break
		}
	}

	rows, cols := numChannels, 1
	plots := make([][]*plot.Plot, rows)
	
	for i := 0; i < rows; i++ {
		p := plot.New()
		p.Title.Text = "Channel: " + channelNames[i]
		p.Y.Min = 0
		p.Y.Max = 4096
		p.Add(plotter.NewGrid()) // Добавим сетку для наглядности

		line, _ := plotter.NewLine(allData[i])
		p.Add(line)
		
		plots[i] = make([]*plot.Plot, cols)
		plots[i][0] = p // Индекс [0], так как у нас всего 1 колонка
	}

	// Создание холста
	imgWidth := 10 * vg.Inch
	imgHeight := 15 * vg.Inch
	canvas := vgimg.New(imgWidth, imgHeight)
	
	// Настройка плиток (Используем умножение вместо вызова функции)
	dc := draw.New(canvas)
	t := draw.Tiles{
		Rows: rows,
		Cols: cols,
		PadX: 5 * vg.Millimeter, 
		PadY: 5 * vg.Millimeter,
	}

	canvases := plot.Align(plots, t, dc)
	for i := 0; i < rows; i++ {
		for j := 0; j < cols; j++ {
			if plots[i][j] != nil {
				plots[i][j].Draw(canvases[i][j])
			}
		}
	}

	// Сохранение в PNG
	f, err = os.Create("all_channels_plot.png")
	f, err = os.Create("all_channels_plot.png")
	if err != nil {
		panic(err)
	}
	defer f.Close()
	
	png := vgimg.PngCanvas{Canvas: canvas}
	if _, err := png.WriteTo(f); err != nil {
		panic(err)
	}
	
	fmt.Println("Многоканальный график успешно сохранен в all_channels_plot.png")
}

func generateECG(phase float64) float64 {
	// Модель смещена так, чтобы начало и конец фазы были "пустыми" (изолиния)
	waves := []struct{ pos, width, amp float64 }{
		{0.2, 0.02, 0.12},   // P
		{0.3, 0.01, -0.06},  // Q
		{0.32, 0.007, 1.3},  // R
		{0.34, 0.012, -0.2}, // S
		{0.5, 0.04, 0.25},   // T
	}

	var res float64
	for _, w := range waves {
		// Используем полные имена полей: w.amp, w.pos, w.width
		res += w.amp * math.Exp(-math.Pow(phase-w.pos, 2)/(2*math.Pow(w.width, 2)))
	}
	return res
}
