#ifndef _LED_H
#define _LED_H

#include <stdio.h>
#include "stm32f2xx.h"

#define LED_GPIO_PIN              GPIO_Pin_14
#define LED_GPIO_PORT             GPIOB
#define LED_GPIO_CLK              RCC_AHB1Periph_GPIOB
#define LED_GPIO_CLKCMD           RCC_AHB1PeriphClockCmd
#define LED_SOURCE                GPIO_PinSource14

void DRIVER_LED_On (void);
void DRIVER_LED_Off (void);
void DRIVER_LED_Toggle (void);
void DRIVER_LED_Config (void);

#endif /*_LED_H*/
