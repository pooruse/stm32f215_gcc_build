/**
 * @file igs_ikv.h
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief 
 */

#ifndef IGS_IKV_H
#define IGS_IKV_H

#include <stdint.h>

void igs_ikv_init(void);
uint16_t igs_ikv_get_lock_ofs(void);
void igs_ikv_polling(void);

#endif

