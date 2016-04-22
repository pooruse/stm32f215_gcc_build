/**
 * @file igs_pulseDevice.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */
 
#ifndef IGS_PULSEDEVICE_H
#define IGS_PULSEDEVICE_H

#include <stdint.h>

#define DEBOUNCE_MASK 0x1F // 8'b0001_1111

void pulseDevicePinSet(uint32_t id, uint8_t activeType, uint32_t IsActive);

#endif

