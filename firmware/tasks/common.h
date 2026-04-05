/*************************************************************************
Title:    Kardtest tasks/common.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define LCD_ARROW_LEFT_CHAR 0
#define LCD_ARROW_RIGHT_CHAR 1

#define STATE_TASK_MENU_SETTINGS 0
#define STATE_TASK_MENU_SETTINGS_RUN 1
#define STATE_TASK_MENU_SETTINGS_PROCCESSING 2
#define STATE_TASK_MENU_GENERATE 3
#define STATE_TASK_MENU_PAUSE 4
#define STATE_TASK_MENU_EXIT 5

#define STATE_TASK_MENU_SETTINGS_S 0
#define STATE_TASK_MENU_SETTINGS_C 1
#define STATE_TASK_MENU_SETTINGS_F 2
#define STATE_TASK_MENU_SETTINGS_A 3
#define STATE_TASK_MENU_SETTINGS_M 4

#define TASK_MENU_CHAR_SETTINGS 'S'
#define TASK_MENU_CHAR_GENERATE 'G'
#define TASK_MENU_CHAR_PAUSE 'P'
#define TASK_MENU_CHAR_EXIT 'E'

#define STATE_SIGNAL_GENERATE 1
#define STATE_SIGNAL_PAUSE 2
#define STATE_SIGNAL_WAIT_PAUSE 3

void task_read_common_settings(volatile int16_t *inpase_amplitude, volatile int16_t *const_amplitude);
void task_save_common_settings(volatile int16_t inpase_amplitude, volatile int16_t const_amplitude);

bool task_control(volatile uint8_t *state_menu, volatile uint8_t *state_signal, bool have_settings_menu);
void task_move_cursor(uint8_t xp, uint8_t yp, uint8_t xn, uint8_t yn, uint8_t cursor);
int16_t task_inc_option_settings(int16_t option, int16_t option_min, int16_t option_max);
int16_t task_dec_option_settings(int16_t option, int16_t option_min, int16_t option_max);

void print_menu_settings_inphase_const_channels(volatile int16_t inpase_amplitude, volatile int16_t const_amplitude);
void inc_inphase_const_channels_values(uint8_t state_settings_menu, volatile int16_t *inpase_amplitude, volatile int16_t *const_amplitude);
void dec_inphase_const_channels_values(uint8_t state_settings_menu, volatile int16_t *inpase_amplitude, volatile int16_t *const_amplitude);
bool select_inphase_const_channels_parameters(volatile uint8_t *state_settings_menu);

#define TARGET_MODE_DEFAULT 1
#define TARGET_MODE_MIN 1
#define TARGET_MODE_MAX 2

#define INPHASE_AMPLITUDE_DEFAULT 10 // 10 V
#define INPHASE_AMPLITUDE_MIN 0      // 0 V
#define INPHASE_AMPLITUDE_MAX 20     // 20 V

#define FREQUENCY_50_HZ 50 // 50 Hz
#define PHASE_INC_FACTOR_50_HZ 4194304UL

extern const uint32_t phase_inc_factors[] PROGMEM;

extern int16_t EEMEM eep_inpase_amplitude;
extern int16_t EEMEM eep_const_amplitude;

#define CONST_AMPLITUDE_DEFAULT 300 // 300 mV
#define CONST_AMPLITUDE_MIN -300    // -300 mV
#define CONST_AMPLITUDE_MAX 300     // 300 mV

void set_ecs_channels_values(uint16_t value, volatile int16_t target_amplitude, volatile int16_t target_mode);
void set_inpase_const_channels_values(volatile uint32_t phase_acc_s, volatile int16_t inpase_amplitude, volatile int16_t const_amplitude);
