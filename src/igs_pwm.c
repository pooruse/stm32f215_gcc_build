/**
 * @file igs_pwm.c
 * @author Sheng Wen Peng
 * @date 19 Jan 2015
 * @brief IGS uart functions
 *    pwm devices, including pwm out device and pwm in device (Rotary VR)
 */

#include <stdint.h>
#include "igs_pwm.h"
#include "stm32f2xx.h"
#include "igs_gpio.h"
#include "igs_queue.h"
#include "igs_task.h"
#include "igs_command.h"

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_PWM_INIT 0x0
#define COMMAND_PWM_SET 0x1
#define COMMAND_ROTARY_VR_INIT 0x0
#define COMMAND_ROTARY_VR_RET 0x1

#define NUM_OF_OUT_PWM 3
#define OUT_PWM_PERIOD 1000
#define IN_PWM_LEVEL 4096
#define NUM_OF_IN_PWM 1

/**
 *  @breif OUT PWM DEVICE USER DEFINE STRUCTURE
 */

static NVIC_InitTypeDef NVIC_InitStructure;

struct pwm_out_table_t
{
	uint32_t pin;
	TIM_TypeDef* TIMx;
	void (*SetCompare)(TIM_TypeDef*, uint32_t);
};
	
struct pwm_out_table_t pwm_out_table[NUM_OF_OUT_PWM] = 
{
	{5, TIM4, TIM_SetCompare3},
	{6, TIM4, TIM_SetCompare2},
	{7, TIM4, TIM_SetCompare1},
};

struct pwm_out_device_t
{
	uint32_t IsEnable;
	struct pwm_out_table_t *dev;
}pwm_out_device[NUM_OF_OUT_PWM];

/**
 *  @breif IN PWM DEVICE USER DEFINE STRUCTURE
 */

struct pwm_in_device_t
{
	uint32_t IsEnable;
	gpio_map_t io;
	/**
	 *  @brief
	 *         +---------------+      +---------------+
	 *   ______|               |______|               |______
   *
   *         |<- hold_time ->|
	 *         |<-      period      ->|                
	 */
	uint16_t period;
	uint16_t hold_time;
	uint16_t absolute_value;
	int16_t relative_value;
	uint8_t capture_done;
	uint32_t IRQ;
};

#define IN_PWM1_ID 0
struct pwm_in_device_t pwm_in_device[NUM_OF_IN_PWM] =
{
	{0,{GPIOB,GPIO_Pin_0}, 0, 0, 0, 0, 0, TIM3_IRQn},
};

static void pwm_out_command(uint8_t *buf, uint32_t len);
static void pwm_out_command_init(uint8_t *buf, uint32_t len);
static void pwm_out_command_set(uint8_t *buf, uint32_t len);

static void pwm_in_polling(void);
static void pwm_in_send_command_polling(void);
static void pwm_in_command(uint8_t *buf, uint32_t len);
static void pwm_in_command_init(uint8_t *buf, uint32_t len);

/**
 * @breif igs_pwm_init
 */
void igs_pwm_init()
{			
	uint32_t i;
	
	GPIO_InitTypeDef GPIO_InitStructure; 
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_TIM4);

	/**
	 *  @breif PWM out setting 
	 */
	// Time base configuration 
	TIM_TimeBaseStructure.TIM_Period = OUT_PWM_PERIOD; 
	TIM_TimeBaseStructure.TIM_Prescaler = 3;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
	
	TIM_Cmd(TIM4, ENABLE);
	
	// pwmDevice Init
	for(i=0;i<NUM_OF_OUT_PWM;i++)
	{
		pwm_out_device[i].IsEnable = 0;
	}
	
	igs_protocol_command_register(COMMAND_PWM,pwm_out_command);
	
	/**
	 *  @breif PWM in setting 
	 */
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);
	
	for(i=0;i<NUM_OF_IN_PWM;i++)
	{
		GPIO_InitStructure.GPIO_Pin = pwm_in_device[i].io.pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
		GPIO_Init(pwm_in_device[i].io.group, &GPIO_InitStructure); 
	}
	
	// Time base configuration 
	TIM_TimeBaseStructure.TIM_Period = 0xFFFF; 
	TIM_TimeBaseStructure.TIM_Prescaler = 60-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM3, &TIM_ICInitStructure);
	
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_IndirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	
  // TIM enable counter 
  TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_CC3|TIM_IT_CC4, ENABLE);
	
	igs_protocol_command_register(COMMAND_ROTARY_VR,pwm_in_command);
	igs_task_add(IGS_TASK_POLLING, 1,pwm_in_polling);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 32,pwm_in_send_command_polling);
}

/**
 * @breif pwm_out_command
 */
static void pwm_out_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_PWM_INIT: 
			pwm_out_command_init(&buf[1],len-1);
			break;
		case COMMAND_PWM_SET:
			pwm_out_command_set(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @breif pwm_out_command_init
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> IsEnable
 *           buf[2] -> pin number
 * 
 */
static void pwm_out_command_init(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	GPIO_InitTypeDef GPIO_InitStructure; 
	
	uint8_t pin_id;
	uint8_t IsEnable;
	uint8_t id;
	
	id = buf[0];
	IsEnable = buf[1];
	pin_id = buf[2];
	
	if(id < NUM_OF_OUT_PWM)
	{
		for(i=0;i<NUM_OF_OUT_PWM;i++)
		{
			if(pin_id == pwm_out_table[i].pin)
			{
				if(IsEnable == 0)
				{
					pwm_out_device[id].IsEnable = 0;
					igs_gpio_set_output_mask(pin_id,ENABLE_GPIO_MASK);
					
					GPIO_InitStructure.GPIO_Pin = output_map[pin_id].pin;
					GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
					GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
					GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
					GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
					GPIO_Init(output_map[pin_id].group, &GPIO_InitStructure); 
				}
				else
				{
					pwm_out_device[id].IsEnable = 1;
					pwm_out_device[id].dev = &(pwm_out_table[i]);
					
					igs_gpio_set_output_mask(pin_id,DISABLE_GPIO_MASK);
					
					GPIO_InitStructure.GPIO_Pin = output_map[pin_id].pin;
					GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
					GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
					GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
					GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
					GPIO_Init(output_map[pin_id].group, &GPIO_InitStructure); 
				}
				return;
			}
		}
	}

}

/**
 * @breif pwm_out_command_set
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> level (0~255)
 * 
 */
static void pwm_out_command_set(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t level;
	uint32_t pulse;
	
	id = buf[0];
	level = buf[1];
	
	if(id < NUM_OF_OUT_PWM)
	{
		pulse = OUT_PWM_PERIOD * level / 254;
		pwm_out_device[id].dev->SetCompare(pwm_out_device[id].dev->TIMx, pulse);

	}
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
/**
 * @breif pwm_in_polling
 */
static void pwm_in_polling()
{
	uint32_t i;
	uint16_t tmp;
	int16_t relative_value;
	
	for(i=0;i<NUM_OF_IN_PWM;i++)
	{
		if(pwm_in_device[i].IsEnable == 1)
		{
			if(pwm_in_device[i].capture_done == 1)
			{
				pwm_in_device[i].capture_done = 0;
				if(pwm_in_device[i].hold_time>pwm_in_device[i].period)
				{
					pwm_in_device[i].hold_time %= pwm_in_device[i].period;
				}
				
				tmp = pwm_in_device[i].hold_time * IN_PWM_LEVEL / pwm_in_device[i].period;
				
				relative_value = tmp - pwm_in_device[i].absolute_value;
				if(relative_value > (IN_PWM_LEVEL/2))
				{
					relative_value -= IN_PWM_LEVEL;
				}
				
				if(relative_value < ((IN_PWM_LEVEL/2)*(-1)))
				{
					relative_value += IN_PWM_LEVEL;
				}
				
				pwm_in_device[i].absolute_value = tmp;
				pwm_in_device[i].relative_value += relative_value;
				
			}
		}
	}
}
/**
 * @breif pwm_in_send_command_polling
 */
void pwm_in_send_command_polling(void)
{
	uint32_t i;
	uint16_t tmp;
	int16_t relative_value;

	for(i=0;i<NUM_OF_IN_PWM;i++)
	{
		if(pwm_in_device[i].IsEnable == 1)
		{
			
			tmp = pwm_in_device[i].absolute_value;
			
			if(pwm_in_device[i].relative_value > 127)
			{
				relative_value = 127;//127
				pwm_in_device[i].relative_value = 0;
			}
			else if(pwm_in_device[i].relative_value < -128)
			{
				relative_value = -128; // -128
				pwm_in_device[i].relative_value = 0;
			}
			else if((pwm_in_device[i].relative_value > 2) || (pwm_in_device[i].relative_value < -2))
			{
				relative_value = (int16_t)pwm_in_device[i].relative_value;
				pwm_in_device[i].relative_value = 0;
			}
			else
			{
				relative_value = 0;
			}
			
			igs_command_add_dat(COMMAND_ROTARY_VR_RET);
			igs_command_add_dat((uint8_t)i); //id
			igs_command_add_dat(((uint8_t)(tmp & 0xFF)));
			igs_command_add_dat(tmp >> 8);
			igs_command_add_dat(relative_value);
			igs_command_insert(COMMAND_ROTARY_VR);
		}
	}
}
/**
 * @breif pwm_in_command
 */
static void pwm_in_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_ROTARY_VR_INIT: 
			pwm_in_command_init(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @breif pwm_in_command_init
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> IsEnable
 * 
 */
static void pwm_in_command_init(uint8_t *buf, uint32_t len)
{	
	uint8_t IsEnable;
	uint8_t id;
	
	id = buf[0];
	IsEnable = buf[1];
	
	if(id < NUM_OF_IN_PWM)
	{
		if(IsEnable == 0)
		{
			pwm_in_device[id].IsEnable = 0;
			/* Enable the TIM3 global Interrupt */
			NVIC_InitStructure.NVIC_IRQChannel = pwm_in_device[id].IRQ;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
			NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
			NVIC_Init(&NVIC_InitStructure);
		}
		else
		{
			pwm_in_device[id].IsEnable = 1;
			NVIC_InitStructure.NVIC_IRQChannel = pwm_in_device[id].IRQ;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
			NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
			NVIC_Init(&NVIC_InitStructure);
		}
	}
}

void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_CC3) == SET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC3);
		pwm_in_device[IN_PWM1_ID].period = TIM_GetCapture3(TIM3);
		TIM3->CNT = 0;
		pwm_in_device[IN_PWM1_ID].capture_done = 1;
	}
	
	if(TIM_GetITStatus(TIM3, TIM_IT_CC4) == SET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC4);
		pwm_in_device[IN_PWM1_ID].hold_time = TIM_GetCapture4(TIM3);
	}
	


}
