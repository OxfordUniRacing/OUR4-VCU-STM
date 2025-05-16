#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdint.h>

// ============================= DEBUG DEFINES ===========================

//#define DEBUG_NO_SOUND

// If defined, does not require BMS to be connected to operate
//#define DEBUG_IGNORE_BMS

// If defined, assumes ASS loop is always closed
//#define DEBUG_IGNORE_ASS

// If defined, will assume that the TS is on and has no issues
//#define DEBUG_IGNORE_TS

// If defined, does not requrie ignition to operate
// #define DEBUG_IGNORE_IGNITION

// If defined, assumes brake is always pressed
// #define DEBUG_PERMENANT_BRAKE

// If defined, prints inverter messages to the console
// #define DEBUG_PRINT_INVERTER_MESSAGES

//#define DEBUG_PRINT_LOOP_TIMES

//#define DEBUG_PRINT_INPUTS

//#define DEBUG_PRINT_ANALOGS

// ========================= GLOBAL PARAMETERS ===========================

#define THROTTLE_SCALE_DOWN 10

#define ASS_CLOSED 1
#define ASS_OPEN 0
#define TS_ACTIVE_BOUNCE_TIME 2000
#define RTD_SOUND_TIME 2500
#define ASS_LOOP_STOP_TIME  300
#define BRAKE_PRESSURE_THRESHOLD 2.0f //Threshold to consider the brakes active (BAR)

#define INV_RIGHT_ADDRESS	(0x71)	//INV1
#define INV_LEFT_ADDRESS	(0x72)	//INV2
#define SOURCE_ADDRESS		(0x01)

#define PGN_HC1				(uint32_t)(0x11000)
#define PGN_HC2				(uint32_t)(0x11100)
#define PGN_HC3				(uint32_t)(0x11200)

#define PGN_HS1				(uint32_t)(0x11800)
#define PGN_HS2				(uint32_t)(0x11900)
#define PGN_HS3				(uint32_t)(0x11A00)
#define PGN_HS4				(uint32_t)(0x11B00)

typedef enum {
	STATUSWORD_NOTREADY	= 0x01,
	STATUSWORD_SHUTDOWN = 0x02,
	STATUSWORD_PRECHARGE = 0x04,
	STATUSWORD_ENERGISED = 0x07,
	STATUSWORD_ENABLED = 0x08,
	STATUSWORD_FAULTREACTION = 0x0B,
	STATUSWORD_FAULTOFF = 0x0D
} statusword_t;


enum INVERTER_STATES {
	INV_SHUTDOWN,
	INV_ENERGISE,
	INV_ENABLE,
};

typedef struct
{
	uint16_t LV_bat;
	uint16_t pedal_1;
	uint16_t pedal_2;
	uint16_t brake_sens;
}analogs_t;

typedef struct
{
	//MAIN ONES TO CONSIDER
	enum INVERTER_STATES inverter_state;										//State sent to the inverter

	bool ready;														//True if the car is ready to drive (NOT SWITCH STATE)

	bool precharge_start;
    bool precharge_ready;														//True if the precharge sequence has been completed

    bool IGN_Switch;														//Ignition switch state from the dashboard
    bool RTD_Switch;														//RTD switch state from the dashboard
    bool EMSDC;

    bool RTD;																//True if car is ready to drive

    bool sounder_enable;

    uint8_t ins_error_code;

    bool error_state;

	//User values
	uint8_t user_pedal_value;
    float user_steering_value;
    bool brake_on;																//True when the current brake pressure is above the brake threshold
    float brake_pressure;

    float torque;
    float rpm_limit;

    //car state data
    float yaw_rate;
    float v_x;
    float v_acc;
    float a_x;
}control_t;


#endif /* GLOBALS_H */
