#ifndef PRECHARGE_H
#define	PRECHARGE_H

#include <stdbool.h>

typedef struct {
	// Inputs
	float voltage;
	float current;
	bool ams_precharge_enabled;

	uint16_t pack_dlc;	// Max current the battery can output

	// Outputs
	bool precharge_enable;
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
