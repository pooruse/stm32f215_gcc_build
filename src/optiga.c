#include "bif_protocol.h"
#include "ikv_crypto.h"
#include "optiga.h"

BOOL optiga_get_vendor_info (uint16_t *pid_ptr, uint16_t *vid_ptr)
{
	UBYTE val[2];
	int   ret;
	
	ret = bif_read_addr_space (PID_ADDR, sizeof(val), val);
	if (ret != sizeof(val)) {
          return FALSE;
        }
	*pid_ptr = val[0] * 0x100 + val[1];
	ret = bif_read_addr_space (VID_ADDR, sizeof(val), val);
	if (ret != sizeof(val)) {
          return FALSE;
        }
	*vid_ptr = val[0] * 0x100 + val[1];
	return TRUE;
}

BOOL optiga_get_special_counters (uint16_t *lspan_ptr, uint16_t *trans_ptr)
{
  int   ret;
  
  ret = bif_read_addr_reverse (LIFESPAN_CTR_ADDR, sizeof (uint16_t), (UBYTE*)lspan_ptr);
  if (ret != sizeof(uint16_t)) {
    return FALSE;
  }
  ret = bif_read_addr_reverse (TRANSIT_CTR_ADDR, sizeof(uint16_t), (UBYTE*)trans_ptr);
  if (ret != sizeof(uint16_t)) {
    return FALSE;
  }
  return TRUE;
}

BOOL optiga_set_bif_tau (uint8_t tau_value)
{
  return bif_set_tau(tau_value);
}

uint8_t optiga_get_bif_tau()
{
  return bif_get_tau();
}
