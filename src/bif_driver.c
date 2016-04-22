
#include "stm32f2xx.h"
#include "bif_driver.h"
#include "driver_bifgpio.h"
#include "driver_timer.h"

#define bif_identify_bit(t,h) (t>h? 1: 0)

uint8_t bif_set_tau (uint8_t TauValue)
{
	return DRIVER_TIMER_SetTau(TauValue);
}

uint8_t bif_get_tau ()
{
  return DRIVER_TIMER_GetTau();
}

static uint32_t bif_make_pkt(UWORD code, UWORD data)
{
	U_BIF_WORD     uBifWord;
	uint32_t 			 reference = 0;
	
  uBifWord.uwWord = 0;
  uBifWord.sBifAbstract.CODE = code;
  uBifWord.sBifAbstract.PAYLOAD = data;

  bif_set_parity_bits(&uBifWord);
  bif_treat_invert_flag(&uBifWord);	

	// composing reference for sending bit, can be more elegent if modify struct of S_BIF_WORDBITS
	reference |= uBifWord.sBifBits.BCF 	<< 31;
	reference |= uBifWord.sBifBits._BCF << 30;
	reference |= uBifWord.sBifBits.D9 	<< 29;
	reference |= uBifWord.sBifBits.D8		<< 28;
	reference |= uBifWord.sBifBits.D7		<< 27;
	reference |= uBifWord.sBifBits.D6		<< 26;
	reference |= uBifWord.sBifBits.D5		<< 25;
	reference |= uBifWord.sBifBits.D4		<< 24;
	reference |= uBifWord.sBifBits.P3		<< 23;	// P3
	reference |= uBifWord.sBifBits.D3		<< 22;
	reference |= uBifWord.sBifBits.D2		<< 21;
	reference |= uBifWord.sBifBits.D1		<< 20;
	reference |= uBifWord.sBifBits.P2		<< 19;	// P2
	reference |= uBifWord.sBifBits.D0		<< 18;
	reference |= uBifWord.sBifBits.P1		<< 17;	// P1
	reference |= uBifWord.sBifBits.P0		<< 16;	// P0
	reference |= uBifWord.sBifBits.INV	<< 15;	// INV
	return reference;
}


uint8_t bif_send_pkt_only (UWORD code, UWORD data)
{
	BIF_STATE state;
	if (DRIVER_TIMER_SetTask(bif_make_pkt(code, data), TASK_TYPE_SEND_ONLY) == BIF_STATE_BEGIN) {
		do {
			state = DRIVER_TIMER_GetCurrState();
		} while(state != BIF_STATE_IDLE && state != BIF_STATE_TIMEOUT);
	}
	bif_guard_delay();	// ensure stop sending

	if (state == BIF_STATE_IDLE) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t bif_send_pkt_poll_int (UWORD code, UWORD data)
{
	BIF_STATE state;
	if (DRIVER_TIMER_SetTask(bif_make_pkt(code, data), TASK_TYPE_POLL_INT) == BIF_STATE_BEGIN) {
		do {
			state = DRIVER_TIMER_GetCurrState();
		} while(state != BIF_STATE_IDLE && state != BIF_STATE_TIMEOUT);
	}
	bif_guard_delay();	// ensure stop sending

	if (state == BIF_STATE_IDLE) {
		return 1;
	} else {
		return 0;
	}
}
uint8_t bif_send_pkt_poll_pkt (UWORD code, UWORD data, uint8_t *rcvData)
{
	BIF_STATE state;
	if (DRIVER_TIMER_SetTask(bif_make_pkt(code, data), TASK_TYPE_POLL_PKT) == BIF_STATE_BEGIN) {
		do {
			state = DRIVER_TIMER_GetCurrState();
		} while(state != BIF_STATE_IDLE && state != BIF_STATE_TIMEOUT);
	}
	bif_guard_delay();	// ensure stop sending

	
	if (state == BIF_STATE_IDLE) {
		int i;
		uint16_t timing[18] = {0};  
		uint32_t * poll_results = DRIVER_BIF_GetPollResults();
		for(i = 0; i < 18; i++) {
			timing[i] = poll_results[i + 1] - poll_results[i];
		}
		
		return bif_check_pkt (&timing[0], rcvData);
	} else {
		return 0xff;
	}
}

void bif_shutdown (void)
{
  DRIVER_BIF_Mode_Set (BIF_MODE_OUTPUT);
  DRIVER_BIF_SetLow ();	
  DRIVER_TIMER_Delayus (600);
  DRIVER_BIF_SetHigh ();
	DRIVER_BIF_Mode_Set(BIF_MODE_INPUT);
}

void bif_guard_delay(void)
{
  DRIVER_TIMER_Delayus (5 * DRIVER_TIMER_GetTau());
}

void bif_reset_delay(void)
{
  DRIVER_TIMER_Delayus (1000);
}
	
UBYTE bif_check_pkt (uint16_t timing[], UBYTE *data)
{
  U_BIF_WORD rx;
  S_BIF_WORDBITS *spBifBits = &rx.sBifBits;
  uint8_t    i, inv;
  uint16_t   max = 0, min= 5 * DRIVER_TIMER_GetTau(), threshold;
  
  // Getting min and max value of the timing array
  for(i=1; i<17; i++)
  {
    if(timing[i] < min) {
      min = timing[i];
    } else if(timing[i] > max) {
      max = timing[i];
    }
  }

  // find the threshold between zero and one.
  threshold = ((max + min) >> 1);

  inv = spBifBits->INV  = bif_identify_bit (timing[17],threshold);    
  spBifBits->BCF  = bif_identify_bit (timing[1],threshold);
  spBifBits->_BCF = bif_identify_bit (timing[2],threshold);
  spBifBits->D9   = bif_identify_bit (timing[3],threshold)^inv;    
  spBifBits->D8   = bif_identify_bit (timing[4],threshold)^inv;
  spBifBits->D7   = bif_identify_bit (timing[5],threshold)^inv;
  spBifBits->D6   = bif_identify_bit (timing[6],threshold)^inv;
  spBifBits->D5   = bif_identify_bit (timing[7],threshold)^inv;    
  spBifBits->D4   = bif_identify_bit (timing[8],threshold)^inv;    
  spBifBits->P3   = bif_identify_bit (timing[9],threshold)^inv;    
  spBifBits->D3   = bif_identify_bit (timing[10],threshold)^inv;    
  spBifBits->D2   = bif_identify_bit (timing[11],threshold)^inv;    
  spBifBits->D1   = bif_identify_bit (timing[12],threshold)^inv;    
  spBifBits->P2   = bif_identify_bit (timing[13],threshold)^inv;    
  spBifBits->D0   = bif_identify_bit (timing[14],threshold)^inv;    
  spBifBits->P1   = bif_identify_bit (timing[15],threshold)^inv;    
  spBifBits->P0   = bif_identify_bit (timing[16],threshold)^inv;    
  
  *data = (UBYTE)rx.sBifAbstract.PAYLOAD;
  return (UBYTE)(rx.sBifAbstract.CODE&0x0f);
}
