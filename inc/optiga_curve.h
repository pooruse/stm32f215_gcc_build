/**
 * @file   sls10ere_curve.h
 * @date   September, 2012
 * @brief  Definition of the curve parameters for ECC and ECDSA.
 *
 */
#ifndef _OPTIGA_CURVE_H_
#define _OPTIGA_CURVE_H_
#include "ikv_crypto.h"
#include "ikv_types.h"
extern curve_parameter_t *curve_163, *curve_193;
extern eccpoint_t *issuerPubKey;
BOOL optiga_curve_init(uint16_t pid, uint16_t vid);
void optiga_curve_reset(void);
#endif

