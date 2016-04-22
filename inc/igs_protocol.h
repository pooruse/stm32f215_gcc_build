/**
 * @file igs_protocol.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */
 
#ifndef IGS_PROTOCOL_H
#define IGS_PROTOCOL_H

#define IGS_COMMAND1 0x01
#define IGS_COMMAND2 0x02

#include <stdint.h>

#define IGS_PROTOCOL_PC_PACKAGE_ID 0x53
#define IGS_PROTOCOL_MCU_PACKAGE_ID 0x52

#define MAX_NUM_OF_COMMAND 256

#define COMMAND_GPIO 0x1
#define COMMAND_ACCEPT 0x2 //COIN
#define COMMAND_DISPENSER 0x3 //HOPPER/TICKET
#define COMMAND_COUNTER 0x4
#define COMMAND_PWM 0x5
#define COMMAND_ROTARY_VR 0x6
#define COMMAND_CAN 0x7
#define COMMAND_MARQUEE 0x8
#define COMMAND_UART 0x9
#define COMMAND_IKV 0xA
#define COMMAND_SWITCH 0xB
#define COMMAND_BATTERY 0xC
#define COMMAND_SCAN_KEY 0xD
#define COMMAND_FAN 0xE
#define COMMAND_HANDLER 0xF

#define PACKAGE_LENGTH 256


extern uint8_t igs_protocol_rx_buffer[];
extern uint8_t igs_protocol_tx_buffer[];

void igs_protocol_init(void);
void igs_protocol_command_register(uint8_t id, void (*go)(uint8_t *buf, uint32_t len));

#endif
