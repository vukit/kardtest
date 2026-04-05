#include <stdint.h>

/*************************************************************************
Title:    Kardtest tasks/tasks.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
void print_integer_number(int16_t val, uint8_t x, uint8_t y, uint8_t width);
void print_number_with_fixed_point(int16_t val, uint8_t x, uint8_t y, uint8_t width, uint8_t precision, uint8_t id);
#define ALL_BUILTIN_TASKS 4

void task_0(void);
void task_1(void);
void task_2(void);
void task_3(void);

void task_f(uint16_t task_number);

typedef void (*task_type)(void);

task_type const tasks[] = {
    &task_0,
    &task_1,
    &task_2,
    &task_3,
};
