/*************************************************************************
Title:    Kardtest tasks/task_3.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdbool.h>
#include "../encoder/encoder.h"
#include "../lcd/lcd.h"
#include "../flash/flash.h"
#include "../dacs/dacs.h"
#include "../utils/print.h"
#include "names.h"
#include "common.h"

static void task_print_menu(uint8_t page);
static bool task_settings(volatile uint8_t *state_settings_menu);
static void task_read_settings(void);
static void task_save_settings(void);

volatile int16_t inpase_amplitude;
volatile int16_t const_amplitude;
volatile uint32_t phase_inc_s;
volatile uint8_t state_menu;
volatile uint8_t state_signal;
volatile uint8_t state_settings_menu;

// Состояние LFSR (регистр сдвига). 32 бита обеспечат период в несколько суток.
static uint32_t lfsr = 0xACE1uL;

static inline uint16_t get_noise(void)
{
    // Алгоритм Галуа (Galois LFSR). Очень быстрый на 8-битных AVR.
    if (lfsr & 1)
    {
        lfsr = (lfsr >> 1) ^ 0xD0000001uL;
    }
    else
    {
        lfsr >>= 1;
    }
    return (uint16_t)(lfsr & 0x0FFF); // Маска для 12 бит (0...4095)
}

// Синтез белого шума для 9-ти каналов ЭКС
void task_3(void)
{
    task_read_settings();

    state_menu = STATE_TASK_MENU_SETTINGS;
    state_signal = STATE_SIGNAL_PAUSE;
    state_settings_menu = STATE_TASK_MENU_SETTINGS_S;

    volatile uint32_t phase_acc_s = 0;

    set_dac_vaules_to_value(DAC_MIDDLE_VALUE);

    task_print_menu(0);

    while (task_control(&state_menu, &state_signal, true))
    {
        if (state_menu == STATE_TASK_MENU_SETTINGS_RUN)
        {
            task_move_cursor(14, 0, 3, 1, LCD_ARROW_LEFT_CHAR);
            state_settings_menu = STATE_TASK_MENU_SETTINGS_S;
            state_menu = STATE_TASK_MENU_SETTINGS_PROCCESSING;
        }
        if (state_menu == STATE_TASK_MENU_SETTINGS_PROCCESSING)
        {
            if (!task_settings(&state_settings_menu))
            {
                task_move_cursor(9, 1, 14, 0, LCD_ARROW_RIGHT_CHAR);
                state_menu = STATE_TASK_MENU_SETTINGS;
            }
        }

        if (state_signal == STATE_SIGNAL_WAIT_PAUSE)
        {
            dacs_set_all_channels_to_value(DAC_MIDDLE_VALUE);
            is_set_new_dac_values = true;
            state_signal = STATE_SIGNAL_PAUSE;
        }

        if (state_signal == STATE_SIGNAL_GENERATE && !is_set_new_dac_values)
        {
            dac_values[CNANNEL_R] = get_noise();
            dac_values[CNANNEL_L] = get_noise();
            dac_values[CNANNEL_F] = get_noise();
            dac_values[CNANNEL_C1] = get_noise();
            dac_values[CNANNEL_C2] = get_noise();
            dac_values[CNANNEL_C3] = get_noise();
            dac_values[CNANNEL_C4] = get_noise();
            dac_values[CNANNEL_C5] = get_noise();
            dac_values[CNANNEL_C6] = get_noise();

            phase_acc_s += FREQUENCY_50_HZ * PHASE_INC_FACTOR_50_HZ;
            set_inpase_const_channels_values(phase_acc_s, inpase_amplitude, const_amplitude);

            dacs_set_all_channels_to_dac_values();

            is_set_new_dac_values = true;
        }
    }

    dacs_set_all_channels_to_value(DAC_MIDDLE_VALUE);
    set_dac_vaules_to_value(DAC_MIDDLE_VALUE);
    is_set_new_dac_values = true;
}

static void task_print_menu(uint8_t page)
{
    lcd_clrscr();

    switch (page)
    {
    case 0:
        print_menu_settings_inphase_const_channels(inpase_amplitude, const_amplitude);
        lcd_gotoxy(14, 0);
        lcd_putc(LCD_ARROW_RIGHT_CHAR);
        break;
    }

    lcd_gotoxy(15, 0);
    switch (state_menu)
    {
    case STATE_TASK_MENU_SETTINGS:
    case STATE_TASK_MENU_SETTINGS_PROCCESSING:
        lcd_putc(TASK_MENU_CHAR_SETTINGS);
        break;
    case STATE_TASK_MENU_GENERATE:
        lcd_putc(TASK_MENU_CHAR_GENERATE);
        break;
    case STATE_TASK_MENU_PAUSE:
        lcd_putc(TASK_MENU_CHAR_PAUSE);
        break;
    }

    lcd_gotoxy(15, 1);
    switch (state_signal)
    {
    case STATE_SIGNAL_GENERATE:
        lcd_putc(TASK_MENU_CHAR_GENERATE);
        break;
    case STATE_SIGNAL_PAUSE:
        lcd_putc(TASK_MENU_CHAR_PAUSE);
        break;
    default:
        break;
    }
}

static bool task_settings(volatile uint8_t *state_settings_menu)
{
    switch (get_encoder_state())
    {
    case ENCODER_CW:
        inc_inphase_const_channels_values(*state_settings_menu, &inpase_amplitude, &const_amplitude);
        break;
    case ENCODER_CCW:
        dec_inphase_const_channels_values(*state_settings_menu, &inpase_amplitude, &const_amplitude);
        break;
    case ENCODER_KEY_CLICK:
        if (!select_inphase_const_channels_parameters(state_settings_menu))
        {
            task_save_settings();
            return false;
        }
        break;
    }

    return true;
}

static void task_read_settings(void)
{
    task_read_common_settings(&inpase_amplitude, &const_amplitude);
}

static void task_save_settings(void)
{
    task_save_common_settings(inpase_amplitude, const_amplitude);
}