//====================INLUCDES=============================

#include <stdbool.h>
#include "app.h"
#include "globals.h"
#include "inverter.h"
#include "main.h"
#include "precharge.h"
#include "timer.h"


//====================================DEFINITIONS===============================
#define TIMEOUT_INV1_ACTIVE	2500
#define TIMEOUT_INV2_ACTIVE	2500
#define TIMEOUT_INV1_INACTIVE   2500
#define TIMEOUT_INV2_INACTIVE   2500
#define TIMEOUT_PB		200
#define TIMEOUT_BMS		500
#define TIMEOUT_DASH    500
#define TIMEOUT_STEERING    500

#define TX_PERIOD_INV1  50
#define TX_PERIOD_INV2  50
#define TX_PERIOD_BMS   100
#define TX_PERIOD_LOGGER    100
#define TX_PERIOD_STATUS    100

#define HC1_DRIVE_PERIOD		10
#define HC1_nDRIVE_PERIOD		200
#define HC2_PERIOD				200
#define HC3_PERIOD				200

#define STARTUP_GRACE_PERIOD    2000

#define TIM2_TICKS_PER_MS 2
#define TIM1_TICKS_PER_MS 1
//=====================================GLOBAL VARIABLES========================

comms_active_t comms_active = {0};
comms_active_t comms_active_snapshot = {0};
comms_time_t comms_time = {0};
tx_ready_t tx_ready = {0};
tx_time_t tx_time = {

    .hc1 = 0,
    .hc2 = 0,
	.hc3 = 0,
    .bms = 0,
	.status = 0,
};

//=====================================LOCAL VARIABLES=========================

static volatile uint32_t ms_timer_0 = 0;
static volatile uint32_t ms_timer_1 = 1;
//static volatile bool ms_timer_flipflop;

static volatile bool startup_flag = false;
static volatile uint32_t startup_timer = 0;

//=================================GLOBAL FUNCTIONS=============================

bool has_delay_passed(uint32_t start_time, uint32_t delay)
{
	return (current_time_ms() - start_time) > delay;
}

void pause(uint32_t delay_ms){
    uint32_t start_time = current_time_ms();
    while(!has_delay_passed(start_time,delay_ms));
}

uint32_t current_time_ms(void)
{
	return tim2_value() / TIM2_TICKS_PER_MS;
}
uint16_t current_time_10us(void)
{
	return tim1_value() / TIM1_TICKS_PER_MS;
}

void handle_timeouts(void)
{
    if(!startup_flag){
        startup_flag = true;
        startup_timer = current_time_ms();
    }

	if(has_delay_passed(comms_time.inv1,TIMEOUT_INV1_ACTIVE) && inv1.active_drive)	{comms_active.inv1 = false; //SYS_CONSOLE_PRINT("inv1 active timeout");
    }
    else if(has_delay_passed(comms_time.inv1,TIMEOUT_INV1_INACTIVE) && !inv1.active_drive)  {comms_active.inv1 = false; //SYS_CONSOLE_PRINT("inv1 inactive timeout");
    }
	else												comms_active.inv1 = true;

	if(has_delay_passed(comms_time.inv2,TIMEOUT_INV2_ACTIVE) && inv2.active_drive)	{comms_active.inv2 = false; //SYS_CONSOLE_PRINT("inv2 active timeout");
    }
    else if(has_delay_passed(comms_time.inv2,TIMEOUT_INV1_INACTIVE) && !inv2.active_drive)  {comms_active.inv2 = false; //SYS_CONSOLE_PRINT("inv1 inactive timeout");
    }
	else												comms_active.inv2 = true;

	if(has_delay_passed(comms_time.pb,TIMEOUT_PB))		{comms_active.pb = false; //SYS_CONSOLE_PRINT("pb timeout");
    }
	else												comms_active.pb = true;

	if(has_delay_passed(comms_time.bms,TIMEOUT_BMS))	{comms_active.bms = false; //SYS_CONSOLE_PRINT("bms timeout");
    }
	else												comms_active.bms = true;

    if(has_delay_passed(comms_time.dash,TIMEOUT_DASH))	{comms_active.dash = false; //SYS_CONSOLE_PRINT("dash timeout");
    }
	else												comms_active.dash = true;

    if(has_delay_passed(comms_time.dash,TIMEOUT_STEERING))	{comms_active.steering = false; //SYS_CONSOLE_PRINT("steering timeout");
    }
	else												comms_active.steering = true;

    //handle all of the vital timeouts breaking the ass loop
    bool ass_break_loop_condition = (!comms_active.inv1 && car_control.precharge_ready) ||
                                (!comms_active.inv2 && car_control.precharge_ready)
#ifndef DEBUG_IGNORE_BMS
                                || !comms_active.bms;
#else
    							;
#endif

    if(has_delay_passed(startup_timer,STARTUP_GRACE_PERIOD) && ass_break_loop_condition && !ass.break_loop_timeout)
    {
        comms_active_snapshot = comms_active;
        ass.break_loop_timeout = true;
    }
}

void handle_tx_timer(void)
{
	switch(car_control.inverter_state)
	{
		case INV_ENABLE:

			if (has_delay_passed(tx_time.hc1, HC1_DRIVE_PERIOD))		tx_ready.hc1 = true;
			else														tx_ready.hc1 = false;
			break;
		case INV_ENERGISE:

			if (has_delay_passed(tx_time.hc1, HC1_DRIVE_PERIOD))		tx_ready.hc1 = true;
			else														tx_ready.hc1 = false;
			break;
		case INV_SHUTDOWN:
			if (has_delay_passed(tx_time.hc1, HC1_nDRIVE_PERIOD)) 		tx_ready.hc1 = true;
			else														tx_ready.hc1 = false;
			break;
		default:
			break;
	}

	if (has_delay_passed(tx_time.hc2, HC2_PERIOD)) 					tx_ready.hc2 = true;
	else															tx_ready.hc2 = false;


	if (has_delay_passed(tx_time.hc3, HC3_PERIOD)) 					tx_ready.hc3 = true;
	else															tx_ready.hc3 = false;


	if(has_delay_passed(tx_time.bms,TX_PERIOD_BMS))						tx_ready.bms = true; //SYS_CONSOLE_PRINT("bms tx");
	else																tx_ready.bms = false;

    if(has_delay_passed(tx_time.status,TX_PERIOD_STATUS))				tx_ready.status = true; //SYS_CONSOLE_PRINT("status tx");
	else																tx_ready.status = false;
}
