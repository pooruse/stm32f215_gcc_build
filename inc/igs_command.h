#ifndef IGS_COMMAND_H
#define IGS_COMMAND_H

#include <stdint.h>

void igs_command_add_buf(uint8_t *buf, uint32_t length);
void igs_command_add_dat(uint8_t data);
void igs_command_rst(void);
uint32_t igs_command_insert(uint8_t head);
int32_t igs_command_get(uint8_t **buf);

#endif
