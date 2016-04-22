/**
 * @file igs_timer.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IKV Device IGS example
 */
 
#include "stm32f2xx.h"
#include "igs_timer.h"
#include "igs_task.h"

static volatile uint32_t IGS_TIMER_Ticks;

/**
 * @brief igs_timer_init
 *    igs timer driver init
 * @param
 * @return
 */
void igs_timer_init()
{
  NVIC_InitTypeDef        NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Prescaler = 6 - 1;  // 60 MHz /  6 = 10 MHz
  TIM_TimeBaseStructure.TIM_Period = 10000 - 1;    // 10 MHz / 10000 = 1 KHz --> 1 ms
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);

  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
  
  TIM_Cmd(TIM2, ENABLE);
	IGS_TIMER_Ticks = 0;
}

/**
 * @brief TIM2_IRQHandler
 *    igs timer handler
 * @param
 * @return
 */
void TIM2_IRQHandler (void)
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	igs_task_main(IGS_TASK_ROUTINE_INT);
  IGS_TIMER_Ticks++;
}

/**
 * @brief igs_get_time_tick
 *    get timer ticks
 * @param
 * @return
 */
uint32_t igs_get_time_tick()
{
	return IGS_TIMER_Ticks;
}
