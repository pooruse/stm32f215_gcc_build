
/*********************************************************************
改Boot Loader 流程:
2. 呼叫 IAP_Init();
3. 修改Memory Areas ROM 為 0x8005000
4. 設定自動產生.bin。

!!請至IGS_STM32_IAP.h 修改韌體版本序號!!

@version	V1.0
@date			2014-12-02
@author		Lin,ChihYuan
*********************************************************************/

#include <string.h>
#include "stm32f2xx.h"
#include "flash_if.h"
#include "IGS_STM32_IAP_APP.h"


static uint8_t FirmwareVer[VERSION_LENGTH] = VERSION;


void IAP_COM_IRQHandler(void)
{
	uint32_t for_tmp;
	
	if (USART_GetITStatus(IAP_COM, USART_IT_RXNE) == SET) {
		switch(USART_ReceiveData(IAP_COM)){
			case CMD_Return_Ver:
				DMA_Cmd(IAP_TX_DMA_STREAM, ENABLE);
			break;
			
			case CMD_RunPROG:
				FLASH_If_Init();
				
				FLASH_If_DisableWriteProtection();
				FLASH_ProgramWord(APPLICATION_ADDRESS, 0x00000000);
				
				for(for_tmp=0;for_tmp<100000;for_tmp++);
				
				NVIC_SystemReset();
				
			break;
		}
	}
}

/***********************************************************
  * @brief  IAP_Init
  * @param  None
  * @retval None
  */
void IAP_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	
	#ifdef DEBUG
		NVIC_SetVectorTable(NVIC_VectTab_FLASH, (0x08000000&0x0000FFFF));
	#else
		NVIC_SetVectorTable(NVIC_VectTab_FLASH, (APPLICATION_ADDRESS&0x0000FFFF));
	#endif
	
	/* Enable GPIO clock */
	RCC_AHB1PeriphClockCmd(IAP_TX_GPIO_CLK, ENABLE);
	
	/* Enable UART clock */
#ifdef IAP_COM_USART1
	RCC_APB2PeriphClockCmd(IAP_CLK, ENABLE);
#else
	RCC_APB1PeriphClockCmd(IAP_CLK, ENABLE);
#endif
	
	/* Enable DMA clock */
	RCC_AHB1PeriphClockCmd(IAP_DMA_CLK, ENABLE);
	
	/* Connect Tx*/
	GPIO_PinAFConfig(IAP_TX_GPIO_PORT, IAP_TX_SOURCE, IAP_TX_AF);
	/* Connect Rx*/
	GPIO_PinAFConfig(IAP_RX_GPIO_PORT, IAP_RX_SOURCE, IAP_RX_AF);

	/* Configure IAP_COM Tx as alternate function  */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	
	GPIO_InitStructure.GPIO_Pin = IAP_TX_PIN;
	GPIO_Init(IAP_TX_GPIO_PORT, &GPIO_InitStructure);
	/* Configure USART Rx as alternate function  */
	GPIO_InitStructure.GPIO_Pin = IAP_RX_PIN;
	GPIO_Init(IAP_RX_GPIO_PORT, &GPIO_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = IAP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(IAP_COM, &USART_InitStructure);
	USART_ITConfig(IAP_COM, USART_IT_RXNE, ENABLE);
	USART_Cmd(IAP_COM, ENABLE);
	USART_DMACmd(IAP_COM, USART_DMAReq_Tx,ENABLE);
	
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	
	DMA_DeInit(IAP_TX_DMA_STREAM);
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&IAP_COM->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)FirmwareVer;
	DMA_InitStructure.DMA_Channel = IAP_TX_DMA_CHANNEL;
	DMA_InitStructure.DMA_BufferSize = VERSION_LENGTH;
	DMA_Init(IAP_TX_DMA_STREAM, &DMA_InitStructure);
	DMA_ITConfig(IAP_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
	DMA_Cmd(IAP_TX_DMA_STREAM, DISABLE);
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = IAP_TX_DMA_RX_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * @bref IAP_TX_DMA_TX_IRQHandler
 */
void IAP_TX_DMA_TX_IRQHandler(void)
{
	if(DMA_GetITStatus(IAP_TX_DMA_STREAM,IAP_TX_IT_TCIF))    
	{
		DMA_ClearITPendingBit(IAP_TX_DMA_STREAM,IAP_TX_IT_TCIF);
	}
}
