/**
 * @file igs_protocol.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS protocol mechanism
 */
 
#include <string.h>
#include "igs_protocol.h"
#include "igs_spi.h"
#include "igs_command.h"
#include "igs_task.h"
#include "igs_gpio.h"
#include "igs_timer.h"
#include "igs_counter.h"
#include "igs_dispenser.h"

#define TIME_OUT_MAX 100 // minisecound
#define POLLING_RESET_PC 0x81
#define POLLING_RESET_MCU 0x80
#define POLLING_ERROR 0xFF
#define POLLING_START 0x01

/**
 * @brief command struct
 */
static struct _command_t
{
	uint8_t id; /**< command id */
	void (*go)(uint8_t *buf, uint32_t len); /**< function point for correspond command */
}commands[MAX_NUM_OF_COMMAND];


/**
 * @brief communication buffer definition
 */
uint8_t igs_protocol_rx_buffer[PACKAGE_LENGTH];
uint8_t igs_protocol_tx_buffer[PACKAGE_LENGTH];

/**
 * @brief protocol variables
 */
static uint8_t pollingNumber = 0;
static uint32_t num_of_cmd = 0;
static uint32_t time_out = 0;
/**
 * @brief local functions defines
 */
static uint8_t get_checksum(uint8_t *buf, uint32_t size);
static void time_out_detector(void);
static void time_out_reset(void);
static void protocol_tx_prepare(void);
static void protocol_rx_polling(void);
static void protocol_cmd_process(uint8_t *buf);

/**
 * @brief time_out_detector
 *    communication time out handler
 * @param
 * @reuturn none
 */
static void time_out_detector()
{
	if(time_out > TIME_OUT_MAX)
	{
		igs_dispenser_reset();
		igs_counter_reset();
		igs_gpio_set_led3();
	}
	else
	{
		time_out++;
	}
}
/**
 * @brief time_out_detector
 *    communication time out handler
 * @param
 * @reuturn none
 */
static void time_out_reset()
{
	time_out = 0;
}
/**
 * @brief igs_protocol_init
 *    initialization for igs protocol
 * @param
 * @reuturn none
 */
void igs_protocol_init()
{
	uint32_t i;
	
	num_of_cmd = 0;
	
	for(i=0;i<PACKAGE_LENGTH;i++)
	{
		igs_protocol_tx_buffer[i]= 0xAA;
	}
	//igs_protocol_tx_buffer[PACKAGE_LENGTH-1] = 0x55;
	//igs_protocol_tx_buffer[0] = IGS_PROTOCOL_MCU_PACKAGE_ID;
	//igs_protocol_tx_buffer[PACKAGE_LENGTH-1] = IGS_PROTOCOL_MCU_PACKAGE_ID;
	igs_task_add(IGS_TASK_POLLING,1,protocol_rx_polling);
	igs_task_add(IGS_TASK_ROUTINE_POLLING,1, time_out_detector);

	igs_spi_trigger();
}

/**
 * @brief igs_protocol_command_register
 *    connect command id to corresponding behavior
 * @param id
 *     command id
 * @param go
 *     corresponding behavior
 * @return
 */
void igs_protocol_command_register(uint8_t id, void (*go)(uint8_t *buf, uint32_t len))
{
	commands[num_of_cmd].id = id;
	commands[num_of_cmd].go = go;
	num_of_cmd++;
}

/**
 * @brief get_checksum
 *    caculate checksum value in the uint8_t array
 * @param buf
 *    uint8_t array
 * @param size
 *     array size
 * @return checksum
 */
static uint8_t get_checksum(uint8_t *buf, uint32_t size)
{
	uint32_t i;
	uint8_t tmp;
	
	tmp = 0;
	for(i=0;i<size;i++)
	{
		tmp += buf[i]; 
	}
	return tmp;
}

/**
 * @brief protocol_tx_prepare
 *    fill up tx buffer value according to the protocol
 * @param
 * @return
 */
static void protocol_tx_prepare()
{
	int32_t len;
	uint8_t *buf;
	uint8_t num_of_cmd;
	int32_t pacakge_index;
	
	num_of_cmd = 0;
	pacakge_index = 3;
	
	// get all tx data from the queue
	while(1)
	{
		len = igs_command_get(&buf);
		if(len < 0)
		{
			break;
		}
		
		/**
		 *  detect the package size is exceed PACKAGE_LENGTH
		 *  -2 because of the last one is global checksum 
		 *  and the local checksum in command.
		 */
		if(len > (PACKAGE_LENGTH - pacakge_index - 2))
		{
			//return to the tx queue
			break;
		}
		
		num_of_cmd++;
		
		memcpy(&igs_protocol_tx_buffer[pacakge_index],buf,len);
		
		pacakge_index += len;
		
		igs_protocol_tx_buffer[pacakge_index] = get_checksum(buf,len);
		pacakge_index++;
	};
	
	igs_protocol_tx_buffer[0] = IGS_PROTOCOL_MCU_PACKAGE_ID;
	igs_protocol_tx_buffer[1] = pollingNumber;
	igs_protocol_tx_buffer[2] = num_of_cmd;
	
	//clear package remain bytes
	igs_command_rst();
	memset(&igs_protocol_tx_buffer[pacakge_index],0,PACKAGE_LENGTH - pacakge_index);
	igs_protocol_tx_buffer[PACKAGE_LENGTH - 1] = get_checksum(igs_protocol_tx_buffer,PACKAGE_LENGTH - 1);
}

/**
 * @brief protocol_rx_polling
 *    if rx buffer exist any new data, push it into the igs rx queue
 * @param
 * @return
 */
static void protocol_rx_polling()
{
	uint8_t checkRx;
	uint8_t *buf;
	uint8_t id,pn,checksum;
	//uint32_t i;
	// check whether is any new data in the rx buffer
	checkRx = igs_spi_check_rx();
	if(checkRx != IGS_SPI_DATA_STANDBY)
	{
		return;
	}

	buf = igs_protocol_rx_buffer;

	/*
	 * package content
	 *
	 *              0 ->   package id = 0x53 --
	 *                     polling number     |
	 *                     num of command     |
	 *               |--   command data       |
	 * command 1  -> |     command data       |
	 *               |--   command data       |
	 *               |--   command data       | <- checksum range
	 * command 2  -> |     command data       |
	 *               |--   command data       |
	 *                        .               |
	 *                        .               |
	 *                        .               |
	 *             254->      .              --
	 *             255->   checksum
	 */
	
	//check package header
	id = buf[0];
	if(id != IGS_PROTOCOL_PC_PACKAGE_ID)
	{
		goto EXIT;
	}
	
	//check package checksum
	checksum = buf[ PACKAGE_LENGTH -1 ];
	if(checksum != get_checksum(buf,PACKAGE_LENGTH-1))
	{
		goto EXIT;
	}
	

	//check package polling number
	/**
	 *   @brief polling number information
	 *		    M01 side              |    MCU side
	 *		1.  POLLING_RESET_PC			|    don't care
	 *    2.  POLLING_RESET_PC      |    POLLING_RESET_MCU
	 *    3.  0x02                  |    0x01
	 *    4.  0x03                  |    0x02
	 *           .                  |       .
	 *           .                  |       .
	 *           .                  |       .
	 *  128.  0x7F                  |    0x7E
	 *  129.  0x01                  |    0x7F
	 *           .                  |       .
	 *           .                  |       .
	 *           .                  |       .
	 *   
	 *  @note
	 *      POLLING_RESET_PC     = 0x81
	 *      POLLING_RESET_MCU    = 0x80
	 *      valid polling number = 0x01~0x7F
	 */
	 
	pn = buf[1];
	if( (pn == POLLING_RESET_PC) && (pollingNumber == POLLING_RESET_MCU)) 
		pollingNumber = POLLING_START;
	
	else if(pn == POLLING_RESET_PC) 
		pollingNumber = POLLING_RESET_MCU;
	
	else if( ((pn & 0x80) != 0) || (pn == pollingNumber)) 
		goto EXIT;
	
	else
	{
		pollingNumber = pn;
	}
	
	//process rx command
	protocol_cmd_process(&buf[2]); //buf[2] = num of command
	
	//prepare tx command
	protocol_tx_prepare();
	
	time_out_reset(); //time out reset for led3, dispenser and counter
	igs_gpio_toggle_led3();
	
	
EXIT:
	igs_spi_trigger();
}

/**
 * @brief protocol_cmd_polling
 *    if the rx queue exist any command need be dealt with,
 * execute it
 *
 * @param
 * @return
 */
static void protocol_cmd_process(uint8_t *buf)
{
	uint32_t i;
	uint32_t cmd_index;
	uint32_t pacakge_index;
	
	uint8_t num_of_cmd;
	uint8_t id;
	uint8_t checksum,len;
	
	//command execute
	num_of_cmd = buf[0];
	pacakge_index = 1;
	for(cmd_index=0;cmd_index<num_of_cmd;cmd_index++)
	{
		len = buf[pacakge_index+1];
		
		if(pacakge_index > PACKAGE_LENGTH-len-2)
		{
			return;
		}
		/*
		 * command content
		 *
		 * pacckage_index ->   Command id   --
		 *                     Command len   |
		 *                   --data[0]       |
		 *                   | data[1]       | <- checksum range
		 *        len = 5 -> | data[2]       |
		 *                   | data[3]       |
		 *                   --data[4]      --
		 *                     checksum   
		 *
		 */
		checksum = buf[len + pacakge_index + 2];
		if(checksum != get_checksum(&buf[pacakge_index],len+2))
		{
			continue;
		}
		
		id  = buf[pacakge_index];
		
		for(i=0;i<MAX_NUM_OF_COMMAND;i++)
		{
			
			if( (id == commands[i].id) && (id != 0x0) )
			{
				//execute command
				commands[i].go(&buf[pacakge_index+2],len);
				break;
			}
		}
		pacakge_index += len+3;
	}
}


