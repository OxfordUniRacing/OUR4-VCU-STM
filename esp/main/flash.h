#ifndef FLASH_H
#define FLASH_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define FILE_LOCATION "/spiffs/stm_binary.bin"

// file handling
void clear_binary(void);
int save_binary_chunk(FILE *f, uint8_t *data, int len);

// flash workflow
void init_flash(void);
void flash_stm(void);

// optional GPIO control (if used elsewhere)
void init_pins(void);
void stm32_enter_bootloader(void);
void stm32_exit_bootloader(void);

#endif