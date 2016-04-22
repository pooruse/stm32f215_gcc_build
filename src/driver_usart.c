#include <stdio.h>
#include "stm32f2xx.h"
#include "driver_usart.h"

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

void DRIVER_USART_Config(void)
{ 												
  GPIO_InitTypeDef      GPIO_InitStructure;
  USART_InitTypeDef     USART_InitStructure; 

  /* Config TX Pin*/
  TSTPORT_COM_TX_GPIO_CLKCMD(TSTPORT_COM_TX_GPIO_CLK, ENABLE);
  GPIO_PinAFConfig(TSTPORT_COM_TX_GPIO_PORT, TSTPORT_COM_TX_SOURCE, TSTPORT_COM_TX_AF);
  GPIO_InitStructure.GPIO_Pin = TSTPORT_COM_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(TSTPORT_COM_TX_GPIO_PORT, &GPIO_InitStructure);

  /* Config RX Pin*/
  TSTPORT_COM_RX_GPIO_CLKCMD(TSTPORT_COM_RX_GPIO_CLK, ENABLE);
  GPIO_PinAFConfig(TSTPORT_COM_RX_GPIO_PORT, TSTPORT_COM_RX_SOURCE, TSTPORT_COM_RX_AF);
  GPIO_InitStructure.GPIO_Pin = TSTPORT_COM_RX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(TSTPORT_COM_RX_GPIO_PORT, &GPIO_InitStructure);

  /* Config USART Module */
  TSTPORT_COM_CLKCMD(TSTPORT_COM_CLK, ENABLE);
  USART_InitStructure.USART_BaudRate = TSTPORT_COM_BAUDRATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(TSTPORT_COM, &USART_InitStructure);
  USART_Cmd(TSTPORT_COM, ENABLE);
}

void DRIVER_USART_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Config the USART3 Interrupt */
  USART_ITConfig(TSTPORT_COM, USART_IT_RXNE, ENABLE);
  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TSTPORT_COM_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

}

uint8_t DRIVER_USART_GetChar(void)
{
  while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET);
  return (uint8_t) USART_ReceiveData(TSTPORT_COM);
}

int DRIVER_USART_GetLine(uint8_t *dataBuf, int maxLength)
{
  int i;
  for(i = 0; i < maxLength; i++) {
    dataBuf[i] = DRIVER_USART_GetChar();
    
    if(dataBuf[i] == '\r') {
      printf("\r\n");
      break;
    } else {
      printf("%c", dataBuf[i]);
    }
  }
  dataBuf[i] = 0;
  return i;
}

/**
  * @brief  Retargets the C library printf function to the USART3.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  while (USART_GetFlagStatus(TSTPORT_COM, USART_FLAG_TXE) == RESET){}
  USART_SendData(TSTPORT_COM, (uint8_t) ch);
  return ch;
}
