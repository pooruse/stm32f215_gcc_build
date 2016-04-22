#include "bif_driver.h"
#include "bif_protocol.h"

#define OPTIGA_RECVIE_RETRY_MAX		(1)
static bif_control_t ctrl_map_ptr[] = {
    {0x0a02,1},  // ERR_CODE
    {0x0c00,1},  // IRQ_EN_0
    {0x0c01,1},  // IRQ_STS_0
    {0x0c02,1},  // TASK_BSY_0
    {0x0c03,1},  // TASK_AUTO_0
    {0x0c28,1},  // IRQ_EN_10
    {0x0c29,1},  // IRQ_STS_10
    {0x0c2a,1},  // TASK_BSY_10
    {0x1704,2},  // RNG_DATA 
    {0x0b00,8},  // DEV_ID
    {0x1304,22}, // AUTH_CHAL
    {0x1404,22}, // AUTH_RESP
    {0x1602,10}, // MAC_DATA
    {0x1600,2},  // MAC_READ
    {0x0f06,2},  // WR_OFS
    {0x0f08,1},  // WR_CNT
    {0x0f00,2},  // LOCK_OFS
    {0x0f02,1},  // NVM_REG_IDX
    {0x0f03,1},  // NVM_FRZ_REG
    {0x0b00,8},  // PRTCL_DEV_ID
    {0x0004,2}   // PRTCL_MFG_ID
};

void bif_reverse_byte_order (UBYTE *buf, uint16_t len)
{
  uint16_t i;
  UBYTE    swap;
  
  for (i = 0; i < (len/2); i++) {
    swap = buf[(len-1)-i]; 
    buf[(len-1)-i] = buf[i];
    buf[i] = swap;
  }	
}

int bif_write_addr_space (uint16_t addr, uint8_t cnt, UBYTE *data)
{
  UBYTE addr_hi, addr_lo;
  UBYTE *ptr=data, cc=cnt;
    
  addr_hi = addr/(0x100);
  addr_lo = addr&(0x00ff);
  bif_send_pkt_only (BIF_ERA,addr_hi);
  bif_send_pkt_only (BIF_WRA,addr_lo);    
  while (cc-->0) {
    bif_send_pkt_only (BIF_WD,*ptr++);
  }
  return cnt;	
}

int bif_read_addr_space (uint16_t addr, uint8_t cnt, UBYTE *data)
{
  UBYTE addr_hi, addr_lo;
  UBYTE *ptr = data, ret;
	
	if (cnt == 1) {
		int i;
		addr_hi = addr / (0x100);
		addr_lo = addr & (0x00ff);
		for(i = 0; i < OPTIGA_RECVIE_RETRY_MAX; i++) {
			bif_send_pkt_only (BIF_ERA, addr_hi);
			ret = bif_send_pkt_poll_pkt (BIF_RRA, addr_lo, ptr++);
			if(ret == 0xff) {										// BIF_STATE_TIMEOUT
				continue;
			} else if ((ret & 0x01) == 0x01) {  // EOT found!!
				return 1;
			}
		}
		return 0;
	} else {
		int i, j;
		
		for (i = 0; i < cnt; i++) {
			addr_hi = (addr + i) / (0x100);
			addr_lo = (addr + i) & (0x00ff);
			for (j = 0; j < OPTIGA_RECVIE_RETRY_MAX; j++) {
				bif_send_pkt_only (BIF_ERA, addr_hi);
				ret = bif_send_pkt_poll_pkt (BIF_RRA, addr_lo, ptr++);
				if(ret == 0xff) {										// BIF_STATE_TIMEOUT
					continue;
				} else if ((ret & 0x01) == 0x01) {  // EOT found!!
					break;
				}
			}
			if ((ret == 0xff) || (ret & 0x01) == 0x00) {
				break;
			}
		}
		return i;  // over-read due to no EOT found!!
	}
}


int bif_read_addr_reverse (uint16_t addr, uint8_t size, UBYTE buf[])
{
  int      ret;
  
  if (size) {
    ret = bif_read_addr_space (addr, size, buf);
    if (ret != size) {
      return -1;
    }
    if (size == 1) {
      return 1;
    }
    bif_reverse_byte_order (buf,size);
  } 
  return size;
}

int bif_read_registers (uint16_t ctrl_id, UBYTE *buf, uint8_t buf_len)
{
  uint16_t addr;
  uint8_t  len;
  int i;
  // should assert ctrl_id < max_id
  addr = (ctrl_map_ptr + ctrl_id)->addr;
  len  = (ctrl_map_ptr + ctrl_id)->len;
  if (buf_len < len) {
		return 0;
	}
	for (i = 0; i < len; i++) {
		if(bif_read_addr_space (addr + i, 1, buf + i) != 1) {
			break;
		}
	}
	return i;
}

void bif_write_registers (uint16_t ctrl_id, UBYTE *values)
{
  uint16_t addr;
  uint8_t  len;
  
  // should assert ctrl_id < max_id
  addr = (ctrl_map_ptr + ctrl_id)->addr;
  len  = (ctrl_map_ptr + ctrl_id)->len;
  bif_write_addr_space (addr, len, values);	
}
