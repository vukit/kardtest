/*************************************************************************
Title:    Kardtest tasks/common.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdbool.h>
#include "../encoder/encoder.h"
#include "../lcd/lcd.h"
#include "../utils/print.h"
#include "../flash/flash.h"
#include "../dacs/dacs.h"
#include "common.h"

const uint32_t phase_inc_factors[] PROGMEM = {
    41943,    /* 0.01 Гц */
    83886,    /* 0.02 Гц */
    125829,   /* 0.03 Гц */
    167772,   /* 0.04 Гц */
    209715,   /* 0.05 Гц */
    251658,   /* 0.06 Гц */
    293601,   /* 0.07 Гц */
    335544,   /* 0.08 Гц */
    377487,   /* 0.09 Гц */
    419430,   /* 0.10 Гц */
    838861,   /* 0.20 Гц */
    1258291,  /* 0.30 Гц */
    1677722,  /* 0.40 Гц */
    2097152,  /* 0.50 Гц */
    2516582,  /* 0.60 Гц */
    2936013,  /* 0.70 Гц */
    3355443,  /* 0.80 Гц */
    3774874,  /* 0.90 Гц */
    4194304,  /* >= 1 Гц с шагом 1 Гц */
    26214400, /* 6.25 Гц*/
    43620762, /* 10.4 Гц*/
};

int16_t EEMEM eep_inpase_amplitude;
int16_t EEMEM eep_const_amplitude;

static void set_option_menu(char option)
{
    lcd_gotoxy(15, 0);
    lcd_putc(option);
}

static void set_selected_option_menu(char option)
{
    lcd_gotoxy(15, 1);
    lcd_putc(option);
}

bool task_control(volatile uint8_t *state_menu, volatile uint8_t *state_signal, bool have_settings_menu)
{
    if (*state_menu == STATE_TASK_MENU_SETTINGS_PROCCESSING)
    {
        return true;
    }

    switch (get_encoder_state())
    {
    case ENCODER_CW:
        if (*state_menu == STATE_TASK_MENU_SETTINGS)
        {
            set_option_menu(TASK_MENU_CHAR_GENERATE);
            *state_menu = STATE_TASK_MENU_GENERATE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_GENERATE)
        {
            set_option_menu(TASK_MENU_CHAR_PAUSE);
            *state_menu = STATE_TASK_MENU_PAUSE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_PAUSE)
        {
            set_option_menu(TASK_MENU_CHAR_EXIT);
            *state_menu = STATE_TASK_MENU_EXIT;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_EXIT)
        {
            if (have_settings_menu)
            {
                set_option_menu(TASK_MENU_CHAR_SETTINGS);
                *state_menu = STATE_TASK_MENU_SETTINGS;
                break;
            }
            set_option_menu(TASK_MENU_CHAR_GENERATE);
            *state_menu = STATE_TASK_MENU_GENERATE;
            break;
        }
        break;
    case ENCODER_CCW:
        if (*state_menu == STATE_TASK_MENU_SETTINGS)
        {
            set_option_menu(TASK_MENU_CHAR_EXIT);
            *state_menu = STATE_TASK_MENU_EXIT;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_EXIT)
        {
            set_option_menu(TASK_MENU_CHAR_PAUSE);
            *state_menu = STATE_TASK_MENU_PAUSE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_PAUSE)
        {
            set_option_menu(TASK_MENU_CHAR_GENERATE);
            *state_menu = STATE_TASK_MENU_GENERATE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_GENERATE)
        {
            if (have_settings_menu)
            {
                set_option_menu(TASK_MENU_CHAR_SETTINGS);
                *state_menu = STATE_TASK_MENU_SETTINGS;
                break;
            }
            set_option_menu(TASK_MENU_CHAR_EXIT);
            *state_menu = STATE_TASK_MENU_EXIT;
        }
        break;
    case ENCODER_KEY_CLICK:
        if (*state_menu == STATE_TASK_MENU_SETTINGS)
        {
            *state_menu = STATE_TASK_MENU_SETTINGS_RUN;
            return true;
        }
        if (*state_menu == STATE_TASK_MENU_GENERATE)
        {
            set_selected_option_menu(TASK_MENU_CHAR_GENERATE);
            *state_signal = STATE_SIGNAL_GENERATE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_PAUSE)
        {
            set_selected_option_menu(TASK_MENU_CHAR_PAUSE);
            *state_signal = STATE_SIGNAL_WAIT_PAUSE;
            break;
        }
        if (*state_menu == STATE_TASK_MENU_EXIT)
        {
            return false;
        }
        break;
    }

    return true;
}
__attribute__((noinline)) int16_t task_inc_option_settings(int16_t option, int16_t option_min, int16_t option_max)
{
    option++;
    if (option > option_max)
    {
        option = option_min;
    }

    return option;
}

__attribute__((noinline)) int16_t task_dec_option_settings(int16_t option, int16_t option_min, int16_t option_max)
{
    option--;
    if (option < option_min)
    {
        option = option_max;
    }

    return option;
}

void print_menu_settings_inphase_const_channels(int16_t inpase_amplitude, int16_t const_amplitude)
{
    lcd_puts("S,V  C,mV      ");
    print_integer_number(inpase_amplitude, 0, 1, 3);
    print_integer_number(const_amplitude, 5, 1, 4);
}

void inc_inphase_const_channels_values(
    uint8_t state_settings_menu,
    volatile int16_t *inpase_amplitude,
    volatile int16_t *const_amplitude)
{
    if (state_settings_menu == STATE_TASK_MENU_SETTINGS_S)
    {
        *inpase_amplitude = task_inc_option_settings(*inpase_amplitude, INPHASE_AMPLITUDE_MIN, INPHASE_AMPLITUDE_MAX);
        print_integer_number(*inpase_amplitude, 0, 1, 3);
    }
    if (state_settings_menu == STATE_TASK_MENU_SETTINGS_C)
    {
        *const_amplitude = task_inc_option_settings(*const_amplitude, CONST_AMPLITUDE_MIN, CONST_AMPLITUDE_MAX);
        print_integer_number(*const_amplitude, 5, 1, 4);
    }
}

void dec_inphase_const_channels_values(
    uint8_t state_settings_menu,
    volatile int16_t *inpase_amplitude,
    volatile int16_t *const_amplitude)
{
    if (state_settings_menu == STATE_TASK_MENU_SETTINGS_S)
    {
        *inpase_amplitude = task_dec_option_settings(*inpase_amplitude, INPHASE_AMPLITUDE_MIN, INPHASE_AMPLITUDE_MAX);
        print_integer_number(*inpase_amplitude, 0, 1, 3);
    }
    if (state_settings_menu == STATE_TASK_MENU_SETTINGS_C)
    {
        *const_amplitude = task_dec_option_settings(*const_amplitude, CONST_AMPLITUDE_MIN, CONST_AMPLITUDE_MAX);
        print_integer_number(*const_amplitude, 5, 1, 4);
    }
}

__attribute__((noinline)) void task_move_cursor(uint8_t xp, uint8_t yp, uint8_t xn, uint8_t yn, uint8_t cursor)
{
    lcd_gotoxy(xp, yp);
    lcd_putc(' ');
    lcd_gotoxy(xn, yn);
    lcd_putc(cursor);
}

bool select_inphase_const_channels_parameters(volatile uint8_t *state_settings_menu)
{
    if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_C)
    {
        return false;
    }
    if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_S)
    {
        task_move_cursor(3, 1, 9, 1, LCD_ARROW_LEFT_CHAR);
        *state_settings_menu = STATE_TASK_MENU_SETTINGS_C;
    }
    return true;
}

void set_ecs_channels_values(uint16_t value, volatile int16_t target_amplitude, volatile int16_t target_mode)
{
    value = (uint16_t)((((int32_t)value - DAC_MIDDLE_VALUE) * target_amplitude * 1310L) >> 16) + DAC_MIDDLE_VALUE;
    switch (target_mode)
    {
    case 2:
        dac_values[CNANNEL_R] = DAC_MIDDLE_VALUE;
        break;
    case 1:
    default:
        dac_values[CNANNEL_R] = value;
    }
    dac_values[CNANNEL_L] = value;
    dac_values[CNANNEL_F] = value;
    dac_values[CNANNEL_C1] = value;
    dac_values[CNANNEL_C2] = value;
    dac_values[CNANNEL_C3] = value;
    dac_values[CNANNEL_C4] = value;
    dac_values[CNANNEL_C5] = value;
    dac_values[CNANNEL_C6] = value;
}

void set_inpase_const_channels_values(volatile uint32_t phase_acc_s, volatile int16_t inpase_amplitude, volatile int16_t const_amplitude)
{
    // Сдвиг вправо на 22 бита (32 - 10), так как размер LUT = 2^10 (1024),
    // и влево на 1, т.к. адресуем 16 битые значения
    uint16_t value_offest = (uint16_t)(phase_acc_s >> 21);
    uint16_t value_page_address = value_offest / FLASH_PAGE_SIZE;
    uint8_t value_byte_address = value_offest % FLASH_PAGE_SIZE;
    uint16_t value = flash_read_word(value_page_address, value_byte_address);
    dac_values[CNANNEL_S] = (uint16_t)((((uint32_t)value - DAC_MIDDLE_VALUE) * inpase_amplitude * 3277UL) >> 16) + DAC_MIDDLE_VALUE;

    dac_values[CNANNEL_C] = (uint16_t)(2048 + (int16_t)(((int32_t)const_amplitude * 1747L) >> 8));
}

void task_read_common_settings(volatile int16_t *inpase_amplitude, volatile int16_t *const_amplitude)
{
    *inpase_amplitude = (int16_t)eeprom_read_word((uint16_t *)&eep_inpase_amplitude);
    if ((uint16_t)*inpase_amplitude == 0xFFFF)
    {
        eeprom_update_word((uint16_t *)&eep_inpase_amplitude, (uint16_t)INPHASE_AMPLITUDE_DEFAULT);
        *inpase_amplitude = INPHASE_AMPLITUDE_DEFAULT;
    }

    *const_amplitude = (int16_t)eeprom_read_word((uint16_t *)&eep_const_amplitude);
    if ((uint16_t)*const_amplitude == 0xFFFF)
    {
        eeprom_update_word((uint16_t *)&eep_const_amplitude, (uint16_t)CONST_AMPLITUDE_DEFAULT);
        *const_amplitude = CONST_AMPLITUDE_DEFAULT;
    }
}
void task_save_common_settings(volatile int16_t inpase_amplitude, volatile int16_t const_amplitude)
{
    eeprom_update_word((uint16_t *)&eep_inpase_amplitude, (uint16_t)inpase_amplitude);
    eeprom_update_word((uint16_t *)&eep_const_amplitude, (uint16_t)const_amplitude);
}