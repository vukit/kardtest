/*************************************************************************
Title:    Kardtest dacs/dacs.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#define ALL_DAC_CHANNELS 12
#define SIGNAL_DAC_CHANNELS 11
#define ALL_ECG_CHANNELS 9

#define DAC_MAX_VALUE 0x0FFF
#define DAC_MIDDLE_VALUE 0x0800
#define DAC_MIN_VALUE 0x0000

#define CNANNEL_R 0
#define CNANNEL_L 1
#define CNANNEL_F 2
#define CNANNEL_C1 3
#define CNANNEL_C2 4
#define CNANNEL_C3 5
#define CNANNEL_C4 6
#define CNANNEL_C5 7
#define CNANNEL_C6 8
#define CNANNEL_C 9
#define CNANNEL_S 10
#define CNANNEL_O 11

void dacs_init(void);
void dacs_set_all_channels_to_dac_values(void);
void dacs_set_all_channels_to_value(uint16_t value);
void set_dac_vaules_to_value(uint16_t value);

extern volatile uint16_t dac_values[ALL_DAC_CHANNELS];
extern volatile bool is_set_new_dac_values;