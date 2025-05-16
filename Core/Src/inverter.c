#include <string.h>

#include "inverter_control.h"
#include "globals.h"
#include "inverter.h"
#include "timer.h"
#include "app.h"

//==========================DEFINITIONS

// How often we send messages to the inverter (ms))


//==========================GLOBAL VAR
inv_t inv1;
inv_t inv2;
//==========================LOCAL VAR

//==========================LOCAL FUN DECLARATIONS

//=========================GLOBAL FUNCTIONS

void handle_inverters(void)
{

	// If comms have broken to the inverters, set the voltage to default (0))
	if (!comms_active.inv1)
		inv1.capacitor_voltage = 0;

	if (!comms_active.inv2)
		inv2.capacitor_voltage = 0;

	//If any inverters report a fault, stop the car.
	inv1.fault_active = (inv1.statusword == STATUSWORD_FAULTREACTION);
	inv2.fault_active = (inv2.statusword == STATUSWORD_FAULTREACTION);
}

float get_inv_lowest_voltage(void)
{
	if (inv1.capacitor_voltage < inv2.capacitor_voltage)
		return inv1.capacitor_voltage;
	else
		return inv2.capacitor_voltage;
}

//=========================LOCAL FUNCTIONS======================================
