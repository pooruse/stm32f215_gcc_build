/**
 * @file igs_task.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS pulseDevice functions including
 *    dispenser  hopper
 *               ticket
 *    acceptor   coin
 *    another    counter
 */
 
#include "igs_pulseDevice.h"
#include "igs_gpio.h"

/**
 * @brief pulseDevicePinSet
 *    change the gpio id value accroding the value and active type
 * @pram id
 *    gpio id
 * @param activeType
 *    active type 0 is low active, 1 is high active
 * @param IsActive
 *    ENABLE (1) = active
 *    DISABLE (0) = inactive
 */
void pulseDevicePinSet(uint32_t id, uint8_t activeType, uint32_t IsActive)
{
	if(activeType == 1)
	{
		if(IsActive == ENABLE)
			igs_gpio_set_out_real(id, 1);
		else
			igs_gpio_set_out_real(id, 0);
	}
	else
	{
		if(IsActive == ENABLE)
			igs_gpio_set_out_real(id, 0);
		else
			igs_gpio_set_out_real(id, 1);
	}
}
