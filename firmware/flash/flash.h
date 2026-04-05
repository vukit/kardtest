/*************************************************************************
Title:    Kardtest flash/flash.h
Author:   Mark Vaisman  <mark.v.vaisman@gmail.com>  https://vukit.ru/articles/ecs-simulator/
Software: AVR-GCC 5.4.x
License:  MIT License
**************************************************************************/
#define FLASH_ALL_PAGES 8192
#define FLASH_PAGE_SIZE 256

#define FLASH_WRITE_MODE 0x01
#define FLASH_READ_MODE 0x02

#define FLASH_START_PAGE_ADDRESS_TASKS 8
#define TASK_INFO_SIZE 38

void flash_init(void);
void flash_bulk_erase(void callback(void));
void flash_open(uint16_t page_address, uint8_t first_byte_address, uint8_t mode);
void flash_write_byte(uint8_t data);
uint8_t flash_read_byte(void);
uint16_t flash_read_word(uint16_t start_page_address, uint8_t start_byte_address);
void flash_read_words(volatile uint16_t *data, uint8_t all_words, volatile uint16_t *page_address, volatile uint8_t *byte_address);
void flash_hold_down(void);
void flash_hold_up(void);
void flash_close(void);
