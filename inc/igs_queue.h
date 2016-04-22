/**
 * @file igs_queue.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */

#ifndef IGS_QUEUE_H
#define IGS_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "igs_protocol.h"

#define IGS_QUEUE_EMPTY -1
#define IGS_QUEUE_FULL -2
#define IGS_QUEUE_SIZE_ERROR -3
#define IGS_QUEUE_HEAD_INDEX_ERROR -4
#define IGS_QUEUE_TAIL_INDEX_ERROR -5
/*
 * Exception Handling
 * Function return
 * -1 Queue is Empty (When pop process execute)
 * -2 Queue is Full (When push process execute)
 * -3 Function input size is larger than queue slot size (only in push function)
 * -4 Index Head is out of range
 * -5 Index Tail is out of range
 */

enum igs_queue_type_t
{
	IGS_QUEUE_IKV_CHALLENGE,
	IGS_QUEUE_IKV_NVM_WRITE,
	IGS_QUEUE_IKV_NVM_READ,
};
 

void igs_queue_init(void);
int32_t igs_queue_push(enum igs_queue_type_t, uint8_t * data,int32_t size);
int32_t igs_queue_pop(enum igs_queue_type_t, uint8_t ** data);

#endif

