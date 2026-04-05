/*************************************************************************
Title:    Kardtest encoder/encoder.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <inttypes.h>
#include <stdbool.h>

#define ENCODER_KEY_AT_REST -2
#define ENCODER_CCW -1
#define ENCODER_KEY_CLICK 0
#define ENCODER_CW 1
#define ENCODER_KEY_PRESSED 2

void encoder_init(void);
int8_t get_encoder_state(void);
bool is_encoder_key_pressed(void);