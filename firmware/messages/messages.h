/*************************************************************************
Title:    Kardtest messages/messages.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <avr/pgmspace.h>

extern const char device_name_msg[] PROGMEM;
extern const char site_url_msg[] PROGMEM;
extern const char loading_data_msg[] PROGMEM;
extern const char failed_load_msg[] PROGMEM;
extern const char load_crc_error_msg[] PROGMEM;
extern const char load_timeout_error_msg[] PROGMEM;
extern const char load_frame_error_msg[] PROGMEM;
extern const char load_overrun_error_msg[] PROGMEM;
extern const char load_overflow_error_msg[] PROGMEM;
extern const char data_verification_msg[] PROGMEM;
extern const char flash_erase_msg[] PROGMEM;