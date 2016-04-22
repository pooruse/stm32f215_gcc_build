#include "igs_fan.h"
#include "stm32f2xx.h"
#include "igs_task.h"
#include "igs_command.h"
#include "igs_protocol.h"

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_FAN_INIT 0x0
#define COMMAND_FAN_REPORT 0x1
#define COMMAND_FAN_SET 0x2

#define FAN_PWM_PERIOD 1000

#define FAN0_ID 0 
#define FAN0_OUT_GPIO_PIN GPIO_Pin_9
#define FAN0_OUT_GPIO_GROUP GPIOC
#define FAN0_OUT_GPIO_SOURCE GPIO_PinSource9
#define FAN0_IN_GPIO_PIN GPIO_Pin_8
#define FAN0_IN_GPIO_GROUP GPIOA
#define FAN0_IN_GPIO_SOURCE GPIO_PinSource8

/**
 *  @breif FAN DEVICE USER DEFINE STRUCTURE
 */
 
struct fan_device_t
{
	uint8_t IsEnable;
	uint8_t slow_check; /*< if fan is stop, it do not trigger the timer. check this bit. */
	TIM_TypeDef* TIMx;
	void (*SetCompare)(TIM_TypeDef*, uint32_t);
	uint16_t period;
};

struct fan_device_t fan_device[NUM_OF_FAN] = 
{
	{1, 0, TIM8, TIM_SetCompare4, 0,},
};


static void fan_polling(void);
static void fan_command(uint8_t *buf, uint32_t len);
static void fan_command_init(uint8_t *buf, uint32_t len);
static void fan_command_set(uint8_t *buf, uint32_t len);
static void fan_command_report(uint8_t *buf, uint32_t len);

void igs_fan_init()
{
	
	uint32_t i;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure; 
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	GPIO_PinAFConfig(FAN0_OUT_GPIO_GROUP, FAN0_OUT_GPIO_SOURCE, GPIO_AF_TIM8);
	
	/**
	 *  @breif PWM out setting 
	 */
	// Time base configuration 
	TIM_TimeBaseStructure.TIM_Period = FAN_PWM_PERIOD; 
	TIM_TimeBaseStructure.TIM_Prescaler = 8-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	
	TIM_OC4Init(TIM8, &TIM_OCInitStructure);
  TIM_OC4PreloadConfig(TIM8, TIM_OCPreload_Disable);
	
	TIM_Cmd(TIM8, ENABLE);
	TIM_CtrlPWMOutputs(TIM8, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = FAN0_OUT_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(FAN0_OUT_GPIO_GROUP, &GPIO_InitStructure); 

	for(i=0;i<NUM_OF_FAN;i++)
	{
		igs_fan_set_speed(i, DEFAULT_FAN_SPEED);
	}
	
	/**
	 *  @breif PWM in setting 
	 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_PinAFConfig(FAN0_IN_GPIO_GROUP, FAN0_IN_GPIO_SOURCE, GPIO_AF_TIM1);
	
	GPIO_InitStructure.GPIO_Pin = FAN0_IN_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(FAN0_IN_GPIO_GROUP, &GPIO_InitStructure); 

	
	// Time base configuration 
	TIM_TimeBaseStructure.TIM_Period = 0xFFFF; 
	TIM_TimeBaseStructure.TIM_Prescaler = 120-1; // 1 us TIM_CK
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM1, &TIM_ICInitStructure);
	
  // TIM enable counter 
  TIM_Cmd(TIM1, ENABLE);
	TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	igs_protocol_command_register(COMMAND_FAN,fan_command);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 1000,fan_polling);
}

/**
 * @breif igs_fan_set_speed
 * @param id
 *    device id
 * @param speed
 *    fan speed 0~100%
 */
void igs_fan_set_speed(uint32_t id, uint32_t speed)
{
	uint32_t tmp;
	if(id < NUM_OF_FAN)
	{
		if(speed > 100) 
		{
			tmp = 100;
		}
		else
		{
			tmp = speed;
		}
		
		tmp = FAN_PWM_PERIOD * tmp / 100;
		fan_device[id].SetCompare(fan_device[id].TIMx, tmp);
	}
}


/**
 * @breif TIM1_CC_IRQHandler
 */
void TIM1_CC_IRQHandler()
{
	if(TIM_GetITStatus(TIM1, TIM_IT_CC1) == SET)
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);
		fan_device[FAN0_ID].period = TIM_GetCapture1(TIM1);
		fan_device[FAN0_ID].slow_check = 0;
		TIM1->CNT = 0;
		
	}
}

/**
 * @breif fan_polling
 */
static void fan_polling(void)
{
	uint32_t i;
	for(i=0;i<NUM_OF_FAN;i++)
	{
		if(fan_device[i].IsEnable == 1)
		{
			if(fan_device[i].slow_check == 1)
			{
				fan_device[i].period = 0;
			}
			fan_device[i].slow_check = 1;
		}
	}
}

/**
 * @breif pwm_out_command
 */
static void fan_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_FAN_INIT: 
			fan_command_init(&buf[1],len-1);
			break;
		case COMMAND_FAN_REPORT:
			fan_command_report(&buf[1],len-1);
			break;
		case COMMAND_FAN_SET:
			fan_command_set(&buf[1],len-1);
			break;
		default: break;
	}
}
/**
 * @breif fan_command_init
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> IsEnable
 * 
 */
static void fan_command_init(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t IsEnable;
	id = buf[0];
	IsEnable = buf[1];
	if(id < NUM_OF_FAN)
	{
		fan_device[id].IsEnable = IsEnable;
		igs_fan_set_speed(id, 100);
	}
}

/**
 * @breif fan_command_report
 * @param buf
 *    buf[0] -> Device ID
 * 
 */
static void fan_command_report(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint32_t tmp;
	
	id = buf[0];
	if(id < NUM_OF_FAN)
	{
		tmp = fan_device[id].period;
		if(tmp != 0) 
		{
			tmp = 60*10e5 / tmp / 2;
		}
		igs_command_add_dat(COMMAND_FAN_REPORT);
		igs_command_add_dat(id);
		igs_command_add_dat((tmp >> 0) & 0xFF);
		igs_command_add_dat((tmp >> 8) & 0xFF);
		igs_command_add_dat((tmp >> 16) & 0xFF);
		igs_command_add_dat((tmp >> 24) & 0xFF);
		igs_command_insert(COMMAND_FAN);
	}

}


/**
 * @breif fan_command_set
 * @param buf
 *           buf[0] -> device id
 *           buf[1] -> Speed
 * 
 */
static void fan_command_set(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t speed;
	id = buf[0];
	if(id < NUM_OF_FAN)
	{
		igs_fan_set_speed(id, speed);
	}
}




