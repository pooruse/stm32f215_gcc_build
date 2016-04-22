/**
 * @file igs_can.c
 * @author Sheng Wen Peng
 * @date 16 Jan 2015
 * @brief IGS can device
 */
 
#include <stdio.h>
#include <stdint.h>
#include "stm32f2xx.h"

#include "igs_protocol.h"
#include "igs_queue.h"
#include "igs_task.h"
#include "igs_command.h"
#include "igs_can.h"

#define NUM_OF_CAN 1

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_CAN_INIT 0x0
#define COMMAND_CAN_DATA 0x1

/**
 *  @brief can id can be 0x00000000~0x1FFFFFFF
 *     but in igs can application, we just use 0x08004000~0x080040FF
 */
#define CAN_BASE_ID 0x08004000
#define CAN_IGS_MASK 0x000000FF

#define CAN2_GPIO_CLK               RCC_AHB1Periph_GPIOB
#define CAN2_GPIO_PORT  						GPIOB
#define CAN2_AF_PORT 								GPIO_AF_CAN2
#define CAN2_RX_SOURCE 							GPIO_PinSource12
#define CAN2_TX_SOURCE 							GPIO_PinSource13
#define CAN2_RX_PIN 								GPIO_Pin_12
#define CAN2_TX_PIN 								GPIO_Pin_13

static CAN_InitTypeDef        CAN_InitStructure;
static CAN_FilterInitTypeDef  CAN_FilterInitStructure;
static CanTxMsg TxMessage;
static CanRxMsg RxMessage;
static uint8_t target_id;

static void igs_can_command(uint8_t *buf, uint32_t len);
static void igs_can_command_init(uint8_t *buf, uint32_t len);
static void igs_can_command_data(uint8_t *buf, uint32_t len);
static void can_rx_polling(void);

struct can_device_t
{
	uint8_t IsEnable;
	uint8_t rx_ok;
	CAN_TypeDef *CANx;
};
struct can_device_t can_device[NUM_OF_CAN] = {0, 0, CAN2};

/**
 * @breif igs_can_init
 */
void igs_can_init(void)
{
	
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);
	RCC_AHB1PeriphClockCmd(CAN2_GPIO_CLK, ENABLE);
	
	GPIO_PinAFConfig(CAN2_GPIO_PORT, CAN2_RX_SOURCE, CAN2_AF_PORT);
  GPIO_PinAFConfig(CAN2_GPIO_PORT, CAN2_TX_SOURCE, CAN2_AF_PORT); 
	
  /* Configure CAN RX and TX pins */
  GPIO_InitStructure.GPIO_Pin = CAN2_RX_PIN | CAN2_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(CAN2_GPIO_PORT, &GPIO_InitStructure);
	
	CAN_DeInit(CAN2);
	
  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    
  /* CAN Baudrate = 500 KBps (CAN clocked at 30 MHz) */
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
  CAN_InitStructure.CAN_Prescaler = 4;
  CAN_Init(CAN2, &CAN_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	igs_protocol_command_register(COMMAND_CAN,igs_can_command);
	igs_task_add(IGS_TASK_POLLING , 1, can_rx_polling);
}

/**
 * @breif igs_can_command
 */
static void  igs_can_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_CAN_INIT: 
			igs_can_command_init(&buf[1],len-1);
			break;
		case COMMAND_CAN_DATA:
			igs_can_command_data(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @breif igs_can_init
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> IsEnable
 *           buf[2] -> can id
 */
static void igs_can_command_init(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t IsEnable;
	uint8_t can_id;
	uint32_t filter_tmp;
	
	id = buf[0]; 
	IsEnable = buf[1];
	can_id = buf[2];
	
	if(id >= NUM_OF_CAN)
	{
		return;
	}
		
	if(IsEnable == 1)
	{
		/* CAN filter init */
		can_device[id].IsEnable = 1;
		CAN_FilterInitStructure.CAN_FilterNumber = 14;
		CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
		CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
		
		/**
		 * @brief filter_tmp
		 *    filter_tmp[31:24] == STID[10:3] (EXID[28:21])
		 *    filter_tmp[23:21] == STID[2:0] (EXID[20:18])
		 *    filter_tmp[20:16] == EXID[17:13]
		 *    filter_tmp[15:8] == EXID[12:5]
		 *    filter_tmp[7:3] == EXID[4:0]
		 *    filter_tmp[2] == IDE
		 *    filter_tmp[1] == RTR
		 *    filter_tmp[0] == 0
		 */
		filter_tmp = 0; 
		filter_tmp |= (CAN_BASE_ID | can_id) << 3;

		CAN_FilterInitStructure.CAN_FilterIdHigh = (uint16_t)(filter_tmp >> 16);
		CAN_FilterInitStructure.CAN_FilterIdLow = (uint16_t)(filter_tmp & 0xFFFF);
		
		CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFF;
		CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0xF8;
		CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;
		CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
		CAN_FilterInit(&CAN_FilterInitStructure);
		
		CAN_ITConfig(can_device[id].CANx, CAN_IT_FMP0, ENABLE);
	}
	else
	{
		can_device[id].IsEnable = 0;
		CAN_ITConfig(can_device[id].CANx, CAN_IT_FMP0, DISABLE);
	}
}
/**
 * @breif igs_can_data
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> target ID
 *           buf[2] -> data[0]
 *           buf[3] -> data[1]
 *           buf[4] -> data[2]
 *           buf[5] -> data[3]
 *           buf[6] -> data[4]
 *           buf[7] -> data[5]
 *           buf[8] -> data[6]
 *           buf[9] -> data[7]
 * 
 */
static void igs_can_command_data(uint8_t *buf, uint32_t len)
{
	uint8_t i;
	uint8_t id;
	
	id = buf[0];
	target_id = buf[1];
	
	if(id >= NUM_OF_CAN)
	{
		return;
	}
  /* Transmit Structure preparation */
  TxMessage.StdId = 0;
  TxMessage.ExtId = CAN_BASE_ID | target_id;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_EXT;
  TxMessage.DLC = 8;
	for(i=0;i<8;i++)
	{
		TxMessage.Data[i] = buf[i+2];
	}
	CAN_Transmit(CAN2, &TxMessage);
}

/**
 * @breif can_rx_polling
 */
static void can_rx_polling(void)
{
	uint32_t i;
	for(i=0;i<NUM_OF_CAN;i++)
	{
		if(can_device[i].IsEnable == 1)
		{
			if(can_device[i].rx_ok == 1)
			{
				can_device[i].rx_ok = 0;
				
				igs_command_add_dat(COMMAND_CAN_DATA);
				igs_command_add_dat((uint8_t)i); //id
				igs_command_add_dat(target_id);
				igs_command_add_buf(RxMessage.Data,8);
				igs_command_insert(COMMAND_CAN);
			}
		}
	}

}
/**
 * @breif CAN2_RX0_IRQHandler
 */
void CAN2_RX0_IRQHandler(void)
{
	if(CAN_GetITStatus(CAN2, CAN_IT_FMP0))
	{
		CAN_ClearITPendingBit(CAN2, CAN_IT_FMP0);
		CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);
		can_device[0].rx_ok = 1;
	}
}
