#include "igs_pulseDevice.h"
#include "igs_dispenser.h"
#include "igs_gpio.h"
#include "igs_protocol.h"
#include "igs_command.h"
#include "igs_task.h"

#define DISPENSER_SCAN_INTERVAL 2

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_DISPENSER_INIT 0x0
#define COMMAND_DISPENSER_CTL 0x1
#define COMMAND_DISPENSER_RVI_SET 0x2

#define MAX_NUM_OF_DISPENSER 16 //hopper, ticket

/**
 * @brief define the payout device state
 */
#define PAYOUT_DEVICE_OK 0 /**< hopper/ticket is ok */
#define PAYOUT_DEVICE_EMPTY 1 /**< hopper/ticket is empty */
#define PAYOUT_DEVICE_JAM 2 /**< hopper/ticket is jam */

/**
 * @brief structure for ticket/hopper
 */
struct dispenser_t
{
	uint8_t IsEnable; /**< device error flag */
	uint32_t ack_value;
	uint32_t err_value; /**< device error flag */
	uint32_t input_IO_id; /**< input io id */
	
	uint32_t output_IO_id_forward; /**< output io id  */ 
	uint32_t output_IO_id_backward; /**< output io id  */ 
	uint32_t value; /**< the pay out ticket/coin value */
	
	uint32_t counterResponse; /**< counter if the hopper is no action */
	uint32_t counterActive; /**< counter the input active signal */
	
	uint8_t input_active_type; /**< input io active type, low active (0) or high active (1) */
	uint8_t output_active_type; /**< output io active type, low active (0) or high active (1) */
	
	uint32_t jam_time;
	uint32_t responseTime; /**< if payout device is no response default 10s */
	
	/**
	 * @brief if the input signal length is between bouncingTime and activeTime,
	 *    it is a valid input.
	 */
	uint32_t min_of_confirm_range;
	uint32_t max_of_confirm_range;
	
	/**
	 * @brief RVI_time = Resonable Valid Input interval Time
     *        RVI_count = Resonable Valid Input interval count
     * ex. if the dispenser can dispense 1 unit per second in max speed physically
     * resonable_valid_input_interval can set to 1000 (unit: ms)
     * dispenser will ignore all valid input upon this time, because this must be noise
     *              ___     ___                 ___
     *              | |     | |                 | |
     *              | |     | |                 | |
     * _____________| |_____| |_________________| |_____________________
     *              value++                     value++
     * all this three pulse is valid input, but the second pulse was ignored,
     * because the setting of RVI_time
	 */
    uint32_t RVI_time;
    uint32_t RVI_count;
	
	/**
	 *  @brief when bounceInValue&DEBOUNCE_MASK == 0 , currentInValue = 0
	 *         or   currentInValue = 1
	 */
	uint8_t bounceInValue;
	uint8_t currentInValue;
	uint8_t previousInValue; /**< we scan io per 2ms, this register contains save the last one io value */

	/**
	 * @brief waveform timing spec          
	 */
	 
	 uint8_t wave_IsStart; 
	 uint32_t counterWaveform; /**< hopper pattern count */
	 uint32_t wave_forward_time;
	 uint32_t wave_stop_time;
	 uint32_t wave_backward_time;

}dispenser[MAX_NUM_OF_DISPENSER];

/**
 * @brief local function prototypes
 */
static void dispenser_polling(void);
static void dispenser_command(uint8_t *buf, uint32_t len);
static void dispenser_control(uint8_t *buf, uint32_t len);
static void dispenser_addDevice(uint8_t *buf, uint32_t len);
static void dispenser_rvi_set(uint8_t *buf, uint32_t len);

/**
 *  @brief igs_dispenser_init
 */
void igs_dispenser_init(void)
{
	igs_task_add(IGS_TASK_ROUTINE_POLLING, DISPENSER_SCAN_INTERVAL,dispenser_polling);
	igs_protocol_command_register(COMMAND_DISPENSER,dispenser_command);
}

void igs_dispenser_reset(void)
{
	uint32_t i;
	for(i=0;i<MAX_NUM_OF_DISPENSER;i++)
	{
		if(dispenser[i].IsEnable == 1)
		{
			dispenser[i].value = 0;
			dispenser[i].err_value = 0;
			dispenser[i].wave_IsStart = 0;
		}
	}
}

/**
 * @brief dispenserRoutine
 *    pay out device handler with input
 * @param elapstedTime
 *    tell the function the period time (unit ms)
 * @return
 */
static void dispenser_polling(void)
{
	uint32_t i;
	uint8_t valueTmp;
	uint32_t wave_timing[4];
	uint32_t ret;
	for(i=0;i<MAX_NUM_OF_DISPENSER;i++)
	{
		if(dispenser[i].IsEnable != 1)
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
		if(dispenser[i].ack_value != 0)
		{
			igs_command_add_dat(COMMAND_DISPENSER_CTL);
			igs_command_add_dat((uint8_t)i); // id
			if(dispenser[i].ack_value >= 0xFF)
			{
				igs_command_add_dat(0xFF); // 1 payout
				igs_command_add_dat(PAYOUT_DEVICE_OK);
				ret = igs_command_insert(COMMAND_DISPENSER);
				if(ret == 1)
				{
					dispenser[i].ack_value -= 0xFF;
				}
			}
			else
			{
				igs_command_add_dat(dispenser[i].ack_value); // payout value
				igs_command_add_dat(PAYOUT_DEVICE_OK);
				ret = igs_command_insert(COMMAND_DISPENSER);
				if(ret == 1)
				{
					dispenser[i].ack_value = 0;
				}
			}
		}
		
		if(dispenser[i].err_value != 0)
		{
			igs_command_add_dat(COMMAND_DISPENSER_CTL);
			igs_command_add_dat((uint8_t)i); // id
			igs_command_add_dat(0);
			if(dispenser[i].err_value == PAYOUT_DEVICE_EMPTY)
			{
				//empty error
				igs_command_add_dat(PAYOUT_DEVICE_EMPTY); 
			}
			else 
			{
				//jam error
				igs_command_add_dat(PAYOUT_DEVICE_JAM);
			}		
			ret = igs_command_insert(COMMAND_DISPENSER);
			if(ret == 1)
			{
				dispenser[i].IsEnable = 0;
			}
		}
		
		//error handle
		if(
			 dispenser[i].counterResponse >= dispenser[i].responseTime ||
			 dispenser[i].counterActive >= dispenser[i].jam_time) 
		{
			//empty, report error
			dispenser[i].wave_IsStart = 0;
			
			pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, DISABLE);
			pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
			
			if(dispenser[i].counterActive == 0)
			{
				//empty error
				dispenser[i].err_value = PAYOUT_DEVICE_EMPTY;
			}
			else
			{
				//jam error
				dispenser[i].err_value = PAYOUT_DEVICE_JAM;
			}		
			continue;
		}
		
		if(dispenser[i].value != 0)
		{
			//need pay out
			dispenser[i].counterResponse += DISPENSER_SCAN_INTERVAL;
			dispenser[i].wave_IsStart = 1;
		}
		else
		{
			// idle
			dispenser[i].wave_IsStart = 0;
			pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, DISABLE);
			pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
			continue;
		}
		
		//valid input interval check
		if(dispenser[i].RVI_time > dispenser[i].RVI_count)
		{
				dispenser[i].RVI_count+= DISPENSER_SCAN_INTERVAL;
				continue;
		}
		
		// catch input value
		valueTmp = igs_gpio_get_in_real(dispenser[i].input_IO_id);
		dispenser[i].bounceInValue <<= 1;
		dispenser[i].bounceInValue |= valueTmp;
		if((dispenser[i].bounceInValue & DEBOUNCE_MASK) == 0 )
		{
			dispenser[i].currentInValue = 0;
		}
		else if((dispenser[i].bounceInValue & DEBOUNCE_MASK) == DEBOUNCE_MASK)
		{
			dispenser[i].currentInValue = 1;
		}
		// if input value now is active, counter add
		if(dispenser[i].currentInValue == dispenser[i].input_active_type)
		{
			dispenser[i].counterActive += DISPENSER_SCAN_INTERVAL;
		}
		
		if(
			dispenser[i].currentInValue != dispenser[i].previousInValue && // input value is different
			 dispenser[i].currentInValue != dispenser[i].input_active_type) // and now is inactive
		{
			if(dispenser[i].counterActive >= dispenser[i].min_of_confirm_range &&
				 dispenser[i].counterActive <= dispenser[i].max_of_confirm_range)
			{
				//valid input
				dispenser[i].counterResponse = 0;
				dispenser[i].RVI_count = 0;
				
				dispenser[i].counterWaveform = 0;
				if(dispenser[i].value != 0)
					dispenser[i].value--;
				if(dispenser[i].value == 0)
					dispenser[i].wave_IsStart = 0;

				
				dispenser[i].ack_value++;

			}

			dispenser[i].counterActive = 0;
		}
		
		dispenser[i].previousInValue = dispenser[i].currentInValue;
		
    //
		// waveform generate
		//
		if(dispenser[i].wave_IsStart == 1)
		{

			wave_timing[0] = dispenser[i].wave_forward_time;
			wave_timing[1] = wave_timing[0] + dispenser[i].wave_stop_time;
			wave_timing[2] = wave_timing[1] + dispenser[i].wave_backward_time;
			wave_timing[3] = wave_timing[2] + dispenser[i].wave_stop_time;
			
			if(dispenser[i].counterWaveform == 0)
			{
				pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, ENABLE);
				pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
			}
			else if(dispenser[i].counterWaveform == wave_timing[3])
			{
				dispenser[i].counterWaveform = 0;
			}
			else if(dispenser[i].counterWaveform == wave_timing[2])
			{
				pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, ENABLE);
				pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
			}
			else if(dispenser[i].counterWaveform == wave_timing[1])
			{
				pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, DISABLE);
				pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, ENABLE);
			}
			else if(dispenser[i].counterWaveform == wave_timing[0])
			{
				pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, DISABLE);
				pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
			}

			dispenser[i].counterWaveform += DISPENSER_SCAN_INTERVAL;
		}
		else
		{
			dispenser[i].counterWaveform = 0;
			pulseDevicePinSet(dispenser[i].output_IO_id_forward, dispenser[i].output_active_type, DISABLE);
			pulseDevicePinSet(dispenser[i].output_IO_id_backward, dispenser[i].output_active_type, DISABLE);
		}
	}
}

/**
 * @brief dispenser_command
 *    pay out device control command handler
 * @param buf
 *   format:   buf[0] sub command
 *             buf[1] command content1
 *             buf[2] command content2
 *             buf[3] command content3
 * @param len
 *    buf length
 */
static void dispenser_command(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_DISPENSER_INIT:
			dispenser_addDevice(&buf[1], len-1);
			break;
		case COMMAND_DISPENSER_CTL:
			dispenser_control(&buf[1],len-1);
			break;
		case COMMAND_DISPENSER_RVI_SET:
			dispenser_rvi_set(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @brief dispenser_control
 *    pay out device control command handler
 * @param buf
 *   format:   buf[0] device id
 *             buf[1] pay out value[7:0]
 *             buf[2] pay out value[15:8]
 *             buf[3] reset flag, 1 is active
 * @param len
 *    buf length
 */
static void dispenser_control(uint8_t *buf, uint32_t len)
{
	uint32_t id,value;
	if(buf[0] < MAX_NUM_OF_DISPENSER)
	{
		id = buf[0];
		value = buf[2];
		value = value << 8;
		value += buf[1];
		
		// reset
		if(buf[3] == 1) 
		{
			dispenser[id].value = 0;
			dispenser[id].RVI_count = 0;
			dispenser[id].err_value = 0;
			dispenser[id].counterResponse = 0;
			dispenser[id].counterWaveform = 0;
			dispenser[id].counterActive = 0;
			dispenser[id].IsEnable = 1;

			igs_command_add_dat(COMMAND_DISPENSER_CTL);
			igs_command_add_dat(id);
			igs_command_add_dat(0); // no payout
			igs_command_add_dat(PAYOUT_DEVICE_OK); // no error
			igs_command_insert(COMMAND_DISPENSER);
			return;
		}
		dispenser[id].value += value;
	}
}

/**
 * @brief dispenser_addDevice
 *    pay out device add
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 *              buf[2] output active type
 *              buf[3] input io id
 *              buf[4] output io id (forward)
 *              buf[5] output io id (backward)
 *              buf[6] min_of_confirm_range[7:0]
 *              buf[7] min_of_confirm_range[15:8]
 *              buf[8] max_of_confirm_range[7:0]
 *              buf[9] max_of_confirm_range[15:8]
 *              buf[10] wave_forward_time[7:0]
 *              buf[11] wave_forward_time[15:8]
 *              buf[12] wave_stop_time[7:0]
 *              buf[13] wave_stop_time[15:8]
 *              buf[14] wave_backward_time[7:0]
 *              buf[15] wave_backward_time[15:8]
 *              buf[16] jam_time[7:0]
 *              buf[17] jam_time[15:8]
 * @param len
 *    buf length
 * @return
 */
static void dispenser_addDevice(uint8_t *buf, uint32_t len)
{
	uint32_t IsEnable;
	uint32_t id,tmp,io_f,io_b,io_in;
	uint32_t wave_forward_time;
	uint32_t wave_stop_time;
	uint32_t wave_backward_time;
	if(buf[0] < MAX_NUM_OF_DISPENSER)
	{
		
		id = buf[0];
		io_f = buf[4];
		io_b = buf[5];
		io_in = buf[3];
		IsEnable = buf[1];
		
		dispenser[id].wave_IsStart = 0;
		dispenser[id].IsEnable = IsEnable;
		dispenser[id].ack_value = 0;
		dispenser[id].err_value = 0;
		dispenser[id].input_IO_id = io_in;
		dispenser[id].output_IO_id_forward = io_f;
		dispenser[id].output_IO_id_backward = io_b;
		dispenser[id].value = 0;
		dispenser[id].counterResponse = 0;
		dispenser[id].counterActive = 0;
		dispenser[id].counterWaveform = 0;
		dispenser[id].RVI_count = dispenser[id].RVI_time;
		
		tmp = buf[17];
		tmp = tmp << 8;
		tmp += buf[16];
		dispenser[id].jam_time = tmp;
		
		tmp = buf[11];
		tmp = tmp << 8;
		tmp += buf[10];
		wave_forward_time = tmp;
		dispenser[id].wave_forward_time = wave_forward_time;
		
		//we need time parameters in pay out device is divisible by PULSE_SCAN_INTERVAL
		wave_forward_time += (wave_forward_time%DISPENSER_SCAN_INTERVAL);
		
		tmp = buf[15];
		tmp = tmp << 8;
		tmp += buf[14];
		wave_stop_time = tmp;
		dispenser[id].wave_stop_time = wave_stop_time;
		//we need time parameters in pay out device is divisible by PULSE_SCAN_INTERVAL
		wave_stop_time += (wave_stop_time%DISPENSER_SCAN_INTERVAL);
		
		
		tmp = buf[13];
		tmp = tmp << 8;
		tmp += buf[12];
		wave_backward_time = tmp;
		dispenser[id].wave_backward_time = wave_backward_time;
		//we need time parameters in pay out device is divisible by PULSE_SCAN_INTERVAL
		wave_backward_time += (wave_backward_time%DISPENSER_SCAN_INTERVAL);
		
		dispenser[id].responseTime = wave_backward_time + wave_stop_time*2 + wave_forward_time;
		dispenser[id].responseTime *= 3;
		
		tmp = buf[7];
		tmp = tmp << 8;
		tmp += buf[6];
		dispenser[id].min_of_confirm_range = tmp;
		
		tmp = buf[9];
		tmp = tmp << 8;
		tmp += buf[8];
		dispenser[id].max_of_confirm_range = tmp;

		dispenser[id].input_active_type = buf[2]?0:1;
		dispenser[id].output_active_type = 1;
		dispenser[id].previousInValue = buf[2]?1:0;
		
		if(IsEnable == 1)
		{
			pulseDevicePinSet(dispenser[id].output_IO_id_forward, dispenser[id].output_active_type,DISABLE); //disable contorl pin
			pulseDevicePinSet(dispenser[id].output_IO_id_backward, dispenser[id].output_active_type,DISABLE); //disable contorl pin
			
			igs_gpio_set_input_mask(io_in,DISABLE_GPIO_MASK);
			igs_gpio_set_output_mask(io_f,DISABLE_GPIO_MASK);
			igs_gpio_set_output_mask(io_b,DISABLE_GPIO_MASK);
		}
		else
		{
			igs_gpio_set_input_mask(io_in,ENABLE_GPIO_MASK);
			igs_gpio_set_output_mask(io_f,ENABLE_GPIO_MASK);
			igs_gpio_set_output_mask(io_b,ENABLE_GPIO_MASK);
		}

	}
}

/**
 * @brief dispenser_rvi_set
 *    ball machine support
 * @param buf
 *    format:   buf[0] device id
 *              buf[1] enable(1)/disable(0)
 *              buf[2] RVI_time [7:0]
 *              buf[3] RVI_time [15:8]
 * @param len
 *    buf length
 * @return
 */
static void dispenser_rvi_set(uint8_t *buf, uint32_t len)
{
	uint8_t id;
	id = buf[0];
	if(id < MAX_NUM_OF_DISPENSER)
	{
		if(buf[1] == 1)
		{
			dispenser[id].RVI_time  = buf[2];
			dispenser[id].RVI_time += buf[3] << 8;
			dispenser[id].RVI_count = dispenser[id].RVI_time;
		}
		else
		{
			dispenser[id].RVI_time = 0;
			dispenser[id].RVI_count = dispenser[id].RVI_time;
		}
	}

}
