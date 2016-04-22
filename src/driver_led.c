#include "stm32f2xx.h"
#include "driver_led.h"

void DRIVER_LED_On (void) 
{
  GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
}

void DRIVER_LED_Off (void) 
{
  GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
}

void DRIVER_LED_Toggle (void) 
{
  GPIO_ToggleBits (LED_GPIO_PORT, LED_GPIO_PIN);
}

void DRIVER_LED_Config (void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  LED_GPIO_CLKCMD(LED_GPIO_CLK, ENABLE);
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStruct.GPIO_Pin = LED_GPIO_PIN;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
  
  DRIVER_LED_On();
}	
