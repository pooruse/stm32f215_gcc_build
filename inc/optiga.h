#ifndef _OPTIGA_H_
#define _OPTIGA_H_

#include "stdint.h"
#include "bif_driver.h"

#define OPTIGA_KEY_BYTE_LEN       21
#define OPTIGA_ECDSA_KEY_BYTE_LEN 25
#define OPTIGA_ODC_RFU2           6
#define OPTIGA_ODC_RFU1           2
#define OPTIGA_ECDSA_SVAL_LEN     24     
#define OPTIGA_ECDSA_RVAL_LEN     24
#define OPTIGA_ODC_SVAL_OFFSET    (OPTIGA_ECDSA_RVAL_LEN+OPTIGA_ODC_RFU1)
#define OPTIGA_ODC_PK_OFFSET      (OPTIGA_ECDSA_SVAL_LEN+OPTIGA_ECDSA_RVAL_LEN+\
                                    OPTIGA_ODC_RFU1+OPTIGA_ODC_RFU2)

#define OPTIGA_UID_BITS_NUM   (80)
#define OPTIGA_ODC_BYTE_LEN   (78)

#define OPTIGA_AUTH_CHAL_BASE	(0x1300)
#define OPTIGA_CHALLENGE_LEN  (22)
#define OPTIGA_RESPONSE_LEN   (22)
#define OPTIGA_MAC_VALUE_LEN  (10)


#define OPTIGA_SEARCH_1_FIRST	(1)
#define OPTIGA_SEARCH_0_FIRST (0)

#define OPTIGA_VENDOR_ID_H    (0x01)
#define OPTIGA_VENDOR_ID_L    (0x1a)

#define ODC_ADDR              (0x1512)
#define TRANSIT_CTR_ADDR      (0x1700)
#define LIFESPAN_CTR_ADDR     (0x1702)
#define PID_ADDR              (0x1706)
#define VID_ADDR              (0x1708)
#define AUTH_RESP_ADDR        (0x1404)
#define MAC_DATA_ADDR         (0x1602)


#define OPTIGA_ERR_CODE_CTRL     0
#define OPTIGA_IRQ_EN_0_CTRL     1
#define OPTIGA_IRQ_STS_0_CTRL    2
#define OPTIGA_TASK_BSY_0_CTRL   3
#define OPTIGA_TASK_AUTO_CTRL    4
#define OPTIGA_IRQ_EN_10_CTRL    5
#define OPTIGA_IRQ_STS_10_CTRL   6
#define OPTIGA_TASK_BSY_10_CTRL  7
#define OPTIGA_RNG_DATA_CTRL     8
#define OPTIGA_DEV_ID_CTRL       9
#define OPTIGA_AUTH_CHAL_CTRL    10
#define OPTIGA_AUTH_RESP_CTRL    11
#define OPTIGA_MAC_DATA_CTRL     12
#define OPTIGA_MAC_READ_CTRL     13
#define OPTIGA_NVM_WR_OFS_CTRL   14
#define OPTIGA_NVM_WR_CNT_CTRL   15
#define OPTIGA_NVM_LOCK_OFS_CTRL 16
#define OPTIGA_NVM_REG_IDX_CTRL  17
#define OPTIGA_NVM_FRZ_REG_CTRL  18
#define OPTIGA_PRTCL_DEV_ID_CTRL 19
#define OPTIGA_PRTCL_MFG_ID_CTRL 20

#define OPTIGA_RETRY_TIMES       20

typedef BIF_UID optiga_uid;

BOOL optiga_set_bif_tau (uint8_t tau_value);
uint8_t optiga_get_bif_tau (void);

BOOL optiga_init (optiga_uid *id_ptr, uint16_t *rand_ptr);
BOOL optiga_get_vendor_info (uint16_t *pid_ptr, uint16_t *vid_ptr);

uint16_t optiga_get_lockofs (void);
int optiga_update_lockofs (uint16_t addr, int perm);

int optiga_read_nvm (uint16_t offset, uint8_t cnt, UBYTE *data);


/* for asynchronized API */
#define OPTIGA_ASYNC_CMD_NONE                           0x0A00
#define OPTIGA_ASYNC_CMD_INIT                           0x0A01
#define OPTIGA_ASYNC_CMD_GET_VENDOR_INFO                0x0A02
#define OPTIGA_ASYNC_CMD_GET_ODC                        0x0A03
#define OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS           0x0A04
#define OPTIGA_ASYNC_CMD_GET_LOCKOFS                    0x0A05

#define OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE     0x0A06
#define OPTIGA_ASYNC_CMD_PROCESS_MAC                    0x0A07

#define OPTIGA_ASYNC_CMD_WRITE_NVM                      0x0A08
#define OPTIGA_ASYNC_CMD_READ_NVM                       0x0A09

#define OPTIGA_ASYNC_STATE_IDLE                         0x0B00
#define OPTIGA_ASYNC_STATE_WAITING                      0x0B01
#define OPTIGA_ASYNC_STATE_OK                           0x0B02
#define OPTIGA_ASYNC_STATE_ERROR                        0x0B03

uint32_t optiga_async_init (optiga_uid *id_ptr, uint16_t *rand_ptr);
uint32_t optiga_async_get_vendor_info (uint16_t *pid_ptr, uint16_t *vid_ptr);
uint32_t optiga_async_get_odc (uint8_t *cert_ptr, uint8_t size);

uint32_t optiga_async_get_special_counters (uint16_t *lspan_ptr, uint16_t *trans_ptr);
uint32_t optiga_async_get_lockofs (uint16_t *lock_ofs_ptr);

uint32_t optiga_async_process_challenge_response 
                                       (uint8_t *ch_ptr, uint8_t ch_size,
                                        uint8_t *rsp_ptr, uint8_t rsp_size,
                                        uint8_t *mac_ptr, uint8_t mac_size);
uint32_t optiga_async_process_mac (uint16_t offset, uint8_t *mac_ptr, uint8_t mac_size);

uint32_t optiga_async_write_nvm (uint16_t offset, uint8_t cnt, uint8_t *data);
uint32_t optiga_async_read_nvm (uint16_t offset, uint8_t cnt, uint8_t *data);

uint32_t optiga_async_pooling_bif(void);
uint32_t optiga_async_reset_bif(void);


#endif	/* _OPTIGA_H_ */
