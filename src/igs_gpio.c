/**
 * @file igs_gpio.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS gpio driver
 */
 

#include "igs_gpio.h"
#include "igs_fan.h"
#include "igs_task.h"
#include "igs_command.h"
#include "igs_protocol.h"

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_GPIO_GET_VALUE 0x0
#define COMMAND_GPIO_WRITE 0x1
#define COMMAND_GPIO_READ 0x2
#define COMMAND_SWITCH_INIT 0x0
#define COMMAND_SWITCH_READ 0x1

#define NUM_OF_SWITCH 8
#define NUM_OF_INPUT 12
#define NUM_OF_OUTPUT 20
#define BUFFER_SIZE 8

#define NUM_OF_SWITCH_BUF CEILING(NUM_OF_SWITCH,BUFFER_SIZE)
#define NUM_OF_INPUT_BUF CEILING(NUM_OF_INPUT,BUFFER_SIZE)
#define NUM_OF_OUTPUT_BUF CEILING(NUM_OF_OUTPUT,BUFFER_SIZE)

/**
 *  @brief power good debounce pin
 */
#define PWG_IN_PORT GPIOF
#define PWG_IN_PIN GPIO_Pin_11
#define PWG_OUT_PORT GPIOA
#define PWG_OUT_PIN GPIO_Pin_12

// power good debounce behavior
//---------    0xFE   0xFC   0xF8   0xF0   0xE0   0xC0 & 0x3F == 0
//        |      |      |      |      |      |      | 
//        |_______________________________________________
#define PWG_GOOD_MASK 0x3F


/**
 * @brief debug LED light in PC board
 */
#define LED0_PORT GPIOD
#define LED0_PIN GPIO_Pin_9
#define LED1_PORT GPIOD
#define LED1_PIN GPIO_Pin_8
#define LED2_PORT GPIOB
#define LED2_PIN GPIO_Pin_15
//#define LED3_PERIPH RCC_AHB1Periph_GPIOB //this LED is for IKV initial LED initial in driver_led.c
//#define LED3_PORT GPIOB
//#define LED3_PIN GPIO_Pin_14

/**
 * @brief structure to convert gpio group and pin to id format
 *    gpio_map[id] = {gpio_port, gpio_pin}
 */

gpio_map_t input_map[NUM_OF_INPUT] =  /**< input mapping data register */
{
	{GPIOF,	GPIO_Pin_10, 1}, // DOWN
	{GPIOC,	GPIO_Pin_0, 1},  // UP
	{GPIOC,	GPIO_Pin_2, 1},  // SERV
	{GPIOC,	GPIO_Pin_3, 1},  // TEST
	{GPIOF,	GPIO_Pin_9, 1},  // Reserve
	{GPIOF,	GPIO_Pin_7, 1},  // COIN1
	{GPIOF,	GPIO_Pin_6, 1},  // COIN2
	{GPIOF,	GPIO_Pin_8, 1},  // TICKET_I
	{GPIOE,	GPIO_Pin_2, 1},  // MCU_KEY_IN1
	{GPIOE,	GPIO_Pin_3, 1},  // MCU_KEY_IN2
	{GPIOE,	GPIO_Pin_4, 1},  // MCU_KEY_IN3
	{GPIOE,	GPIO_Pin_5, 1},  // MCU_KEY_IN4
};

gpio_map_t switch_map[NUM_OF_SWITCH] =  /**< switch mapping data register */
{
	{GPIOB,	GPIO_Pin_7, 0},   // SW_D1
	{GPIOB,	GPIO_Pin_6, 0},   // SW_D2
	{GPIOB,	GPIO_Pin_5, 0},   // SW_D3
	{GPIOG,	GPIO_Pin_14, 0},  // SW_D4
	{GPIOG,	GPIO_Pin_13, 0},  // SW_D5
	{GPIOG,	GPIO_Pin_12, 0},  // SW_D6
	{GPIOG,	GPIO_Pin_11, 0},  // SW_D7
	{GPIOG,	GPIO_Pin_10, 0},  // SW_D8

};

/**
 * @info 
 *    output_map invert bit, not implement yet (2015/03/05 Sheng-Wen Peng)
 */
gpio_map_t output_map[NUM_OF_OUTPUT] = /**< output mapping data register */
{
	{GPIOE,	GPIO_Pin_10, 0}, // LED0 
	{GPIOE,	GPIO_Pin_11, 0}, // LED1
	{GPIOE,	GPIO_Pin_12, 0}, // LED2
	{GPIOE,	GPIO_Pin_13, 0}, // LED3
	{GPIOE,	GPIO_Pin_9 , 0}, // TICKET_O
	{GPIOD,	GPIO_Pin_14, 0}, // PWM1
	{GPIOD,	GPIO_Pin_13, 0}, // PWM2
	{GPIOD,	GPIO_Pin_12, 0}, // PWM3
	{GPIOE,	GPIO_Pin_7 , 0}, // COUNTER1
	{GPIOE,	GPIO_Pin_8 , 0}, // COUNTER2
	{GPIOF, GPIO_Pin_0 , 0}, // LED_CK
	{GPIOF, GPIO_Pin_1 , 0}, // LED_DATA1
	{GPIOF, GPIO_Pin_2 , 0}, // LED_DATA2
	{GPIOF, GPIO_Pin_3 , 0}, // LED_DATA3
	{GPIOF, GPIO_Pin_4 , 0}, // LED_DATA4
	{GPIOF,	GPIO_Pin_15, 0},  // LED4
	{GPIOG,	GPIO_Pin_0, 0},   // LED5
	{GPIOF,	GPIO_Pin_12, 1},  // MCU_KEY_OUT1
	{GPIOF,	GPIO_Pin_13, 1},  // MCU_KEY_OUT2
	{GPIOF,	GPIO_Pin_14, 1},  // MCU_KEY_OUT3
};

uint8_t igs_gpio_in_buf[NUM_OF_INPUT_BUF]; /**< store input data now in each bit */
uint8_t igs_switch_buf[NUM_OF_SWITCH_BUF]; /**< store switch data now in each bit */
uint8_t igs_gpio_out_buf[NUM_OF_OUTPUT_BUF]; /**< store output data now in each bit */

/**
 * @brief input/output mask, if the correspond bit is set to 0
 *    you can't change this pin value by the command
 */
uint8_t out_buf_mask[NUM_OF_OUTPUT_BUF];
uint8_t switch_buf_mask[NUM_OF_SWITCH_BUF];
uint8_t in_buf_mask[NUM_OF_INPUT_BUF];


/**
 * @brief local function prototype
 */
static void gpio_init(GPIO_TypeDef * port, uint32_t pin, GPIOPuPd_TypeDef PuPd, GPIOMode_TypeDef mode, BitAction value);
static void gpio_command(uint8_t *buf, uint32_t len);
static void gpio_command_write(uint8_t *buf, uint32_t len);
static void gpio_command_get_value(uint8_t *buf, uint32_t len);
static void gpio_command_set_switch_mask(uint8_t *buf, uint32_t len);
static void pwg_debounce_routine(void);

static uint32_t updateInput(void);
static void sendInput(void);
static void sendSwitch(void);
static void updateSwitch(void);
static void updateOutput(void);
static void updateIO(void);

/**
 * @brief gpio init function in ST32
 *    this define can let initial function be more readable
 */
static void gpio_init(GPIO_TypeDef * port, uint32_t pin, GPIOPuPd_TypeDef PuPd, GPIOMode_TypeDef mode, BitAction value) 
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_WriteBit(port,pin,value);
	GPIO_InitStruct.GPIO_Mode = mode;
	GPIO_InitStruct.GPIO_Pin = pin;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = PuPd;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_Init(port, &GPIO_InitStruct);
	GPIO_WriteBit(port,pin,value);
}

/**
 * @brief igs_gpio_toggle_led1
 *    for test the igs_task function and this is interrupt style 2ms blink 
 * @param
 * @return
 */
void igs_gpio_toggle_led1()
{
	GPIO_ToggleBits(LED0_PORT,LED0_PIN);
}

/**
 * @brief igs_gpio_toggle_led2
 *    for test the igs_task function and this is pollling style 2ms blink
 * @param
 * @return
 */
void igs_gpio_toggle_led2()
{
	GPIO_ToggleBits(LED1_PORT,LED1_PIN);
}

/**
 * @brief igs_gpio_toggle_led3
 *    for test the igs_task function and this is polling style 1s blink
 * @param
 * @return
 */
void igs_gpio_toggle_led3()
{
	GPIO_ToggleBits(LED2_PORT,LED2_PIN);
}

/**
 * @brief igs_gpio_set_led3
 *    
 * @param
 * @return
 */
void igs_gpio_set_led3()
{
	GPIO_SetBits(LED2_PORT,LED2_PIN);
}

/**
 *  @brief gpio_command
 */
static void gpio_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_GPIO_GET_VALUE:
			gpio_command_get_value(&buf[1],len-1);
			break;
		case COMMAND_GPIO_WRITE:
			gpio_command_write(&buf[1],len-1);
			break;
		default: break;
	}
}
/**
 * @brief gpio_command_get_value
 *    RK request GPIO status from MCU
 * @param buf
 *    command data from main chip
 * format:    buf[0] -> Enable  (reserve if Enable == 1 return status)

 * @param len buf length
 * @return 
 */
static void gpio_command_get_value(uint8_t *buf, uint32_t len)
{
	if(buf[0] == ENABLE)
	{
		sendInput();
	}
}
/**
 * @brief gpio_command_write
 *    resolve the command and change the bit data in output_buff
 * @param buf
 *    command data from main chip
 * format:    buf[0] -> GPIO ID1
 *            buf[1] -> GPIO ID1 value
 *            buf[2] -> GPIO ID2 
 *            buf[3] -> GPIO ID2 value
 *              .
 *              .
 *              .
 * @param len buf length
 * @return 
 */
static void gpio_command_write(uint8_t *buf, uint32_t len)
{
	
	uint32_t i;

	for(i=0;i<len;i+=2)
	{
		igs_gpio_set_out_buf(buf[i],buf[i+1]);
	}
}
/**
 * @brief gpio_command_set_switch_mask
 *    resolve the command and change the bit data in switch mask buffer
 * @param buf
 *    command data from main chip
 * format:    buf[0] -> 0x00
 *            buf[1] -> ID
 *            buf[2] -> STATE 
 *            buf[3] -> GPIO ID2 value
 *              .
 *              .
 *              .
 * @param len buf length
 * @return 
 */
static void gpio_command_set_switch_mask(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	uint8_t IsEnable;
	if(buf[0] != 0x00)
	{
		return;
	}
	id = buf[1];
	IsEnable = buf[2];
	if(IsEnable == 0)
	{
		switch_buf_mask[id] = 0x0;
	}
	else
	{
		switch_buf_mask[id] = 0xFF;
		sendSwitch();
	}
	
}

/**
 * @brief pwg_debounce_routine
 *    power good signal debounce
 * @param
 * @return
 */
void pwg_debounce_routine()
{
	uint32_t i;
	static uint8_t pwg_good_check = 0xFF;
	pwg_good_check <<= 1;
	pwg_good_check |= GPIO_ReadInputDataBit(PWG_IN_PORT,PWG_IN_PIN);
	if( (pwg_good_check & PWG_GOOD_MASK) == 0)
	{
		GPIO_ResetBits(PWG_OUT_PORT,PWG_OUT_PIN);
		for(i=0;i<NUM_OF_FAN;i++)
		{
			// turn off fan devices
			igs_fan_set_speed(i, 0);
		}
	}
	else if( (pwg_good_check & PWG_GOOD_MASK) == PWG_GOOD_MASK)
	{
		GPIO_SetBits(PWG_OUT_PORT,PWG_OUT_PIN);
		for(i=0;i<NUM_OF_FAN;i++)
		{
			// turn on fan devices
			igs_fan_set_speed(i, DEFAULT_FAN_SPEED);
		}
	}
}

/**
 * @brief sendInput
 *    update input buffer and send it
 * @param
 * @return
 */
static void sendInput(void)
{
	updateInput();
	igs_command_add_buf(igs_gpio_in_buf,NUM_OF_INPUT_BUF);
	igs_command_insert(COMMAND_GPIO);
}
/**
 * @brief updateInput
 *    update input buffer from the board
 * @param
 * @return
 */
static uint32_t updateInput(void)
{
	int32_t i,j;
	uint32_t id;
	uint8_t tmp;
	uint32_t send_f;
	
	send_f = 0;
	for(i=NUM_OF_INPUT_BUF-1;i>=0;i--)
	{
		tmp = 0;
		for(j=BUFFER_SIZE-1;j>=0;j--)
		{
			id = i*8+j;
			tmp = tmp << 1;
			if( id < NUM_OF_INPUT )
			{
				tmp |= GPIO_ReadInputDataBit(input_map[id].group,input_map[id].pin);
				tmp ^= input_map[id].inverse;
			}
		}

		if(igs_gpio_in_buf[i] != tmp)
		{
			igs_gpio_in_buf[i] = tmp;
			send_f = 1;
		}
	}
	
	return send_f;
}

/**
 * @brief sendSwitch
 *    scan and send switch data
 * @param
 * @return
 */
static void sendSwitch(void)
{
	int32_t i,j;
	uint32_t id;
	uint8_t tmp;
	
	for(i=NUM_OF_SWITCH_BUF-1;i>=0;i--)
	{
		tmp = 0;
		for(j=BUFFER_SIZE-1;j>=0;j--)
		{
			id = i*8+j;
			tmp = tmp << 1;
			if( id < NUM_OF_INPUT )
			{
				tmp |= GPIO_ReadInputDataBit(switch_map[id].group,switch_map[id].pin);
				tmp ^= switch_map[id].inverse;
			}
		}
		
		//if igs_gpio_switch_buf_mask[i] contains any 0 bit in it, return
		if( (~switch_buf_mask[i] & 0xFF) != 0)
		{
			return;
		}
		tmp = ~tmp;
		igs_switch_buf[i] = tmp;
		
		igs_command_add_dat(COMMAND_SWITCH_READ);
		igs_command_add_dat((uint8_t)i);
		igs_command_add_dat(tmp);
		igs_command_insert(COMMAND_SWITCH);
		igs_switch_buf[i] = tmp;
	}
}

/**
 * @brief updateSwitch
 *    update input buffer from the board
 * @param
 * @return
 */
static void updateSwitch(void)
{
	int32_t i,j;
	uint32_t id;
	uint8_t tmp;
	
	for(i=NUM_OF_SWITCH_BUF-1;i>=0;i--)
	{
		tmp = 0;
		for(j=BUFFER_SIZE-1;j>=0;j--)
		{
			id = i*8+j;
			tmp = tmp << 1;
			if( id < NUM_OF_INPUT )
			{
				tmp |= GPIO_ReadInputDataBit(switch_map[id].group,switch_map[id].pin);
				tmp ^= switch_map[id].inverse;
			}
		}
		
		//if igs_gpio_switch_buf_mask[i] contains any 0 bit in it, return
		if( (~switch_buf_mask[i] & 0xFF) != 0)
		{
			return;
		}
		tmp = ~tmp;
		if(igs_switch_buf[i] != tmp)
		{
			
			igs_command_add_dat(COMMAND_SWITCH_READ);
			igs_command_add_dat((uint8_t)i);
			igs_command_add_dat(tmp);
			igs_command_insert(COMMAND_SWITCH);
			igs_switch_buf[i] = tmp;
		}
	}
}
/**
 * @brief updateOutput
 *    update output buffer from the board
 * @param
 * @return
 */
static void updateOutput(void)
{
	int32_t i,j;
	uint32_t id;
	uint8_t tmp,mask;
	uint32_t group,offset;
	// output scan if output buffer is change
	for(i=0;i<NUM_OF_OUTPUT_BUF;i++)
	{
		
		for(j=0;j<BUFFER_SIZE;j++)
		{
			
			id = i*8+j;
			group = id / BUFFER_SIZE;
			offset = id % BUFFER_SIZE;
			
			tmp = 0x1 << offset;
			tmp = ((igs_gpio_out_buf[group]&tmp) == tmp);

			mask = 0x1 << offset;
			mask &= out_buf_mask[group];
			if( id < NUM_OF_OUTPUT && mask != 0)
			{
				GPIO_WriteBit(output_map[id].group,output_map[id].pin,(BitAction)((tmp & 0x1) ^ output_map[id].inverse));
			}
			tmp = tmp >> 1;
		}
	}
}
/**
 * @brief updateIO
 *    update input and output buffer from the board,
 * we call this function periodically
 * @param
 * @return
 */
static void updateIO()
{
	uint32_t send_f;
	// input scan

	send_f = updateInput();
	
	// send command if input is change
	if(send_f == 1)
	{
		igs_command_add_buf(igs_gpio_in_buf,NUM_OF_INPUT_BUF);
		igs_command_insert(COMMAND_GPIO);
	}
	
	updateOutput();
	updateSwitch();
	
}

/**
 * @brief igs_gpio_init
 *    igs gpio init function
 * @param
 * @return
 */
void igs_gpio_init()
{
	uint32_t i;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
	
	gpio_init(PWG_IN_PORT,PWG_IN_PIN,GPIO_PuPd_DOWN, GPIO_Mode_IN, (BitAction)0);
	gpio_init(PWG_OUT_PORT,PWG_OUT_PIN,GPIO_PuPd_DOWN,GPIO_Mode_OUT, (BitAction)1);
	
	gpio_init(LED0_PORT,LED0_PIN,GPIO_PuPd_NOPULL,GPIO_Mode_OUT, (BitAction)1);
	gpio_init(LED1_PORT,LED1_PIN,GPIO_PuPd_NOPULL,GPIO_Mode_OUT, (BitAction)1);
	gpio_init(LED2_PORT,LED2_PIN,GPIO_PuPd_NOPULL,GPIO_Mode_OUT, (BitAction)1);
	
	for(i=0;i<NUM_OF_INPUT;i++)
	{
		gpio_init(input_map[i].group, input_map[i].pin,GPIO_PuPd_NOPULL,GPIO_Mode_IN,(BitAction)0);
	}
	
	for(i=0;i<NUM_OF_SWITCH;i++)
	{
		gpio_init(switch_map[i].group, switch_map[i].pin,GPIO_PuPd_NOPULL,GPIO_Mode_IN,(BitAction)0);
	}	
	
	for(i=0;i<NUM_OF_OUTPUT;i++)
	{
		gpio_init(output_map[i].group, output_map[i].pin,GPIO_PuPd_NOPULL,GPIO_Mode_OUT,(BitAction)output_map[i].inverse);
	}
	
	//initial each register value
	for(i=0;i<NUM_OF_OUTPUT_BUF;i++)
	{
		out_buf_mask[i] = 0xFF;
	}
	for(i=0;i<NUM_OF_INPUT_BUF;i++)
	{
		in_buf_mask[i] = 0xFF;
	}
	for(i=0;i<NUM_OF_SWITCH_BUF;i++)
	{
		switch_buf_mask[i] = 0xFF;
	}
	
	updateOutput();
	sendInput();
	
	//register output command
	igs_protocol_command_register(COMMAND_GPIO,gpio_command);
	igs_protocol_command_register(COMMAND_SWITCH,gpio_command_set_switch_mask);
	//periodically tasks
	igs_task_add(IGS_TASK_ROUTINE_INT, 2,igs_gpio_toggle_led1);
	i = igs_task_add(IGS_TASK_ROUTINE_POLLING, 1000,igs_gpio_toggle_led2);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 2,updateIO);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 1,pwg_debounce_routine);
}

/**
 * @brief igs_gpio_set_input_mask
 *    set the output gpio mask 
 * @param id
 *    GPIO ID define by user
 * @param val 
 *     ENABLE_GPIO_MASK: output can be change by command
 *     DISABLE_GPIO_MASK: output can't be change by command
 * @return
 */
void igs_gpio_set_input_mask(uint32_t id, uint8_t val)
{
	uint8_t group,offset;
	uint8_t tmp;
	
	if(id >=  NUM_OF_INPUT)
	{
		return;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	
	tmp = 0x1; 
	tmp = tmp << offset;
	
	if(val == 0)
	{
		in_buf_mask[group] &= ~tmp;
	}
	else
	{
		in_buf_mask[group] |= tmp;
	}
}

/**
 * @brief igs_gpio_set_switch_mask
 *    set the switch gpio mask 
 * @param id
 *    GPIO ID define by user
 * @param val 
 *     ENABLE_GPIO_MASK: output can be change by command
 *     DISABLE_GPIO_MASK: output can't be change by command
 * @return
 */
void igs_gpio_set_switch_mask(uint32_t id, uint8_t val)
{
	uint8_t group,offset;
	uint8_t tmp;
	
	if(id >= NUM_OF_SWITCH)
	{
		return;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	
	tmp = 0x1; 
	tmp = tmp << offset;
	
	if(val == 0)
	{
		switch_buf_mask[group] &= ~tmp;
	}
	else
	{
		switch_buf_mask[group] |= tmp;
	}
}

/**
 * @brief igs_gpio_set_output_mask
 *    set the output gpio mask 
 * @param id
 *    GPIO ID define by user
 * @param val 
 *     ENABLE_GPIO_MASK: output can be change by command
 *     DISABLE_GPIO_MASK: output can't be change by command
 * @return
 */
void igs_gpio_set_output_mask(uint32_t id, uint8_t val)
{
	uint8_t group,offset;
	uint8_t tmp;
	
	if(id >= NUM_OF_OUTPUT)
	{
		return;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	
	tmp = 0x1; 
	tmp = tmp << offset;
	
	if(val == 0)
	{
		out_buf_mask[group] &= ~tmp;
	}
	else
	{
		out_buf_mask[group] |= tmp;
	}
}

/**
 * @brief igs_gpio_set_out_buf
 *    set each bit value in output_buffer, output_buffer_mask is no effect here
 * @param id
 *    gpio id 
 * @param val
 *    gpio value
 * @return
 */
void igs_gpio_set_out_buf(uint32_t id, uint8_t val)
{
	uint8_t group,offset;
	uint8_t tmp;

	if(id >= NUM_OF_OUTPUT)
	{
		return;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	tmp = 0x1; 
	tmp = tmp << offset;
	tmp &= out_buf_mask[group];
	if(val == 0)
	{
		igs_gpio_out_buf[group] &= ~tmp;
	}
	else
	{
		igs_gpio_out_buf[group] |= tmp;
	}
}

/**
 * @brief igs_gpio_get_in_buf
 *    get input value from the input_buffer
 * @param id
 *     id
 * @return gpio value
 */
uint8_t igs_gpio_get_in_buf(uint32_t id)
{
	
	uint8_t group,offset;
	uint8_t tmp;

	if(id >= NUM_OF_INPUT)
	{
		return 1;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	tmp = 0x1; 
	tmp = tmp << offset;
	
	tmp = igs_gpio_in_buf[group] & tmp;
	tmp &= in_buf_mask[group];
	if(tmp == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
	
}

/**
 * @brief igs_gpio_get_switch_buf
 *    get input value from the input_buffer
 * @param id
 *     id
 * @return gpio value
 */
uint8_t igs_gpio_get_switch_buf(uint32_t id)
{
	
	uint8_t group,offset;
	uint8_t tmp;

	if(id >= NUM_OF_SWITCH)
	{
		return 1;
	}
	
	group = id / BUFFER_SIZE;
	offset = id % BUFFER_SIZE;
	tmp = 0x1; 
	tmp = tmp << offset;
	
	tmp = igs_switch_buf[group] & tmp;
	tmp &= switch_buf_mask[group];
	if(tmp == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
	
}


/**
 * @brief igs_gpio_set_out_real
 *    get input value from the MCU real pin
 * @param id
 *     id
 * @return gpio value
 */
void igs_gpio_set_out_real(uint32_t id, uint8_t val)
{
	uint32_t tmp;
	if(id >= NUM_OF_OUTPUT)
	{
		return;
	}
	tmp = (val & 0x1) ^ output_map[id].inverse;
	GPIO_WriteBit(output_map[id].group,output_map[id].pin,(BitAction)(tmp & 0x1));
}

/**
 * @brief igs_gpio_get_in_real
 *    set output value to the MCU real pin
 * @param id
 *     id
 * @return gpio value
 */
uint8_t igs_gpio_get_in_real(uint32_t id)
{
	uint8_t tmp;
	
	if(id >= NUM_OF_INPUT)
	{
		return 1;
	}
	
	
	tmp = GPIO_ReadInputDataBit(input_map[id].group,input_map[id].pin);
	return tmp^input_map[id].inverse;
}
