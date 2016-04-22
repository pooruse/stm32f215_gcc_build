#ifndef _TIMER_H
#define _TIMER_H

#include "stm32f2xx.h"

typedef enum {
	BIF_STATE_IDLE, 
	BIF_STATE_BEGIN,
	BIF_STATE_SENDING,
	BIF_STATE_POLL_INT, 
	BIF_STATE_POLL_PKT, 
	BIF_STATE_TIMEOUT
} BIF_STATE;

typedef enum {
	TASK_TYPE_SEND_ONLY, 
	TASK_TYPE_POLL_INT, 
	TASK_TYPE_POLL_PKT
} TASK_TYPE;

void DRIVER_TIMER_Config(void);
void DRIVER_TIMER_Delayus(uint32_t delay_us);

uint8_t DRIVER_TIMER_GetTau(void);
uint8_t DRIVER_TIMER_SetTau(uint8_t TauValue);

BIF_STATE DRIVER_TIMER_GetCurrState(void);
BIF_STATE DRIVER_TIMER_SetTask(uint32_t pktBits, TASK_TYPE taskType);
uint32_t* DRIVER_BIF_GetPollResults(void);
#endif
