/*************************************************************************
Title:    Kardtest worker/worker.c
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#include "../encoder/encoder.h"
#include "../lcd/lcd.h"
#include "../flash/flash.h"
#include "../tasks/names.h"
#include "../tasks/tasks.h"

#define STATE_MENU 0
#define STATE_RUN_TASK 1

#define LCD_ALL_CHARS 32

volatile uint8_t all_tasks_from_flash;
volatile uint16_t task_number;

void task_number_increase(volatile uint16_t *task_number, uint8_t all_tasks_from_flash)
{
    if (*task_number < ALL_BUILTIN_TASKS + all_tasks_from_flash - 1)
    {
        ++(*task_number);

        return;
    }

    *task_number = 0;
}

void task_number_decrease(volatile uint16_t *task_number, uint8_t all_tasks_from_flash)
{
    if (*task_number > 0)
    {
        --(*task_number);

        return;
    }

    *task_number = ALL_BUILTIN_TASKS + all_tasks_from_flash - 1;
}

void print_task_name(uint16_t task_number, uint8_t all_tasks_from_flash)
{
    lcd_clrscr();
    if (task_number < ALL_BUILTIN_TASKS)
    {
        lcd_puts_p((PGM_P)pgm_read_word(&(task_names[task_number])));
        return;
    }
    if (task_number < ALL_BUILTIN_TASKS + all_tasks_from_flash)
    {
        uint16_t task_name_offest = TASK_INFO_SIZE * (task_number - ALL_BUILTIN_TASKS) + 1;
        uint16_t flash_task_name_page_address = FLASH_START_PAGE_ADDRESS_TASKS + task_name_offest / FLASH_PAGE_SIZE;
        uint8_t flash_task_name_first_byte_address = task_name_offest % FLASH_PAGE_SIZE;

        flash_open(flash_task_name_page_address, flash_task_name_first_byte_address, FLASH_READ_MODE);

        for (uint8_t i = 0; i < LCD_ALL_CHARS; i++)
        {
            lcd_putc(flash_read_byte());
        }

        flash_close();
    }
}

void worker_run(void)
{
    // Читаем количество задач
    flash_open(FLASH_START_PAGE_ADDRESS_TASKS, 0, FLASH_READ_MODE);
    all_tasks_from_flash = flash_read_byte();
    flash_close();

    if (all_tasks_from_flash == 255)
    {
        all_tasks_from_flash = 0;
    }

    task_number = 0;

    uint8_t state = STATE_MENU;

    for (;;)
    {
        switch (state)
        {
        case STATE_MENU:
            lcd_clrscr();
            print_task_name(task_number, all_tasks_from_flash);
            while (state == STATE_MENU)
            {
                switch (get_encoder_state())
                {
                case ENCODER_CW:
                    task_number_increase(&task_number, all_tasks_from_flash);
                    print_task_name(task_number, all_tasks_from_flash);
                    break;
                case ENCODER_CCW:
                    task_number_decrease(&task_number, all_tasks_from_flash);
                    print_task_name(task_number, all_tasks_from_flash);
                    break;
                case ENCODER_KEY_CLICK:
                    state = STATE_RUN_TASK;
                    break;
                }
            }
            break;
        case STATE_RUN_TASK:
            if (task_number < ALL_BUILTIN_TASKS)
            {
                tasks[task_number]();
            }
            else
            {
                task_f(task_number - ALL_BUILTIN_TASKS);
            }
            state = STATE_MENU;
            break;
        }
    }
}