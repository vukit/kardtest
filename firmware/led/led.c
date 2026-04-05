/*************************************************************************
Title:    Kardtest led/led.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <avr/io.h>
#include "led.h"

void led_init(void)
{
    // Порт светодиода на выход
    DDRC |= _BV(PC7);
}

void led_off(void)
{
    PORTC |= _BV(PC7);
}

void led_on(void)
{
    PORTC &= ~(_BV(PC7));
}

void led_toogle(void)
{
    PORTC ^= _BV(PC7);
}