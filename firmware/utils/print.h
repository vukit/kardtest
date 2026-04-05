/*************************************************************************
Title:    Kardtest util/print.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
void print_integer_number(int16_t val, uint8_t x, uint8_t y, uint8_t width);
void print_number_with_fixed_point(int16_t val, uint8_t x, uint8_t y, uint8_t width, uint8_t precision, uint8_t id);
