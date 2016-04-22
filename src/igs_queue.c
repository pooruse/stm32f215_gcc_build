/**
 * @file igs_queue.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS queue functions
 *    I put all data buffer queues in this file.
 */

#include "igs_queue.h"

/**
 * @brief now we have four types of queue
 *    igs_tx_queue
 *    igs_rx_queue
 *    igs_ikv_challenge queue
 *    igs_ikv_nvm_write queue
 */
#define MAX_NUM_OF_QUEUE 5

/**
 * @brief queue struct define
 */
typedef struct _queue_t
{
	uint8_t *queue;     /**< queue body */
	int32_t *sizeArray; /**< save data size in each slot */
	uint32_t queueSize; /**< number of slot */
	uint32_t slotSize; /**< number of bytes in a slot */ 
	int32_t head; /**< queue head */ 
	int32_t tail; /**< queue tail (we implement is a simple type of circle queue */ 
	bool IsFull; /**< flag of queue full */ 
	bool IsEmpty; /**< flag of queue empty */ 
}queue_t;

/**
 * @brief define for queue to reduce the program lines
 */
#define igs_queue_struct_name(name) queue_##name
#define igs_queue_struct_new(name,slots_in_queue,bytes_in_slot)     \
        static uint8_t buf_##name[bytes_in_slot*slots_in_queue];    \
        static int32_t sizeArray_##name[slots_in_queue];            \
        static queue_t igs_queue_struct_name(name) =                \
		    {                                                           \
			      buf_##name,                                             \
			      sizeArray_##name,                                       \
			      slots_in_queue,                                         \
			      bytes_in_slot,                                          \
			      0,                                                      \
			      0,                                                      \
			      false,                                                  \
			      true,                                                   \
		    }

/**
 * @brief defines of each queue
 */

#define IKV_CHALLENGE_SLOTS_IN_QUEUE 1
#define IKV_CHALLENGE_BYTES_IN_SLOT 32

#define IKV_NVM_WRITE_SLOTS_IN_QUEUE 1
#define IKV_NVM_WRITE_BYTES_IN_SLOT 17

#define IKV_NVM_READ_SLOTS_IN_QUEUE 1
#define IKV_NVM_READ_BYTES_IN_SLOT 1
				
/**
 * @brief initailization of each queue
 */
igs_queue_struct_new(IGS_QUEUE_IKV_CHALLENGE,IKV_CHALLENGE_SLOTS_IN_QUEUE,IKV_CHALLENGE_BYTES_IN_SLOT);
igs_queue_struct_new(IGS_QUEUE_IKV_NVM_WRITE,IKV_NVM_WRITE_SLOTS_IN_QUEUE,IKV_NVM_WRITE_BYTES_IN_SLOT);
igs_queue_struct_new(IGS_QUEUE_IKV_NVM_READ,IKV_NVM_READ_SLOTS_IN_QUEUE,IKV_NVM_READ_BYTES_IN_SLOT);

/**
 * @brief function point array for each queue
 */
queue_t * queue_table[MAX_NUM_OF_QUEUE] =
{
	&igs_queue_struct_name(IGS_QUEUE_IKV_CHALLENGE),
	&igs_queue_struct_name(IGS_QUEUE_IKV_NVM_WRITE),
	&igs_queue_struct_name(IGS_QUEUE_IKV_NVM_READ),
};

/**
 * @brief igs_queue_init
 *    none
 * @param
 * @return
 */
void igs_queue_init()
{

}

/**
 * @brief igs_queue_push
 *    store data into the queue
 * @param
 * @return
 */
int32_t igs_queue_push(enum igs_queue_type_t type, uint8_t * data, int32_t size)
{
	uint32_t i;
	
	// check whether this queue is full
	if(queue_table[type]->IsFull == true)
	{
		return -2;
	}
	// check the data size is exceed the queue size or not
	else if(size > queue_table[type]->slotSize)
	{
		return -3;
	}
	// check the head index is valid
	else if(queue_table[type]->head < 0 || queue_table[type]->head > queue_table[type]->queueSize)
	{
		return -4;
	}
	// check the tail index is valid
	else if(queue_table[type]->tail < 0 || queue_table[type]->tail > queue_table[type]->queueSize)
	{
		return -5;
	}
	else
	{
		queue_table[type]->IsEmpty = false;

		//transfer data
		queue_table[type]->sizeArray[queue_table[type]->tail] = size;
		for(i=0;i<size;i++)
		{
			queue_table[type]->queue[queue_table[type]->slotSize * queue_table[type]->tail + i] = data[i];
		}

		//index manage
		queue_table[type]->tail++;
		if(queue_table[type]->tail == queue_table[type]->queueSize)
		{
			//This is a circle queue
			//When Tail is reach the upper bound of the buffer
			//Tail return back to 0
			queue_table[type]->tail = 0;
		}

		//Queue state check
		if(queue_table[type]->tail == queue_table[type]->head)
		{
			queue_table[type]->IsFull = true;
		}


		return size;

	}
}


/**
 * @brief igs_queue_pop
 *    get the data in the queue and delete it
 * @param
 * @return
 */
int32_t igs_queue_pop(enum igs_queue_type_t type, uint8_t ** data)
{
	int32_t size;
	
	// check empty flag
	if(queue_table[type]->IsEmpty == true)
	{
		return -1;
	}
	// head flag check
	else if(queue_table[type]->head < 0 || queue_table[type]->head > queue_table[type]->queueSize)
	{
		return -4;
	}
	// tail flag check
	else if(queue_table[type]->tail < 0 || queue_table[type]->tail > queue_table[type]->queueSize)
	{
		return -5;
	}
	else
	{
		queue_table[type]->IsFull=false;

		//transfer data
		size = queue_table[type]->sizeArray[queue_table[type]->head];
		*data = &(queue_table[type]->queue[queue_table[type]->slotSize * queue_table[type]->head]);

		//Index manage
		queue_table[type]->head++;
		if(queue_table[type]->head == queue_table[type]->queueSize)
		{
			//This is a circle queue
			//When Tail is reach the upper bound of the buffer
			//Tail return back to 0
			queue_table[type]->head = 0;
		}

		//Queue state check
		if(queue_table[type]->tail == queue_table[type]->head)
		{
			queue_table[type]->IsEmpty = true;
		}

		return size;
	}
}
