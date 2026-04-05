/*************************************************************************
Title:    Kardtest tasks/task_0.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include <stdbool.h>
#include <util/atomic.h>
#include "../encoder/encoder.h"
#include "../lcd/lcd.h"
#include "../flash/flash.h"
#include "../dacs/dacs.h"
#include "../utils/print.h"
#include "names.h"
#include "common.h"

#define TARGET_FREQUENCY_DEFAULT 38 // 20 Гц
#define TARGET_FREQUENCY_MIN 1      // 0.01 Гц
#define TARGET_FREQUENCY_MAX 118    // 100 Гц

#define TARGET_AMPLITUDE_DEFAULT 50 // 50 = 5.0 mV
#define TARGET_AMPLITUDE_MIN 1      // 1 = 0.1 mV
#define TARGET_AMPLITUDE_MAX 50     // 50 = 5.0 mV

static void task_print_menu(uint8_t page);
static bool task_settings(volatile uint8_t *state_settings_menu);
static void task_read_settings(void);
static void task_save_settings(void);

int16_t EEMEM eep_target_frequency_task_0;
int16_t EEMEM eep_target_amplitude_task_0;
int16_t EEMEM eep_target_mode_task_0;

volatile int16_t target_frequency;
volatile int16_t target_amplitude;
volatile int16_t target_mode;
volatile int16_t inpase_amplitude;
volatile int16_t const_amplitude;
volatile uint32_t phase_inc;
volatile uint32_t phase_inc_s;
volatile uint8_t state_menu;
volatile uint8_t state_signal;
volatile uint8_t state_settings_menu;

// Синтез синусоидальных сигналов для 9-ти каналов ЭКС
void task_0(void)
{
    task_read_settings();

    state_menu = STATE_TASK_MENU_SETTINGS;
    state_signal = STATE_SIGNAL_PAUSE;
    state_settings_menu = STATE_TASK_MENU_SETTINGS_F;

    volatile uint32_t phase_acc = 0;
    volatile uint32_t phase_acc_s = 0;

    uint16_t value;
    int32_t value1, value2;
    uint16_t values[2];
    uint16_t idx, fract;
    uint16_t value_offest;
    uint16_t value_page_address;
    uint8_t value_byte_address;

    if (target_frequency < 19)
    {

        phase_inc = pgm_read_dword(&phase_inc_factors[target_frequency - 1]);
    }
    else
    {
        phase_inc = (target_frequency - 18) * pgm_read_dword(&phase_inc_factors[18]);
    }

    set_dac_vaules_to_value(DAC_MIDDLE_VALUE);

    task_print_menu(0);

    while (task_control(&state_menu, &state_signal, true))
    {
        if (state_menu == STATE_TASK_MENU_SETTINGS_RUN)
        {
            task_move_cursor(14, 0, 4, 1, LCD_ARROW_LEFT_CHAR);
            state_settings_menu = STATE_TASK_MENU_SETTINGS_F;
            state_menu = STATE_TASK_MENU_SETTINGS_PROCCESSING;
        }
        if (state_menu == STATE_TASK_MENU_SETTINGS_PROCCESSING)
        {
            if (!task_settings(&state_settings_menu))
            {
                task_move_cursor(14, 1, 14, 0, LCD_ARROW_RIGHT_CHAR);
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
            phase_acc += phase_inc;
            idx = (uint16_t)(phase_acc >> 22);
            fract = (uint16_t)(phase_acc >> 6);

            value_offest = idx << 1;

            value_page_address = value_offest / FLASH_PAGE_SIZE;
            value_byte_address = value_offest % FLASH_PAGE_SIZE;

            flash_read_words(
                values,
                2,
                &value_page_address,
                &value_byte_address);

            value1 = (int32_t)values[1];

            if (value_page_address == FLASH_START_PAGE_ADDRESS_TASKS)
            {
                value2 = 2047;
            }
            else
            {
                value2 = (int32_t)values[0];
            }

            int32_t delta = value2 - value1;
            value = (uint16_t)(value1 + ((delta * (int32_t)fract) >> 16));

            set_ecs_channels_values(value, target_amplitude, target_mode);

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
        lcd_puts("F,Hz A,mV Mode ");
        if (target_frequency < 10)
        {
            print_number_with_fixed_point(target_frequency, 0, 1, 4, 2, 2);
        }
        else if (target_frequency < 19)
        {
            print_number_with_fixed_point((target_frequency - 9), 0, 1, 4, 1, 2);
        }
        else
        {
            print_integer_number((target_frequency - 18), 0, 1, 4);
        }
        print_number_with_fixed_point(target_amplitude, 5, 1, 4, 1, 3);
        print_integer_number(target_mode, 10, 1, 4);
        lcd_gotoxy(14, 0);
        lcd_putc(LCD_ARROW_RIGHT_CHAR);
        break;
    case 1:
        print_menu_settings_inphase_const_channels(inpase_amplitude, const_amplitude);
        lcd_gotoxy(3, 1);
        lcd_putc(LCD_ARROW_LEFT_CHAR);
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
    uint32_t next_phase_inc;
    switch (get_encoder_state())
    {
    case ENCODER_CW:
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_F)
        {
            target_frequency = task_inc_option_settings(target_frequency, TARGET_FREQUENCY_MIN, TARGET_FREQUENCY_MAX);
            if (target_frequency < 10)
            {
                next_phase_inc = pgm_read_dword(&phase_inc_factors[target_frequency - 1]);
                print_number_with_fixed_point(target_frequency, 0, 1, 4, 2, 2);
            }
            else if (target_frequency < 19)
            {
                next_phase_inc = pgm_read_dword(&phase_inc_factors[target_frequency - 1]);
                print_number_with_fixed_point((target_frequency - 9), 0, 1, 4, 1, 2);
            }
            else
            {
                next_phase_inc = (target_frequency - 18) * pgm_read_dword(&phase_inc_factors[18]);
                print_integer_number((target_frequency - 18), 0, 1, 4);
            }
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                phase_inc = next_phase_inc;
            }
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_A)
        {
            target_amplitude = task_inc_option_settings(target_amplitude, TARGET_AMPLITUDE_MIN, TARGET_AMPLITUDE_MAX);
            print_number_with_fixed_point(target_amplitude, 5, 1, 4, 1, 3);
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_M)
        {
            target_mode = task_inc_option_settings(target_mode, TARGET_MODE_MIN, TARGET_MODE_MAX);
            print_integer_number(target_mode, 10, 1, 4);
            break;
        }
        inc_inphase_const_channels_values(*state_settings_menu, &inpase_amplitude, &const_amplitude);
        break;
    case ENCODER_CCW:
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_F)
        {
            target_frequency = task_dec_option_settings(target_frequency, TARGET_FREQUENCY_MIN, TARGET_FREQUENCY_MAX);
            if (target_frequency < 10)
            {
                next_phase_inc = pgm_read_dword(&phase_inc_factors[target_frequency - 1]);
                print_number_with_fixed_point(target_frequency, 0, 1, 4, 2, 2);
            }
            else if (target_frequency < 19)
            {
                next_phase_inc = pgm_read_dword(&phase_inc_factors[target_frequency - 1]);
                print_number_with_fixed_point((target_frequency - 9), 0, 1, 4, 1, 2);
            }
            else
            {
                next_phase_inc = (target_frequency - 18) * pgm_read_dword(&phase_inc_factors[18]);
                print_integer_number((target_frequency - 18), 0, 1, 4);
            }
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                phase_inc = next_phase_inc;
            }
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_A)
        {
            target_amplitude = task_dec_option_settings(target_amplitude, TARGET_AMPLITUDE_MIN, TARGET_AMPLITUDE_MAX);
            print_number_with_fixed_point(target_amplitude, 5, 1, 4, 1, 3);
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_M)
        {
            target_mode = task_dec_option_settings(target_mode, TARGET_MODE_MIN, TARGET_MODE_MAX);
            print_integer_number(target_mode, 10, 1, 4);
            break;
        }
        dec_inphase_const_channels_values(*state_settings_menu, &inpase_amplitude, &const_amplitude);
        break;
    case ENCODER_KEY_CLICK:
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_F)
        {
            task_move_cursor(4, 1, 9, 1, LCD_ARROW_LEFT_CHAR);
            *state_settings_menu = STATE_TASK_MENU_SETTINGS_A;
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_A)
        {
            task_move_cursor(9, 1, 14, 1, LCD_ARROW_LEFT_CHAR);
            *state_settings_menu = STATE_TASK_MENU_SETTINGS_M;
            break;
        }
        if (*state_settings_menu == STATE_TASK_MENU_SETTINGS_M)
        {
            task_print_menu(1);
            *state_settings_menu = STATE_TASK_MENU_SETTINGS_S;
            break;
        }
        if (!select_inphase_const_channels_parameters(state_settings_menu))
        {
            task_print_menu(0);
            task_save_settings();
            return false;
        }
        break;
    }

    return true;
}

static void task_read_settings(void)
{
    target_frequency = (int16_t)eeprom_read_word((uint16_t *)&eep_target_frequency_task_0);
    if ((uint16_t)target_frequency == 0xFFFF)
    {
        eeprom_update_word((uint16_t *)&eep_target_frequency_task_0, (uint16_t)TARGET_FREQUENCY_DEFAULT);
        target_frequency = TARGET_FREQUENCY_DEFAULT;
    }

    target_amplitude = (int16_t)eeprom_read_word((uint16_t *)&eep_target_amplitude_task_0);
    if ((uint16_t)target_amplitude == 0xFFFF)
    {
        eeprom_update_word((uint16_t *)&eep_target_amplitude_task_0, (uint16_t)TARGET_AMPLITUDE_DEFAULT);
        target_amplitude = TARGET_AMPLITUDE_DEFAULT;
    }

    target_mode = (int16_t)eeprom_read_word((uint16_t *)&eep_target_mode_task_0);
    if ((uint16_t)target_mode == 0xFFFF)
    {
        eeprom_update_word((uint16_t *)&eep_target_mode_task_0, (uint16_t)TARGET_MODE_DEFAULT);
        target_mode = TARGET_MODE_DEFAULT;
    }

    task_read_common_settings(&inpase_amplitude, &const_amplitude);
}

static void task_save_settings(void)
{
    eeprom_update_word((uint16_t *)&eep_target_frequency_task_0, (uint16_t)target_frequency);
    eeprom_update_word((uint16_t *)&eep_target_amplitude_task_0, (uint16_t)target_amplitude);
    eeprom_update_word((uint16_t *)&eep_target_mode_task_0, (uint16_t)target_mode);
    task_save_common_settings(inpase_amplitude, const_amplitude);
}