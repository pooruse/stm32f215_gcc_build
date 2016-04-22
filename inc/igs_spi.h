/**
 * @file igs_spi.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */

#ifndef IGS_SPI_H
#define IGS_SPI_H

#include <stdint.h>


#define IGS_SPI_DATA_STANDBY 1 
#define IGS_SPI_RX_EMPTY 0

void igs_spi_enable(void);
uint8_t igs_spi_check_rx(void);
void igs_spi_init(void);
void igs_spi_trigger(void);
#endif

