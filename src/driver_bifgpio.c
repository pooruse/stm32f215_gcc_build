#include "stm32f2xx.h"
#include "driver_bifgpio.h"
#include "driver_timer.h"
#include <stdio.h>

static GPIO_InitTypeDef DRIVER_BIF_GPIO_InitStruct = 
{
  BIF_GPIO_PIN,
  GPIO_Mode_OUT,
	GPIO_Speed_25MHz,
	GPIO_OType_PP,
  GPIO_PuPd_UP
};

void DRIVER_BIF_Config (void)
{ 
	/* GPIO setup */
  BIF_GPIO_CLKCMD(BIF_GPIO_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  DRIVER_BIF_Mode_Set(BIF_MODE_INPUT);
}

void DRIVER_BIF_Mode_Set (uint8_t mode)
{
	/* NVIC Config */
  if (mode == BIF_MODE_INPUT) {
		DRIVER_BIF_GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
  } else if (mode == BIF_MODE_OUTPUT) {
    DRIVER_BIF_GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  }
	GPIO_Init(BIF_GPIO_PORT, &DRIVER_BIF_GPIO_InitStruct);
}

void DRIVER_BIF_SetHigh (void)
{
  GPIO_SetBits(BIF_GPIO_PORT, BIF_GPIO_PIN);
}
void DRIVER_BIF_SetLow (void)
{
  GPIO_ResetBits(BIF_GPIO_PORT, BIF_GPIO_PIN);
}
void DRIVER_BIF_Toggle (void)
{
  GPIO_ToggleBits(BIF_GPIO_PORT, BIF_GPIO_PIN);
}
uint8_t DRIVER_BIF_GetValue (void)
{
  return GPIO_ReadInputDataBit(BIF_GPIO_PORT, BIF_GPIO_PIN);
}
