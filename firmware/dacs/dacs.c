/*************************************************************************
Title:    Kardtest dacs/dacs.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "dacs.h"

#define DACS_DDR DDRC
#define DACS_PORT PORTC
#define DAC_0_CS PC0
#define DAC_1_CS PC1
#define DAC_2_CS PC2
#define DACS_ALL_CS (_BV(DAC_0_CS) | _BV(DAC_1_CS) | _BV(DAC_2_CS))

volatile uint16_t dac_values[ALL_DAC_CHANNELS];
volatile bool is_set_new_dac_values = false;

void dacs_init(void)
{
    // Настройки таймера обновления выходов ЦАП
    TCCR1A = 0;
    // Режим CTC (WGM12), Делитель 8 (CS11)
    TCCR1B = _BV(WGM12) | _BV(CS11);
    // Порог: (3686400 / (8 * 1024)) - 1 = 449
    OCR1A = 449;
    // Разрешаем прерывание по совпадению (Compare Match A)
    TIMSK |= _BV(OCIE1A);

    // DAC_N_CS как выходы
    DACS_DDR |= DACS_ALL_CS;
    // Установить все каналы ЦАП в медианное значение
    dacs_set_all_channels_to_value(DAC_MIDDLE_VALUE);
    set_dac_vaules_to_value(DAC_MIDDLE_VALUE);
    dac_values[CNANNEL_O] = DAC_MIDDLE_VALUE;
    is_set_new_dac_values = true;
}

ISR(TIMER1_COMPA_vect)
{
    //  PORTC ^= _BV(PC6);
    if (is_set_new_dac_values)
    {
        DACS_PORT &= ~(DACS_ALL_CS);
        SPDR = 0x40;
        loop_until_bit_is_set(SPSR, SPIF);
        SPDR = 0x00;
        loop_until_bit_is_set(SPSR, SPIF);
        DACS_PORT |= DACS_ALL_CS;
        is_set_new_dac_values = false;
    }

    return;
}

void dacs_set_all_channels_to_value(uint16_t value)
{
    DACS_PORT &= ~(DACS_ALL_CS);
    SPDR = ((value >> 8) | 0x0080);
    loop_until_bit_is_set(SPSR, SPIF);
    SPDR = (value & 0x00FF);
    loop_until_bit_is_set(SPSR, SPIF);
    DACS_PORT |= DACS_ALL_CS;
}

void dacs_set_all_channels_to_dac_values()
{
    uint8_t dac_cs_port, channel = 0, first_byte = 0, second_byte = 0;

    for (uint8_t i = 0; i < SIGNAL_DAC_CHANNELS; i++)
    {
        switch (i)
        {
        case 0:
            dac_cs_port = DAC_0_CS;
            break;
        case 4:
            dac_cs_port = DAC_1_CS;
            channel = 0;
            break;
        case 8:
            dac_cs_port = DAC_2_CS;
            channel = 0;
            break;
        }

        first_byte = (dac_values[i] >> 8) & 0x000F;
        first_byte |= (channel << 6);
        first_byte |= (0x01 << 4);
        second_byte = dac_values[i] & 0x00FF;

        DACS_PORT &= ~(_BV(dac_cs_port));
        SPDR = first_byte;
        loop_until_bit_is_set(SPSR, SPIF);
        SPDR = second_byte;
        loop_until_bit_is_set(SPSR, SPIF);
        DACS_PORT |= _BV(dac_cs_port);

        channel++;
    }
}

void set_dac_vaules_to_value(uint16_t value)
{
    uint8_t i = SIGNAL_DAC_CHANNELS;
    while (i--)
    {
        dac_values[i] = value;
    }
}