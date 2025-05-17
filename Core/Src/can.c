/*
 * can.c
 *
 *  Created on: May 3, 2025
 *      Author: georg
 */

#include "main.h"
#include "app.h"
#include "inverter_control.h"
#include "timer.h"
#include "precharge.h"
#include "can.h"
#include "globals.h"

//CAN receive Ids
#define CAN_ID_PEDAL_BOARD	0x100
#define CAN_ID_BMS_CELL_BROADCAST 0x6B0
#define CAN_ID_RELAY_STATE 0x009
#define CAN_ID_AUX_STATES		0x900
#define CAN_ID_RTD          0x469
#define CAN_ID_IGNITION     0x500
#define CAN_ID_BMS_DLC		0x6B1
#define CAN_ID_STEERING_SENSOR  0x101
#define CAN_ID_SBG_IMU_DELTA_ANGLE 0x124
#define CAN_ID_SBG_IMU_DELTA_VEL    0x123
#define CAN_ID_SBG_EKF_VEL_BODY 0x139
#define CAN_ID_SBG_EKF_VEL_NED_ACC  0x138

#define CAN_ID_INV1_HS1		(uint32_t)((PGN_HS1 << 8) + (0xFF << 8) + INV_RIGHT_ADDRESS)	//0x118ff71
#define CAN_ID_INV1_HS2		(uint32_t)((PGN_HS2 << 8) + (0xFF << 8) + INV_RIGHT_ADDRESS)	//0x119ff71
#define CAN_ID_INV1_HS3		(uint32_t)((PGN_HS3 << 8) + (0xFF << 8) + INV_RIGHT_ADDRESS)	//0x11aff71
#define CAN_ID_INV1_HS4		(uint32_t)((PGN_HS4 << 8) + (0xFF << 8) + INV_RIGHT_ADDRESS)	//0x11bff71

#define CAN_ID_INV2_HS1		(uint32_t)((PGN_HS1 << 8) + (0xFF << 8) + INV_LEFT_ADDRESS)	//0x118ff72
#define CAN_ID_INV2_HS2		(uint32_t)((PGN_HS2 << 8) + (0xFF << 8) + INV_LEFT_ADDRESS)	//0x119ff72
#define CAN_ID_INV2_HS3		(uint32_t)((PGN_HS3 << 8) + (0xFF << 8) + INV_LEFT_ADDRESS)	//0x11aff72
#define CAN_ID_INV2_HS4		(uint32_t)((PGN_HS4 << 8) + (0xFF << 8) + INV_LEFT_ADDRESS)	//0x11bff72

																				//0x192cff71
																				//0x1928ff72
//CAN send Ids
#define CAN_ID_TX_TO_BMS    0x008
#define CAN_ID_TX_STATUS        0x7A4





static volatile bool rtd_startup_flag = false;

union {
    int16_t i;
    uint8_t bytes[2];
} int16_bytes_converter;


void handle_can_tx(void);
void handle_can_rx(void);


void handle_can(void)
{
	handle_can_tx();
	handle_can_rx();
}

void handle_can_rx(void)
{
	//Handle RX
	if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0)
	{
		uint8_t rxData[16];
		CAN_RxHeaderTypeDef rxHeader;
		HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData);


		if(rxHeader.IDE)	//=============== EXT IDS ======================
		{
			uint32_t cropped_id = rxHeader.ExtId & 0x01FFFFFF;

			switch(cropped_id)
			{
				case CAN_ID_INV1_HS1:
					parse_HS1(&inv1, rxData);
					comms_time.inv1 = current_time_ms();
					break;

				case CAN_ID_INV1_HS2:
					parse_HS2(&inv1, rxData);
					comms_time.inv1 = current_time_ms();
					break;

				case CAN_ID_INV1_HS3:
					parse_HS3(&inv1, rxData);
					comms_time.inv1 = current_time_ms();
					break;

				case CAN_ID_INV2_HS1:
					parse_HS1(&inv2, rxData);
					comms_time.inv2 = current_time_ms();
					break;

				case CAN_ID_INV2_HS2:
					parse_HS2(&inv2, rxData);
					comms_time.inv2 = current_time_ms();
					break;

				case CAN_ID_INV2_HS3:
					parse_HS3(&inv2, rxData);
					comms_time.inv2 = current_time_ms();
					break;

				default:
					break;
			}
		}
		else	//=============== STD IDS ======================
		{
			switch(rxHeader.StdId)
			{
				case CAN_ID_STEERING_SENSOR:
					comms_time.steering = current_time_ms();
					float measurement = (rxData[0]*256 + rxData[1] - 440)/390.0f;
					car_control.user_steering_value = 0.18f*measurement + car_control.user_steering_value*0.82f;
					break;

				case CAN_ID_BMS_CELL_BROADCAST:
					bms.voltage = (((uint16_t)rxData[2] << 8) + rxData[3])/10.0f;
					bms.current = (rxData[0]*256 + rxData[1])/10.0f;
					bms.SOC = rxData[4];
					if(rxData[5] >> 7 == 0)
					{
						bms.ams_precharge_enabled = true;
					}
					else{
						bms.ams_precharge_enabled = false;
					}

					comms_time.bms = current_time_ms();
					break;

				case CAN_ID_BMS_DLC:

					bms.pack_dlc = 0;
					bms.pack_dlc = (uint16_t)rxData[0] << 8;
					bms.pack_dlc += rxData[1];

					bms.high_temp = rxData[4];
					bms.low_temp = rxData[5];

					comms_time.bms = current_time_ms();

					break;

				case CAN_ID_SBG_IMU_DELTA_ANGLE:
					int16_bytes_converter.bytes[0] = rxData[4];
					int16_bytes_converter.bytes[1] = rxData[5];
					car_control.yaw_rate = (int16_bytes_converter.i)*0.0001f+0.9f*car_control.yaw_rate;
					break;

				case CAN_ID_SBG_IMU_DELTA_VEL:
					int16_bytes_converter.bytes[0] = rxData[0];
					int16_bytes_converter.bytes[1] = rxData[1];
					car_control.a_x = (int16_bytes_converter.i)*0.01f;
					break;

				case CAN_ID_SBG_EKF_VEL_BODY:
					int16_bytes_converter.bytes[0] = rxData[0];
					int16_bytes_converter.bytes[1] = rxData[1];
					car_control.v_x = (int16_bytes_converter.i)*0.01f;
					break;

				case CAN_ID_SBG_EKF_VEL_NED_ACC:
					int16_bytes_converter.bytes[0] = rxData[0];
					int16_bytes_converter.bytes[1] = rxData[1];
					float v_n_acc = int16_bytes_converter.i*0.01f;
					int16_bytes_converter.bytes[0] = rxData[2];
					int16_bytes_converter.bytes[1] = rxData[3];
					float v_e_acc = int16_bytes_converter.i*0.01f;
					car_control.v_acc = (v_n_acc+v_e_acc)/2;
					break;

				default:
					break;
			}
		}
	}
}

void handle_can_tx(void)
{
	// If we don't have 2 mailbox's free, then don't try and send anything
	// Inverter messages sends two messages, so must have 2 mailbox's free

	uint32_t mailbox_free = CAN_TX_NUM_MAILBOX_FREE(&hcan1);


	if(mailbox_free < 2)
	{
		return;
	}

	if(tx_ready.hc1)
	{
		switch(car_control.inverter_state)
		{
		case INV_ENABLE:
			update_HC1_drive();
			break;
		case INV_ENERGISE:
			update_HC1_energise();
			break;
		case INV_SHUTDOWN:
			update_HC1_shutdown();
			break;
		}
		tx_time.hc1 = current_time_ms();
	}
	else if(tx_ready.hc2)
	{
		update_HC2();
		tx_time.hc2 = current_time_ms();
	}
	else if(tx_ready.hc3)
	{
		update_HC3();
		tx_time.hc3 = current_time_ms();
	}
	else if(tx_ready.bms)	//If we are ready to send a message to the BMS
	{
		uint8_t data[1] = {0};

		if(bms.precharge_enable) data[0] = 1;

		CAN_TxHeaderTypeDef header = {
				.StdId = CAN_ID_TX_TO_BMS,
				.ExtId = 0,
				.IDE = CAN_ID_STD,
				.RTR = CAN_RTR_DATA,
				.DLC = 1,
				.TransmitGlobalTime = DISABLE
			};

		uint32_t mailbox;

		HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox);

		tx_time.bms = current_time_ms();
	}
	else if(tx_ready.status)		//If we are ready to send a message to the dash
	{
		/*
		tx_time.status = current_time_ms();

		uint8_t status_data[] =
			{
			ass.break_loop_inverter_error,
			ass.break_loop_precharge,
			ass.break_loop_timeout,
			ass.break_loop_ts_deactive,
			car_control.ins_error_code,
			0,
			0,
			(comms_active_snapshot.bms<<5)
					+(comms_active_snapshot.dash<<4)+(comms_active_snapshot.inv1<<3)
					+(comms_active_snapshot.inv2<<2)+(comms_active_snapshot.pb<<1)
					+comms_active_snapshot.steering};

		uint32_t mailbox;

		HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox);
		*/
		tx_time.status = current_time_ms();
	}
}
