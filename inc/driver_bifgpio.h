#ifndef _BIF_H
#define _BIF_H

#include "stm32f2xx.h"

#define BIF_GPIO_PIN              GPIO_Pin_8
#define BIF_GPIO_PORT             GPIOC
#define BIF_GPIO_CLK              RCC_AHB1Periph_GPIOC
#define BIF_GPIO_CLKCMD           RCC_AHB1PeriphClockCmd
#define BIF_GPIO_PIN_SOURCE       GPIO_PinSource8

#define BIF_MODE_INPUT            0
#define BIF_MODE_OUTPUT           1

void DRIVER_BIF_Config (void);
void DRIVER_BIF_Mode_Set (uint8_t mode);

void DRIVER_BIF_SetHigh (void);
void DRIVER_BIF_SetLow (void);
void DRIVER_BIF_Toggle (void);
uint8_t DRIVER_BIF_GetValue (void);

#endif
