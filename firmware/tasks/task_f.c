/*************************************************************************
Title:    Kardtest tasks/task_f.c
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

typedef struct
{
    uint16_t first_page_address;
    uint8_t first_byte_address;
    uint16_t last_page_address;
    uint8_t last_byte_address;
} task_info_t;

static void get_task_info(uint16_t task_number, volatile task_info_t *task_info);
static void task_print_menu(uint8_t page);
static bool task_settings(volatile uint8_t *state_settings_menu);
static void task_read_settings(void);
static void task_save_settings(void);

volatile int16_t inpase_amplitude;
volatile int16_t const_amplitude;
volatile uint32_t phase_acc_s = 0;
volatile uint8_t state_menu;
volatile uint8_t state_signal;
volatile uint8_t state_settings_menu;
volatile task_info_t task_info;
volatile uint16_t current_page_address;
volatile uint8_t current_byte_address;

// Чтение отсчётов сигналов из flash для 12-ти каналов
void task_f(uint16_t task_number)
{
    get_task_info(task_number, &task_info);

    task_read_settings();

    state_menu = STATE_TASK_MENU_SETTINGS;
    state_signal = STATE_SIGNAL_PAUSE;
    state_settings_menu = STATE_TASK_MENU_SETTINGS_S;

    set_dac_vaules_to_value(DAC_MIDDLE_VALUE);

    task_print_menu(0);

    current_page_address = task_info.first_page_address;
    current_byte_address = task_info.first_byte_address;

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
            flash_read_words(
                dac_values,
                ALL_ECG_CHANNELS,
                &current_page_address,
                &current_byte_address);

            if (
                (current_page_address == task_info.last_page_address &&
                 current_byte_address > task_info.last_byte_address) ||
                (current_page_address > task_info.last_page_address))
            {
                current_page_address = task_info.first_page_address;
                current_byte_address = task_info.first_byte_address;
            }

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

static void get_task_info(uint16_t task_number, volatile task_info_t *task_info)
{
    uint16_t task_fields_offest = TASK_INFO_SIZE * task_number + 33;
    uint16_t flash_task_fields_page_address = FLASH_START_PAGE_ADDRESS_TASKS + task_fields_offest / FLASH_PAGE_SIZE;
    uint8_t flash_task_fields_first_byte_address = task_fields_offest % FLASH_PAGE_SIZE;

    flash_open(flash_task_fields_page_address, flash_task_fields_first_byte_address, FLASH_READ_MODE);

    for (uint8_t i = 0; i < 10; i++)
    {
        switch (i)
        {
        case 0:
            task_info->first_page_address = ((uint16_t)flash_read_byte() << 8);
            break;
        case 1:
            task_info->first_page_address |= flash_read_byte();
            break;
        case 2:
            task_info->first_byte_address = flash_read_byte();
            break;
        case 3:
            task_info->last_page_address = ((uint16_t)flash_read_byte() << 8);
            break;
        case 4:
            task_info->last_page_address |= flash_read_byte();
            break;
        case 5:
            task_info->last_byte_address = flash_read_byte();
            break;
        }
    }

    flash_close();
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