#ifndef IGS_FAN_H
#define IGS_FAN_H

#include <stdint.h>

#define NUM_OF_FAN 1
#define DEFAULT_FAN_SPEED 70 // unit: %

void igs_fan_init(void);
void igs_fan_set_speed(uint32_t id, uint32_t speed); /*< speed: 0~100% */
#endif
