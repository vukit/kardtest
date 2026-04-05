/*************************************************************************
Title:    Kardtest tasks/names.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include "names.h"

// Синусоидальный\nсигнал
const char task_0_name[] PROGMEM = "\x43\xB8\xBD\x79\x63\x6F\xB8\xE3\x61\xBB\xC4\xBD\xC3\xB9\n\x63\xB8\xB4\xBD\x61\xBB";
// Прямоугольный импульс
const char task_1_name[] PROGMEM = "\xA8\x70\xC7\xBC\x6F\x79\xB4\x6F\xBB\xC4\xBD\xC3\xB9\n\xB8\xBC\xBE\x79\xBB\xC4\x63";
// Треугольный сигнал
const char task_2_name[] PROGMEM = "\x54\x70\x65\x79\xB4\x6F\xBB\xC4\xBD\xC3\xB9\n\x63\xB8\xB4\xBD\x61\xBB";
// Белый шум
const char task_3_name[] PROGMEM = "\xA0\x65\xBB\xC3\xB9\x20\xC1\x79\xBC";

PGM_P const task_names[] PROGMEM = {
    task_0_name,
    task_1_name,
    task_2_name,
    task_3_name,
};
