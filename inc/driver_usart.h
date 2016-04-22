#ifndef _USART_H
#define _USART_H

#include <stdio.h>
#include "stm32f2xx.h"

#define TSTPORT_COM                        USART3
#define TSTPORT_COM_CLK                    RCC_APB1Periph_USART3
#define TSTPORT_COM_CLKCMD                 RCC_APB1PeriphClockCmd
#define TSTPORT_COM_BAUDRATE               115200    
#define TSTPORT_COM_IRQn                   USART3_IRQn

#define TSTPORT_COM_TX_PIN                 GPIO_Pin_10
#define TSTPORT_COM_TX_GPIO_PORT           GPIOB
#define TSTPORT_COM_TX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define TSTPORT_COM_TX_GPIO_CLKCMD         RCC_AHB1PeriphClockCmd
#define TSTPORT_COM_TX_SOURCE              GPIO_PinSource10
#define TSTPORT_COM_TX_AF                  GPIO_AF_USART3

#define TSTPORT_COM_RX_PIN                 GPIO_Pin_11
#define TSTPORT_COM_RX_GPIO_PORT           GPIOB
#define TSTPORT_COM_RX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define TSTPORT_COM_RX_GPIO_CLKCMD         RCC_AHB1PeriphClockCmd
#define TSTPORT_COM_RX_SOURCE              GPIO_PinSource11
#define TSTPORT_COM_RX_AF                  GPIO_AF_USART3

void DRIVER_USART_Config(void);
void DRIVER_USART_NVIC_Config(void);
uint8_t DRIVER_USART_GetChar(void);
int DRIVER_USART_GetLine(uint8_t *dataBuf, int maxLength);
#endif /*_USART_H*/
