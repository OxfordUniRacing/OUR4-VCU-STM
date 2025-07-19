//=============================INCLUDES
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "globals.h"
#include "inverter.h"
#include "precharge.h"
#include "timer.h"

//==============================DEFINITIONS
#define WRITE_PARAMETERS    false

#define INVERTER_PRECHARGE_CURRENT		0.04
#define INVERTER_PRECHARGE_RESISTANCE	220

#define INVERTER_POSITIVE_SLEW_RATE_COMMAND  "ri"
#define INVERTER_NEGATIVE_SLEW_RATE_COMMAND  "rd"
#define INVERTER_RESPONSE_TIME_COMMAND       "ot"
#define INVERTER_CURRENT_LIMIT_COMMAND       "il"
#define INVERTER_PHASE_CURRENT_LIMIT_COMMAND "cl"
#define INVERTER_POLE_PAIRS_COMMAND          "pp"
#define INVERTER_UNDERVOLTAGE_COMMAND        "uv040"

//=============================GLOBAL VAR
precharge_t PRECHARGE_STATE = PC_TS_OFF;

bms_t bms = {
	.pack_dlc = 10, //minimum value we can set inverters to
	.precharge_enable = true,
	.voltage = 0
};
ass_t ass = { 0 };

bool ts_active(void){
    //return ts_active_local;
#ifndef DEBUG_IGNORE_TS
    return car_control.EMSDC;
#else
	return true;
#endif
}

//============================LOCAL VAR
static volatile uint32_t precharge_start_time = 0;
static volatile uint32_t parameter_write_start_time = 0;

//===================================LOCAL FUNC DECLARATIONS
// static bool handle_inverter_parameters(void);

//===========================================GLOBAL FUNC
void handle_precharge(void)
{



    //Neither does this
    if ((bms.voltage < 75 || bms.voltage > 125) && ts_active() && !car_control.precharge_ready) {
        //PRECHARGE_STATE = PC_FAILED;
    }

	//=======================================================
	//HANDLE THE TS LATCHING OFF
	/*static bool old_ts;
	if(!car_control.ignition)
	{
		ass.break_loop_ts_deactive = false;
	}
	else if(PRECHARGE_STATE != PC_TS_OFF && PRECHARGE_STATE != PC_FAILED )	//If the ts is active, keep it active
	{
		ass.break_loop_ts_deactive = false;
		old_ts = false;
	}
	else							//If at any point the ts has been deactivated, then latch it off
	{
		ass.break_loop_ts_deactive = true;

		if(old_ts == false)
		{
			old_ts = true;
			SYS_CONSOLE_PRINT("TS DEACTIVE - EMSDC LATCHED\n\r");
			SYS_CONSOLE_PRINT("POWER CYCLE TO RESET\n\r");
			PRECHARGE_STATE = PC_FAILED;
		}
	}*/

	//=====================================================
	// Handles the precharge states
	switch (PRECHARGE_STATE)
	{
		case PC_TS_OFF:
			/*
			 * While waiting for the TS to turn on, ensure that our ASS relay is closed
			 *
			 * Moved on when we detect the ts is active
			 */
			bms.precharge_enable = true;
            ass.break_loop_precharge = false;
            car_control.inverter_state = INV_SHUTDOWN;
            car_control.precharge_ready = false;
            ass.precharge_request_close = false;


			if(car_control.precharge_start)
			{
#ifdef DEBUG_IGNORE_BMS
				PRECHARGE_STATE = PC_EMSDC_ON;
				precharge_start_time = current_time_ms();
				printf("PC_TS_OFF_SUCCESS\n\r");
#else
				if(!bms.ams_precharge_enabled)
				{
					PRECHARGE_STATE = PC_FAILED;
					printf("PC BMS NOT IN PRECHARGE\n\r");
				}
				else
				{
					PRECHARGE_STATE = PC_EMSDC_ON;
					precharge_start_time = current_time_ms();
					printf("PC_TS_OFF_SUCCESS\n\r");
				}
#endif
			}

			break;


        case PC_EMSDC_ON:
			/*
			 * Once ignition has started, and we have confirmed the battery to be in precharge
			 * We want to close the EMSDC loop
			 * We can move on once we sense that TSMS- has gone high
			 */
			bms.precharge_enable = true;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_SHUTDOWN;
			car_control.precharge_ready = false;

			ass.precharge_request_close = true;

            if(ts_active())
            {
                PRECHARGE_STATE = PC_WAIT_FOR_INVERTER;
                printf("PC_EMSDC_SUCCESS\n\r");
            }
            if(has_delay_passed(precharge_start_time,5000))
			{
                PRECHARGE_STATE = PC_FAILED;
                printf("PC_EMSDC_FAIL\n\r");
            }
            break;

        case PC_WAIT_FOR_INVERTER:
			/*
			 * Once we have entered precharge mode, wait for both inverters to start that they are ready to begin energising
			 *
			 *This requires the statusword == shutdown instead of satusword == not_ready
			 *
			 * Once inverters are commmunicating, move on
			 *
			 */
			bms.precharge_enable = true;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_SHUTDOWN;
			car_control.precharge_ready = false;

			if(inv1.statusword == STATUSWORD_NOTREADY && inv2.statusword == STATUSWORD_NOTREADY)
			{
				PRECHARGE_STATE = PC_WAIT_FOR_INVERTER_ENERGISE;
                printf("PC_INVERTER_COMMS_SUCCESS\n\r");
			}
			else
			{

				if(has_delay_passed(precharge_start_time, 3000))
				{
					PRECHARGE_STATE = PC_FAILED;
                    printf("PC_INVERTER_COMMS_FAIL\n\r");
				}
				//Checked we have received communication from inverter
				//else check how much time has passed
			}


			break;

		case PC_WAIT_FOR_INVERTER_ENERGISE:

			bms.precharge_enable = true;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_ENERGISE;
			car_control.precharge_ready = false;


			if(inv1.statusword == STATUSWORD_ENERGISED && inv2.statusword == STATUSWORD_ENERGISED)
			{
				PRECHARGE_STATE = PC_WAIT_FOR_FINAL_VOLTAGE;
                printf("PC_INVERTERS_ENERGISED_SUCCESS\n\r");
			}
			else
			{
				if(has_delay_passed(precharge_start_time, 5000))
				{
					PRECHARGE_STATE = PC_FAILED;
                    printf("PC_INVERTERS_ENERGISED_FAILED\n\r");
				}
			}

			break;

		case PC_WAIT_FOR_FINAL_VOLTAGE:
			/*
			 * Once inverters are communicating, wait until the inverters have reached 95% of the battery votlage
			 *
			 * The inverters are being charged through a 220 ohm resistor, with approximately 40mA of current.
			 *	-> have to add on an additional 9 Volts when checking if we have reached the final voltage
			 *
			 * After this, the car is ready to start
			 */

            /*if(inv1.id == strtol(INVERTER_RIGHT_ID,NULL,10) || inv2.id == strtol(INVERTER_LEFT_ID,NULL,10)){
                ass.break_loop_inverter_error = true;
                PRECHARGE_STATE = PC_FAILED;
            }*/

			bms.precharge_enable = true;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_ENERGISE;
			car_control.precharge_ready = false;


#ifdef DEBUG_IGNORE_BMS
			PRECHARGE_STATE = PC_READY;
#else

			if(		get_inv_lowest_voltage() > ((bms.voltage - 7)* 0.99))
			{
				PRECHARGE_STATE = PC_WAIT_FOR_PRECHARGE_DISABLE;
                printf("PC_WAIT_FOR_FINAL_VOLTAGE_SUCCESS\n\r");
			}
			else
			{
				//t = 1000* -RCln(1-0.95) =1000* RCln20 = 3743
				//if(has_delay_passed(precharge_start_time, 10000))
				if(false)
				{
					PRECHARGE_STATE = PC_FAILED;
                    printf("PC_WAIT_FOR_FINAL_VOLTAGE_FAIL\n\r");
				}
			}
#endif
			break;

		case PC_WAIT_FOR_PRECHARGE_DISABLE:
			bms.precharge_enable = false;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_ENERGISE;
			car_control.precharge_ready = false;


			if(!bms.ams_precharge_enabled)
			{
				PRECHARGE_STATE = PC_WAIT_FOR_INVERTER_ENABLED;
				printf("PC_WAIT_FOR_BMS_PRECHARGE_DISABLE_SUCCESS\n\r");
			}
			else
			{
				if(has_delay_passed(precharge_start_time, 10000))
				{
					PRECHARGE_STATE = PC_FAILED;
                    printf("PC_WAIT_FOR_BMS_PRECHARGE_DISABLE_FAILED\n\r");
				}
			}

			break;

		case PC_WAIT_FOR_INVERTER_ENABLED:

			bms.precharge_enable = false;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_ENABLE;
			car_control.precharge_ready = false;


			if (inv1.statusword == STATUSWORD_ENABLED && inv2.statusword == STATUSWORD_ENABLED)
			{
				PRECHARGE_STATE = PC_READY;
                printf("PC_INVERTERS_ENABLED_SUCCESS\n\r");
			}
			else
			{
				if (has_delay_passed(precharge_start_time, 10000))
				{
					PRECHARGE_STATE = PC_FAILED;
                    // 1("PC_INVERTERS_ENABLED_FAILED\n\r");
				}
			}
			break;

		case PC_READY:
			bms.precharge_enable = false;
			ass.break_loop_precharge = false;
			car_control.inverter_state = INV_ENABLE;
            car_control.precharge_ready = true;

			break;

		case PC_FAILED:
			bms.precharge_enable = true;
			ass.break_loop_precharge = true;
			car_control.inverter_state = INV_SHUTDOWN;
            car_control.precharge_ready = false;
            ass.precharge_request_close = false;
			break;
	}
}
