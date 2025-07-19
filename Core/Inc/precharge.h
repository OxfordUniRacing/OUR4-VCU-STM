#ifndef PRECHARGE_H
#define	PRECHARGE_H

#include <stdbool.h>

typedef enum{
	PC_TS_OFF,                      //The TS is not active
	PC_EMSDC_ON,                   //For handling the BMS relay
	PC_WAIT_FOR_INVERTER,           //Waiting for communication from the inverters
	PC_WAIT_FOR_INVERTER_ENERGISE,  //Waiting fot the inverters to enter "energised" state
	PC_WAIT_FOR_INVERTER_ENABLED,   //Waiting for the inverters to enter "enabled" state
	PC_WAIT_FOR_FINAL_VOLTAGE,      //Waiting for the inverter voltage to reach 95% of battery voltage
	PC_WAIT_FOR_PRECHARGE_DISABLE,
	PC_READY,                       //Ready to for BMS to exit precharge
	PC_FAILED
}precharge_t;

extern precharge_t PRECHARGE_STATE;

typedef struct {
	// Inputs
	float voltage;
	float current;
	bool ams_precharge_enabled;


	// Outputs
	bool precharge_enable;
	uint16_t pack_dlc;	// Max current the battery can output
	uint16_t high_temp;
	uint16_t low_temp;
	uint16_t SOC;
} bms_t;

extern bms_t bms;

typedef struct {
	// Turn off VCU ass relay conditions
	bool break_loop_precharge;      //True when we want to break ASS due to precharge
	bool break_loop_ins_detect;
	bool break_loop_ts_deactive;    //True when we want to break ASS due to TS cutting out
	bool break_loop_inverter_error; //True when we want to break ASS due to an inverter error
	bool break_loop_timeout;
	bool break_loop_pedal_invalid;


	bool precharge_request_close;	//True if the precharge requests that the ass loop be closed
} ass_t;

extern ass_t ass;

void handle_precharge(void);

#endif /* PRECHARGE_H */
