/*************************************************************************
Title:    Kardtest util/print.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "print.h"
#include "../lcd/lcd.h"

// Массив цифр 0-9 (каждая по 7 байт)
// 8-я строка (точка) добавляется динамически при записи в CGRAM
const uint8_t digits_compact[10][7] PROGMEM = {
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1E}, // 2
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}  // 9
};

void print_integer_number(int16_t val, uint8_t x, uint8_t y, uint8_t width)
{
    // Для int16_t максимум 6 символов (-32768) + 1 для нуля
    char buffer[7];

    // Преобразуем число в строку (десятичная система)
    itoa(val, buffer, 10);

    uint8_t len = strlen(buffer);

    // Установка курсора в начало вывода
    lcd_gotoxy(x, y);

    // Заполнение ведущими пробелами для сохранения ширины и очистки старых цифр
    while (len < width)
    {
        lcd_putc(' ');
        len++;
    }

    // Вывод самого числа
    lcd_puts(buffer);
}

void print_number_with_fixed_point(
    int16_t val,
    uint8_t x, uint8_t y,
    uint8_t width,
    uint8_t precision,
    uint8_t id)
{

    // Обработка чистого нуля
    if (val == 0)
    {
        lcd_gotoxy(x, y);
        // Печатаем width-1 пробелов и один '0'
        for (uint8_t i = 0; i < width - 1; i++)
        {
            lcd_putc(' ');
        }
        lcd_putc('0');
        return;
    }

    // Подготовка данных
    uint16_t divisor = (precision == 2) ? 100 : 10;
    uint16_t abs_val = abs(val);
    uint16_t int_part = abs_val / divisor;
    uint16_t frac_part = abs_val % divisor;
    uint8_t units = int_part % 10;

    // Запись спецсимвола в CGRAM
    lcd_command(0x40 | (id << 3));
    for (uint8_t i = 0; i < 7; i++)
    {
        lcd_data(pgm_read_byte(&digits_compact[units][i]));
    }
    lcd_data(0x01); // Точка

    // Расчет длины
    uint8_t used_len = 1 + precision; // Единицы с точкой + дробь
    if (int_part >= 100)
        used_len += 2;
    else if (int_part >= 10)
        used_len += 1;
    if (val < 0)
        used_len += 1;

    lcd_gotoxy(x, y);

    // Вывод
    for (uint8_t i = 0; i < (width > used_len ? width - used_len : 0); i++)
    {
        lcd_putc(' ');
    }

    if (val < 0)
    {
        lcd_putc('-');
    }

    if (int_part >= 100)
    {
        lcd_putc((int_part / 100) + '0');
    }
    if (int_part >= 10)
    {
        lcd_putc(((int_part / 10) % 10) + '0');
    }

    lcd_putc(id); // Наш спецсимвол

    if (precision == 2)
    {
        lcd_putc((frac_part / 10) + '0');
        lcd_putc((frac_part % 10) + '0');
    }
    else
    {
        lcd_putc(frac_part + '0');
    }
}
