/*
 * data_logging.c
 *
 *  Created on: Jun 27, 2025
 *      Author: georg
 */


#include <string.h>

#include "logging.h"
#include "data_logging.h"
#include "main.h"
#include "timer.h"
#include "precharge.h"
#include "inverter.h"
#include "app.h"

#include <stdio.h>

void log_item(void);

static struct msdCard card;

#define LOG_TIME (100)

/*
 * Generally Logging has 2 types, messages which occur during events (i.e. each precharge event) and periodic data recordings.
 *
 * Both will start with the current_time_ms followed by either the message or data
 *
 */

void log_header(void);


void init_logging(void)
{
	makeCard(&hspi3, &card);
	log_header();
}


void handle_logging(void)
{
	static uint32_t last_log_time = 0;

	if(has_delay_passed(last_log_time, LOG_TIME))
	{
		log_item();

		last_log_time = current_time_ms();
	}
}


void log_header(void)
{
	char temp_char[512];

	strcpy(temp_char,
			"Time(ms), Bat SOC, BAT Volt, BAT Curr, BAT DLC, BAT Temp Hi, BAT Temp Lo, INV1 Speed, INV1 Torque, INV1 Motor Temp, INV2 Speed, INV2 Torque, INV2 Motor Temp, Brake Pressure, Pedal Value\n\r");
	log_str(&card, temp_char);
}

void log_item(void)
{
	char temp_char[512];

	snprintf(temp_char, sizeof(temp_char), "%lu, %u, %.1f, %.1f, %u, %u, %u, %i, %.1f, %i, %i, %.1f, %i, %u, %u\n\r",
			current_time_ms,

			bms.SOC,
			bms.voltage,
			bms.current,
			bms.pack_dlc,
			bms.high_temp,
			bms.low_temp,

			inv1.measured_motor_speed,
			inv1.measured_torque,
			inv1.motor_temp,

			inv2.measured_motor_speed,
			inv2.measured_torque,
			inv2.motor_temp,

			car_control.brake_pressure,
			car_control.user_pedal_value
	);


	log_str(&card, temp_char);
}

// Reads over all blocks and just prints them
void print_items(struct msdCard* card)
{
	uint16_t block = card->_first_block;

	char temp_char [512];

	while(block < card->_write_pos)
	{
		read_block(card, temp_char, block);
		printf(temp_char);
		printf("\n\r");
	}

	printf("LOG COMPLETE");
}
