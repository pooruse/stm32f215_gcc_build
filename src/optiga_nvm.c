
#include "string.h"
#include "bif_protocol.h"
#include "optiga.h"


#define OPTIGA_NVM_BASE 0x1000
#define NVM_WR_BUF_ADDR  0x0f09


static UBYTE magic[10];
static void prepare_magic_num (void)
{
    int     ret;
    UBYTE   *ptr = magic;
    uint8_t c;
    
    memset (magic,0,sizeof(magic));
    ret = bif_read_registers (OPTIGA_PRTCL_MFG_ID_CTRL,ptr,sizeof(magic));
    if (ret<2)
        return;
    c = ret;
    ptr += c;
    bif_read_registers (OPTIGA_PRTCL_DEV_ID_CTRL,ptr,sizeof(magic)-c);
}

#define NVM_MAGIC_ADDR 0x0f05
#define NVM_RETRIES    10
static int update_nvm_lockofs (void)
{
  UBYTE ctrl, ok;
  int   ret, c;
    
  ret = bif_read_registers (OPTIGA_NVM_FRZ_REG_CTRL, &ctrl, sizeof(ctrl));
  if ((ret!=sizeof(ctrl))||(ctrl)) {
    return 0;
  }
		 
  // set auto-trigger of nvm write task
  ctrl = 0x03;
  bif_write_registers (OPTIGA_TASK_AUTO_CTRL, &ctrl);
  
  prepare_magic_num ();
  ctrl = 1;  // idx = LOCK_OFS_REG
  bif_write_registers (OPTIGA_NVM_REG_IDX_CTRL, &ctrl);
  for (ret=0;ret<sizeof(magic);ret++) {
    bif_write_addr_space (NVM_MAGIC_ADDR,1,&magic[ret]);    
  }
  c = NVM_RETRIES*100;
  ok = 0;
  while (c-->0) {
    ret = bif_read_registers (OPTIGA_TASK_BSY_0_CTRL, &ctrl,sizeof(ctrl));
    if ((ret==sizeof(ctrl))&&(ctrl==0)) {
      ok = 1;
      break;
    }
  }     
  
  ctrl = 0;
  bif_write_registers (OPTIGA_TASK_AUTO_CTRL, &ctrl);  
  return (ok? 2: 0);
}

uint16_t optiga_get_lockofs (void)
{
  UBYTE addr[2];	
  int   ret;
	
  ret = bif_read_registers (OPTIGA_NVM_LOCK_OFS_CTRL, addr, sizeof(addr));
  if (ret != sizeof(addr)) {
    return 0xffff;
  } else {
    return (uint16_t)(addr[0] * 0x100 + addr[1]);
  }
}

int optiga_update_lockofs (uint16_t addr, int perm)
{
  UBYTE c[2];
  uint16_t ofs;
 
  c[0] = addr/0x100;
  c[1] = addr&(0x00ff);
  bif_write_registers (OPTIGA_NVM_LOCK_OFS_CTRL, c);
 
  if (perm) {
    return update_nvm_lockofs ();
  } else {
    ofs = optiga_get_lockofs ();
    if (ofs != addr) {
      return 0;
    } else {
      return 2;
    }
  }
}

int optiga_read_nvm (uint16_t offset, uint8_t cnt, UBYTE *data)
{
  uint16_t addr = offset + OPTIGA_NVM_BASE;
  return bif_read_addr_space (addr, cnt, data);
}
/* End of File */
