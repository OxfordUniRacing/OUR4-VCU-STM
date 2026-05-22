#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "flash.h"
#include "stm_flash.h"
#include "stm_pro_mode.h"
#include "logger.h"

// #define PIN_BOOT0   GPIO_NUM_0
// #define PIN_RESET   GPIO_NUM_1

static const char *TAG = "flashing";
#define FILE_LOCATION "/spiffs/stm_binary.bin"

// void init_pins(void){
// 	// some magic to set up the pins - we essnetially want to set the RESET and BOOT0 pins, want to be able to output (not read) to them and then disable a bunch of stuff
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << PIN_BOOT0) | (1ULL << PIN_RESET),
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     gpio_config(&io_conf);

// 	// start the STM to be running normally
//     gpio_set_level(PIN_BOOT0, 0);
//     gpio_set_level(PIN_RESET, 1);
// }

// void stm32_enter_bootloader(void) {
//     gpio_set_level(PIN_BOOT0, 1);   // BOOT0 high = bootloader mode
//     vTaskDelay(pdMS_TO_TICKS(10));   // settle time
//     gpio_set_level(PIN_RESET, 0);   // hold in reset
//     vTaskDelay(pdMS_TO_TICKS(100));
//     gpio_set_level(PIN_RESET, 1);   // release reset — STM32 boots into bootloader
//     vTaskDelay(pdMS_TO_TICKS(200)); // wait for bootloader to initialise
// }

// void stm32_exit_bootloader(void) {
//     gpio_set_level(PIN_BOOT0, 0);   // BOOT0 low = normal flash boot
// 	vTaskDelay(pdMS_TO_TICKS(10));   // settle time
//     gpio_set_level(PIN_RESET, 0);   // pulse reset
//     vTaskDelay(pdMS_TO_TICKS(100));
//     gpio_set_level(PIN_RESET, 1);   // STM32 boots into new firmware
// }

void clear_binary(void){
	remove(FILE_LOCATION);
}

int save_binary_chunk(FILE *f, uint8_t *data, int len){
	ESP_LOGI(TAG, "saving binary to SPIFFS");
	size_t amount_written = fwrite(data, 1, len, f);
	if (amount_written < len){ // potential disk space issue 
		ESP_LOGW(TAG, "Only wrote %d bytes out of total %d bytes", amount_written, len);
	}

	return 0;
}

void init_flash(void){
	initFlashUART(); 	// set up uart to talk to the STM
    initGPIO();			// sort out the GPIO pins including setting BOOT0 to high, which puts STM into bootloader mode
}

void flash_stm(){
	init_flash();					// setup uart, pins

	flashSTM("/spiffs/stm_binary.bin");		// TODO - Maybe we need to prepend "/spiffs/"
	
	endConn(); 						// reset the pins
}