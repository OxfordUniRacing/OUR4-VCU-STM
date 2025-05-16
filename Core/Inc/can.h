/*
 * can.h
 *
 *  Created on: May 3, 2025
 *      Author: georg
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

void handle_can(void);

#define CAN_TX_MAILBOX_FREE(hcan) \
    ((hcan)->Instance->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))

#define CAN_TX_NUM_MAILBOX_FREE(hcan) \
		((((hcan)->Instance->TSR & CAN_TSR_TME0) >> CAN_TSR_TME0_Pos) + (((hcan)->Instance->TSR & CAN_TSR_TME1) >> CAN_TSR_TME1_Pos) + (((hcan)->Instance->TSR & CAN_TSR_TME2) >> CAN_TSR_TME2_Pos))




#endif /* INC_CAN_H_ */
