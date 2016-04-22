#include <stdint.h>
#include "igs_handler.h"
#include "igs_gpio.h"
#include "igs_task.h"
#include "igs_protocol.h"
#include "igs_command.h"

#define LEFT 0
#define RIGHT 1

#define NUM_OF_HANDLER 4
#define MAX_HANDLER_COUNT 200

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_HANDLER_INIT 0x0
#define COMMAND_HANDLER_GET 0x1
#define OCMMAND_HANDLER_ENABLE_PIN 0x2

/**
 * @brief structure for handler
 */
struct handler_t
{
	uint8_t IsEnable;
	uint8_t in1; 
	uint8_t in2;
	uint8_t enable_pin;
	
	uint8_t type; /*< please check the handler_typex comment below  */
	uint8_t dir; /*< handler speed when it turn right */
	uint8_t state; /*< previous state */
	
	uint32_t speed; /*< handler speed when it turn left */
	uint32_t count; 
	
}handler[NUM_OF_HANDLER];

/**
 * @brief local function prototypes
 */
static void handler_polling(void);
static void handler_type0(uint32_t id, uint8_t cur_state);
static void handler_type1(uint32_t id, uint8_t cur_state);
static void handler_type2(uint32_t id, uint8_t cur_state);
static void handler_type3(uint32_t id, uint8_t cur_state);
static void handler_command(uint8_t *buf, uint32_t len);
static void handler_addDevice(uint8_t *buf, uint32_t len);
static void handler_set_enable_pin(uint8_t *buf, uint32_t len);

/**
 *  @brief igs_handler_init
 */
void igs_handler_init(void)
{
	
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 1,handler_polling);
	igs_protocol_command_register(COMMAND_HANDLER,handler_command);
}

/**
 *  @brief handler_command
 */
static void handler_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_HANDLER_INIT:
			handler_addDevice(&buf[1], len-1);
			break;
		case OCMMAND_HANDLER_ENABLE_PIN:
			handler_set_enable_pin(&buf[1], len-1);
			break;
		default: break;
	}
}

/**
 * @brief handler_addDevice
 *    add handler device
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 *              buf[2] type
 *              buf[3] input1 io id
 *              buf[4] input2 io id
 *              buf[5] enable_pin
 * @param len
 *    buf length
 * @return
 */
static void handler_addDevice(uint8_t *buf, uint32_t len)
{
	uint8_t IsEnable;
	uint8_t id;
	id = buf[0];
	if(id < NUM_OF_HANDLER)
	{
		IsEnable = buf[1];
		handler[id].IsEnable = IsEnable;
		handler[id].type = buf[2];
		handler[id].in1 = buf[3];
		handler[id].in2 = buf[4];
		handler[id].enable_pin = buf[5];
		handler[id].dir = 0;
		handler[id].speed = 0;
		handler[id].count = 0;
		handler[id].state = 0;
		if(IsEnable == 1)
		{
			igs_gpio_set_input_mask(handler[id].in1,DISABLE_GPIO_MASK);
			igs_gpio_set_input_mask(handler[id].in2,DISABLE_GPIO_MASK);
			igs_gpio_set_output_mask(handler[id].enable_pin,DISABLE_GPIO_MASK);
		}
		else
		{
			igs_gpio_set_input_mask(handler[id].in1,ENABLE_GPIO_MASK);
			igs_gpio_set_input_mask(handler[id].in2,ENABLE_GPIO_MASK);
			igs_gpio_set_output_mask(handler[id].enable_pin,ENABLE_GPIO_MASK);
		}
	}
}

/**
 * @brief handler_set_enable_pin
 *    set handler enable pin
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 * @param len
 *    buf length
 * @return
 */
static void handler_set_enable_pin(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	id = buf[0];
	if(id < NUM_OF_HANDLER)
	{
		if(handler[id].IsEnable == 0)
		{
			return;
		}
		if(buf[1] == 1)
		{
			igs_gpio_set_out_real(handler[id].enable_pin,1);
		}
		else
		{
			igs_gpio_set_out_real(handler[id].enable_pin,0);
		}
	}
}

/**
 *  @brief handler_polling
 */
static void handler_polling(void)
{
	uint32_t id;
	uint8_t cur_state;
	uint8_t in1_value,in2_value;
	uint32_t pre_speed;
	uint8_t pre_dir;
	for(id=0;id<NUM_OF_HANDLER;id++)
	{
		if(handler[id].IsEnable == 0)
		{
			continue;
		}

		pre_speed = handler[id].speed;
		pre_dir = handler[id].dir;
		
		// get current state
		in1_value = igs_gpio_get_in_real(handler[id].in1);
		in2_value = igs_gpio_get_in_real(handler[id].in2);
		cur_state = 0;
		cur_state = in1_value | (in2_value << 1);

		// handler check
		switch(handler[id].type)
		{
			case 0:
				handler_type0(id, cur_state);
				break;
			case 1:
				handler_type1(id, cur_state);
				break;
			case 2:
				handler_type2(id, cur_state);
				break;
			case 3:
				handler_type3(id, cur_state);
				break;
			default: break;
		}
		
		handler[id].state = cur_state;
		
		//if handler is too slow or stop, set right and left to 0
		if(handler[id].count >= MAX_HANDLER_COUNT)
		{
			handler[id].speed = 0;
		}
		else
		{
			handler[id].count++;
		}
		
		// report handler speed if the dir or speed of handler is change
		if(
			pre_speed != handler[id].speed ||
			pre_dir != handler[id].dir
		)
		{
			igs_command_add_dat(COMMAND_HANDLER_GET);
			igs_command_add_dat(id);
			igs_command_add_dat(handler[id].dir); 
			igs_command_add_buf((uint8_t *)&handler[id].speed,sizeof(uint32_t)); 
			igs_command_insert(COMMAND_HANDLER);
		}
		

	}
}

/**
 *  @brief handler_type0
 *     input1 and input2 are two pulse
 *  with 90 degree phase out to each other
 *  @param id
 *     handler device id
 *  @param cur_state
 */
static void handler_type0(uint32_t id, uint8_t cur_state)
{
	switch(handler[id].state)
	{
		case 1:
			switch(cur_state)
			{
				case 0:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = RIGHT;
					handler[id].count = 0;
					break;
				case 3:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = LEFT;
					handler[id].count = 0;
					break;
				case 2:
					handler[id].count = 0;
					break;
				default: 
					break;
			}
			break;
		case 0:
			switch(cur_state)
			{
				case 2:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = RIGHT;
					handler[id].count = 0;
					break;
				case 1:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = LEFT;
					handler[id].count = 0;
					break;
				case 3:
					handler[id].count = 0;
					break;
				default: 
					break;
			}
			break;
		case 2:
			switch(cur_state)
			{
				case 3:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = RIGHT;
					handler[id].count = 0;
					break;
				case 0:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = LEFT;
					handler[id].count = 0;
					break;
				case 1:
					handler[id].count = 0;
					break;
				default: 
					break;
			}
			break;
		case 3:
			switch(cur_state)
			{
				case 1:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = RIGHT;
					handler[id].count = 0;
					break;
				case 2:
					handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
					handler[id].dir = LEFT;
					handler[id].count = 0;
					break;
				case 0:
					handler[id].count = 0;
					break;
				default: 
					break;
			}
		break;
	}
}

/**
 *  @brief handler_type1
 *     input1 is on/off. on means handler turn left
 *     input2 is on/off. on means handler turn right
 *  @param id
 *     handler device id
 *  @param cur_state
 */
static void handler_type1(uint32_t id, uint8_t cur_state)
{
	if((cur_state & 0x01) != 0)
	{
		handler[id].speed = 1;
		handler[id].dir = LEFT;
		handler[id].count = 0;
	}
	else if((cur_state & 0x02) != 0)
	{
		handler[id].speed = 1;
		handler[id].dir = RIGHT;
		handler[id].count = 0;
	}
	else
	{
		handler[id].speed = 1;
		handler[id].count = 0;
	}
}

/**
 *  @brief handler_type2
 *     input1 is pulse. pulse frequency is equal to speed of handler
 *     input2 is on/off. each state means different direction
 *  @param id
 *     handler device id
 *  @param cur_state
 */
static void handler_type2(uint32_t id, uint8_t cur_state)
{
	uint8_t tmp1,tmp2;
	
	tmp1 = handler[id].state & 0x1;
	tmp2 = cur_state & 0x1;
	
	if((cur_state & 0x2) == 0)
	{
		if(tmp1 != tmp2)
		{
			handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
			handler[id].dir = LEFT;
			handler[id].count = 0;
		}
	}
	else
	{
		if(tmp1 != tmp2)
		{
			handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
			handler[id].dir = RIGHT;
			handler[id].count = 0;
		}
	}
}

/**
 *  @brief handler_type3
 *     input1 is pulse. pulse frequency is equal to speed of left (or right) of handler
 *		 input2 is pulse. pulse frequency is equal to speed of another direction of handler
 *  @param id
 *     handler device id
 *  @param cur_state
 */
static void handler_type3(uint32_t id, uint8_t cur_state)
{
	uint8_t tmp1,tmp2;
	uint8_t tmp3,tmp4;
	
	tmp1 = handler[id].state & 0x1;
	tmp2 = cur_state & 0x1;
	tmp3 = handler[id].state & 0x2;
	tmp4 = cur_state & 0x2;

	if(tmp1 != tmp2)
	{
		handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
		handler[id].dir = LEFT;
		handler[id].count = 0;
	}
	else if(tmp3 != tmp4)
	{
		handler[id].speed = MAX_HANDLER_COUNT - handler[id].count;
		handler[id].dir = RIGHT;
		handler[id].count = 0;
	}
}
