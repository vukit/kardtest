/*************************************************************************
Title:    Kardtest flash/flash.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include "flash.h"

#define FLASH_DDR DDRB
#define FLASH_PORT PORTB
#define FLASH_PIN PINB
#define FLASH_CS PB0
#define FLASH_HOLD PB1

#define FLASH_READ_STATUS_REGISTER_CMD 0x05
#define FLASH_BULK_ERASE_CMD 0xC7
#define FLASH_WRITE_ENABLE_CMD 0x06
#define FLASH_READ_DATA_BYTES_CMD 0x03
#define FLASH_PAGE_PROGRAM_CMD 0x02
#define FLASH_SPACING_BYTE 0x00

#define FLASH_READ_STATUS_REGISTER_SRWD 0x80
#define FLASH_READ_STATUS_REGISTER_BP2 0x10
#define FLASH_READ_STATUS_REGISTER_BP1 0x08
#define FLASH_READ_STATUS_REGISTER_BP0 0x04
#define FLASH_READ_STATUS_REGISTER_WEL 0x02
#define FLASH_READ_STATUS_REGISTER_WIP 0x01

static void flash_cs_down(void);
static void flash_cs_up(void);
static void flash_write_enable(void);
static void flash_ready_wait(void callback(void));

void flash_init(void)
{
    // FLASH CS, HOLD как выходы
    FLASH_DDR |= _BV(FLASH_CS) | _BV(FLASH_HOLD);
    // Flash chip select, hold disable
    FLASH_PORT |= (_BV(FLASH_CS)) | _BV(FLASH_HOLD);
}

void flash_open(uint16_t page_address, uint8_t first_byte_address, uint8_t mode)
{
    uint8_t cmd_bytes[3] = {
        page_address >> 8,
        page_address,
        first_byte_address};

    flash_ready_wait(NULL);

    switch (mode)
    {
    case FLASH_WRITE_MODE:
        flash_write_enable();
        flash_cs_down();
        flash_write_byte(FLASH_PAGE_PROGRAM_CMD);
        flash_write_byte(cmd_bytes[0]);
        flash_write_byte(cmd_bytes[1]);
        flash_write_byte(cmd_bytes[2]);
        break;
    case FLASH_READ_MODE:
        flash_cs_down();
        flash_write_byte(FLASH_READ_DATA_BYTES_CMD);
        flash_write_byte(cmd_bytes[0]);
        flash_write_byte(cmd_bytes[1]);
        flash_write_byte(cmd_bytes[2]);
        break;
    }
}

void flash_bulk_erase(void callback(void))
{
    flash_write_enable();
    flash_cs_down();
    flash_write_byte(FLASH_BULK_ERASE_CMD);
    flash_cs_up();
    flash_ready_wait(callback);
}

void flash_close(void)
{
    flash_cs_up();
}

void flash_write_byte(uint8_t data)
{
    SPDR = data;
    loop_until_bit_is_set(SPSR, SPIF);
}

uint8_t flash_read_byte(void)
{
    SPDR = FLASH_SPACING_BYTE;
    loop_until_bit_is_set(SPSR, SPIF);

    return SPDR;
}

uint16_t flash_read_word(uint16_t start_page_address, uint8_t start_byte_address)
{
    uint16_t word;
    flash_open(start_page_address, start_byte_address, FLASH_READ_MODE);

    SPDR = FLASH_SPACING_BYTE;
    loop_until_bit_is_set(SPSR, SPIF);
    word = ((uint16_t)SPDR << 8);

    if (start_byte_address == 255)
    {
        start_byte_address = 0;
        start_page_address++;
        flash_close();
        flash_open(start_page_address, start_byte_address, FLASH_READ_MODE);
    }

    SPDR = FLASH_SPACING_BYTE;
    loop_until_bit_is_set(SPSR, SPIF);
    word |= SPDR;

    flash_close();

    return word;
}

void flash_read_words(volatile uint16_t *data, uint8_t all_words, volatile uint16_t *page_address, volatile uint8_t *byte_address)
{
    flash_open(*page_address, *byte_address, FLASH_READ_MODE);

    uint8_t i = all_words;
    while (i--)
    {
        SPDR = FLASH_SPACING_BYTE;
        loop_until_bit_is_set(SPSR, SPIF);
        data[i] = ((uint16_t)SPDR << 8);
        if (*byte_address == 255)
        {
            *byte_address = 0;
            *page_address += 1;
            flash_close();
            flash_open(*page_address, *byte_address, FLASH_READ_MODE);
        }
        else
        {
            *byte_address += 1;
        }

        SPDR = FLASH_SPACING_BYTE;
        loop_until_bit_is_set(SPSR, SPIF);
        data[i] |= SPDR;
        if (*byte_address == 255)
        {
            *byte_address = 0;
            *page_address += 1;
            flash_close();
            flash_open(*page_address, *byte_address, FLASH_READ_MODE);
        }
        else
        {
            *byte_address += 1;
        }
    }

    flash_close();
}

void flash_hold_down(void)
{
    FLASH_PORT &= ~(_BV(FLASH_HOLD));
}

void flash_hold_up(void)
{
    FLASH_PORT |= (_BV(FLASH_HOLD));
}

static void flash_write_enable(void)
{
    flash_cs_down();
    flash_write_byte(FLASH_WRITE_ENABLE_CMD);
    flash_cs_up();
}

static void flash_ready_wait(void callback(void))
{
    flash_cs_down();

    flash_write_byte(FLASH_READ_STATUS_REGISTER_CMD);

    while (flash_read_byte() & FLASH_READ_STATUS_REGISTER_WIP)
    {
        if (callback == NULL)
        {
            continue;
        }

        callback();
    }

    flash_cs_up();
}

static void flash_cs_down(void)
{
    FLASH_PORT &= ~(_BV(FLASH_CS));
}

static void flash_cs_up(void)
{
    FLASH_PORT |= (_BV(FLASH_CS));
}
