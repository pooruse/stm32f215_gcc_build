#include "igs_malloc.h"
#include <string.h>
#include "igs_gpio.h"

#define MAX_MEMORY_SIZE 0x6000 //96K byte free memory
static uint32_t memory_pool[MAX_MEMORY_SIZE];
static uint32_t memory_pool_prt = 0;

/**
 *  @brief igs_malloc
 *     allocate memory from memory_pool
 *  @param length
 *     memory size you want use
 *  @return 
 *     NULL -> no free memory
 *     another -> point to the array
 */
uint8_t * igs_malloc(uint32_t length) 
{
	uint8_t *tmp;
	
	if(length == 0)
	{
		return NULL;
	}
	
	if( (length+memory_pool_prt) < MAX_MEMORY_SIZE)
	{
		tmp = (uint8_t *)&memory_pool[memory_pool_prt];
		length = (length)/4+1;
		memory_pool_prt += length;
	}
	else
	{
		tmp = NULL;
	}
	return tmp;
}

