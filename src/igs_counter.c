#include "igs_counter.h"
#include "igs_pulseDevice.h"
#include "igs_gpio.h"
#include "igs_protocol.h"
#include "igs_command.h"
#include "igs_task.h"

#define COUNTER_SCAN_INTERVAL 2

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_COUNTER_INIT 0x0
#define COMMAND_COUNTER_ADD 0x1
#define COMMAND_COUNTER_ACK 0x1

#define MAX_NUM_OF_COUNTER 16 //total, coin, service

/**
 * @brief local function prototypes
 */
static void counter_polling(void);
static void counter_command(uint8_t *buf, uint32_t len);
static void counter_addDevice(uint8_t *buf, uint32_t len);
static void counter_control(uint8_t *buf, uint32_t len);


/**
 * @brief structure for counter
 */
struct counter_t
{
	uint8_t IsEnable; /**< device error flag */
	uint32_t IO_id; /**< output io id */
	uint32_t value; /**< number of count still not count */
	uint32_t ack_value; /**< number of count ack */
	uint32_t counter; /**< waveform count */
	
	/**
	 * @brief 
	 *    
	 *    low active
	 *    ----------|             |--------------|             |-------------
	 *              |  hold time  |  Setup time  |  hold time  |
	 *              |-------------|              |-------------|
	 *
	 *
	 *    high active
	 *                |-------------|              |-------------|
	 *                |  hold time  |  Setup time  |  hold time  |
	 *    ------------|             |--------------|             |------------
	 *
	 */
	uint32_t setupTime; /**< setup time */
	uint32_t holdTime; /**< hold time */
	uint32_t active_type; /**< low active (0) or high active (1) */
	
}counter[MAX_NUM_OF_COUNTER];

/**
 *  @brief igs_counter_init
 */
void igs_counter_init(void)
{
	igs_task_add(IGS_TASK_ROUTINE_POLLING, COUNTER_SCAN_INTERVAL,counter_polling);
	igs_protocol_command_register(COMMAND_COUNTER,counter_command);
}

/**
 *  @brief igs_counter_reset
 */
void igs_counter_reset(void)
{
	uint32_t i;
	for(i=0;i<MAX_NUM_OF_COUNTER;i++)
	{
		if(counter[i].IsEnable == 1)
		{
			counter[i].value = 0;
			pulseDevicePinSet(counter[i].IO_id, counter[i].active_type, DISABLE); //disable contorl pin
		}
	}
}


/**
 *  @brief counter_polling
 *     counter handler with the input parameter
 *  @param elapstedTime
 *     tell the function the period time (unit ms)
 *  @return
 */
static void counter_polling(void)
{
	uint32_t i;
	uint32_t sumOfActiveTime;
	uint32_t ret;
	
	for(i=0;i<MAX_NUM_OF_COUNTER;i++)
	{
		if(counter[i].IsEnable == 0)
		{
			continue;
		}
		
		if(counter[i].ack_value != 0)
		{
			igs_command_add_dat(COMMAND_COUNTER_ACK);
			igs_command_add_dat((uint8_t)i);
			if(counter[i].ack_value >= 0xFFFF)
			{
				igs_command_add_dat(0xFF);
				igs_command_add_dat(0xFF);
				ret = igs_command_insert(COMMAND_COUNTER);
				if(ret == 1)
				{
					counter[i].ack_value -= 0xFFFF;
				}
			}
			else
			{
				igs_command_add_dat(counter[i].ack_value & 0xFF); // 1 count
				igs_command_add_dat((counter[i].ack_value > 8) & 0xFF);
				ret = igs_command_insert(COMMAND_COUNTER);
				if(ret == 1)
				{
					counter[i].ack_value = 0;
				}
			}
		}
		
		sumOfActiveTime = counter[i].setupTime + counter[i].holdTime;
		
		if(counter[i].value != 0)
		{
			counter[i].counter += COUNTER_SCAN_INTERVAL;
			
			//setup time
			if(counter[i].counter < counter[i].setupTime)
			{
				pulseDevicePinSet(counter[i].IO_id, counter[i].active_type, DISABLE);
			}
			//hold time
			else if(counter[i].counter >= counter[i].setupTime && counter[i].counter < sumOfActiveTime)
			{
				pulseDevicePinSet(counter[i].IO_id, counter[i].active_type, ENABLE);
			}
			//over
			else if(counter[i].counter >= sumOfActiveTime)
			{
				counter[i].value--;
				counter[i].counter = 0;
				pulseDevicePinSet(counter[i].IO_id, counter[i].active_type, DISABLE);
				
				counter[i].ack_value++;

			}
		}
	}
}

/**
 * @brief counter_command
 *    counter command handler
 * @param buf
 *   format:  buf[0] sub command
 *            buf[1] command content1
 *            buf[2] command content2
 *            buf[3]         .
 *            buf[4]         .
 *            buf[5]         .
 * @param len
 *    buf length
 *  @return
 */
static void counter_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_COUNTER_INIT:
			counter_addDevice(&buf[1],len-1);
			break;
		case COMMAND_COUNTER_ADD:
			counter_control(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @brief counter_addDevice
 *    counter add device
 * @param buf
 *   format:  buf[0] counter id
 *            buf[1] device enable(1)/disable(0)
 *            buf[2] high/low active
 *            buf[3] io id
 *            buf[4] hold time
 *            buf[5] hold time
 *            buf[6] setup time
 *            buf[7] setup time
 * @param len
 *    buf length
 *  @return
 */
static void counter_addDevice(uint8_t *buf, uint32_t len)
{
	uint32_t id,io,setup,hold,tmp;
	uint8_t IsEnable;
	if(buf[0] < MAX_NUM_OF_COUNTER)
	{
		IsEnable = buf[1];
		

		
		tmp = buf[5];
		hold = buf[4] + (tmp << 8);
		
		tmp = buf[7];
		setup = buf[6] + (tmp << 8);
		id = buf[0];
		io = buf[3];
		
		counter[id].IsEnable = IsEnable; //enable/disable
		counter[id].IO_id = io; // IO
		counter[id].value = 0;
		counter[id].ack_value = 0;
		counter[id].counter = 0;
		counter[id].setupTime = setup;
		counter[id].holdTime = hold;
		counter[id].active_type= (buf[2]==0)?1:0; // active type

		if(IsEnable == 1)
		{
			pulseDevicePinSet(counter[id].IO_id, counter[id].active_type, DISABLE); //disable contorl pin
			igs_gpio_set_output_mask(io,DISABLE_GPIO_MASK);
		}
		else
		{
			igs_gpio_set_output_mask(io,ENABLE_GPIO_MASK);
		}

	}
}

/**
 * @brief counter_control
 *    add counter value
 * @param buf
 *   format:   buf[0] counter id 
 *             buf[1] value[7:0] 
 *             buf[2] value[15:8]
 *             buf[3] reset
 * @param len
 *    buf length
 * @return
 */
static void counter_control(uint8_t *buf, uint32_t len)
{
	uint32_t id,value;
	if(buf[0] < MAX_NUM_OF_COUNTER)
	{
		id = buf[0];
		if(buf[3] == 1)
		{
			counter[id].value = 0;
			counter[id].counter = 0;
			pulseDevicePinSet(counter[id].IO_id, counter[id].active_type, DISABLE); //disable contorl pin
		}
		else
		{
			value = buf[2];
			value = value << 8;
			value += buf[1];
			counter[id].value += value;
		}
	}
}
