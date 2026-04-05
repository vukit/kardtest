/*************************************************************************
Title:    Kardtest messages/messages.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include "messages.h"

// Кардтест 2000
const char device_name_msg[] PROGMEM = "\x4B\x61\x70\xE3\xBF\x65\x63\xBF\x20\x32\x30\x30\x30";
// vukit.ru
const char site_url_msg[] PROGMEM = "vukit.ru";
// Загрузка данных
const char loading_data_msg[] PROGMEM = "\xA4\x61\xB4\x70\x79\xB7\xBA\x61\x20\xE3\x61\xBD\xBD\xC3\x78";
// Ошибка загрузки
const char failed_load_msg[] PROGMEM = "\x4F\xC1\xB8\xB2\xBA\x61\x20\xB7\x61\xB4\x70\x79\xB7\xBA\xB8";
// Некорректный CRC
const char load_crc_error_msg[] PROGMEM = "CRC error";
// Таймаут загрузки
const char load_timeout_error_msg[] PROGMEM = "Timeout error";
// UART frame error
const char load_frame_error_msg[] PROGMEM = "Frame error";
// UART overrun error
const char load_overrun_error_msg[] PROGMEM = "Overrun error";
// UART overflow error
const char load_overflow_error_msg[] PROGMEM = "Overflow error";
// Проверка данных
const char data_verification_msg[] PROGMEM = "\xA8\x70\x6F\xB3\x65\x70\xBA\x61\x20\xE3\x61\xBD\xBD\xC3\x78";
// Очистка памяти
const char flash_erase_msg[] PROGMEM = "\x4F\xC0\xB8\x63\xBF\xBA\x61\x20\xBE\x61\xBC\xC7\xBF\xB8";