#include <stdio.h>
#include <stdint.h>
#include "ikv_crypto.h"
#include "ikv_types.h"
/** affine x-coordinate of the base point */
const uint32_t xP_163_ES[ARRAY_LEN(GF2_163)] =
 {0x40f7702cu, 0x6fa72ca5u, 0x2aa84157u, 0x58a0df74u, 0x474a0364u, 0x4u};
/** square root of curve parameter b */
const uint32_t coeff_sqrt_b_163_ES[ARRAY_LEN(GF2_163)] =
 { 0x3153d194u, 0xca99ad8fu, 0xcecc452du, 0x5cf77251u, 0xab8e0812u, 0x0u };
const curve_parameter_t _curve_163_ES = 
{
    /* degree       */ GF2_163,
    /* coeff_a      */ NULL, /* not needed for authentication protocol */
    /* coeff_sqrt_b */ //(uint32_t *)coeff_sqrt_b_163_A12,
    /* coeff_sqrt_b */ (uint32_t *)coeff_sqrt_b_163_ES,	
    /* base_point_x */ //(uint32_t *)xP_163_A12,
    /* base_point_x */ (uint32_t *)xP_163_ES,	
    /* base_point_y */ NULL, /* not needed for authentication protocol */
    /* order        */ NULL
}; /* not needed for authentication protocol */ 


/** affine x-coordinate of the base point */
const uint32_t xP_163_IKV[ARRAY_LEN(GF2_163)] =
 {0x27c936e9u, 0x5faea26du, 0x6a98c34fu, 0xb8fd1b94u, 0xe57e78ffu, 0x1u};
/** square root of curve parameter b */
const uint32_t coeff_sqrt_b_163_IKV[ARRAY_LEN(GF2_163)] =
 { 0xec06bf96u, 0x6e2876eeu, 0xa434d6aeu, 0x48ea557fu, 0xb4a2c808u, 0x0u };
const curve_parameter_t _curve_163_IKV = 
{
    /* degree       */ GF2_163,
    /* coeff_a      */ NULL, /* not needed for authentication protocol */
    /* coeff_sqrt_b */ //(uint32_t *)coeff_sqrt_b_163_A12,
    /* coeff_sqrt_b */ (uint32_t *)coeff_sqrt_b_163_IKV,	
    /* base_point_x */ //(uint32_t *)xP_163_A12,
    /* base_point_x */ (uint32_t *)xP_163_IKV,	
    /* base_point_y */ NULL, /* not needed for authentication protocol */
    /* order        */ NULL
}; /* not needed for authentication protocol */

#if 0
 /** affine x-coordinate of the base point */
static const uint32_t xP_163_A12[ARRAY_LEN(GF2_163)] =
 { 0x1a8b7f78u, 0x502959d8u, 0x2b894868u, 0x6c0356a7u, 0x8cdb7ffu, 0x7u };
 /** square root of curve parameter b */
static const uint32_t coeff_sqrt_b_163_A12[ARRAY_LEN(GF2_163)] =
 { 0xa3186141u, 0xaaeefdf8u, 0x7e01ab08u, 0x4417016bu, 0xdf7f74a5u, 0x0u };
#endif



/** affine x-coordinate of the base point */
const uint32_t xP_193[ARRAY_LEN(GF2_193)] =
  {0xd8c0c5e1, 0x79625372, 0xdef4bf61, 0xad6cdf6f, 0xff84a74, 0xf481bc5f, 0x1};

/** affine y-coordinate of the base point */
const uint32_t yP_193[ARRAY_LEN(GF2_193)] =
  {0xf7ce1b05, 0xb3201b6a, 0x1ad17fb0, 0xf3ea9e3a, 0x903712cc, 0x25e399f2, 0x0};

/** curve parameter a */
const uint32_t coeff_a_193[ARRAY_LEN(GF2_193)] =
  {0x11df7b01, 0x98ac8a9, 0x7b4087de, 0x69e171f7, 0x7a989751, 0x17858feb, 0x0};
      
/** square root of curve parameter b */
const uint32_t coeff_sqrt_b_193[ARRAY_LEN(GF2_193)] =
  {0x52fdfb06, 0xd43f8be7, 0xd24e42e9, 0x139483af, 0xddee67cd, 0xde5fb3d7, 0x1};

/** order of base point 6277101735386680763835789423269548053691575186051040197193 */
const uint32_t order_193[ARRAY_LEN(GF2_193)] =
  {0x920eba49, 0x8f443acc, 0xc7f34a77, 0x0, 0x0, 0x0, 0x1};
  
const curve_parameter_t _curve_193 = 
{
    /* degree       */ GF2_193,
    /* coeff_a      */ (uint32_t *)coeff_a_193,
    /* coeff_sqrt_b */ (uint32_t *)coeff_sqrt_b_193,
    /* base_point_x */ (uint32_t *)xP_193,
    /* base_point_y */ (uint32_t *)yP_193,
    /* order        */ (uint32_t *)order_193
};      
/** Public key for ECDSA */

const eccpoint_t _issuerPubKey_ES = {
  {0x354D265B, 0x0AF31FB1, 0x493A6120, 0xD0496678, 0xA41654E1, 0x55DCB1A5, 0x0},
  {0x446BB75A, 0xB1423C04, 0x75C4AE44, 0x40DA7281, 0xAF13FF1C, 0x0A3BB28A, 0x0}
};

const eccpoint_t _issuerPubKey_IKV = {
  {0x26030D95, 0x4A8E9C37, 0x3B106F9D, 0x348EC763, 0xB9B5038E, 0x4AAEE57A, 0x0},
  {0xD512BDB3, 0x2EADD97F, 0xF90DB84E, 0x2AE29FD7, 0x07FA34F8, 0xF6070D14, 0x0}
};

const curve_parameter_t *curve_163, *curve_193;
const eccpoint_t *issuerPubKey;

BOOL optiga_curve_init(uint16_t pid, uint16_t vid)
{
  if (pid == 0x0005 && vid == 0x0005) {
    curve_163 = &_curve_163_ES;
    issuerPubKey = &_issuerPubKey_ES;
    curve_193 = &_curve_193;
    return TRUE;
  } else if (pid == 0x0005 && vid == 0x000E){
    curve_163 = &_curve_163_IKV;
    issuerPubKey = &_issuerPubKey_IKV;
    curve_193 = &_curve_193;
    return TRUE;
  } else {
    curve_163 = NULL;
    curve_193 = NULL;
    issuerPubKey = NULL;
    return FALSE;
  }
}

void optiga_curve_reset()
{
  curve_163 = NULL;
  curve_193 = NULL;
  issuerPubKey = NULL;
}
