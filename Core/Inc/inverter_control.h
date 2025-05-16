#ifndef INVERTER_CONTROL_H
#define INVERTER_CONTROL_H

#include "globals.h"
#include "inverter.h"


void update_HC1_drive(void);
void update_HC1_energise(void);
void update_HC1_shutdown(void);
void update_HC2(void);
void update_HC3(void);

void parse_HS1(inv_t* inv, uint8_t data[]);
void parse_HS2(inv_t* inv, uint8_t data[]);
void parse_HS3(inv_t* inv, uint8_t data[]);


#endif
