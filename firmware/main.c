/*************************************************************************
Title:    Kardtest main.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "logger/logger.h"
#include "encoder/encoder.h"
#include "lcd/lcd.h"
#include "uart/uart.h"
#include "flash/flash.h"
#include "dacs/dacs.h"
#include "loader/loader.h"
#include "messages/messages.h"
#include "worker/worker.h"

void init(void)
{
    // Все порты в pullup
    PORTA = PORTB = PORTC = PORTD = 0xFF;
    // MOSI, SCK как выходы
    DDRB |= _BV(PB5) | _BV(PB7);
    // Разрешить работу SPI, режим Master
    SPCR = _BV(SPE) | _BV(MSTR); // SPI включен, Master, делитель fosc/4
    SPSR = _BV(SPI2X);           // // Удваиваем скорость до fosc/2
    // Выход для осциллографа
    DDRC |= _BV(PC6);
    // Разрешить прерывания
    sei();
}

static void print_copyrigth(void)
{
    lcd_clrscr();
    lcd_gotoxy(1, 0);
    lcd_puts_p(device_name_msg);
    lcd_gotoxy(3, 1);
    lcd_puts_p(site_url_msg);
    _delay_ms(1500);
}

const char lcd_arrow_left_char[] PROGMEM = "\x00\x00\x08\x10\x08\x00\x00\x00";
const char lcd_arrow_right_char[] PROGMEM = "\x00\x00\x02\x01\x02\x00\x00\x00";

static void lcd_load_chars(void)
{
    lcd_command(0x40);

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t byte_to_send = pgm_read_byte(&lcd_arrow_left_char[i]);
        lcd_data(byte_to_send);
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t byte_to_send = pgm_read_byte(&lcd_arrow_right_char[i]);
        lcd_data(byte_to_send);
    }
}

int main(void)
{
    init();

    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));

    lcd_init(LCD_DISP_ON);
    lcd_load_chars();

    logger_init();

    flash_init();

    print_copyrigth();

    if (is_encoder_key_pressed())
    {
        loader();
    }

    encoder_init();

    dacs_init();

    worker_run();
}
