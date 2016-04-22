#ifndef IGS_MARQUEE_DRIVER_H
#define IGS_MARQUEE_DRIVER_H

#include <stdint.h>

#define CLK_ACTIVE 1
#define CLK_INACTIVE 0

#define LATCH_ACTIVE 0
#define LATCH_INACTIVE 1

void marquee_chip_init(uint8_t chip_id, uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint32_t len);
void marquee_chip_set(uint8_t chip_id, uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len);

#endif
