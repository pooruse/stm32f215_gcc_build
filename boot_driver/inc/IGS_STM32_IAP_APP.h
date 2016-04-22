
/*********************************************************************
可修改內容:
1. FirmwareVer: 修改韌體版本序號。
2. 設定 IAP COM Port。

@version	V1.0
@date			2014-12-02
@author		Lin,ChihYuan
*********************************************************************/

#ifndef IGS_STM32_IAP_APP_H
#define IGS_STM32_IAP_APP_H

//Note: VERSION do not exceed VERSION_LENGTH length
#define VERSION	"MH_AQ000_V101"
#define VERSION_LENGTH 20

#define IAP_COM_USART1

#ifdef IAP_COM_USART1
	#define IAP_COM										 USART1
	#define IAP_CLK                    RCC_APB2Periph_USART1
	#define IAP_TX_PIN                 GPIO_Pin_9
	#define IAP_TX_GPIO_PORT           GPIOA
	#define IAP_TX_GPIO_CLK            RCC_AHB1Periph_GPIOA
	#define IAP_TX_SOURCE              GPIO_PinSource9
	#define IAP_TX_AF                  GPIO_AF_USART1
	#define IAP_RX_PIN                 GPIO_Pin_10
	#define IAP_RX_GPIO_PORT           GPIOA
	#define IAP_RX_GPIO_CLK            RCC_AHB1Periph_GPIOA
	#define IAP_RX_SOURCE              GPIO_PinSource10
	#define IAP_RX_AF                  GPIO_AF_USART1
	#define IAP_IRQn                   USART1_IRQn
	#define IAP_COM_IRQHandler				 USART1_IRQHandler
	
	#define IAP_DMA                    DMA2
	#define IAP_DMA_CLK                RCC_AHB1Periph_DMA2
	#define IAP_TX_DMA_CHANNEL         DMA_Channel_4
	#define IAP_TX_DMA_STREAM          DMA2_Stream7
	
	#define IAP_TX_IT_TCIF             DMA_IT_TCIF7
	#define IAP_TX_DMA_RX_IRQn         DMA2_Stream7_IRQn
	#define IAP_TX_DMA_TX_IRQHandler   DMA2_Stream7_IRQHandler
#endif

#ifdef  IAP_COM_USART2
	#define IAP_COM										 USART2
	#define IAP_CLK                    RCC_APB1Periph_USART2
	#define IAP_TX_PIN                 GPIO_Pin_2
	#define IAP_TX_GPIO_PORT           GPIOA
	#define IAP_TX_GPIO_CLK            RCC_AHB1Periph_GPIOA
	#define IAP_TX_SOURCE              GPIO_PinSource2
	#define IAP_TX_AF                  GPIO_AF_USART2
	#define IAP_RX_PIN                 GPIO_Pin_3
	#define IAP_RX_GPIO_PORT           GPIOA
	#define IAP_RX_GPIO_CLK            RCC_AHB1Periph_GPIOA
	#define IAP_RX_SOURCE              GPIO_PinSource3
	#define IAP_RX_AF                  GPIO_AF_USART2
	#define IAP_IRQn                   USART2_IRQn
	#define IAP_COM_IRQHandler				 USART2_IRQHandler
	
	#define IAP_DMA                    DMA1
	#define IAP_DMA_CLK                RCC_AHB1Periph_DMA1
	#define IAP_TX_DMA_CHANNEL         DMA_Channel_4
	#define IAP_TX_DMA_STREAM          DMA1_Stream5
	
	#define IAP_TX_IT_TCIF             DMA_IT_TCIF5
	#define IAP_TX_DMA_RX_IRQn         DMA1_Stream5_IRQn
	#define IAP_TX_DMA_TX_IRQHandler   DMA1_Stream5_IRQHandler
#endif

#ifdef  IAP_COM_USART3
	#define IAP_COM										 USART3
	#define IAP_CLK                    RCC_APB1Periph_USART3
	#define IAP_TX_PIN                 GPIO_Pin_10
	#define IAP_TX_GPIO_PORT           GPIOB
	#define IAP_TX_GPIO_CLK            RCC_AHB1Periph_GPIOB
	#define IAP_TX_SOURCE              GPIO_PinSource10
	#define IAP_TX_AF                  GPIO_AF_USART3
	#define IAP_RX_PIN                 GPIO_Pin_11
	#define IAP_RX_GPIO_PORT           GPIOB
	#define IAP_RX_GPIO_CLK            RCC_AHB1Periph_GPIOB
	#define IAP_RX_SOURCE              GPIO_PinSource11
	#define IAP_RX_AF                  GPIO_AF_USART3
	#define IAP_IRQn                   USART3_IRQn
	#define IAP_COM_IRQHandler				 USART3_IRQHandler
	
	#define IAP_DMA                    DMA1
	#define IAP_DMA_CLK                RCC_AHB1Periph_DMA1
	#define IAP_TX_DMA_CHANNEL         DMA_Channel_4
	#define IAP_TX_DMA_STREAM          DMA1_Stream3
	
	#define IAP_TX_IT_TCIF             DMA_IT_TCIF3
	#define IAP_TX_DMA_RX_IRQn         DMA1_Stream3_IRQn
	#define IAP_TX_DMA_TX_IRQHandler   DMA1_Stream3_IRQHandler
#endif

enum {
	CMD_Return_Ver	= 0xC1,
	CMD_RunPROG			=0xC2,
	
	CMD_ACK		= 0xA3,
	CMD_NACK	= 0xA4,
};

void IAP_Init(void);

#endif
