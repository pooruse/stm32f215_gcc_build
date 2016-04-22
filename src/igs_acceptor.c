#include "igs_acceptor.h"
#include "igs_pulseDevice.h"
#include "igs_gpio.h"
#include "igs_protocol.h"
#include "igs_command.h"
#include "igs_task.h"

#define ACCEPTOR_SCAN_INTERVAL 2

/** 
 *  @brief spi subpackage command list
 */
 
#define COMMAND_ACCEPT_INIT 0x0
#define COMMAND_ACCEPT_CTL 0x1
#define OCMMAND_ACCEPT_REPORT_DELAY_SET 0x2

#define MAX_NUM_OF_ACCEPTOR 16 // coin
#define INHABIT_PIN_ACTIVE_TYPE 1 /**< pay in device inhabit pin active type */


/**
 * @brief structure for coin
 */
struct acceptor_t
{
	uint32_t IsErr; /**< device error flag */
	uint8_t IsEnable; /**< enable/disable this device */
	uint32_t value; /**<acceptor coin temporarily value store in MCU */
	uint32_t ack_value; /**< acceptor value store in MCU */
	uint32_t err_value; /**< number of coin error, when spi communication is hold */
	uint32_t input_IO_id; /**< input io id */
	uint32_t output_IO_id; /**< output io id (inhabit pin) */
	uint32_t counterActive; /**< counter for coin */
	
	/**
	 * @brief if the input signal length is between bouncingTime and activeTime,
	 *    it is a valid input.
	 */
	uint32_t min_of_confirm_range; /**<  */
	uint32_t max_of_confirm_range; /**<  */
	uint32_t jam_time; /**<  */
	uint8_t input_active_type; /**< low active (0) or high active (1) */
	
	/**
	 *  @brief bill acceptor need those two parameters
	 *  acceptor will send value when report_count reach report_delay
	 */
	uint32_t report_delay;
	uint32_t report_count;
	
	/**
	 *  @brief when bounceInValue&DEBOUNCE_MASK == 0 , currentInValue = 0
	 *         or   currentInValue = 1
	 */
	uint8_t bounceInValue;
	uint8_t currentInValue;
	uint8_t previousInValue; /**< we scan io per 2ms, this register contains save the last one io value */
}acceptor[MAX_NUM_OF_ACCEPTOR];


/**
 * @brief local function prototypes
 */
static void acceptor_polling(void);
static void acceptor_command(uint8_t *buf, uint32_t len);
static void acceptor_control(uint8_t *buf, uint32_t len);
static void acceptor_addDevice(uint8_t *buf, uint32_t len);
static void acceptor_report_delay_set(uint8_t *buf, uint32_t len);

void igs_acceptor_init(void)
{
	igs_task_add(IGS_TASK_ROUTINE_POLLING, ACCEPTOR_SCAN_INTERVAL,acceptor_polling);
	igs_protocol_command_register(COMMAND_ACCEPT,acceptor_command);
}


/**
 * @brief acceptorRoutine
 *    pay in device handler with time input
 * @param elapstedTime
 *    period function time
 * @return
 */
static void acceptor_polling(void)
{
	uint32_t i;
	uint8_t valueTmp;
	uint32_t ret;
	for(i=0;i<MAX_NUM_OF_ACCEPTOR;i++)
	{
		if(acceptor[i].IsEnable != 1)
		{
			continue;
		}
		
		/**
		 *  @brief if ack_value is not zero
		 *  try to send ack message by spi before every scan
		 *  this program block is work when spi communication is not stable.
		 *  ex. If RK3288 (or another cpu) is communicate with MCU per 1s or more,
		 *  It is possible that some dispenser/acceptor input success (ex. coin in or out).
		 *  In this case, if the input success package is too much, 
		 *  MCU can't send it in one communication, and it will loss input data. 
		 */ 
		if(acceptor[i].ack_value != 0)
		{
				igs_command_add_dat(COMMAND_ACCEPT_CTL);
				igs_command_add_dat((uint8_t)i); // id
				if(acceptor[i].ack_value >= 0xFF)
				{
					igs_command_add_dat(0xFF); //pay in
					igs_command_add_dat(acceptor[i].IsErr); // error flag
					ret = igs_command_insert(COMMAND_ACCEPT);
					if(ret == 1)
					{
						acceptor[i].ack_value -= 0xFF;
					}
				}
				else
				{
					igs_command_add_dat(acceptor[i].ack_value); // pay in value
					igs_command_add_dat(acceptor[i].IsErr); // error flag 
					ret = igs_command_insert(COMMAND_ACCEPT);
					if(ret == 1)
					{
						acceptor[i].ack_value = 0;
					}
				}
		}
		
		/**
		 *  @brief report delay
		 *  if you use bill acceptor and you need know the value of bill,
		 *  this program block will help you delete the bill value
		 */
		if(acceptor[i].report_count >= acceptor[i].report_delay)
		{
			if(acceptor[i].value != 0)
			{
				acceptor[i].ack_value = acceptor[i].value;
				acceptor[i].value = 0;
			}
		}
		else
		{
			acceptor[i].report_count += ACCEPTOR_SCAN_INTERVAL;
		}
		
		/**
		 *  @brief error report manage
		 *  try to send error pacakge.
		 *  if fail, try again next time
		 */
		if(acceptor[i].err_value != 0)
		{
				igs_command_add_dat(COMMAND_ACCEPT_CTL);
				igs_command_add_dat((uint8_t)i);
				igs_command_add_dat(0);
				igs_command_add_dat(1);
				ret = igs_command_insert(COMMAND_ACCEPT);
				if(ret == 1)
				{
					acceptor[i].IsErr = 1;
					acceptor[i].err_value = 0;
				}
		}
		
		// error occur
		if(acceptor[i].counterActive >= acceptor[i].jam_time)
		{
			if(acceptor[i].IsErr != 1)
			{
				acceptor[i].err_value++;
			}
		}
		
		// catch input value
		valueTmp = igs_gpio_get_in_real(acceptor[i].input_IO_id);
		
		//debounce
		acceptor[i].bounceInValue <<= 1;
		acceptor[i].bounceInValue |= valueTmp;
		if((acceptor[i].bounceInValue & DEBOUNCE_MASK) == 0)
		{
			acceptor[i].currentInValue = 0;
		}
		else if((acceptor[i].bounceInValue & DEBOUNCE_MASK) == DEBOUNCE_MASK)
		{
			acceptor[i].currentInValue = 1;
		}
		
		// counter add when the input is active
		if(acceptor[i].currentInValue == acceptor[i].input_active_type)
		{
			acceptor[i].counterActive += ACCEPTOR_SCAN_INTERVAL;
		}
		
		if(
			acceptor[i].currentInValue != acceptor[i].previousInValue && // value is different
			 acceptor[i].currentInValue != acceptor[i].input_active_type) // and now is inactive
		{
			if(acceptor[i].counterActive >= acceptor[i].min_of_confirm_range &&
				 acceptor[i].counterActive <= acceptor[i].max_of_confirm_range   )
			{
				
				//valid input
				acceptor[i].report_count = 0;
				acceptor[i].value++;
			}

			acceptor[i].counterActive = 0;
		}
		acceptor[i].previousInValue = acceptor[i].currentInValue;
	}
}

static void acceptor_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_ACCEPT_INIT:
			acceptor_addDevice(&buf[1],len-1);
			break;
		case COMMAND_ACCEPT_CTL:
			acceptor_control(&buf[1],len-1);
			break;
		case OCMMAND_ACCEPT_REPORT_DELAY_SET:
			acceptor_report_delay_set(&buf[1],len-1);
			break;
		default:break;
	}
}

/**
 * @brief acceptor_control
 *    pay in device control command handler
 * @param buf
 *   format:   buf[0] device id
 *             buf[1] clear err
 *             buf[2] inhibit pin control
 * @param len
 *    buf length
 */
static void acceptor_control(uint8_t *buf, uint32_t len)
{
	uint32_t id;
	if(buf[0] < MAX_NUM_OF_ACCEPTOR)
	{
		id = buf[0];
		
		// clear err
		if(buf[1] == 1) //reset
		{
			acceptor[id].IsErr = 0;
			acceptor[id].err_value = 0;
			igs_command_add_dat(COMMAND_ACCEPT_CTL);
			igs_command_add_dat(id);
			igs_command_add_dat(0);
			igs_command_add_dat(acceptor[id].IsErr);
			igs_command_insert(COMMAND_ACCEPT);
		}
		if(buf[2] == 1) //inhabit pin
		{
			pulseDevicePinSet(acceptor[id].output_IO_id, INHABIT_PIN_ACTIVE_TYPE,ENABLE); //enable contorl pin
		}
		else
		{
			pulseDevicePinSet(acceptor[id].output_IO_id, INHABIT_PIN_ACTIVE_TYPE,DISABLE); //disable contorl pin
		}
	}
}

/**
 * @brief acceptor_addDevice
 *    pay out device add
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 *              buf[2] input active type
 *              buf[3] input io id
 *              buf[4] output io id (inhabit pin)
 *              buf[5] min_of_confirm_range[7:0]
 *              buf[6] max_of_confirm_range[7:0]
 *              buf[7] jam time value[7:0] 
 * @param len
 *    buf length
 * @return
 */
static void acceptor_addDevice(uint8_t *buf, uint32_t len)
{
	uint8_t IsEnable;
	uint32_t id;
	if(buf[0] < MAX_NUM_OF_ACCEPTOR)
	{
		id = buf[0];
		IsEnable = buf[1];
		
		acceptor[id].IsEnable = IsEnable;
		acceptor[id].IsErr = 0;
		acceptor[id].err_value = 0;
		acceptor[id].ack_value = 0;
		acceptor[id].input_IO_id = buf[3];
		acceptor[id].output_IO_id = buf[4];
		acceptor[id].counterActive = 0;
		
		acceptor[id].min_of_confirm_range = buf[5];
		acceptor[id].max_of_confirm_range = buf[6];
		acceptor[id].jam_time = buf[7];
		
		acceptor[id].input_active_type = buf[2]?0:1;
		acceptor[id].previousInValue = buf[2]?1:0;
		
		if(IsEnable == 1)
		{
			igs_gpio_set_output_mask(acceptor[id].output_IO_id ,DISABLE_GPIO_MASK);
			igs_gpio_set_input_mask(buf[3],DISABLE_GPIO_MASK);
		}
		else
		{
			igs_gpio_set_output_mask(acceptor[id].output_IO_id ,ENABLE_GPIO_MASK);
			igs_gpio_set_input_mask(buf[3],ENABLE_GPIO_MASK);
		}
	}
}

/**
 * @brief acceptor_addDevice
 *    pay out device add
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 *              buf[2] report_delay[7:0]
 *              buf[3] report_delay[7:0]
 * @param len
 *    buf length
 * @return
 */
static void acceptor_report_delay_set(uint8_t *buf, uint32_t len)
{
	uint32_t id;
	id = buf[0];
	if(id < MAX_NUM_OF_ACCEPTOR)
	{
		if(buf[1] == 1)
		{
			acceptor[id].report_delay  = buf[2];
			acceptor[id].report_delay += buf[3] << 8;
			acceptor[id].report_count = 0;
		}
		else
		{
			acceptor[id].report_delay = 0;
			acceptor[id].report_count = 0;
		}
	}
}
