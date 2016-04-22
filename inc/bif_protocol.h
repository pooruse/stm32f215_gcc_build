#ifndef BIF_PROTOCOL_H_
#define BIF_PROTOCOL_H_

#include "ikv_types.h"

// transaction elements
// bif protocol codes (BCF/~BCF/B9/B8)
// broadcast
#define BIF_BC				0x08  	// bus command
#define BIF_EDA				0x09  	// extended device address
#define BIF_SDA				0x0A  	// slave device address

// multicast
#define BIF_WD				0x04  	// write data
#define BIF_ERA				0x05  	// extended register address
#define BIF_WRA				0x06  	// write register address
#define BIF_RRA				0x07  	// read register address

// unicast
#define BIF_RD_ACK			0x06  	// read data - ACK and not end of transmission
#define BIF_RD_NACK	    	0x04  	// read data - no ACK and not end of transmission
#define BIF_RD_ACK_EOT  	0x07  	// read data - ACK and end of transmission
#define BIF_RD_NACK_EOT 	0x05  	// read data - no ACK and end of transmission
#define BIF_TACK        	0x07  	// transaction acknowledge

// bus command
#define BIF_BRES  			0x00  	// Bus Reset
#define BIF_PDWN        	0x02    // Put slave devices into Power Down mode
#define BIF_STBY        	0x03    // Put slave devices into Standby mode
#define BIF_EINT       		0x10    // Enable interrupts for all Slave devices
#define BIF_ISTS        	0x11	// Poll Interrupt Status
#define BIF_DASM 	        0x40    // Slave devices remain selected despite a new Device Address is set
#define BIF_DISS			0x80    // Starts a UID search at the MSB
#define BIF_DILC			0x81	// Probes if the last bit of the UID is reached
#define BIF_DIE0	   		0x84	// Enters the 0 UID branch
#define BIF_DIE1			0x85	// Enters the 1 UID branch
#define BIF_DIP0			0x86	// Probes the 0 UID branch
#define BIF_DIP1			0x87	// Probes the 1 UID branch 	
#define BIF_DRES			0xc0	// Reset the selected device
#define BIF_TQ          	0xc2	// Transaction Query
#define BIF_AIO             0xc4	// Address Increment Off


#define	BIF_RBL0	0x20  	
#define	BIF_RBL1 	0x21
#define	BIF_RBL2 	0x22 	
#define	BIF_RBL3 	0x23  	
#define	BIF_RBL4 	0x24  	
#define	BIF_RBL5 	0x25  	
#define	BIF_RBL6 	0x26  	
#define	BIF_RBL7 	0x27  	
#define	BIF_RBL8 	0x28  	
#define	BIF_RBL9 	0x29  	
#define	BIF_RBLA 	0x2A  	
#define	BIF_RBLB 	0x2B  
#define	BIF_RBLC 	0x2C  	
#define	BIF_RBLD 	0x2D  	
#define	BIF_RBLE 	0x2E  	
#define	BIF_RBLF 	0x2F  	

#define	BIF_RBE0 	0x30  	
#define	BIF_RBE1 	0x31
#define	BIF_RBE2 	0x32 	
#define	BIF_RBE3 	0x33  	
#define	BIF_RBE4 	0x34  	
#define	BIF_RBE5 	0x35  	
#define	BIF_RBE6 	0x36  	
#define	BIF_RBE7 	0x37  	
#define	BIF_RBE8 	0x38  	
#define	BIF_RBE9 	0x39  	
#define	BIF_RBEA 	0x3A  	
#define	BIF_RBEB 	0x3B  
#define	BIF_RBEC 	0x3C  	
#define	BIF_RBED 	0x3D  	
#define	BIF_RBEE 	0x3E  	
#define	BIF_RBEF 	0x3F  	


enum rcv_status
{
    RCV_RAW_NO_ERR,
	RCV_RAW_NO_ERR_END,
    RCV_RAW_ERR_TIMEOUT,
	RCV_RAW_RD_ERROR,
	RCV_RAW_TNACK_ERROR,
    RCV_RAW_ERR_PROCESS
};

typedef struct _ddb_l1_dir_raw
{
    UBYTE  revision;
    UBYTE  level;
    UBYTE  devClass_hi;
    UBYTE  devClass_lo;
    UBYTE  mfgId_hi;
    UBYTE  mfgId_lo;	
    UBYTE  devId_hi;
    UBYTE  devId_lo;	
    UBYTE  l2DirLength_hi;
    UBYTE  l2DirLength_lo;	
} ddb_l1_dir_raw;

typedef struct _ddb_l1_dir
{
    UBYTE  revision;
    UBYTE  level;
    UWORD  devClass;
    UWORD  mfgId;
    UWORD  devId;
    UWORD  l2DirLength;
} ddb_l1_dir;

typedef struct _ddb_func_dir_raw
{
	UBYTE type;
	UBYTE version;
	UBYTE offset_hi;
	UBYTE offset_lo;
} ddb_func_dir_raw;	

typedef struct _ddb_func_dir
{
	UBYTE    type;
	UBYTE    version;
	uint16_t offset;
} ddb_func_dir;	


typedef struct _S_BIF_WORDBITS_TX
{
  	UWORD unused	:15; // LSB
  	UWORD INV  	  	:1;
  	UWORD P0	    :1;
  	UWORD P1      	:1;
  	UWORD D0  	  	:1;	
  	UWORD P2     	:1;
   	UWORD D1  	  	:1;
  	UWORD D2  	  	:1;
  	UWORD D3  	  	:1;
  	UWORD P3  	  	:1;	
  	UWORD D4  	  	:1;
  	UWORD D5  	  	:1;
  	UWORD D6    	:1;
  	UWORD D7  	  	:1;
  	UWORD D8  	  	:1;
  	UWORD D9  	  	:1;
  	UWORD _BCF    	:1;
  	UWORD BCF  	  	:1; 
} S_BIF_WORDBITS_TX;

typedef struct _S_BIF_WORDBITS
{
  	UWORD unused	:15; // LSB
  	UWORD INV  	  :1;
  	UWORD P0	    :1;
  	UWORD P1      :1;
  	UWORD P2     	:1;
  	UWORD P3      :1;  
  	UWORD D0  	  :1;
   	UWORD D1  	  :1;
  	UWORD D2  	  :1;
  	UWORD D3  	  :1;
  	UWORD D4  	  :1;
  	UWORD D5  	  :1;
  	UWORD D6    	:1;
  	UWORD D7  	  :1;
  	UWORD D8  	  :1;
  	UWORD D9  	  :1;
  	UWORD _BCF    :1;
  	UWORD BCF  	  :1;
} S_BIF_WORDBITS;

typedef struct _S_BIF_WORDABSTRACT
{
  	UWORD unused 	:15; // LSB
  	UWORD INV  	  :1;
  	UWORD PARITY  :4;  
  	UWORD PAYLOAD	:8;
  	UWORD CODE  	:4;   //BCF + _BCF + D9 + D8
} S_BIF_WORDABSTRACT;

typedef struct _S_BIF_WORDINVERT
{
  	UWORD unused 	:15;
  	UWORD INV  	  	:1;
  	UWORD DATA  	:14;  // D9..P0
  	UWORD BCF  	  	:2;   // BCF + _BCF
} S_BIF_WORDINVERT;

typedef union _U_BIF_WORD
{
  	S_BIF_WORDBITS      sBifBits;
 	S_BIF_WORDABSTRACT  sBifAbstract;
 	S_BIF_WORDINVERT    sBifInvRelevant;
  	ULONG               uwWord;
}	U_BIF_WORD;

struct UID_IN_BYTES {
	UBYTE bn[10];
};

struct UID {
	UBYTE DevID[8];
	UBYTE MfgID[2];
};

typedef union {
	struct UID_IN_BYTES uid_raw;
	struct UID          uid;
} BIF_UID;


typedef struct {
  uint16_t addr;
  uint8_t  len;
} bif_control_t;

void bif_reverse_byte_order (UBYTE *buf, uint16_t len);

void bif_treat_invert_flag(U_BIF_WORD *sp_DataWord);
void bif_set_parity_bits(U_BIF_WORD *sp_DataWord);
int bif_check_parity_bit(U_BIF_WORD * sp_DataWord);
int bif_check_bit_inversion(U_BIF_WORD * uwp_Word);
int bif_write_addr_space (uint16_t addr, uint8_t cnt, UBYTE *data);
int bif_read_addr_reverse (uint16_t addr, uint8_t size, UBYTE buf[]);
int bif_read_addr_space (uint16_t addr, uint8_t cnt, UBYTE *data);
int bif_read_nvm (uint16_t offset, uint8_t cnt, UBYTE *data);
int bif_read_registers (uint16_t ctrl_id, UBYTE *buf, uint8_t buf_len);
void bif_write_registers (uint16_t ctrl_id, UBYTE *values);
int bif_get_listedCapabilities (void);
int  bif_get_ddbl1 (void);

#endif	/* _BIF_PROTOCOL_H_ */
