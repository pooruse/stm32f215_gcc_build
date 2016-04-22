/**
 * @file igs_task.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS task functions
 *    there are three type of tasks which are polling, routine polling, 
 * and routine int. each task will be execute when the delay set to 0.
 *    1. polling task will reduce the delay in the main function.
 *    2. routine polling task will reduce the delay when the igs timer is change, 
 * and will be executed in main function
 *    3. routine int task will reduce the delay in the igs timer interrput,
 * and when delay return to zero, task will be executed. 
 *    
 */

#include "igs_task.h"
#include <stdlib.h>
#include <string.h>
#include "igs_malloc.h"

typedef struct _task_t{
	uint32_t delay;
	uint32_t period;
	struct _task_t * next;
	void (*body_function)(void);
}task_t;


static task_t *firstRoutineIntTask = NULL; /**< point to the first routine int task */
static task_t *firstRoutinePollingTask = NULL; /**< point to the first routine polling task */
static task_t *firstPollingTask = NULL; /**< point to the first polling task */

/**
 * @brief igs_task_init
 *    igs task init function
 * @param
 * @return
 */
void igs_task_init()
{
	firstRoutineIntTask = NULL;
	firstRoutinePollingTask = NULL;
  firstPollingTask = NULL;
}

/**
 * @brief igs_task_add:
 *        add new task into the linking list
 *        now only support period task
 * @param type
 *    task type
 * @param period
 *    period of the new task
 * @param body
 *    function point pointing the new task main function
 * @return task id
 */
uint32_t igs_task_add(enum igs_task_type_t type, uint32_t period, void (*body)(void))
{
	uint32_t i;
	task_t *taskObj_p;
	task_t **taskFirstObj_p;
	
	switch(type)
	{
		case IGS_TASK_ROUTINE_INT:
		{
			taskObj_p = firstRoutineIntTask;
			taskFirstObj_p = &firstRoutineIntTask;
			break;
		}
		case IGS_TASK_ROUTINE_POLLING:
		{
			taskObj_p = firstRoutinePollingTask;
			taskFirstObj_p = &firstRoutinePollingTask;
			break;
		}
		case IGS_TASK_POLLING:
		{
			taskObj_p = firstPollingTask;
			taskFirstObj_p = &firstPollingTask;
			break;
		}
		default:break;
	}
	//task index
	i = 0;
	
	/* when first task is added */
	if (taskObj_p == NULL) 
	{
		// allocate memory from array
		taskObj_p = (task_t *)igs_malloc(sizeof(task_t));
		if(taskObj_p == NULL) while(1); // memory run out
		
		// initialization
		taskObj_p->delay = period;       
		taskObj_p->period = period;
		taskObj_p->body_function = body;
		taskObj_p->next = NULL;
		*taskFirstObj_p = taskObj_p;
		return i;
	}
	
	/* if two or more task is be added */
	//find the tail in the list
	i++;
	while (taskObj_p->next != NULL) 
	{
		i++;
		taskObj_p = taskObj_p->next;
	}
	
	// allocate memory from array
	taskObj_p->next = (task_t *)igs_malloc(sizeof(task_t));
	if(taskObj_p->next == NULL) while(1); // memory run out
	
	// initialization
	taskObj_p = taskObj_p->next;
	taskObj_p->delay = period;
	taskObj_p->period = period;
	taskObj_p->body_function = body;
	taskObj_p->next = NULL;
	return i;
}

/**
 * @breif igs_task_set_task
 *    change period of specify task, period 0 means disable the task
 * @param type 
 *    task type
 * @param id
 *    task id
 * @param period
 *    new period
 */
void igs_task_set_task(enum igs_task_type_t type, uint32_t id, uint32_t period)
{
	task_t *taskObj_p;
	uint32_t i;
	switch(type)
	{
		case IGS_TASK_ROUTINE_INT:
		{
			taskObj_p = firstRoutineIntTask;
			break;
		}
		case IGS_TASK_ROUTINE_POLLING:
		{
			taskObj_p = firstRoutinePollingTask;
			break;
		}
		case IGS_TASK_POLLING:
		{
			taskObj_p = firstPollingTask;
			break;
		}
		default:break;
	}	
	

	if(taskObj_p == NULL) return; //assign period fail
	
	for(i=0;i<id;i++)
	{
		if(taskObj_p->next == NULL)
		{
			//assign period fail
			return;
		}

		taskObj_p = taskObj_p->next;
	}

	taskObj_p->period = period;
	taskObj_p->delay = period;

}

/**
 * @brief igs_task_main:
 *    external function of task_exe()
 * @param type
 *    type of task
 * @return
 */
void igs_task_main(enum igs_task_type_t type)
{

	task_t *taskObj_p;
	
	switch(type)
	{
		case IGS_TASK_ROUTINE_INT:
		{
			taskObj_p = firstRoutineIntTask;
			break;
		}
		case IGS_TASK_ROUTINE_POLLING:
		{
			taskObj_p = firstRoutinePollingTask;
			break;
		}
		case IGS_TASK_POLLING:
		{
			taskObj_p = firstPollingTask;
			break;
		}
		default:break;
	}
	
	// reduce the delay value in the whole task list
	while (taskObj_p != NULL) {
		if(taskObj_p->period==0)
		{
			//task disable
			taskObj_p = taskObj_p->next;
			continue;
		}
			
		taskObj_p->delay--;
		
		//   if delay value is reduce to 0, 
		// execute the task and set delay value back to its period.
		if (taskObj_p->delay == 0) {
			taskObj_p->delay = taskObj_p->period;
			taskObj_p->body_function();
		}
		taskObj_p = taskObj_p->next;
	}
}
