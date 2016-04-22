#include <string.h>
#include "igs_command.h"
#include "igs_protocol.h"
#include "igs_queue.h"

static uint8_t command[PACKAGE_LENGTH];
static uint32_t read_index = 0;
static uint32_t write_index = 0;
static uint32_t num_of_cmd = 0;
static uint32_t i_cmd = 2;
#define COMMAND_INDEX_INIT 2

/**
 *  @brief igs command data struct
 *         buf[0] = head1
 *         buf[1] = 2 (command data length)
 *         buf[2] = data[0]
 *         buf[3] = data[1]
 *         buf[4] = head2
 *            .
 *            .
 *            .
 *
 */
 
/**
 *  @brief igs_command_add_buf
 */
void igs_command_add_buf(uint8_t *buf, uint32_t length)
{
	uint32_t tmp;
	tmp = write_index+i_cmd;
	if( tmp >= PACKAGE_LENGTH)
	{
		return;
	}
	
	if( (PACKAGE_LENGTH - tmp ) < length)
	{
		return;
	}
	
	memcpy(&command[write_index+i_cmd],buf,length);
	i_cmd += length;
}

/**
 *  @brief igs_command_add_dat
 */
void igs_command_add_dat(uint8_t data)
{
	if(write_index+i_cmd >= PACKAGE_LENGTH)
	{
		return;
	}
	
	command[write_index+(i_cmd++)] = data;
}

/**
 *  @brief igs command rst
 */
void igs_command_rst()
{
	read_index = 0;
	write_index = 0;
	num_of_cmd = 0;
	i_cmd = COMMAND_INDEX_INIT;
}

/**
 *  @brief igs command insert
 *
 *    if you call igs_command_insert(head)
 *   
 *   write_index ->  |-buf[4] = head
 *                   | buf[5] = i_cmd - 2 (include head and length)
 *                   | buf[6] = data[0]
 *                   |    .   =   .  
 *                   |    .   =   .
 *                   |-buf[x] = data[i_cmdd-1]
 *    x = i_cmd + write_index - 1 
 */

uint32_t igs_command_insert(uint8_t head)
{
	uint8_t len;
	if(write_index <= (PACKAGE_LENGTH-2))
	{
		len = i_cmd - COMMAND_INDEX_INIT;
		command[write_index++] = head;
		command[write_index++] = len;
		i_cmd = COMMAND_INDEX_INIT;
		write_index += len;
		num_of_cmd++;
		return 1;
	}
	else
	{
		//disable command insert function
		write_index = PACKAGE_LENGTH;
		return 0;
	}
}

/**
 *  @brief igs command get
 *
 *    if you call igs_command_get(uint8_t **buf)
 *   
 *    read_index  ->  buf[4] = head
 *                    buf[5] = 2 
 *                    buf[6] = data[0]
 *                    buf[7] = data[1]
 *
 *    buf will be &buf[4] and return len = 4
 *
 */
int32_t igs_command_get(uint8_t **buf)
{
	uint8_t len;
	if(num_of_cmd == 0)
	{
		//no command in command buf
		return -1;
	}
	else
	{
		num_of_cmd--;
		*buf = &(command[read_index]);
		len = command[read_index+1] + 2;
		read_index += len;
		return len;
	}
}

