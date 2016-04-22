/**
 * @file igs_task.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */

#ifndef IGS_TASK_H
#define IGS_TASK_H

#include <stdint.h>

enum igs_task_type_t
{

	IGS_TASK_ROUTINE_INT,
	IGS_TASK_ROUTINE_POLLING,
	IGS_TASK_POLLING,

};

void igs_task_init(void);
uint32_t igs_task_add(enum igs_task_type_t type, uint32_t period, void (*body)(void));
void igs_task_set_task(enum igs_task_type_t type, uint32_t id, uint32_t period);
void igs_task_main(enum  igs_task_type_t type);

#endif

