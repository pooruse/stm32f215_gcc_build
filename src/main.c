#include <stdint.h>
#include "stm32f2xx.h"
#include "igs_timer.h"
#include "igs_task.h"
#include "igs_init.h"
#include "IGS_STM32_IAP_APP.h"

int main(void)
{
	static uint32_t tmp;
	
  /* peripheral init */
	SystemCoreClockUpdate();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	//excutable binary file shift
	IAP_Init();
	
	/* igs init function */
	igs_init();
	
  while(1) 
	{ 
		
		if(tmp != igs_get_time_tick())
		{
			tmp = igs_get_time_tick();
			igs_task_main(IGS_TASK_ROUTINE_POLLING);
		}
		
		igs_task_main(IGS_TASK_POLLING);

  }
}

