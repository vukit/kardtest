/*************************************************************************
Title:    Kardtest logger/logger.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#ifdef LOGGER
#include <stdio.h>
#define LOG_PRINTF(...) printf(__VA_ARGS__);
#else
#define LOG_PRINTF(...) ;
#endif

void logger_init(void);