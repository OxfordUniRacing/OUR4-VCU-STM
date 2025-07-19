#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pedals.h"

#include "can.h"
#include "precharge.h"
#include "app.h"
#include "inverter.h"
#include "timer.h"


uint32_t Mailbox = 0;
control_t car_control = { 0 };
analogs_t analogs = { 0 };

void handle_analogs(void);
void handle_inputs(void);
void handle_brake(void);
void handle_pedals(void);
void handle_car_control(void);
void handle_emsdc(void);
void handle_apps_brake_plausability(void);

void app_init(void)
{
	CAN_FilterTypeDef filter = {
	      .FilterActivation = ENABLE,
	      .SlaveStartFilterBank = 14,
	      .FilterBank = 0,
	      .FilterFIFOAssignment = CAN_RX_FIFO0,
	      .FilterMode = CAN_FILTERMODE_IDMASK,
	      .FilterIdHigh = 0x00,
	      .FilterIdLow = 0x000,
	      .FilterMaskIdHigh = 0,
	      .FilterMaskIdLow = 0,
	      .FilterScale = CAN_FILTERSCALE_32BIT,
	    };

	  HAL_CAN_ConfigFilter(&hcan1, &filter);
	  HAL_CAN_Start(&hcan1);

	  printf("car on \n\r");

	  //init_logging();

}

void app_main(void)
{

#ifdef DEBUG_PRINT_LOOP_TIMES
	uint16_t time_us = current_time_10us();
#endif


	handle_precharge();

	handle_analogs();

	handle_brake();

	handle_can();

	handle_inverters();

	handle_timeouts();

	handle_tx_timer();

	handle_inputs();

	handle_pedals();

	handle_car_control();

	handle_emsdc();

	handle_apps_brake_plausability();

	//handle_logging();

#ifdef DEBUG_PRINT_LOOP_TIMES
	time_us = current_time_10us() - time_us;

	static uint16_t max_time;

	if(time_us > max_time) max_time = time_us;

	static uint32_t count = 0;
	static uint32_t max_count = 10000;

	static uint32_t time_avg = 0;
	time_avg += time_us;

	count++;
	if(count >= max_count)
	{
		count = 0;
		printf("Max Loop Time: %u us,Avg Loop Time: %lu \n\r", 10*max_time, time_avg / max_count);
		max_time = 0;
		time_avg = 0;
	}
#endif

	//HAL_GPIO_TogglePin(GPIOD,BRAKE_CTRL_Pin);
	//HAL_GPIO_TogglePin(GPIOC,PUSH_PULL_1_Pin);	//Toggles Relay
	//HAL_GPIO_WritePin(GPIOB,GDO_LOW_3_Pin,GPIO_PIN_SET);		//RHS FAN
	//HAL_GPIO_WritePin(GPIOB,GDO_LOW_5_Pin,GPIO_PIN_SET);		//RHS FAN
	// Handle PIO

	//GPIO E

	//printf("RTD: %u, IGN: %u\n\r", RTD, IGN);

	// Handle Precharge

	// Handle CAN

/*
	uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	  CAN_TxHeaderTypeDef header = {
		.StdId = 0x11,
		.ExtId = 0,
		.IDE = CAN_ID_STD,
		.RTR = CAN_RTR_DATA,
		.DLC = 8,
	  };
	if (!HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX0))
	{
	  if (HAL_CAN_AddTxMessage(&hcan1, &header, data, &Mailbox) != HAL_OK)
	  {
		  Error_Handler();
	  }
	}

	if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
		uint8_t rxData[8];
		CAN_RxHeaderTypeDef rxHeader;
		HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData);
		HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
		printf("Received ch1 @ %lx: %x %x %x %x %x %x %x %x\n\r", rxHeader.IDE ? rxHeader.ExtId : rxHeader.StdId, rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5], rxData[6], rxData[7]);
	}
	*/
	//Read ADC Channel
	/*
	ADC_ChannelConfTypeDef sConfig;
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Offset = 0;
	sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;

	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,100);

	uint32_t LV_BAT_val;

	if((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
	{

	  LV_BAT_val = (HAL_ADC_GetValue(&hadc1) * 3300 * (18/3)) >> 12 ;
	  printf("LV Bat val: %d\n\r", LV_BAT_val);
	}
	*/
}


// Handle car control

// General idea:
// ignition switch -> precharge -> RTD switch -> sounder -> RTD

#define IGN_HOLD_TIME	1000
#define SOUNDER_ON_TIME	4000

void handle_car_control(void)
{
	static uint32_t sounder_on_time;

	//Hanlde Ignition
	// If Ignition is pressed for more than 1s, than start precharge
	static bool old_IGN;
	static uint32_t IGN_pressed_time;
	if(car_control.IGN_Switch & !car_control.RTD_Switch) // If RTD is pressed, do not progress
	{
		if(!old_IGN) IGN_pressed_time = current_time_ms();	//If we have gone from low -> high state, start timer
		else if(has_delay_passed(IGN_pressed_time, IGN_HOLD_TIME)) car_control.precharge_start = true;	//Otherwise if we have waited 1s, then we can start precharge
	}
	old_IGN = car_control.IGN_Switch;


	// Handle RTD_Switch
	// The car is RTD after:
	//	Precharge Complete
	//	RTD Pressed while brake pedal pressed
	// RTD must not be pressed before precharge complete - if it is, halt any precharge and display warning


	if(car_control.RTD_Switch)
	{
		if(!car_control.precharge_ready)	//User error, need to set a flag for the dash, if not a user error than something has gone seriously wrong
		{

		}
		else
		{
			if(!car_control.RTD && !car_control.sounder_enable && !car_control.error_state && car_control.brake_on)
			{
				// set timer for sounder
				car_control.sounder_enable = true;
				sounder_on_time = current_time_ms();
			}
		}
	}


	//Handle Sounder
	// If sounde enabled, check to see if we have been on for more than the required time
	// If the sounder
	if(car_control.sounder_enable)
	{
#ifndef DEBUG_NO_SOUND
		HAL_GPIO_WritePin(GPIOD, SPEAKER_CTRL_Pin, GPIO_PIN_SET);
#endif
		if(has_delay_passed(sounder_on_time, SOUNDER_ON_TIME))
		{
			car_control.sounder_enable = false;
			car_control.RTD = true;
		}
	}
	else
	{
		HAL_GPIO_WritePin(GPIOD, SPEAKER_CTRL_Pin, GPIO_PIN_RESET);
	}


	if(car_control.RTD)
	{
		// We have the first 10% of the pedal as a deadzone

		if(car_control.user_pedal_value >= 10 && car_control.apps_brake_plausiblity)
		{
			car_control.torque = ((float)car_control.user_pedal_value - 10.0) * TORQUE_MAX / 90.0;
		}
		else
		{
			car_control.torque = 0;
		}

		if(!car_control.RTD_Switch)	car_control.RTD = false;
	}
	else
	{
		car_control.torque = 0;
	}
}


void handle_emsdc(void)
{
	static bool ass_state = false;						// Current ass state
	static bool old_ass_error = false;
	static uint32_t ass_error_time = 0;

	bool ass_error =								//True if there is an error that means the ass loop needs closing
				ass.break_loop_ins_detect ||		//Not implemented
				ass.break_loop_inverter_error ||
				ass.break_loop_pedal_invalid ||
				ass.break_loop_precharge ||
				ass.break_loop_timeout ||
				ass.break_loop_ts_deactive
				;


	static bool old_emsdc = false;
	if(	(true  == old_emsdc) && (false == car_control.EMSDC) )
	{
		ass.break_loop_ts_deactive = true;
		ass_error = true;
	}
	old_emsdc = car_control.EMSDC;



	if(ass_state)
	{
		if(car_control.precharge_ready && !car_control.EMSDC)	//If we are past precharge, and the EMSDC reads 0, then we enter error state
		{
			ass.break_loop_ts_deactive = true;
			ass_error = true;
		}

		if(ass_error)											// If there is an error, wait 100ms before turning the loop off, gives a chance for motors to stop drawing current
		{
			car_control.inverter_state = INV_SHUTDOWN;
			if(!old_ass_error) ass_error_time = current_time_ms();

			if(has_delay_passed(ass_error_time,100) || (bms.current < 5))	// we have waited for 100ms, or the bms current is low enough, then we turn off the emsdc
			{
				ass_state = false;
			}
		}
	}
	else
	{
		if(ass.precharge_request_close && !ass_error)
		{
			ass_state = true;
		}
		if(ass.precharge_request_close && ass_error)
		{
			ass_state = false;
		}
	}

	if(ass_state)
	{
		HAL_GPIO_WritePin(GPIOC,PUSH_PULL_1_Pin, GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOC,PUSH_PULL_1_Pin, GPIO_PIN_RESET);
	}


	old_ass_error = ass_error;
}

#define PEDAL_SAMPLING_TIME_10US 100

void handle_pedals(void)
{
#define PEDAL_1_MIN 465.0f
#define PEDAL_1_MAX 2161.0f
#define PEDAL_2_MIN 2211.0f
#define PEDAL_2_MAX 4094.0f

	static const float pedal_1_gains[12] =
	{
			0.059701493,
			0.068181818,
			0.0625,
			0.060606061,
			0.056603774,
			0.063829787,
			0.061452514,
			0.064,
			0.061904762,
			0.061135371,
			0.058823529,
			0.05982906
	};
	static const uint16_t pedal_1_positions[13] =
	{
			493,
			761,
			849,
			945,
			1044,
			1203,
			1156,
			1335,
			1460,
			1670,
			1899,
			2018,
			2135
	};
	static const uint16_t percentages[12] =
	{
			0,
			16,
			22,
			28,
			34,
			43,
			40,
			51,
			59,
			72,
			86,
			93,
	};

static const float pedal_2_gains[12] =
{
		0.066666667,
		0.072289157,
		0.0625,
		0.057692308,
		0.056603774,
		0.063829787,
		0.063953488,
		0.064516129,
		0.064039409,
		0.084337349,
		0.031674208,
		0.025830258
};
static const uint16_t pedal_2_positions[13] =
{
		2237,
		2477,
		2560,
		2656,
		2760,
		2919,
		2872,
		3044,
		3168,
		3371,
		3537,
		3758,
		4029
};



	static uint32_t pedal_fault_count = 0;
	static uint16_t last_pedal_time_10us = 0;

	if( (uint16_t)(current_time_10us() - last_pedal_time_10us) < PEDAL_SAMPLING_TIME_10US ) return;
	last_pedal_time_10us = current_time_10us();

	float pedal_1 = 0;
	float pedal_2 = 0;

	uint32_t i = 0;
	for(i = 0; i < 13; i++)
	{
		if(i == 12)
		{
			pedal_1 = ((float)analogs.pedal_1 - pedal_1_positions[11]) * pedal_1_gains[11] + percentages[11];
		}
		else if(analogs.pedal_1 < pedal_1_positions[i+1])
		{
			pedal_1 = ((float)analogs.pedal_1 - pedal_1_positions[i]) * pedal_1_gains[i] + percentages[i];
			break;
		}
	}
	for(i = 0; i < 13; i++)
	{
		if(i == 12)
		{
			pedal_2 = ((float)analogs.pedal_2 - pedal_2_positions[11]) * pedal_2_gains[11] + percentages[11];
		}
		else if(analogs.pedal_2 < pedal_2_positions[i+1])
		{
			pedal_2 = ((float)analogs.pedal_2 - pedal_2_positions[i]) * pedal_2_gains[i] + percentages[i];
			break;
		}
	}


	#ifdef DEBUG_PRINT_PEDALS
		static uint32_t count = 0;
		count++;
		if(count == 300)	//Display messages every 300ms
		{
			printf("Pedal1: %f, Pedal2: %f\n\r", pedal_1, pedal_2);
			if(ass.break_loop_pedal_invalid) printf("PEDAL FAULT\n\r");
			count = 0;
		}
	#endif


	if( abs(pedal_1 - pedal_2) > 8)
	{
		pedal_fault_count++;
	}
	else if((pedal_1 > 105) | (pedal_2 > 105))
	{
		pedal_fault_count++;
	}
	else if((pedal_1 < -5) | (pedal_2 < -5))
	{
		pedal_fault_count++;
	}
	else
	{
		pedal_fault_count = 0;
	}

	if(pedal_fault_count > 50)
	{
		car_control.user_pedal_value = 0;
		ass.break_loop_pedal_invalid = true;
	}
	else
	{
		car_control.user_pedal_value = (pedal_1 + pedal_2) / 2;
	}

}

void handle_brake(void)
{
	car_control.brake_pressure = analogs.brake_sens;

	if(car_control.brake_pressure > BRAKE_PRESSURE_THRESHOLD)
	{
		car_control.brake_on = true;
	}
	else
	{
		car_control.brake_on = false;
	}


	HAL_GPIO_WritePin(GPIOD, BRAKE_CTRL_Pin, car_control.brake_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
//##########################################################################################################
#define APPS_SAMPLING_TIME_10US	100

void handle_apps_brake_plausability(void)
{

	static uint16_t last_apps_time_10us = 0;

	if( (uint16_t)(current_time_10us() - last_apps_time_10us) < APPS_SAMPLING_TIME_10US ) return;
	last_apps_time_10us = current_time_10us();

	static uint32_t fault_count = 0;

	uint32_t power = bms.voltage * bms.current;

	if(car_control.apps_brake_plausiblity)
	{
		if(car_control.brake_on && ( (power > 4500) || (car_control.user_pedal_value > 25) ) )		fault_count++;
		else fault_count = 0;

		if(fault_count > 450) car_control.apps_brake_plausiblity = false;
	}
	else
	{
		if(car_control.user_pedal_value <= 5)
		{
			fault_count = 0;
			car_control.apps_brake_plausiblity = true;
		}
	}
}


//################################################################################################################
#define ANALOG_SAMPLING_TIME_10US 10
#define ANALOG_BUFFER_SIZE 1024


void handle_analogs(void)
{

	//We want to sample every 100us and complete a 12.8ms rolling average

	static bool startup = true;

	static uint16_t last_analog_time_10us = 0;


	static uint32_t LV_VOLTAGE_BUFFER[ANALOG_BUFFER_SIZE];
	static uint32_t PEDAL_1_RAW_BUFFER[ANALOG_BUFFER_SIZE];
	static uint32_t PEDAL_2_RAW_BUFFER[ANALOG_BUFFER_SIZE];
	static uint32_t BRAKE_RAW_BUFFER[ANALOG_BUFFER_SIZE];

	static uint32_t lv_voltage_avg;
	static uint32_t pedal_1_raw_avg;
	static uint32_t pedal_2_raw_avg;
	static uint32_t brake_raw_avg;

	// All are synced therefore 1 head is required
	static uint32_t head = 0;

	//Delay between analog samples
	if( (uint16_t)(current_time_10us() - last_analog_time_10us) < ANALOG_SAMPLING_TIME_10US ) return;

	// Want to sample, remove last items from buffer
	lv_voltage_avg -= LV_VOLTAGE_BUFFER[head];
	pedal_1_raw_avg -= PEDAL_1_RAW_BUFFER[head];
	pedal_2_raw_avg -= PEDAL_2_RAW_BUFFER[head];
	brake_raw_avg -= BRAKE_RAW_BUFFER[head];



	HAL_ADC_Start(&hadc1);

	// Wait until all conversions are done
	HAL_ADC_PollForConversion(&hadc1, 100); // This blocks until the *first* conversion is done
	LV_VOLTAGE_BUFFER[head] = 	HAL_ADC_GetValue(&hadc1);  // Rank 1
	lv_voltage_avg += LV_VOLTAGE_BUFFER[head];

	HAL_ADC_PollForConversion(&hadc1, 100);
	PEDAL_1_RAW_BUFFER[head] = 	HAL_ADC_GetValue(&hadc1);  // Rank 2
	pedal_1_raw_avg += PEDAL_1_RAW_BUFFER[head];

	HAL_ADC_PollForConversion(&hadc1, 100);
	BRAKE_RAW_BUFFER[head] = 	HAL_ADC_GetValue(&hadc1);  // Rank 3
	brake_raw_avg += BRAKE_RAW_BUFFER[head];

	HAL_ADC_PollForConversion(&hadc1, 100);
	PEDAL_2_RAW_BUFFER[head] = 	HAL_ADC_GetValue(&hadc1);  // Rank 4
	pedal_2_raw_avg += PEDAL_2_RAW_BUFFER[head];

	HAL_ADC_Stop(&hadc1);

	if(head >= ANALOG_BUFFER_SIZE-1)
	{
		head = 0;
		startup = false;
	}
	else
	{
		head++;
	}

	if(startup)	//If we haven't filled the full buffer, just divide by how much we have added
	{
		analogs.LV_bat = lv_voltage_avg / head;
		analogs.pedal_1 = pedal_1_raw_avg / head;
		analogs.pedal_2 = pedal_2_raw_avg / head;
		analogs.brake_sens = brake_raw_avg / head;
	}
	else
	{
		analogs.LV_bat = lv_voltage_avg / ANALOG_BUFFER_SIZE;
		analogs.pedal_1 = pedal_1_raw_avg / ANALOG_BUFFER_SIZE;
		analogs.pedal_2 = pedal_2_raw_avg / ANALOG_BUFFER_SIZE;
		analogs.brake_sens = brake_raw_avg / ANALOG_BUFFER_SIZE;
	}

	if(car_control.fans_on)
	{
		car_control.LV_Bat_voltage = analogs.LV_bat - 0.6;

	}
	else
	{
		car_control.LV_Bat_voltage = analogs.LV_bat - 0.19;

	}




	last_analog_time_10us = current_time_10us();

#ifdef DEBUG_PRINT_ANALOGS
	static uint32_t count = 0;
	count++;
	if(count == 3000)
	{
		printf("LV: %lu, P1: %lu, P2: %lu, BRK: %lu\n\r", lv_voltage_avg / ANALOG_BUFFER_SIZE, pedal_1_raw_avg / ANALOG_BUFFER_SIZE, pedal_2_raw_avg / ANALOG_BUFFER_SIZE, brake_raw_avg / ANALOG_BUFFER_SIZE);
		count = 0;
	}
#endif
}//


void handle_fans(void)
{

}

#define INPUT_SAMPLING_TIME_10US 	10
#define DEBOUNCE_COUNT 				128

void handle_inputs(void)
{
	static uint32_t RTD_count = 0;
	static uint32_t IGN_count = 0;
	static uint32_t EMSDC_count = 0;



	// We want to sample the inputs every 100us.
	static uint16_t last_input_time_10us = 0;

	if( (uint16_t)(current_time_10us() - last_input_time_10us) < INPUT_SAMPLING_TIME_10US ) return;

	bool RTD = !HAL_GPIO_ReadPin(GPIOE, RTD_SWITCH_IN_Pin);
	if(RTD == car_control.RTD_Switch) RTD_count = 0;
	else
	{
		RTD_count++;
		if(RTD_count >= DEBOUNCE_COUNT)car_control.RTD_Switch = RTD;
	}


	bool IGN = !HAL_GPIO_ReadPin(GPIOE, IGNITION_IN_Pin);
	if(IGN == car_control.IGN_Switch) IGN_count = 0;
	else
	{
		IGN_count++;
		if(IGN_count >= DEBOUNCE_COUNT)car_control.IGN_Switch = IGN;
	}

	bool EMSDC = !HAL_GPIO_ReadPin(GPIOE, EMSDC_IN_Pin);
	if(EMSDC == car_control.EMSDC) EMSDC_count = 0;
	else
	{
		EMSDC_count++;
		if(EMSDC_count >= DEBOUNCE_COUNT) car_control.EMSDC = EMSDC;
	}

	last_input_time_10us = current_time_10us();

#ifdef DEBUG_PRINT_INPUTS
	static uint32_t count = 0;
	count++;
	if(count == 10000/INPUT_SAMPLING_TIME_10US)
	{
		printf("RTD: %u, IGN: %u EMSDC: %u\n\r", car_control.RTD_Switch, car_control.IGN_Switch, car_control.EMSDC);
		count = 0;
	}
#endif

#ifdef DEBUG_PRINT_COMMS
	static uint32_t count_2 = 0;
	count_2++;
	if(count_2 == 10000/INPUT_SAMPLING_TIME_10US)
	{
		printf("INV1: %u, INV2: %u BMS: %u\n\r", comms_active.inv1 , comms_active.inv2, comms_active.bms);
		count_2 = 0;
	}
#endif
}

