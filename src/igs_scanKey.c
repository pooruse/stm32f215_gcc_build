#include "igs_scanKey.h"
#include <stdint.h>
#include <string.h>
#include "igs_malloc.h"
#include "igs_task.h"
#include "igs_gpio.h"
#include "igs_command.h"
#include "igs_protocol.h"

#define NUM_OF_SCAN_KEY 1

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_SCAN_KEY_INIT 0x0
#define COMMAND_SCAN_KEY_SET_IN 0x1
#define COMMAND_SCAN_KEY_SET_OUT 0x2
#define COMMAND_SCAN_KEY_READ 0x3

static uint8_t tmp_buf[PACKAGE_LENGTH];

/**
 ** @brief device 0 parameters
 */
 
static uint8_t scan_key_0_inputs[4] =  {8, 9, 10, 11};
static uint8_t scan_key_0_outputs[3] = {17, 18, 19};
static uint8_t scan_key_0_buf[2];
struct scan_key_t
{
	uint8_t IsEnable;
	
	uint8_t num_of_in;
	uint8_t *inputs;
	uint8_t num_of_out;
	uint8_t *outputs;
	
	uint32_t data_len;
	uint8_t *data;
};

struct scan_key_t scan_key_device[NUM_OF_SCAN_KEY] =
{
	0, 4, scan_key_0_inputs, 3, scan_key_0_outputs, 2, scan_key_0_buf
};

static void scanKey_command(uint8_t *buf, uint32_t len);
static void scanKey_command_init(uint8_t *buf, uint32_t len);
static void scanKey_command_setIn(uint8_t *buf, uint32_t len);
static void scanKey_command_setOut(uint8_t *buf, uint32_t len);
static void scanKey_polling(void);


/**
 * @brief igs_scanKey_init
 * @param
 * @return
 */
static void scanKey_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_SCAN_KEY_INIT:
			scanKey_command_init(&buf[1],len-1);
			break;
		case COMMAND_SCAN_KEY_SET_IN:
			scanKey_command_setIn(&buf[1],len-1);
			break;
		case COMMAND_SCAN_KEY_SET_OUT:
			scanKey_command_setOut(&buf[1],len-1);
			break;
		
		default: break;
	}
}
/**
 * @brief scanKey_command_init
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] IsEnable
 * @param len
 *    buf length
 * @return
 */
static void scanKey_command_init(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t id;
	uint8_t IsEnable;
	uint32_t bit_size;
	
	id = buf[0];
	IsEnable = buf[1];
	if(id < NUM_OF_SCAN_KEY)
	{
		scan_key_device[id].IsEnable = IsEnable;
		if(IsEnable == 0)
		{
			for(i=0;i<scan_key_device[id].num_of_in;i++)
			{
				igs_gpio_set_input_mask(scan_key_device[id].inputs[i],ENABLE_GPIO_MASK);
			}
			for(i=0;i<scan_key_device[id].num_of_out;i++)
			{
				igs_gpio_set_output_mask(scan_key_device[id].outputs[i],ENABLE_GPIO_MASK);
			}
		}
		else
		{
			bit_size = scan_key_device[id].num_of_out * scan_key_device[id].num_of_in;
			if(bit_size == 0) return;
			
			for(i=0;i<scan_key_device[id].num_of_in;i++)
			{
				igs_gpio_set_input_mask(scan_key_device[id].inputs[i],DISABLE_GPIO_MASK);
			}
			for(i=0;i<scan_key_device[id].num_of_out;i++)
			{
				igs_gpio_set_output_mask(scan_key_device[id].outputs[i],DISABLE_GPIO_MASK);
				igs_gpio_set_out_real(scan_key_device[id].outputs[i], 0);
			}
		}
	}
}
	
/**
 * @brief scanKey_command_setIn
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] IsEnable
 * @param len
 *    buf length
 * @return
 */
static void scanKey_command_setIn(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t id;
	uint8_t io_id;
	id = buf[0];
	if(id < NUM_OF_SCAN_KEY)
	{
		if(scan_key_device[id].inputs != NULL) return;
		scan_key_device[id].inputs = igs_malloc(len-1);
		if(scan_key_device[id].inputs == NULL) return;
		
		
		for(i = 0; i < (len - 1); i++)
		{
			io_id = buf[i+1];
			scan_key_device[id].inputs[i] = io_id;
			igs_gpio_set_input_mask(io_id,DISABLE_GPIO_MASK);

		}
		scan_key_device[id].num_of_in = i;
	}
}

/**
 * @brief scanKey_command_setOut
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] IsEnable
 * @param len
 *    buf length
 * @return
 */
static void scanKey_command_setOut(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t id;
	uint8_t io_id;
	id = buf[0];
	if(id < NUM_OF_SCAN_KEY)
	{
		if(scan_key_device[id].outputs != NULL) return;
		scan_key_device[id].outputs = igs_malloc(len-1);
		if(scan_key_device[id].outputs == NULL) return;
		
		
		for(i = 0; i < (len - 1); i++)
		{
			io_id = buf[i+1];
			scan_key_device[id].outputs[i] = io_id;
			igs_gpio_set_out_real(io_id, 0);
			igs_gpio_set_output_mask(io_id,DISABLE_GPIO_MASK);
		}
		scan_key_device[id].num_of_out = i;
	}
}

/**
 * @brief scanKey_polling
 * @param
 * @return
 */
static void scanKey_polling(void)
{
	uint32_t bit_size;
	uint32_t buff_len;
	uint32_t i;
	uint32_t j;
	uint32_t out_i;
	uint32_t in_i;
	uint8_t tmp;
	uint8_t offset;
	uint32_t address;
	uint32_t index;
	uint8_t check_diff;

	for(i=0;i<NUM_OF_SCAN_KEY;i++)
	{
		if(scan_key_device[i].IsEnable == 1)
		{
			//Step 1. check setting is complete or not. if it isn't setting, do nothing
			bit_size = scan_key_device[i].num_of_out * scan_key_device[i].num_of_in;
			
			if(bit_size == 0) continue;
			
			//Step 2. check scan key buffer. if it don't exist, create it!
			if(scan_key_device[i].data_len == 0)
			{
				scan_key_device[i].data_len = CEILING(bit_size,8);
				scan_key_device[i].data = igs_malloc(scan_key_device[i].data_len);
				if(scan_key_device[i].data == NULL) 
				{
					// dynamic memory space is run out!
					continue; 
				}
			}
			
			//Step 3. Start Scan key
			check_diff = 0;
			index = 0;
			buff_len = bit_size/8+1;
			memset(tmp_buf, 0, buff_len);
			for(out_i = 0; out_i<scan_key_device[i].num_of_out; out_i++)
			{
				igs_gpio_set_out_real(scan_key_device[i].outputs[out_i], 1);
				
				for(in_i=0;in_i<scan_key_device[i].num_of_in;in_i++)
				{
					
					tmp = igs_gpio_get_in_real(scan_key_device[i].inputs[in_i]);
					
					index = in_i * scan_key_device[i].num_of_out + out_i;
					address = index / 8;
					offset = index % 8;
					tmp_buf[address] |= tmp << offset;
					//index++;

				}
				igs_gpio_set_out_real(scan_key_device[i].outputs[out_i], 0);
			}
			
			//Step 4. check difference
			for(j=0;j<buff_len;j++)
			{
				if(scan_key_device[i].data[j] != tmp_buf[j])
				{
					check_diff = 1;
					scan_key_device[i].data[j] = tmp_buf[j];
				}
			}
			
			//Step 5. send data
			if(check_diff == 1)
			{
				igs_command_add_dat(COMMAND_SCAN_KEY_READ);
				igs_command_add_dat(i); // scan key device id
				igs_command_add_buf(scan_key_device[i].data,buff_len);
				igs_command_insert(COMMAND_SCAN_KEY);
			}
		}
	}
}

/**
 * @brief igs_scanKey_init
 * @param
 * @return
 */
void igs_scanKey_init()
{
	igs_protocol_command_register(COMMAND_SCAN_KEY,scanKey_command);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 10,scanKey_polling);
}

