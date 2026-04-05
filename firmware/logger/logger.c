/*************************************************************************
Title:    Kardtest logger/logger.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#ifdef LOGGER
#include "logger.h"
#include "../uart/uart.h"

static int uart_putchar(char symbol, FILE *stream)
{
    uart_putc(symbol);

    return 0;
}

static FILE kt_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_RW);

void logger_init(void)
{
    stdout = &kt_stdout;
}
#else
void logger_init(void) {}
#endif