#ifndef _OPTIGA_HOST_H_
#define _OPTIGA_HOST_H_

#include "ikv_types.h"
#include "ikv_crypto.h"
#include "optiga.h"
typedef struct _optiga_odc {
  dwordvec_t  chip_pub_key;
  signature_t sig;
} optiga_odc;

typedef struct _optiga_session
{
  optiga_uid chip_id;
  optiga_odc *cert_ptr;
  dwordvec_t lambda;
  dwordvec_t challenge;
  dwordvec_t z_resp;
  dwordvec_t checkvalue;
  uint8_t    state;
} optiga_session;

typedef eccpoint_t issuer_pub_key_t;

// crypto-function abstractions according to the cryptograhpic requirements
//   of optiga host
void optiga_crypto_rand_init (uint16_t seed);
void optiga_crypto_rand (UBYTE *rand_str, uint8_t size);
void optiga_crypto_gen_lambda (dwordvec_t lambda);
BOOL optiga_crypto_gen_ecc_challenge (dwordvec_t lambda, dwordvec_t ch);
BOOL optiga_crypto_gen_checkvalue (dwordvec_t lambda, dwordvec_t pubkey, 
                                   dwordvec_t cval);
BOOL optiga_crypto_gen_session_key (dwordvec_t challenge, dwordvec_t z,
                                    dwordvec_t skey);
BOOL optiga_crypto_ecdsa_cert_validate (optiga_odc *cert_ptr, 
                                        UBYTE      *data_to_sign, 
                                        uint8_t    data_len,
                                        issuer_pub_key_t  *issuer_pk);

BOOL optiga_host_verify_mac (optiga_session *session,
                             UBYTE          *mac_data, 
                             uint8_t         mac_len,
                             UBYTE          *mac_output);

BOOL optiga_hst_verify_odc (optiga_session *session, UBYTE *odc_buf);
void optiga_hst_rand_init (uint16_t seed);
void optiga_hst_session_init (optiga_session *session, optiga_uid *id);
BOOL optiga_hst_gen_challenge (optiga_session *session);
BOOL optiga_hst_verify_response (optiga_session *session, UBYTE *rsp, UBYTE *mac);

BOOL optiga_hst_verify_mac (optiga_session *session,
                            UBYTE*         mac_data,
                            mac_t          mac_value,
                            mac_t          mac_output);

#endif	/* _OPTIGA_HOST_H_ */
