#include "bif_protocol.h"
#include "ikv_crypto.h"
#include "optiga.h"

#define OPTIGA_PROCESS_CRYPTO_MAX	(3000)

typedef enum
{ 
  Polling_STEP0,
  Polling_STEP1,
  Polling_STEP2,
  Polling_STEP3,
  Polling_STEP4,
  Polling_STEP5,
  Polling_END
}PollingState;
/******************************************************************************/
/*                              Local Variables                               */
/******************************************************************************/
static uint32_t         optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_NONE << 16) | OPTIGA_ASYNC_STATE_IDLE;

static PollingState     polling_state;
static uint32_t         polling_count;
// OPTIGA_ASYNC_CMD_INIT
static optiga_uid       *optiga_async_id_ptr;
static uint16_t         *optiga_async_rand_ptr;
// OPTIGA_ASYNC_CMD_GET_VENDOR_INFO
static uint16_t         *optiga_async_pid_ptr;
static uint16_t         *optiga_async_vid_ptr;
// OPTIGA_ASYNC_CMD_GET_ODC
static uint8_t          *optiga_async_cert_ptr;
// OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS
static uint16_t         *optiga_async_lspan_ptr;
static uint16_t         *optiga_async_trans_ptr;
// OPTIGA_ASYNC_CMD_GET_LOCKOFS
static uint16_t         *optiga_async_lock_ofs_ptr;
// OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE
static uint8_t          optiga_async_auth_data[OPTIGA_CHALLENGE_LEN + 4];
static uint8_t          *optiga_async_rsp_ptr;
static uint8_t          *optiga_async_mac_ptr;
// OPTIGA_ASYNC_CMD_PROCESS_MAC
static uint16_t         optiga_async_process_mac_offset;
static uint8_t          *optiga_async_process_mac_ptr;
// OPTIGA_ASYNC_CMD_WRITE_NVM
// OPTIGA_ASYNC_CMD_READ_NVM
static uint16_t         optiga_async_nvm_offset;
static uint8_t          optiga_async_nvm_dataLen;
static uint8_t          *optiga_async_nvm_dataBuf;


/******************************************************************************/
/*                              Private Functions                             */
/******************************************************************************/
static uint16_t bif_id_search_next()
{
  int i;
	for (i = 0; i < 3; i++) {
		if (bif_send_pkt_poll_int (BIF_BC, BIF_DIP1) == 1) {
			bif_send_pkt_only (BIF_BC, BIF_DIE1);
			return 1;
		}
		if (bif_send_pkt_poll_int (BIF_BC, BIF_DIP0) == 1) {
			bif_send_pkt_only (BIF_BC, BIF_DIE0);
			return 0;
		}
	}
  return 0xFF;
}
/******************************************************************************/
/*                              Public Functions                              */
/******************************************************************************/
// OPTIGA_ASYNC_CMD_INIT
uint32_t optiga_async_init (optiga_uid *id_ptr, uint16_t *rand_ptr)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_WAITING;
    /* Process Command */
    optiga_async_id_ptr = id_ptr;
    optiga_async_rand_ptr = rand_ptr;
    polling_state = Polling_STEP0;
    polling_count = 0;
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_GET_VENDOR_INFO
uint32_t optiga_async_get_vendor_info (uint16_t *pid_ptr, uint16_t *vid_ptr)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_VENDOR_INFO << 16) | OPTIGA_ASYNC_STATE_WAITING;
    /* Process Command */
    optiga_async_pid_ptr = pid_ptr;
    optiga_async_vid_ptr = vid_ptr;
    polling_state = Polling_STEP0;
    polling_count = 0;
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_GET_ODC
uint32_t optiga_async_get_odc (uint8_t *cert_ptr, uint8_t size)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    if (size < OPTIGA_ODC_BYTE_LEN) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_ODC << 16) | OPTIGA_ASYNC_STATE_ERROR;
    } else {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_ODC << 16) | OPTIGA_ASYNC_STATE_WAITING;
      optiga_async_cert_ptr = cert_ptr;
      polling_state = Polling_STEP0;
      polling_count = 0;
    }
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS
uint32_t optiga_async_get_special_counters (uint16_t *lspan_ptr, uint16_t *trans_ptr)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS << 16) | OPTIGA_ASYNC_STATE_WAITING;
    optiga_async_lspan_ptr = lspan_ptr;
    optiga_async_trans_ptr = trans_ptr;
    polling_state = Polling_STEP0;
    polling_count = 0;
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_GET_LOCKOFS
uint32_t optiga_async_get_lockofs (uint16_t *lock_ofs_ptr)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_LOCKOFS << 16) | OPTIGA_ASYNC_STATE_WAITING;
    optiga_async_lock_ofs_ptr = lock_ofs_ptr;
    polling_state = Polling_STEP0;
    polling_count = 0;
  }
  return optiga_async_cmd_state;
}


// OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE
uint32_t optiga_async_process_challenge_response 
(UBYTE *ch_ptr, uint8_t ch_size, 
 UBYTE *rsp_ptr, uint8_t rsp_size,
 UBYTE *mac_ptr, uint8_t mac_size)
{
  int i;
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    if (ch_size < OPTIGA_CHALLENGE_LEN || rsp_size < OPTIGA_RESPONSE_LEN || mac_size < OPTIGA_MAC_VALUE_LEN) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_ERROR;
    } else {
      
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_WAITING;
      // get challenge
      optiga_async_auth_data[0] = optiga_async_auth_data[1] = optiga_async_auth_data[2] = optiga_async_auth_data[3] = 0;
      for (i = 0; i < OPTIGA_CHALLENGE_LEN; i++) {
        optiga_async_auth_data[i + 4] = ch_ptr[OPTIGA_CHALLENGE_LEN - i - 1];
      }
      
      // get response ptr
      optiga_async_rsp_ptr = rsp_ptr;
      // get mac ptr
      optiga_async_mac_ptr = mac_ptr;
      polling_state = Polling_STEP0;
      polling_count = 0;
    }
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_PROCESS_MAC
uint32_t optiga_async_process_mac (uint16_t offset, uint8_t *mac_ptr, uint8_t mac_size)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    if (mac_size < OPTIGA_MAC_VALUE_LEN) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_MAC << 16) | OPTIGA_ASYNC_STATE_ERROR;
    } else {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_MAC << 16) | OPTIGA_ASYNC_STATE_WAITING;
      // get mac offset and mac ptr
      optiga_async_process_mac_offset = offset;
      optiga_async_process_mac_ptr = mac_ptr;
      polling_state = Polling_STEP0;
      polling_count = 0;
    }
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_WRITE_NVM
uint32_t optiga_async_write_nvm (uint16_t offset, uint8_t cnt, UBYTE *data)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    if (cnt == 0) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_WRITE_NVM << 16) | OPTIGA_ASYNC_STATE_OK;
      polling_state = Polling_END;
    } else {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_WRITE_NVM << 16) | OPTIGA_ASYNC_STATE_WAITING;
      optiga_async_nvm_offset = offset;
      optiga_async_nvm_dataLen = cnt;
      optiga_async_nvm_dataBuf = data;
      polling_state = Polling_STEP0;
      polling_count = 0;
    }
  }
  return optiga_async_cmd_state;
}

// OPTIGA_ASYNC_CMD_READ_NVM
uint32_t optiga_async_read_nvm (uint16_t offset, uint8_t cnt, UBYTE *data)
{
  if ((optiga_async_cmd_state & 0xFFFF) != OPTIGA_ASYNC_STATE_WAITING) {
    optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_READ_NVM << 16) | OPTIGA_ASYNC_STATE_WAITING;
    optiga_async_nvm_offset = offset;
    optiga_async_nvm_dataLen = cnt;
    optiga_async_nvm_dataBuf = data;
    polling_state = Polling_STEP0;
    polling_count = 0;
  }
  return optiga_async_cmd_state;
}

uint32_t optiga_async_reset_bif()
{
  if (optiga_async_cmd_state >> 16 == OPTIGA_ASYNC_CMD_WRITE_NVM) {
    uint8_t ctrl = 0;
    bif_write_registers (OPTIGA_TASK_AUTO_CTRL, &ctrl);
  }
  optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_NONE << 16) | OPTIGA_ASYNC_STATE_IDLE;
  return optiga_async_cmd_state;
}

uint32_t optiga_async_pooling_bif()
{
  int      ret;
  uint16_t optiga_async_cmd = optiga_async_cmd_state >> 16;
  uint8_t  dataBuffer[8];
  
  switch (optiga_async_cmd) {
  /* OPTIGA_ASYNC_CMD_INIT */
  case OPTIGA_ASYNC_CMD_INIT: {
    if (polling_count == 0) {
      // 1. bif shutdown
      bif_shutdown ();
      bif_send_pkt_only (BIF_BC, BIF_BRES);
      polling_count++;
    } else if (polling_count == 1) {
      // 2. bif bus reset, poll until ok
      uint8_t nextbit;
      bif_send_pkt_only (BIF_BC,BIF_DISS);
      nextbit = bif_id_search_next();
      if (nextbit == 0 || nextbit == 1) {
        polling_count++;
        bif_send_pkt_only (BIF_BC,BIF_DISS);
      } 
    } else if (polling_count < 82) {
      // 3. bif id search
      uint8_t offset = polling_count - 2;
      uint8_t nextbit = bif_id_search_next();
      if (nextbit == 0) {
        optiga_async_id_ptr->uid_raw.bn[9 - offset / 8] &= ~(1 << (7 - (offset % 8)));
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_WAITING;
        polling_count++;
      } else if (nextbit == 1) {
        optiga_async_id_ptr->uid_raw.bn[9 - offset / 8] |= (1 << (7 - (offset % 8)));
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_WAITING;
        polling_count++;
      } else {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_ERROR;
      }
    } else if (polling_count == 82) {
      // 4. bif get rand
      if (bif_read_registers (OPTIGA_RNG_DATA_CTRL, (UBYTE*)optiga_async_rand_ptr, 2) != 2) {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_ERROR;
      } else {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_INIT << 16) | OPTIGA_ASYNC_STATE_OK;
      }
      polling_count++;
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_GET_VENDOR_INFO */
  case OPTIGA_ASYNC_CMD_GET_VENDOR_INFO: {
		UBYTE byteValue;
		ret = 0;
		/* PID */
		ret += bif_read_addr_space (PID_ADDR, 1, &byteValue);
		*optiga_async_pid_ptr = byteValue << 8;
		ret += bif_read_addr_space (PID_ADDR + 1, 1, &byteValue);
		*optiga_async_pid_ptr = *optiga_async_pid_ptr + byteValue;
		/* VID */
		ret += bif_read_addr_space (VID_ADDR, 1, &byteValue);
		*optiga_async_vid_ptr = byteValue << 8;
		ret += bif_read_addr_space (VID_ADDR + 1, 1, &byteValue);
		*optiga_async_vid_ptr = *optiga_async_vid_ptr + byteValue;
		if(ret == 4) {
			optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_VENDOR_INFO << 16) | OPTIGA_ASYNC_STATE_OK;
		} else {
			optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_VENDOR_INFO << 16) | OPTIGA_ASYNC_STATE_ERROR;
		}
    break;
  }
  /* OPTIGA_ASYNC_CMD_GET_ODC */
  case OPTIGA_ASYNC_CMD_GET_ODC: {
    if (polling_count < OPTIGA_ODC_BYTE_LEN) {
      ret = bif_read_addr_space(ODC_ADDR + OPTIGA_ODC_BYTE_LEN - polling_count - 1, 1, dataBuffer);
      if (ret != 1) {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_ODC << 16) | OPTIGA_ASYNC_STATE_ERROR;
      } else {
        optiga_async_cert_ptr[polling_count] = dataBuffer[0];
        polling_count++;
      }
    } else if (polling_count == OPTIGA_ODC_BYTE_LEN) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_ODC << 16) | OPTIGA_ASYNC_STATE_OK;
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS */
  case OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS: {
		
		UBYTE byteValue;
		ret = 0;
		/* LIFESPAN_CTR */
		ret += bif_read_addr_space (LIFESPAN_CTR_ADDR, 1, &byteValue);
		*optiga_async_lspan_ptr = byteValue << 8;
		ret += bif_read_addr_space (LIFESPAN_CTR_ADDR + 1, 1, &byteValue);
		*optiga_async_lspan_ptr = *optiga_async_lspan_ptr + byteValue;
		/* TRANSIT_CTR */
		ret += bif_read_addr_space (TRANSIT_CTR_ADDR, 1, &byteValue);
		*optiga_async_trans_ptr = byteValue << 8;
		ret += bif_read_addr_space (TRANSIT_CTR_ADDR + 1, 1, &byteValue);
		*optiga_async_trans_ptr = *optiga_async_trans_ptr + byteValue;
		
		if(ret == 4) {
			optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS << 16) | OPTIGA_ASYNC_STATE_OK;
		} else {
			optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS << 16) | OPTIGA_ASYNC_STATE_ERROR;
		}
    break;
  }
  /* OPTIGA_ASYNC_CMD_GET_LOCKOFS */
  case OPTIGA_ASYNC_CMD_GET_LOCKOFS: {
    if (polling_count == 0) {
      // 1. bif read register OPTIGA_NVM_LOCK_OFS_CTRL
      ret = bif_read_registers (OPTIGA_NVM_LOCK_OFS_CTRL, (uint8_t*)dataBuffer, sizeof(uint16_t));
      if (ret != sizeof(uint16_t)) {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_LOCKOFS << 16) | OPTIGA_ASYNC_STATE_ERROR;
      } else {
        *optiga_async_lock_ofs_ptr = (dataBuffer[0] << 8) | dataBuffer[1];
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_GET_LOCKOFS << 16) | OPTIGA_ASYNC_STATE_OK;
        polling_count++;
      }
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE */
  case OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE: {
    switch(polling_state) {
    case Polling_STEP0: {        // 1. Set write address for sending challenge
      if (polling_count == 0) { 
        bif_send_pkt_only (BIF_ERA, OPTIGA_AUTH_CHAL_BASE >>   8);
        polling_count++;
      } else if (polling_count == 1) {
        bif_send_pkt_only (BIF_WRA, OPTIGA_AUTH_CHAL_BASE & 0xff); 
        polling_count = 0;
        polling_state++;
      }
      break;
    }
    case Polling_STEP1: {       // 2. Send challenge
      if (polling_count <  OPTIGA_CHALLENGE_LEN + 4) {
        bif_send_pkt_only (BIF_WD, optiga_async_auth_data[polling_count]);
        polling_count++;
      } else {
        polling_count = 0;
        polling_state++;
      }
      break;
    }
    case Polling_STEP2: {       // 3. start processing challenge
      dataBuffer[0] = 0x08;
      bif_write_registers (OPTIGA_TASK_BSY_0_CTRL, dataBuffer);
      polling_count = 0;
      polling_state++;
      break;
    }
    case Polling_STEP3: {       // 4. polling util challenge processing finished
      ret = bif_read_registers (OPTIGA_TASK_BSY_0_CTRL, dataBuffer, 1);
      if ((ret == 1) && (dataBuffer[0] == 0)) {
        polling_count = 0;
        polling_state++;
      } else {
				polling_count++;
				if (polling_count > OPTIGA_PROCESS_CRYPTO_MAX) {
					optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_ERROR;
					polling_state = Polling_END;
				}
			}
      break;
    }
    case Polling_STEP4: {       // 5. read response
      if (polling_count < OPTIGA_RESPONSE_LEN) {
        if (bif_read_addr_space (AUTH_RESP_ADDR + polling_count, 1, optiga_async_rsp_ptr + polling_count) == 1) {
          polling_count++;
        } else {
					optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_ERROR;
					polling_state = Polling_END;
				}
      } else {
        bif_reverse_byte_order (optiga_async_rsp_ptr, OPTIGA_RESPONSE_LEN);
        polling_count = 0;
        polling_state++;
      }
      break;
    }
    case Polling_STEP5: {       // 6. read mac
      if (polling_count < OPTIGA_MAC_VALUE_LEN) {
        if (bif_read_addr_space (MAC_DATA_ADDR + polling_count, 1, optiga_async_mac_ptr + polling_count) == 1) {
          polling_count++;
        } else {
					optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_ERROR;
					polling_state = Polling_END;
				}
      } else {
        bif_reverse_byte_order (optiga_async_mac_ptr, OPTIGA_MAC_VALUE_LEN);
        polling_count = 0;
        polling_state = Polling_END;
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE << 16) | OPTIGA_ASYNC_STATE_OK;
      }
      break;
    }
    case Polling_END: {
      break;
    }
    default: {
      break;
    }
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_PROCESS_MAC */
  case OPTIGA_ASYNC_CMD_PROCESS_MAC: {
    switch(polling_state) {
    case Polling_STEP0: {        // 1. Set write address for mac_read_ctrl
      dataBuffer[0] = optiga_async_process_mac_offset >>    8;
      dataBuffer[1] = optiga_async_process_mac_offset &  0xff;
      bif_write_registers (OPTIGA_MAC_READ_CTRL, dataBuffer);
      polling_count = 0;
      polling_state++;
      break;
    }
    case Polling_STEP1: {       // 2. start to processing mac
      dataBuffer[0] = 0x01;
      bif_write_registers (OPTIGA_TASK_BSY_10_CTRL,dataBuffer);
      polling_count = 0;
      polling_state++;
      break;
    }
    case Polling_STEP2: {       // 3. polling util mac processing finished
      ret = bif_read_registers (OPTIGA_TASK_BSY_10_CTRL, dataBuffer, 1);
      if ((ret == 1) && (dataBuffer[0] == 0)) {
        polling_count = 0;
        polling_state++;
      } else {
				polling_count++;
				if (polling_count > OPTIGA_PROCESS_CRYPTO_MAX) {
					optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_MAC << 16) | OPTIGA_ASYNC_STATE_ERROR;
					polling_state = Polling_END;
				}
			}
      break;
    }
    case Polling_STEP3: {       // 4. read mac
      if (polling_count < OPTIGA_MAC_VALUE_LEN) {
        if (bif_read_addr_space (MAC_DATA_ADDR + polling_count, 1, optiga_async_process_mac_ptr + polling_count) == 1) {
          polling_count++;
        } else {
					optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_MAC << 16) | OPTIGA_ASYNC_STATE_ERROR;
					polling_state = Polling_END;
				}
      } else {
        bif_reverse_byte_order (optiga_async_process_mac_ptr, OPTIGA_MAC_VALUE_LEN);
        polling_count = 0;
        polling_state = Polling_END;
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_PROCESS_MAC << 16) | OPTIGA_ASYNC_STATE_OK;
      }
      break;
    }
    case Polling_END: {
      break;
    }
    default: {
      break;
    }
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_WRITE_NVM */
  case OPTIGA_ASYNC_CMD_WRITE_NVM: {    
    switch(polling_state) {
    case Polling_STEP0: {        // 1. Enable the user NVM Write Task Auto mode
      uint8_t ctrl = 0x03;
      bif_write_registers (OPTIGA_TASK_AUTO_CTRL, &ctrl);
      polling_count = 0;
      polling_state++;
      break;
    }
    case Polling_STEP1: {       // 2. Program the user NVM offset address to NVM_WR_OFS_H/L
      uint8_t     addr[2];
      addr[0] = optiga_async_nvm_offset / 0x100;
      addr[1] = optiga_async_nvm_offset & 0x00ff;
      bif_write_registers (OPTIGA_NVM_WR_OFS_CTRL, addr);
      polling_count = 0;
      polling_state++;
      break;
    }
    case Polling_STEP2: {       // 3. Program the number of bytes to be written
      // check previous writing done
      int ret;
      uint8_t nRemain, nToSend, ctrl;
      ret = bif_read_registers (OPTIGA_TASK_BSY_0_CTRL, &ctrl, sizeof(ctrl));
      if ((ret == sizeof(ctrl)) && (ctrl == 0)) {
        nRemain = optiga_async_nvm_dataLen - polling_count;
        nToSend = (nRemain > 8) ? 8 : (nRemain);
        bif_write_registers (OPTIGA_NVM_WR_CNT_CTRL, &nToSend);
        polling_state++;
      }
			else if(ret != sizeof(ctrl)){
				optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_WRITE_NVM << 16) | OPTIGA_ASYNC_STATE_ERROR;
				polling_state = Polling_END; 
			}
      break;
    }
    case Polling_STEP3: {       // 4. Write to the NVM buffer 
      bif_send_pkt_only (BIF_WD, *(optiga_async_nvm_dataBuf + polling_count));
      polling_count++;
      if (polling_count == optiga_async_nvm_dataLen) {
        polling_count = 0;
        polling_state++;
      } else if (polling_count % 8 == 0) {
        polling_state--;
      } 
      
      break;
    }
    case Polling_STEP4: {
      uint8_t ctrl = 0;
      bif_write_registers (OPTIGA_TASK_AUTO_CTRL, &ctrl);
      polling_count = 0;
      polling_state = Polling_END;
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_WRITE_NVM << 16) | OPTIGA_ASYNC_STATE_OK;
    }
    case Polling_END: {
      break;
    }
    default: {
      break;
    }
    }
    break;
  }
  /* OPTIGA_ASYNC_CMD_READ_NVM */
  case OPTIGA_ASYNC_CMD_READ_NVM: {
    ret = optiga_read_nvm(optiga_async_nvm_offset + polling_count, 1, optiga_async_nvm_dataBuf + polling_count);
    if (ret != 1) {
      optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_READ_NVM << 16) | OPTIGA_ASYNC_STATE_ERROR;
    } else {
      polling_count++;
      if (polling_count == optiga_async_nvm_dataLen) {
        optiga_async_cmd_state = (OPTIGA_ASYNC_CMD_READ_NVM << 16) | OPTIGA_ASYNC_STATE_OK;
      }
    }
    break;
  }
  default: {
    break;
  }
  }
  return optiga_async_cmd_state;
}
