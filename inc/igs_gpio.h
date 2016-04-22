/**
 * @file igs_gpio.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */

#ifndef IGS_GPIO_H
#define IGS_GPIO_H

#include <stdint.h>
#include "stm32f2xx.h"

#define ENABLE_GPIO_MASK 1
#define DISABLE_GPIO_MASK 0
#define CEILING(x,y) (x-1)/y+1

typedef struct _gpio_map_t
{
	GPIO_TypeDef *group;
	uint32_t pin;
	uint8_t inverse; /*< inverse when data before output or after input */
}gpio_map_t;

extern gpio_map_t output_map[];

void igs_gpio_init(void);

void igs_gpio_toggle_led1(void);
void igs_gpio_toggle_led2(void);
void igs_gpio_toggle_led3(void);
void igs_gpio_set_led3(void);

void igs_gpio_set_input_mask(uint32_t id, uint8_t val);
void igs_gpio_set_output_mask(uint32_t id, uint8_t val);
void igs_gpio_set_switch_mask(uint32_t id, uint8_t val);

void igs_gpio_set_out_buf(uint32_t id, uint8_t val);
uint8_t igs_gpio_get_in_buf(uint32_t id);
uint8_t igs_gpio_get_switch_buf(uint32_t id);

void igs_gpio_set_out_real(uint32_t id, uint8_t val);
uint8_t igs_gpio_get_in_real(uint32_t id);
#endif
