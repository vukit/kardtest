/*************************************************************************
Title:    Kardtest encoder/encoder.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "encoder.h"

#define ENCODER_PINS (((PINB & _BV(PINB3)) | (PINB & _BV(PINB2))) >> PINB2)

volatile int16_t enc_counter = 0;
volatile int16_t last_position = 0;
volatile uint8_t hold_timer = 0;
volatile int8_t enc_key = ENCODER_KEY_AT_REST;

#define TIMER_0_TICK ((uint8_t)25) // 4 ms - (uint8_t)(256 - F_CPU / 64.0 * 4e-3 - 0.5)

const int8_t enc_table[] PROGMEM = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};

void encoder_init(void)
{
    DDRD &= ~(_BV(PD2));           // Порт кнопки энкодера на вход
    DDRB &= ~(_BV(DDB2));          // Порт A энкодера на вход
    DDRB &= ~(_BV(DDB3));          // Порт B энкодера на вход
    TCCR0 = _BV(CS01) | _BV(CS00); // F_CPU/64
    TCNT0 = TIMER_0_TICK;
    TIMSK |= _BV(TOIE0);
}

ISR(TIMER0_OVF_vect)
{
    static uint8_t enc_history = 0;

    TCNT0 = TIMER_0_TICK;

    enc_history <<= 2;
    enc_history |= ENCODER_PINS;

    int8_t shift = pgm_read_byte(&enc_table[enc_history & 0x0F]);

    enc_counter += shift;

    if (!(PIND & _BV(PIND2)))
    {
        if (hold_timer < 255)
        {
            hold_timer++;
        }
        if (hold_timer == 125) // 500 мс (125 * 4 мс)
        {
            enc_key = ENCODER_KEY_PRESSED;
        }
    }
    else
    {
        if (hold_timer > 5 && hold_timer < 125) // Клик (от 20 до 500 мс)
        {
            enc_key = ENCODER_KEY_CLICK;
        }
        if (hold_timer > 125)
        {
            enc_key = ENCODER_KEY_AT_REST;
        }
        hold_timer = 0;
    }
}

int8_t get_encoder_state(void)
{
    cli();
    int16_t actual_position = enc_counter / 2;
    uint8_t actual_enc_key = enc_key;
    sei();

    if (actual_position > last_position)
    {
        last_position = actual_position;
        return ENCODER_CW;
    }

    else if (actual_position < last_position)
    {
        last_position = actual_position;
        return ENCODER_CCW;
    }

    if (actual_enc_key == ENCODER_KEY_CLICK)
    {
        enc_key = ENCODER_KEY_AT_REST;
        return actual_enc_key;
    }

    return actual_enc_key;
}

bool is_encoder_key_pressed(void)
{
    return !(PIND & _BV(PIND2));
}