/*************************************************************************
Title:    Kardtest loader/loader.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdbool.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "loader.h"
#include "../uart/uart.h"
#include "../lcd/lcd.h"
#include "../messages/messages.h"
#include "../flash/flash.h"

#define TICKS_PER_SECOND 3600   // При F_CPU = 3.6864 MHz, делитель 1024 => 3600 Гц, 1 секунда = 3600 тиков
#define LOADER_TIMEOUT_VALUE 60 // 60 сек
#define LOADER_ACK_VALUE 0xAA

#define LOADER_PAGES_STATE 1
#define LOADER_LAST_PAGE_BYTES_STATE 2
#define LOADER_DATA_STATE 3
#define LOADER_CRC_STATE 4
#define LOADER_CHECK_FLASH_STATE 5
#define LOADER_END_STATE 6

#define LOADER_SUCCESS_LOAD 0
#define LOADER_CRC_ERROR 1
#define LOADER_TIMEOUT 2
#define LOADER_FRAME_ERROR 3
#define LOADER_OVERRUN_ERROR 4
#define LOADER_OVERFLOW_ERROR 5
#define LOADER_FLASH_CRC_ERROR 6

#define PROGRESS_BAR_LENGTH 14
#define PROGRESS_BAR_CHAR '>'

static void loader_start_timer(void);
static void loader_stop_timer(void);
static uint8_t loader_get_timeout(uint8_t timeout);
static void loader_print_result(uint8_t result);
static void loader_print_start_message(void);
static void loader_print_flash_erase_message(void);
static void loader_print_check_flash(void);
static bool loader_check_flash_data(uint16_t total_page, uint16_t last_page_bytes, uint16_t received_crc);
static void progressbar_show(uint16_t page, uint16_t total_pages);

void flash_bulk_erase_callback(void)
{
    static uint16_t page = 0;
    page++;
    progressbar_show(page, FLASH_ALL_PAGES);
    _delay_ms(1.1);
}

void loader(void)
{
    uint16_t total_pages = 0;
    uint16_t last_page_bytes = 0;
    uint16_t page = 0;
    uint16_t byte_counter = 0;
    uint16_t received_crc = 0;
    uint8_t timeout = LOADER_TIMEOUT_VALUE;
    uint8_t result = LOADER_SUCCESS_LOAD;

    loader_print_flash_erase_message();
    flash_bulk_erase(flash_bulk_erase_callback);

    loader_print_start_message();

    loader_start_timer();

    uint8_t state = LOADER_PAGES_STATE;
    byte_counter = 2;

    while (state)
    {
        if (state == LOADER_END_STATE)
        {
            loader_stop_timer();
            loader_print_result(result);
            break;
        }

        int c = uart_getc();
        if (c & 0xFF00)
        {
            if (c & UART_FRAME_ERROR)
            {
                result = LOADER_FRAME_ERROR;
                state = LOADER_END_STATE;
            }

            if (c & UART_OVERRUN_ERROR)
            {
                result = LOADER_OVERRUN_ERROR;
                state = LOADER_END_STATE;
            }

            if (c & UART_BUFFER_OVERFLOW)
            {
                result = LOADER_OVERFLOW_ERROR;
                state = LOADER_END_STATE;
            }

            timeout = loader_get_timeout(timeout);
            if (timeout == 0)
            {
                result = LOADER_TIMEOUT;
                state = LOADER_END_STATE;
            }
            continue;
        }

        timeout = LOADER_TIMEOUT_VALUE;

        switch (state)
        {
        // Читаем первые 2 байта - количество страниц
        case LOADER_PAGES_STATE:
            if (byte_counter == 2)
            {
                total_pages = (c << 8);
            }

            if (byte_counter == 1)
            {
                total_pages |= c;
            }

            byte_counter--;
            if (byte_counter == 0)
            {
                page = total_pages;
                state = LOADER_LAST_PAGE_BYTES_STATE;
                byte_counter = 2;
            }
            break;
        // Читаем вторые 2 байта - количество байт на последней странице
        case LOADER_LAST_PAGE_BYTES_STATE:
            if (byte_counter == 2)
            {
                last_page_bytes = (c << 8);
            }

            if (byte_counter == 1)
            {
                last_page_bytes |= c;
            }

            byte_counter--;
            if (byte_counter == 0)
            {
                state = LOADER_DATA_STATE;
                byte_counter = total_pages == 1 ? last_page_bytes : FLASH_PAGE_SIZE;
                lcd_putc(PROGRESS_BAR_CHAR);
                flash_open(total_pages - page, 0, FLASH_WRITE_MODE);
            }
            break;
        // Читаем данные и пишем в FLASH
        case LOADER_DATA_STATE:
            received_crc = _crc_xmodem_update(received_crc, c);

            flash_write_byte(c);

            byte_counter--;
            if (byte_counter == 0)
            {
                flash_close();

                progressbar_show(total_pages - page, total_pages);

                page--;

                byte_counter = page == 1 ? last_page_bytes : FLASH_PAGE_SIZE;

                if (page == 0)
                {
                    uart_putc(LOADER_ACK_VALUE);
                    state = LOADER_CRC_STATE;
                    byte_counter = 2;
                }
                else
                {
                    uart_putc(LOADER_ACK_VALUE);
                    flash_open(total_pages - page, 0, FLASH_WRITE_MODE);
                }
            }
            break;
        // Читаем последние 2 байта - CRC
        case LOADER_CRC_STATE:
            if (byte_counter == 2 && ((received_crc >> 8) ^ c))
            {
                result = LOADER_CRC_ERROR;
                state = LOADER_END_STATE;
            }

            if (byte_counter == 1 && ((received_crc & 0x00FF) ^ c))
            {
                result = LOADER_CRC_ERROR;
                state = LOADER_END_STATE;
            }

            byte_counter--;
            if (byte_counter == 0)
            {
                lcd_putc(PROGRESS_BAR_CHAR);
                if (!loader_check_flash_data(total_pages, last_page_bytes, received_crc))
                {
                    result = LOADER_FLASH_CRC_ERROR;
                }
                state = LOADER_END_STATE;
            }
            break;
        default:
            break;
        }
    }
}

static bool loader_check_flash_data(uint16_t total_pages, uint16_t last_page_bytes, uint16_t received_crc)
{
    loader_print_check_flash();
    uint16_t flash_crc = 0;
    uint16_t byte_counter = 0;

    lcd_putc(PROGRESS_BAR_CHAR);

    flash_open(0, 0, FLASH_READ_MODE);

    for (uint16_t page = 0; page < total_pages; page++)
    {
        byte_counter = page == total_pages - 1 ? last_page_bytes : FLASH_PAGE_SIZE;

        do
        {
            flash_crc = _crc_xmodem_update(flash_crc, flash_read_byte());
        } while (--byte_counter);

        progressbar_show(page, total_pages);
    }

    flash_close();

    lcd_putc(PROGRESS_BAR_CHAR);

    return flash_crc == received_crc;
}

static void loader_start_timer(void)
{
    TCNT1 = 0;
    OCR1A = TICKS_PER_SECOND;        // Устанавливаем предел счета
    TCCR1B |= _BV(WGM12);            // Режим CTC (сброс при совпадении с OCR1A)
    TCCR1B |= _BV(CS12) | _BV(CS10); // Предделитель 1024 и запуск
}

static void loader_stop_timer(void)
{
    TCCR1B = 0;
}

static uint8_t loader_get_timeout(uint8_t timeout)
{
    if (TIFR & _BV(OCF1A))
    {
        TIFR |= _BV(OCF1A);
        if (timeout > 0)
        {
            timeout--;
        }
    }
    return timeout;
}

static void loader_print_flash_erase_message(void)
{
    lcd_clrscr();
    lcd_puts_p(flash_erase_msg);
    lcd_gotoxy(0, 1);
}

static void loader_print_start_message(void)
{
    lcd_clrscr();
    lcd_puts_p(loading_data_msg);
    lcd_gotoxy(0, 1);
}

static void loader_print_check_flash(void)
{
    lcd_clrscr();
    lcd_puts_p(data_verification_msg);
    lcd_gotoxy(0, 1);
}

static void progressbar_show(uint16_t page, uint16_t total_pages)
{
    static uint16_t progress_bar_numerator;
    static uint16_t prev_progress_bar_step;
    uint16_t next_progress_bar_step;

    if (page == 0)
    {
        progress_bar_numerator = 0;
        prev_progress_bar_step = 0;
    }

    progress_bar_numerator += PROGRESS_BAR_LENGTH;
    next_progress_bar_step = progress_bar_numerator / total_pages;

    if (next_progress_bar_step != prev_progress_bar_step)
    {
        for (int k = prev_progress_bar_step; k < next_progress_bar_step; k++)
        {
            lcd_putc(PROGRESS_BAR_CHAR);
        }
        prev_progress_bar_step = next_progress_bar_step;
    }
}

static void loader_print_result(uint8_t result)
{
    switch (result)
    {
    case LOADER_CRC_ERROR:
        lcd_clrscr();
        lcd_puts_p(failed_load_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_crc_error_msg);
        _delay_ms(3500);
        break;
    case LOADER_TIMEOUT:
        lcd_clrscr();
        lcd_puts_p(failed_load_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_timeout_error_msg);
        _delay_ms(3500);
        break;
    case LOADER_FRAME_ERROR:
        lcd_clrscr();
        lcd_puts_p(failed_load_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_frame_error_msg);
        _delay_ms(3500);
        break;
    case LOADER_OVERFLOW_ERROR:
        lcd_clrscr();
        lcd_puts_p(failed_load_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_overflow_error_msg);
        _delay_ms(3500);
        break;
    case LOADER_OVERRUN_ERROR:
        lcd_clrscr();
        lcd_puts_p(failed_load_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_overrun_error_msg);
        _delay_ms(3500);
        break;
    case LOADER_FLASH_CRC_ERROR:
        lcd_clrscr();
        lcd_puts_p(data_verification_msg);
        lcd_gotoxy(0, 1);
        lcd_puts_p(load_crc_error_msg);
        _delay_ms(3500);
        break;
    }
    _delay_ms(1500);
}