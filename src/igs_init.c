/**
 * @file igs_init.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IGS init functions
 */
 
#include "igs_init.h"
#include "igs_queue.h"
#include "igs_timer.h"
#include "igs_task.h"
#include "igs_gpio.h"
#include "igs_ikv.h"
#include "igs_spi.h"
#include "igs_uart.h"
#include "igs_pwm.h"
#include "igs_can.h"
#include "igs_marquee.h"
#include "igs_adc.h"
#include "igs_fan.h"
#include "igs_protocol.h"
#include "igs_pulseDevice.h"
#include "igs_dispenser.h"
#include "igs_acceptor.h"
#include "igs_handler.h"
#include "igs_counter.h"
#include "igs_scanKey.h"

/**
 * @brief igs_init
 * @param
 * @return
 */
void igs_init()
{
	//peripheral initial functions
	igs_queue_init();
	igs_timer_init();
	igs_task_init();
	igs_ikv_init();
	igs_gpio_init();
	igs_acceptor_init();
	igs_dispenser_init();
	igs_counter_init();
	igs_handler_init();
	igs_spi_init();
	igs_uart_init();
	igs_pwm_init();
	igs_can_init();
	igs_marquee_init();
	igs_adc_init();
	igs_fan_init();
	igs_scanKey_init();
	igs_protocol_init();
	
}

