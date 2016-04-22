#include <stdint.h>
#include "igs_adc.h"
#include "stm32f2xx.h"
#include "igs_task.h"
#include "igs_queue.h"
#include "igs_command.h"

#define NUM_OF_BATTERY 1

/**
 *  @brief BATTERY_MIN_VOLTAGE
 *  our battery is connect to chip DS3231 (RTC)
 *  base on the spec., the supply voltage of DS3231 is 2.3~5.5
 *  2858 ~= (2.3/3.3)*4096
 */
#define BATTERY_MIN_VOLTAGE 2855
/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_BATTERY_INIT 0x0
#define COMMAND_BATTERY_RET 0x1

/**
 *  @brief ADC device hardware defines
 */
 
//channel
#define ADC1_ID 0
#define ADC1_CHANNEL ADC_Channel_9

//GPIO posision
#define ADC1_GPIO_PIN GPIO_Pin_1
#define ADC1_GPIO_GROUP GPIOB

//DMA
#define ADC1_DMA_CHANNEL            DMA_Channel_0
#define ADC1_DMA_STREAM             DMA2_Stream4

struct battery_device_t
{
	uint8_t IsEnable;
	uint32_t value;
}battery_device[NUM_OF_BATTERY];

static void adc_polling(void);
static void battery_command(uint8_t *buf, uint32_t len);


/**
 *  @brief igs_adc_init
 */
void igs_adc_init(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  GPIO_InitTypeDef      GPIO_InitStructure;
	
  /* Enable ADC1, DMA2 and GPIO clocks ****************************************/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 , ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
  /* DMA2 Stream0 channel0 configuration **************************************/
	
  DMA_InitStructure.DMA_Channel = ADC1_DMA_CHANNEL;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&battery_device[ADC1_ID].value;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(ADC1_DMA_STREAM, &DMA_InitStructure);
  DMA_Cmd(ADC1_DMA_STREAM, ENABLE);
	
  /* Configure ADC3 Channel7 pin as analog input ******************************/
  GPIO_InitStructure.GPIO_Pin = ADC1_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(ADC1_GPIO_GROUP, &GPIO_InitStructure);
	
  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
	
  /* ADC3 Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
	
  /* ADC1 regular channel9 configuration *************************************/
  ADC_RegularChannelConfig(ADC1, ADC1_CHANNEL, 1, ADC_SampleTime_480Cycles);
	
 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
	
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
	
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConv(ADC1);
	
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 5000,adc_polling);
	igs_protocol_command_register(COMMAND_BATTERY,battery_command);
}

/**
 *  @brief adc_polling
 */
static void adc_polling(void)
{
	uint32_t i;
	uint32_t tmp;
	for(i=0;i<NUM_OF_BATTERY;i++)
	{
		if(battery_device[i].IsEnable == 1)
		{
			igs_command_add_dat(COMMAND_BATTERY_RET);
			igs_command_add_dat((uint8_t)i); //id
			
			if(battery_device[i].value < BATTERY_MIN_VOLTAGE)
			{
				tmp = 0;
			}
			else
			{
				tmp = battery_device[i].value - BATTERY_MIN_VOLTAGE;
				tmp = tmp*100/(4096-BATTERY_MIN_VOLTAGE);
			}
			
			igs_command_add_dat(tmp);
			igs_command_insert(COMMAND_BATTERY);
		}
	}
}

/**
 *  @brief battery_command
 */
static void battery_command(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t IsEnable;
	switch(buf[0])
	{
		case COMMAND_BATTERY_INIT:
			id = buf[1];
			if(id < NUM_OF_BATTERY)
			{
				IsEnable = buf[2];
				battery_device[id].IsEnable = IsEnable;
			}
			break;
		default: break;
	}

}
