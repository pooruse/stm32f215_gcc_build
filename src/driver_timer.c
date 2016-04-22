#include "stm32f2xx.h"
#include "driver_timer.h"
#include "driver_bifgpio.h"

#define DEFAULT_TAU_VALUE       (4)
#define TAU_MIN									(2)
#define TAU_MAX									(153)
#define PKT_TIMEOUT							(5 * DRIVER_TIMER_Tau + 20)
#define INT_TIMEOUT							(8 * DRIVER_TIMER_Tau)
#define PKT_TIMINGS							(18)

/* Accumulative tick counter */
static volatile uint32_t DRIVER_TIMER_Ticks;
static uint8_t DRIVER_TIMER_Tau = DEFAULT_TAU_VALUE;


/* For State Control */
static BIF_STATE	DRIVER_TIMER_Curr_State = BIF_STATE_IDLE;
static BIF_STATE	DRIVER_TIMER_Poll_State = BIF_STATE_IDLE;
/* For Send Pkt */
static uint8_t  	DRIVER_TIMER_SndPkt_Count = 0;
static uint32_t 	DRIVER_TIMER_SndPkt_Bits = 0;
static uint32_t 	DRIVER_TIMER_SndPkt_NextTriggerTick = 0;
/* For Poll Result */
static uint8_t  	DRIVER_TIMER_Poll_BusState = 0;
static uint8_t  	DRIVER_TIMER_Poll_Count = 0;
static uint32_t 	DRIVER_TIMER_Poll_Buffer[PKT_TIMINGS + 1] = {0};


static void preparePolling()
{
	DRIVER_BIF_Mode_Set (BIF_MODE_INPUT);
	
	DRIVER_TIMER_Poll_BusState = 1;
	DRIVER_TIMER_Poll_Count = 0;
	DRIVER_TIMER_Poll_Buffer[DRIVER_TIMER_Poll_Count++] = DRIVER_TIMER_Ticks;

	DRIVER_TIMER_Curr_State = DRIVER_TIMER_Poll_State;
}

static void toggleAndSetNextTriggerTick(void)
{
	uint32_t  nextbit;
	// toggle
	DRIVER_BIF_Toggle();
	DRIVER_TIMER_SndPkt_Count++;
	
	// start polling
	if (DRIVER_TIMER_SndPkt_Count == PKT_TIMINGS) {
		preparePolling();
	} else {
		// get next bit
		nextbit = (DRIVER_TIMER_SndPkt_Bits & 0x80000000);
		DRIVER_TIMER_SndPkt_Bits <<= 1;

		// set next trigger tick
		DRIVER_TIMER_SndPkt_NextTriggerTick = DRIVER_TIMER_Ticks + (nextbit ? 3 * DRIVER_TIMER_Tau : DRIVER_TIMER_Tau);
	}
}
BIF_STATE DRIVER_TIMER_GetCurrState(void)
{
	return DRIVER_TIMER_Curr_State;
}
BIF_STATE DRIVER_TIMER_SetTask(uint32_t pktBits, TASK_TYPE taskType)
{
	if (DRIVER_TIMER_Curr_State == BIF_STATE_IDLE || DRIVER_TIMER_Curr_State == BIF_STATE_TIMEOUT) {
		switch (taskType) {
			case TASK_TYPE_SEND_ONLY: {
				DRIVER_TIMER_Poll_State = BIF_STATE_IDLE;
				break;
			}
			case TASK_TYPE_POLL_INT: {
				DRIVER_TIMER_Poll_State = BIF_STATE_POLL_INT;
				break;
			}
			case TASK_TYPE_POLL_PKT: {
				DRIVER_TIMER_Poll_State = BIF_STATE_POLL_PKT;
				break;
			}
			default : {
				return BIF_STATE_IDLE;
			}
		}
		DRIVER_BIF_Mode_Set (BIF_MODE_OUTPUT);
		DRIVER_TIMER_SndPkt_Bits = pktBits;
		DRIVER_TIMER_SndPkt_Count = 0;
		DRIVER_TIMER_Curr_State = BIF_STATE_BEGIN;
		return BIF_STATE_BEGIN;
	} else {
		return DRIVER_TIMER_Curr_State;
	}
}




uint32_t* DRIVER_BIF_GetPollResults(void) 
{
	return DRIVER_TIMER_Poll_Buffer;
}
uint8_t DRIVER_TIMER_GetTau(void)
{
	return DRIVER_TIMER_Tau;
}

uint8_t DRIVER_TIMER_SetTau(uint8_t TauValue)
{
	DRIVER_TIMER_Tau = ((TauValue - TAU_MIN) % (TAU_MAX - TAU_MIN + 1)) + TAU_MIN;
	return DRIVER_TIMER_Tau;
}
void DRIVER_TIMER_Delayus(uint32_t delay_us)
{
	uint32_t tickStart;
	tickStart = DRIVER_TIMER_Ticks;
	while(DRIVER_TIMER_Ticks - tickStart < delay_us);
}

void DRIVER_TIMER_Config(void)
{
  NVIC_InitTypeDef        NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Prescaler = 6 - 1;  // 60 MHz /  6 = 1 MHz
  TIM_TimeBaseStructure.TIM_Period = 10 - 1;    // 10 MHz / 10 = 1 MHz --> 1 us
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM7,&TIM_TimeBaseStructure);
	
  TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
  
  TIM_Cmd(TIM7, ENABLE);
  DRIVER_TIMER_Ticks = 0;
}



void TIM7_IRQHandler (void)
{
  TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
  DRIVER_TIMER_Ticks++;
	switch (DRIVER_TIMER_Curr_State) {
		case BIF_STATE_IDLE: 			{ break; }
		case BIF_STATE_BEGIN: 		{
			toggleAndSetNextTriggerTick();
			DRIVER_TIMER_Curr_State = BIF_STATE_SENDING;
			break;
		}
		case BIF_STATE_SENDING: 	{
			if (DRIVER_TIMER_Ticks == DRIVER_TIMER_SndPkt_NextTriggerTick) {
				toggleAndSetNextTriggerTick();
			}
			break;
		}
		case BIF_STATE_POLL_INT: 	{
			uint8_t BusNewState = DRIVER_BIF_GetValue();
			if (BusNewState != DRIVER_TIMER_Poll_BusState) {
				DRIVER_TIMER_Poll_BusState = BusNewState;
				DRIVER_TIMER_Poll_Buffer[DRIVER_TIMER_Poll_Count++] = DRIVER_TIMER_Ticks;
				if (DRIVER_TIMER_Poll_Count == 2) {
					DRIVER_TIMER_Curr_State = BIF_STATE_IDLE;
				}
			} else if (DRIVER_TIMER_Ticks - DRIVER_TIMER_Poll_Buffer[DRIVER_TIMER_Poll_Count - 1] > INT_TIMEOUT) {
				DRIVER_TIMER_Curr_State = BIF_STATE_TIMEOUT;
			}
			break;
		}
		case BIF_STATE_POLL_PKT: 	{
			uint8_t BusNewState = DRIVER_BIF_GetValue();
			if (BusNewState != DRIVER_TIMER_Poll_BusState) {
				DRIVER_TIMER_Poll_BusState = BusNewState;
				DRIVER_TIMER_Poll_Buffer[DRIVER_TIMER_Poll_Count++] = DRIVER_TIMER_Ticks;
				if (DRIVER_TIMER_Poll_Count == PKT_TIMINGS + 1) {
					DRIVER_TIMER_Curr_State = BIF_STATE_IDLE;
				}
			} else if (DRIVER_TIMER_Ticks - DRIVER_TIMER_Poll_Buffer[DRIVER_TIMER_Poll_Count - 1] > PKT_TIMEOUT) {
				DRIVER_TIMER_Curr_State = BIF_STATE_TIMEOUT;
			}
			break;
		}
		default: {
			break;
		}
	}
}
