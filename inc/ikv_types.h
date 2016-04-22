
#ifndef _IKV_TYPES_H_
#define _IKV_TYPES_H_

#include "stdint.h"

typedef uint16_t UWORD;
typedef uint8_t  UBYTE;
typedef uint32_t ULONG;

typedef uint32_t BOOL;
#define TRUE     (BOOL)1
#define FALSE    (BOOL)0

#ifndef NULL
#define NULL 0
#endif

#endif
