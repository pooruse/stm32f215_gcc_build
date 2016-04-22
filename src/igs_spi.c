/**
 * @file igs_gpio.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS spi driver
 */
 
#include <stdio.h>
#include "stm32f2xx.h"
#include "igs_spi.h"
#include "igs_protocol.h"

//spi initial defines
#define SPIx                           SPI1
#define SPIx_CLK                       RCC_APB2Periph_SPI1
#define SPIx_CLK_INIT                  RCC_APB2PeriphClockCmd
#define SPIx_IRQn                      SPI1_IRQn
#define SPIx_IRQHANDLER                SPI1_IRQHandler

#define SPIx_SCK_PIN                   GPIO_Pin_5
#define SPIx_SCK_GPIO_PORT             GPIOA
#define SPIx_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOA
#define SPIx_SCK_SOURCE                GPIO_PinSource5
#define SPIx_SCK_AF                    GPIO_AF_SPI1

#define SPIx_NSS_PIN                   GPIO_Pin_4
#define SPIx_NSS_GPIO_PORT             GPIOA
#define SPIx_NSS_GPIO_CLK              RCC_AHB1Periph_GPIOA
#define SPIx_NSS_SOURCE              	 GPIO_PinSource4
#define SPIx_NSS_AF                  	 GPIO_AF_SPI1

#define SPIx_MISO_PIN                  GPIO_Pin_6
#define SPIx_MISO_GPIO_PORT            GPIOA
#define SPIx_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define SPIx_MISO_SOURCE               GPIO_PinSource6
#define SPIx_MISO_AF                   GPIO_AF_SPI1

#define SPIx_MOSI_PIN                  GPIO_Pin_7
#define SPIx_MOSI_GPIO_PORT            GPIOA
#define SPIx_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define SPIx_MOSI_SOURCE               GPIO_PinSource7
#define SPIx_MOSI_AF                   GPIO_AF_SPI1


//tx, rx buffer from igs_protocol.c
static uint8_t *RxBuffer = igs_protocol_rx_buffer;
static uint8_t *TxBuffer = igs_protocol_tx_buffer;

//spi rx and tx flag
uint8_t igs_rx_ok = 0;
uint8_t igs_tx_ok = 0;

/**
 * @brief igs_spi_init
 *    spi and correspond dma initial function
 * @param
 * @return
 */ 
void igs_spi_init()
{
	
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  DMA_InitTypeDef  DMA_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
   /* gpio init */
  // enable gpio clock
  RCC_AHB1PeriphClockCmd(SPIx_NSS_GPIO_CLK | SPIx_SCK_GPIO_CLK | SPIx_MISO_GPIO_CLK | SPIx_MOSI_GPIO_CLK, ENABLE);

	// gpio set to be alternative pin
  GPIO_PinAFConfig(SPIx_SCK_GPIO_PORT, SPIx_SCK_SOURCE, SPIx_SCK_AF);
	GPIO_PinAFConfig(SPIx_NSS_GPIO_PORT, SPIx_NSS_SOURCE, SPIx_NSS_AF);
  GPIO_PinAFConfig(SPIx_MOSI_GPIO_PORT, SPIx_MOSI_SOURCE, SPIx_MOSI_AF);
	GPIO_PinAFConfig(SPIx_MISO_GPIO_PORT, SPIx_MISO_SOURCE, SPIx_MISO_AF);
	
	// initial GPIO
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	
	/* spi initialization */
	// spi clock init
  SPIx_CLK_INIT(SPIx_CLK, ENABLE);
	
  // SPI SCK pin configuration 
  GPIO_InitStructure.GPIO_Pin = SPIx_SCK_PIN;
  GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);
	
  // SPI NSS pin configuration 
  GPIO_InitStructure.GPIO_Pin = SPIx_NSS_PIN;
  GPIO_Init(SPIx_NSS_GPIO_PORT, &GPIO_InitStructure);
	
  // SPI  MOSI pin configuration
  GPIO_InitStructure.GPIO_Pin =  SPIx_MOSI_PIN;
  GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStructure);
	
  // SPI  MISO pin configuration
  GPIO_InitStructure.GPIO_Pin =  SPIx_MISO_PIN;
  GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStructure);
	
	SPI_DeInit(SPIx);
	
  // spi initialization
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
	
	SPI_Init(SPIx, &SPI_InitStructure);
	SPI_Cmd(SPIx, ENABLE);
	
	// enable spi device dma request
	SPI_DMACmd(SPIx, SPI_DMAReq_Rx, ENABLE);
	SPI_DMACmd(SPIx, SPI_DMAReq_Tx, ENABLE);
	
	/* DMA initialization */
	// enable dma clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	// reset DMA
	DMA_DeInit(DMA2_Stream0);
	
	// DMA configuration
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;  
	DMA_InitStructure.DMA_PeripheralBaseAddr =  (uint32_t)&(SPIx->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)RxBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = PACKAGE_LENGTH;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	
	// Enable DMA
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA2_Stream0, DISABLE);

	
	// reset DMA
	DMA_DeInit(DMA2_Stream3);
	
	// DMA configuration
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_PeripheralBaseAddr =  (uint32_t)&(SPIx->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)TxBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;  
	DMA_InitStructure.DMA_BufferSize = PACKAGE_LENGTH;
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);

	// Enable DMA
	DMA_ITConfig(DMA2_Stream3, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA2_Stream3, DISABLE);



  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
	/* NVIC interrupt initialization */
	//SPI Rx DMA Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);    
	
	//SPI Tx DMA Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	 
}

/**
 * @brief igs_spi_trigger
 *    enable spi tx and rx dma
 * @param
 * @return
 */
void igs_spi_trigger(void)
{
	
	DMA_Cmd(DMA2_Stream3, ENABLE);
	DMA_Cmd(DMA2_Stream0, ENABLE);
}

/**
 * @brief igs_spi_check_rx
 *    check spi rx and tx is over or not
 * @param
 * @return uint8_t
 *     IGS_SPI_DATA_STANDBY: spi data in buffer is updated
 *     IGS_SPI_RX_EMPTY: no new data in spi buffer
 */
uint8_t igs_spi_check_rx()
{
	
	if(igs_rx_ok == 1 && igs_tx_ok == 1)
	{
		igs_rx_ok = 0;
		igs_tx_ok = 0;

		return IGS_SPI_DATA_STANDBY;
	}
	else
	{
		return IGS_SPI_RX_EMPTY;
	}
}
/**
 * @brief DMA2_Stream0_IRQHandler
 *    interrput handler for spi rx
 * @param
 * @return
 */
void DMA2_Stream0_IRQHandler(void)
{

	if(DMA_GetITStatus(DMA2_Stream0,DMA_IT_TCIF0))    
	{
		DMA_ClearITPendingBit(DMA2_Stream0,DMA_IT_TCIF0);
		igs_rx_ok = 1;
		
	}
}
/**
 * @brief DMA2_Stream3_IRQHandler
 *    interrput handler for spi tx
 * @param
 * @return
 */
void DMA2_Stream3_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA2_Stream3,DMA_IT_TCIF3))
	{
		DMA_ClearITPendingBit(DMA2_Stream3,DMA_IT_TCIF3);
		igs_tx_ok = 1;

	}
}
