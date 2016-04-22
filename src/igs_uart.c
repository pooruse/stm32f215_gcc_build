/**
 * @file igs_uart.c
 * @author Sheng Wen Peng
 * @date 19 Jan 2015
 * @brief IGS uart functions
 *    uart devices, including 485, 232
 */

#include <string.h>
#include "igs_uart.h"
#include "stm32f2xx.h"
#include "igs_task.h"
#include "igs_command.h"
#include "igs_protocol.h"

#define NUM_OF_UART 3

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_UART_INIT 0x0
#define COMMAND_UART_RECEIVE 0x1

/** 
 *  @brief uart status defines
 */
#define UART_STATE_READY 0x0
#define UART_STATE_BUZY 0x1
#define UART_STATE_PACKAGE_ERROR 0x2
#define UART_STATE_ERROR 0x3

#define UART_BUFFER_MAX 248 //256-8 (Checksum = 2, MCU_ID, polling, cmd_num, head_id, len, subcommand)

#define USART1_DATA_LENGTH UART_BUFFER_MAX
#define USART2_DATA_LENGTH UART_BUFFER_MAX
#define USART3_DATA_LENGTH UART_BUFFER_MAX 
#define UART4_DATA_LENGTH UART_BUFFER_MAX
#define UART5_DATA_LENGTH UART_BUFFER_MAX

/**
 * @bref uart_active_type
 */
#define UART_ENABLE 1
#define UART_DISABLE 0

/**
 *  @type uart_baud_type
 */
#define UART_BAUD_300 300
#define UART_BAUD_1200 1200
#define UART_BAUD_2400 2400
#define UART_BAUD_4800 4800
#define UART_BAUD_9600 9600
#define UART_BAUD_19200 19200
#define UART_BAUD_38400 38400
#define UART_BAUD_57600 57600
#define UART_BAUD_115200 115200

/**
 *	@bref uart_baud_table
 *  contains suported baud rate mode in the uart device
 */
static uint32_t uart_baud_table[] =
{
	UART_BAUD_300, 
	UART_BAUD_1200, 
	UART_BAUD_2400, 
	UART_BAUD_4800, 
	UART_BAUD_9600, 
	UART_BAUD_19200, 
	UART_BAUD_38400, 
	UART_BAUD_57600, 
	UART_BAUD_115200, 
};

/**
 *  @bref uart_pass_mode_type
 */
#define UART_MODE_PASS_SLAVE 1 
#define UART_MODE_PASS_MASTER 0

/**
 *  @bref uart_mode_type
 */
#define UART_MODE_PASS 0 
#define UART_MODE_FUNCTION 1

/**
 *  @bref uart_parity_table
 */
static uint16_t uart_parity_table[] =
{
	USART_Parity_No,
	USART_Parity_Odd,
	USART_Parity_Even,
};

/**
 *  @bref uart_stop_bit_table
 */
static uint16_t uart_stop_bit_table[] =
{
	USART_StopBits_1,
	USART_StopBits_2,
};

/**
 *  @bref uart_data_length_table
 */
static uint16_t uart_data_length_table[] = 
{
	USART_WordLength_8b, //0
	USART_WordLength_8b, //1
	USART_WordLength_8b, //2
	USART_WordLength_8b, //3
	USART_WordLength_8b, //4
	USART_WordLength_8b, //5
	USART_WordLength_8b, //6
	USART_WordLength_8b, //7
	USART_WordLength_8b, //8
	USART_WordLength_9b, //9
};

/**
 *  @bref uartDevice_t
 */
struct uartDevice_t
{
	uint8_t IsEnable; /*< 1:ENABLE, 0:DISABLE */
	uint8_t IsSlave; /*< 1:Slave, 0:Master */
	uint8_t mode; /*< define @type uart_mode_type */
	uint32_t tx_length;
	uint32_t rx_length;
	
	uint8_t IsParityError;
	uint8_t rx_IsStart;
	uint32_t rx_time_out_count;
	uint32_t rx_time_out_value; 
	
	uint32_t baud; /*< define @type uart_baud_type */
	uint8_t parity_mode; /*< define in uart_stop_bit_table */
	uint8_t stop_bit; /*< define in uart_stop_bit_table */
	uint8_t data_length; /*< define in uart_data_length_table */
	uint8_t tx_ok; /*< 1:Transmit OK, 0:WAIT */
	uint8_t rx_ok; /*< 1:Receive OK, 0:WAIT */
	uint8_t *rx_buf;
	uint8_t *tx_buf;
}uartDevice[NUM_OF_UART];

/**
 *  @bref Device Structures
 */
static GPIO_InitTypeDef GPIO_InitStructure;
static NVIC_InitTypeDef NVIC_InitStructure;
static DMA_InitTypeDef  DMA_InitStructure;
static USART_InitTypeDef USART_InitStructure;


/**
 *  @bref uart static functions
 */
static void uart_command(uint8_t *buf, uint32_t len);
static void uart_command_init(uint8_t *buf, uint32_t len);
static void uart_command_receive(uint8_t *buf, uint32_t len);
static void uart_timeout_counter(void);

typedef void (*uart_start_t)(void);
typedef void (*uart_init_t)(struct uartDevice_t);

/* Definition for USART3 resources ********************************************/
#define USART2_ID 											 2
#define USART2_CLK                       RCC_APB1Periph_USART2
#define USART2_CLK_INIT                  RCC_APB1PeriphClockCmd
#define USART2_IRQn                      USART2_IRQn
#define USART2_IRQHandler                USART2_IRQHandler

#define USART2_TX_PIN                    GPIO_Pin_2               
#define USART2_TX_GPIO_PORT              GPIOA                       
#define USART2_TX_GPIO_CLK               RCC_AHB1Periph_GPIOA
#define USART2_TX_SOURCE                 GPIO_PinSource2
#define USART2_TX_AF                     GPIO_AF_USART2

#define USART2_RX_PIN                    GPIO_Pin_3               
#define USART2_RX_GPIO_PORT              GPIOA                    
#define USART2_RX_GPIO_CLK               RCC_AHB1Periph_GPIOA
#define USART2_RX_SOURCE                 GPIO_PinSource3
#define USART2_RX_AF                     GPIO_AF_USART2

#define USART2_DMA                       DMA1
#define USART2_DMA1_CLK                  RCC_AHB1Periph_DMA1
   
#define USART2_TX_DMA_CHANNEL            DMA_Channel_4
#define USART2_TX_DMA_STREAM             DMA1_Stream6
#define USART2_TX_DMA_IT_TCIF            DMA_IT_TCIF6

#define USART2_RX_DMA_CHANNEL            DMA_Channel_4
#define USART2_RX_DMA_STREAM             DMA1_Stream5
#define USART2_RX_DMA_IT_TCIF            DMA_IT_TCIF5

#define USART2_DMA_TX_IRQn               DMA1_Stream6_IRQn
#define USART2_DMA_RX_IRQn               DMA1_Stream5_IRQn
#define USART2_DMA_TX_IRQHandler         DMA1_Stream6_IRQHandler
#define USART2_DMA_RX_IRQHandler         DMA1_Stream5_IRQHandler   

/* Definition for USART3 resources ********************************************/
#define USART3_ID 											 0
#define USART3_CLK                       RCC_APB1Periph_USART3
#define USART3_CLK_INIT                  RCC_APB1PeriphClockCmd
#define USART3_IRQn                      USART3_IRQn
#define USART3_IRQHandler                USART3_IRQHandler

#define USART3_TX_PIN                    GPIO_Pin_10                
#define USART3_TX_GPIO_PORT              GPIOB                       
#define USART3_TX_GPIO_CLK               RCC_AHB1Periph_GPIOB
#define USART3_TX_SOURCE                 GPIO_PinSource10
#define USART3_TX_AF                     GPIO_AF_USART3

#define USART3_RX_PIN                    GPIO_Pin_11                
#define USART3_RX_GPIO_PORT              GPIOB                    
#define USART3_RX_GPIO_CLK               RCC_AHB1Periph_GPIOB
#define USART3_RX_SOURCE                 GPIO_PinSource11
#define USART3_RX_AF                     GPIO_AF_USART3

#define USART3_DMA                       DMA1
#define USART3_DMA1_CLK                  RCC_AHB1Periph_DMA1
   
#define USART3_TX_DMA_CHANNEL            DMA_Channel_4
#define USART3_TX_DMA_STREAM             DMA1_Stream3
#define USART3_TX_DMA_IT_TCIF            DMA_IT_TCIF3

#define USART3_RX_DMA_CHANNEL            DMA_Channel_4
#define USART3_RX_DMA_STREAM             DMA1_Stream1
#define USART3_RX_DMA_IT_TCIF            DMA_IT_TCIF1

#define USART3_DMA_TX_IRQn               DMA1_Stream3_IRQn
#define USART3_DMA_RX_IRQn               DMA1_Stream1_IRQn
#define USART3_DMA_TX_IRQHandler         DMA1_Stream3_IRQHandler
#define USART3_DMA_RX_IRQHandler         DMA1_Stream1_IRQHandler   

/* Definition for UART4 resources ********************************************/
#define UART4_ID 											  1
#define UART4_CLK                       RCC_APB1Periph_UART4
#define UART4_CLK_INIT                  RCC_APB1PeriphClockCmd
#define UART4_IRQn                      UART4_IRQn
#define UART4_IRQHandler                UART4_IRQHandler

#define UART4_TX_PIN                    GPIO_Pin_10               
#define UART4_TX_GPIO_PORT              GPIOC                       
#define UART4_TX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define UART4_TX_SOURCE                 GPIO_PinSource10
#define UART4_TX_AF                     GPIO_AF_UART4

#define UART4_RX_PIN                    GPIO_Pin_11               
#define UART4_RX_GPIO_PORT              GPIOC                    
#define UART4_RX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define UART4_RX_SOURCE                 GPIO_PinSource11
#define UART4_RX_AF                     GPIO_AF_UART4

#define UART4_DMA                       DMA1
#define UART4_DMA1_CLK                  RCC_AHB1Periph_DMA1
   
#define UART4_TX_DMA_CHANNEL            DMA_Channel_4
#define UART4_TX_DMA_STREAM             DMA1_Stream4
#define UART4_TX_DMA_IT_TCIF            DMA_IT_TCIF4

#define UART4_RX_DMA_CHANNEL            DMA_Channel_4
#define UART4_RX_DMA_STREAM             DMA1_Stream2
#define UART4_RX_DMA_IT_TCIF            DMA_IT_TCIF2

#define UART4_DMA_TX_IRQn               DMA1_Stream4_IRQn
#define UART4_DMA_RX_IRQn               DMA1_Stream2_IRQn
#define UART4_DMA_TX_IRQHandler         DMA1_Stream4_IRQHandler
#define UART4_DMA_RX_IRQHandler         DMA1_Stream2_IRQHandler

#define DOT_485_IO_Periph								RCC_AHB1Periph_GPIOC
#define DOT_485_IO_PORT									GPIOC
#define DOT_485_IO_PIN									GPIO_Pin_1


/**
 *  @bref usart3 static functions
 */
static void usart3_init(struct uartDevice_t dev);
static void usart3_tx_start(void);
static void usart3_rx_start(void);
static void usart3_polling(void);

/**
 *  @bref uart4 static functions
 */
static void uart4_init(struct uartDevice_t dev);
static void uart4_tx_start(void);
static void uart4_rx_start(void);
static void uart4_polling(void);

/**
 *  @bref uartHandler_t
 */
struct uartHandler_t
{
	uart_start_t tx_start;
	uart_start_t rx_start;
	uart_init_t init;
}uartHandler[NUM_OF_UART];


/**
	* @bref uart_command
	*		global command uart handler
	* @param buf
  *    buf[0] -> sub command id
  *    buf[1] -> command data
	*	@param len
	*	@ret none
	*/
static void uart_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_UART_INIT: 
			uart_command_init(&buf[1],len-1);
			break;
		case COMMAND_UART_RECEIVE:
			uart_command_receive(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @bref uart_command_init
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> IsEnable
 *           buf[2] -> mode (pass or function)
 *           buf[3] -> recieve data length
 *           buf[4] -> recieve time out time[7:0]
 *           buf[5] -> recieve time out time[15:8]
 *           buf[6] -> baud rate
 *           buf[7] -> parity bit mode
 *           buf[8] -> stop bit mode
 *           buf[9] -> data length in the uart communication frame
 *           buf[10]-> IsSlave?
 * 
 */
static void uart_command_init(uint8_t *buf, uint32_t len)
{
	uint8_t id = buf[0];
	if(id >= NUM_OF_UART)
	{
		// if id value is invalid, skip it
		return;
	}
	
	// assign Device parameter
	uartDevice[id].IsEnable = buf[1];
	uartDevice[id].mode = buf[2];
	
	if(buf[3] > UART_BUFFER_MAX)
		uartDevice[id].rx_length = UART_BUFFER_MAX;
	else
		uartDevice[id].rx_length = buf[3];
	
	uartDevice[id].rx_time_out_value = buf[4] + (buf[5]<<8);
	uartDevice[id].baud = buf[6];
	uartDevice[id].parity_mode = buf[7];
	uartDevice[id].stop_bit = buf[8];
	uartDevice[id].data_length = buf[9];
	uartDevice[id].IsSlave = buf[10];
	
	//device init 
	uartHandler[id].init(uartDevice[id]);
	
	if(uartDevice[id].IsSlave == UART_MODE_PASS_SLAVE)
	{
		uartDevice[id].rx_time_out_value=0;
		uartHandler[id].rx_start();
	}
}

/**
 * @bref uart_command_receive
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> data[0]
 *           buf[2] -> data[1]
                     .
                     .
                     .
 *           buf[len] -> data[len-1]
 * 
 */
static void uart_command_receive(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t *tmp_buf;
	uint8_t id;
	id = buf[0];
	if(id >= NUM_OF_UART)
	{
		//invalid id value
		return;
	}
	
	tmp_buf = &buf[1];
	// prepare tx data
	for(i=0;i<(len-1);i++)
	{
		uartDevice[id].tx_buf[i] = tmp_buf[i];
	}
	uartDevice[id].rx_IsStart = 1;
	uartDevice[id].rx_time_out_count = uartDevice[id].rx_time_out_value;
	uartDevice[id].tx_length = len-1;
	
	if(uartDevice[id].IsSlave == UART_MODE_PASS_MASTER)
	{
		uartHandler[id].rx_start();
	}
	uartHandler[id].tx_start();
	
}

/**
 * @bref uart_timeout_counter
 */
static void uart_timeout_counter(void)
{
	uint32_t i;
	for(i=0;i<NUM_OF_UART;i++)
	{
		if(uartDevice[i].rx_time_out_value == 0)
		{
			// no time out setting
			continue;
		}
		
		if(uartDevice[i].rx_IsStart != 0)
		{
			if(uartDevice[i].rx_time_out_count != 0)
			{
				uartDevice[i].rx_time_out_count--;
				continue;
			}
			
			// time out
			igs_command_add_dat(COMMAND_UART_RECEIVE);
			igs_command_add_dat(UART_STATE_PACKAGE_ERROR);
			igs_command_insert(COMMAND_UART);
			
			uartDevice[i].rx_time_out_count--;
			uartDevice[i].rx_IsStart = 0;
			uartDevice[i].rx_time_out_count = 0;
			uartHandler[i].init(uartDevice[i]);
		}
	}
}
/**
 * @bref igs_uart_init
 */
void igs_uart_init()
{
	uint32_t i;
	/**
	 *  clock enable
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	
	/**
	 *  GPIO Structures Init
	 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	
	/**
	 *  DMA Structures Init
	 */
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
	
	/**
	 *  USART Structures Init
	 */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	/**
	 *  NVIC Structures Init
	 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	/**
	 *  uartDevice_t Structures Init
	 */
	for(i=0;i<NUM_OF_UART;i++)
	{
		uartDevice[i].IsEnable = 0;
		uartDevice[i].IsParityError = 0;
		uartDevice[i].rx_IsStart = 0;
		uartDevice[i].rx_time_out_count = 0; 
	}
	
	/**
	 *  uartHandle_t Structures Init
	 */
	uartHandler[USART3_ID].tx_start = usart3_tx_start;
	uartHandler[USART3_ID].rx_start = usart3_rx_start;
	uartHandler[USART3_ID].init = usart3_init;
	 
	/**
	 *  uartHandle_t Structures Init
	 */
	uartHandler[UART4_ID].tx_start = uart4_tx_start;
	uartHandler[UART4_ID].rx_start = uart4_rx_start;
	uartHandler[UART4_ID].init = uart4_init;
	 
	/**
	 *  functions Init
	 */
	igs_protocol_command_register(COMMAND_UART,uart_command);
	igs_task_add(IGS_TASK_POLLING,1,usart3_polling);
	igs_task_add(IGS_TASK_POLLING,1,uart4_polling);
	igs_task_add(IGS_TASK_ROUTINE_POLLING,1,uart_timeout_counter);
	
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/**
 * USART3 functions, variables
 */
static uint8_t igs_usart3_tx[USART3_DATA_LENGTH];
static uint8_t igs_usart3_rx[USART3_DATA_LENGTH];

/**
 * @bref usart3_init
 */
static void usart3_init(struct uartDevice_t dev)
{
	if(dev.IsEnable == UART_DISABLE)
	{
		USART_DeInit(USART3);
		DMA_DeInit(USART3_TX_DMA_STREAM);
		DMA_DeInit(USART3_RX_DMA_STREAM);
	}
	else if(dev.IsEnable == UART_ENABLE)
	{

		/**
		 *  GPIO Init
		 */
		GPIO_PinAFConfig(USART3_TX_GPIO_PORT, USART3_TX_SOURCE, USART3_TX_AF);
		GPIO_PinAFConfig(USART3_RX_GPIO_PORT, USART3_RX_SOURCE, USART3_RX_AF);
		
		GPIO_InitStructure.GPIO_Pin = USART3_TX_PIN;
		GPIO_Init(USART3_TX_GPIO_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = USART3_RX_PIN;
		GPIO_Init(USART3_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/**
		 *  USART Init
		 */
		USART3_CLK_INIT(USART3_CLK, ENABLE);
		USART_DeInit(USART3);
		
		USART_InitStructure.USART_BaudRate = uart_baud_table[dev.baud];
		USART_InitStructure.USART_WordLength = uart_data_length_table[dev.data_length];
		USART_InitStructure.USART_StopBits = uart_stop_bit_table[dev.stop_bit];
		USART_InitStructure.USART_Parity = uart_parity_table[dev.parity_mode];
		USART_Init(USART3, &USART_InitStructure);
		USART_Cmd(USART3, ENABLE);
		
		USART_ITConfig(USART3, USART_IT_PE, ENABLE);
		
		USART_DMACmd(USART3, USART_DMAReq_Tx,ENABLE);
		USART_DMACmd(USART3, USART_DMAReq_Rx,ENABLE);
		
		/**
		 *  DMA Init
		 */
		DMA_DeInit(USART3_TX_DMA_STREAM);
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_usart3_tx;
		DMA_InitStructure.DMA_Channel = USART3_TX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = USART3_DATA_LENGTH;
		DMA_Init(USART3_TX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(USART3_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(USART3_TX_DMA_STREAM, DISABLE);
		
		DMA_DeInit(USART3_RX_DMA_STREAM);
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_usart3_rx;
		DMA_InitStructure.DMA_Channel = USART3_RX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = dev.rx_length;
		DMA_Init(USART3_RX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(USART3_RX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(USART3_RX_DMA_STREAM, DISABLE);
		
		/**
		 *  NVIC Init
		 */
		NVIC_InitStructure.NVIC_IRQChannel = USART3_DMA_TX_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = USART3_DMA_RX_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		
		uartDevice[USART3_ID].rx_buf = igs_usart3_rx;
		uartDevice[USART3_ID].tx_buf = igs_usart3_tx;
		uartDevice[USART3_ID].tx_ok = 1;
		uartDevice[USART3_ID].rx_ok = 0;
	}
	
}

/**
 * @bref usart3_tx_start
 */
static void usart3_tx_start(void)
{
	if( (uartDevice[USART3_ID].tx_length != 0) && (uartDevice[USART3_ID].tx_ok == 1) )
	{
		uartDevice[USART3_ID].tx_ok = 0;
		
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_usart3_tx;
		DMA_InitStructure.DMA_Channel = USART3_TX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = uartDevice[USART3_ID].tx_length;
		DMA_Init(USART3_TX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(USART3_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(USART3_TX_DMA_STREAM, ENABLE);
	}
}

/**
 * @bref usart3_rx_start
 */
static void usart3_rx_start(void)
{
	if(uartDevice[USART3_ID].rx_length != 0)
	{
		DMA_Cmd(USART3_RX_DMA_STREAM, ENABLE);
	}
}
/**
 * @bref usart3_polling
 *    
 */
static void usart3_polling(void)
{
	if(uartDevice[USART3_ID].IsEnable == 1)
	{
		//recieve 
		if(uartDevice[USART3_ID].rx_ok == 1)
		{
			igs_command_add_dat(COMMAND_UART_RECEIVE);
			igs_command_add_dat(USART3_ID);
			if(uartDevice[USART3_ID].IsParityError == 1)
			{
				igs_command_add_dat(UART_STATE_PACKAGE_ERROR);
				uartDevice[USART3_ID].IsParityError = 0;
			}
			else
			{
				igs_command_add_dat(UART_STATE_READY);
			}
			igs_command_add_buf(igs_usart3_rx,uartDevice[USART3_ID].rx_length);
			igs_command_insert(COMMAND_UART);
			
			uartDevice[USART3_ID].rx_ok = 0;
			uartDevice[USART3_ID].rx_IsStart = 0;
			if(uartDevice[USART3_ID].IsSlave == UART_MODE_PASS_SLAVE)
			{
				uartHandler[USART3_ID].rx_start();
			}
		}
		
	}
}

/**
 * @bref USART3_DMA_TX_IRQHandler
 */
void USART3_DMA_TX_IRQHandler(void)
{
	if(DMA_GetITStatus(USART3_TX_DMA_STREAM,USART3_TX_DMA_IT_TCIF))    
	{
		DMA_ClearITPendingBit(USART3_TX_DMA_STREAM,USART3_TX_DMA_IT_TCIF);
		uartDevice[USART3_ID].tx_ok = 1;
	}
}

/**
 * @bref USART3_DMA_RX_IRQHandler
 */
void USART3_DMA_RX_IRQHandler(void)
{
	if(DMA_GetITStatus(USART3_RX_DMA_STREAM,USART3_RX_DMA_IT_TCIF))    
	{
		DMA_ClearITPendingBit(USART3_RX_DMA_STREAM,USART3_RX_DMA_IT_TCIF);
		uartDevice[USART3_ID].rx_ok = 1;
	}
}

/**
 * @bref USART3_Parity_Error_IRQHandler
 */
void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_PE) == SET)
	{
		USART_ClearITPendingBit(USART3, USART_IT_PE);
		uartDevice[USART3_ID].IsParityError = 1; 
	}
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/**
 * UART4 functions, variables
 */
static uint8_t igs_uart4_tx[UART4_DATA_LENGTH];
static uint8_t igs_uart4_rx[UART4_DATA_LENGTH];

/**
 * @bref uart4_init
 */
static void uart4_init(struct uartDevice_t dev)
{
	if(dev.IsEnable == UART_DISABLE)
	{
		USART_DeInit(UART4);
		DMA_DeInit(UART4_TX_DMA_STREAM);
		DMA_DeInit(UART4_RX_DMA_STREAM);
	}
	else if(dev.IsEnable == UART_ENABLE)
	{

		/**
		 *  GPIO Init
		 */
		GPIO_PinAFConfig(UART4_TX_GPIO_PORT, UART4_TX_SOURCE, UART4_TX_AF);
		GPIO_PinAFConfig(UART4_RX_GPIO_PORT, UART4_RX_SOURCE, UART4_RX_AF);

		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Pin = DOT_485_IO_PIN;
		GPIO_Init(DOT_485_IO_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_ResetBits(DOT_485_IO_PORT,DOT_485_IO_PIN);
		
		GPIO_InitStructure.GPIO_Pin = UART4_TX_PIN;
		GPIO_Init(UART4_TX_GPIO_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = UART4_RX_PIN;
		GPIO_Init(UART4_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/**
		 *  USART Init
		 */
		UART4_CLK_INIT(UART4_CLK, ENABLE);
		USART_DeInit(UART4);
		
		USART_InitStructure.USART_BaudRate = uart_baud_table[dev.baud];
		USART_InitStructure.USART_WordLength = uart_data_length_table[dev.data_length];
		USART_InitStructure.USART_StopBits = uart_stop_bit_table[dev.stop_bit];
		USART_InitStructure.USART_Parity = uart_parity_table[dev.parity_mode];
		USART_Init(UART4, &USART_InitStructure);
		USART_Cmd(UART4, ENABLE);
		
		USART_ITConfig(UART4, USART_IT_PE, ENABLE);
		USART_ITConfig(UART4, USART_IT_TC, ENABLE);

		USART_DMACmd(UART4, USART_DMAReq_Tx,ENABLE);
		USART_DMACmd(UART4, USART_DMAReq_Rx,ENABLE);
		
		/**
		 *  DMA Init
		 */
		DMA_DeInit(UART4_TX_DMA_STREAM);
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_uart4_tx;
		DMA_InitStructure.DMA_Channel = UART4_TX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = UART4_DATA_LENGTH;
		DMA_Init(UART4_TX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(UART4_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(UART4_TX_DMA_STREAM, DISABLE);
		
		DMA_DeInit(UART4_RX_DMA_STREAM);
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_uart4_rx;
		DMA_InitStructure.DMA_Channel = UART4_RX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = dev.rx_length;
		DMA_Init(UART4_RX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(UART4_RX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(UART4_RX_DMA_STREAM, DISABLE);
		
		/**
		 *  NVIC Init
		 */
		NVIC_InitStructure.NVIC_IRQChannel = UART4_DMA_TX_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = UART4_DMA_RX_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		
		uartDevice[UART4_ID].rx_buf = igs_uart4_rx;
		uartDevice[UART4_ID].tx_buf = igs_uart4_tx;
		uartDevice[UART4_ID].tx_ok = 1;
		uartDevice[UART4_ID].rx_ok = 0;
	}
	
}

/**
 * @bref uart4_tx_start
 */
static void uart4_tx_start(void)
{
	if( (uartDevice[UART4_ID].tx_length != 0) && (uartDevice[UART4_ID].tx_ok == 1) )
	{
		GPIO_SetBits(DOT_485_IO_PORT,DOT_485_IO_PIN);	
		uartDevice[UART4_ID].tx_ok = 0;
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)igs_uart4_tx;
		DMA_InitStructure.DMA_Channel = UART4_TX_DMA_CHANNEL;
		DMA_InitStructure.DMA_BufferSize = uartDevice[UART4_ID].tx_length;
		DMA_Init(UART4_TX_DMA_STREAM, &DMA_InitStructure);
		DMA_ITConfig(UART4_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
		DMA_Cmd(UART4_TX_DMA_STREAM, ENABLE);
	}
}

/**
 * @bref uart4_rx_start
 */
static void uart4_rx_start(void)
{
	if(uartDevice[UART4_ID].rx_length != 0)
	{
		DMA_Cmd(UART4_RX_DMA_STREAM, ENABLE);
	}
}
/**
 * @bref uart4_polling
 *    
 */
static void uart4_polling(void)
{
	if(uartDevice[UART4_ID].IsEnable == 1)
	{
		//recieve 
		if(uartDevice[UART4_ID].rx_ok == 1)
		{
			igs_command_add_dat(COMMAND_UART_RECEIVE);
			igs_command_add_dat(UART4_ID);
			
			if(uartDevice[UART4_ID].IsParityError == 1)
			{
				igs_command_add_dat(UART_STATE_PACKAGE_ERROR);
				uartDevice[UART4_ID].IsParityError = 0;
			}
			else
			{
				igs_command_add_dat(UART_STATE_READY);
			}
			igs_command_add_buf(igs_uart4_rx,uartDevice[UART4_ID].rx_length);
			igs_command_insert(COMMAND_UART);

			uartDevice[UART4_ID].rx_ok = 0;
			uartDevice[UART4_ID].rx_IsStart = 0;
			if(uartDevice[UART4_ID].IsSlave == UART_MODE_PASS_SLAVE)
			{
				uartHandler[UART4_ID].rx_start();
			}
		}
		
	}
}

/**
 * @bref UART4_DMA_TX_IRQHandler
 */
void UART4_DMA_TX_IRQHandler(void)
{
	if(DMA_GetITStatus(UART4_TX_DMA_STREAM,UART4_TX_DMA_IT_TCIF))    
	{
		DMA_ClearITPendingBit(UART4_TX_DMA_STREAM,UART4_TX_DMA_IT_TCIF);
		uartDevice[UART4_ID].tx_ok = 1;
	}
}

/**
 * @bref UART4_DMA_RX_IRQHandler
 */
void UART4_DMA_RX_IRQHandler(void)
{
	if(DMA_GetITStatus(UART4_RX_DMA_STREAM,UART4_RX_DMA_IT_TCIF))    
	{
		DMA_ClearITPendingBit(UART4_RX_DMA_STREAM,UART4_RX_DMA_IT_TCIF);
		uartDevice[UART4_ID].rx_ok = 1;
	}
}

/**
 * @bref UART4_Parity_Error_IRQHandler & 485 write finish handler
 */
void UART4_IRQHandler(void)
{
	if (USART_GetITStatus(UART4, USART_IT_PE) == SET)
	{
		USART_ClearITPendingBit(UART4, USART_IT_PE);
		uartDevice[UART4_ID].IsParityError = 1; 
	}
	if (USART_GetITStatus(UART4, USART_IT_TC) == SET)
	{
		USART_ClearITPendingBit(UART4, USART_IT_TC);
		GPIO_ResetBits(DOT_485_IO_PORT,DOT_485_IO_PIN);
	}
}
