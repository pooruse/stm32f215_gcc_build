#ifndef _BIF_DRIVER_H_
#define _BIF_DRIVER_H_

#include "stdint.h"
#include "bif_protocol.h"

uint8_t bif_set_tau (uint8_t Tau); // Tau = 2 ~ 153 us, else Tau = (Tau - 2 % 152) + 2
uint8_t bif_get_tau (void);

uint8_t bif_send_pkt_only (UWORD code, UWORD data);
uint8_t bif_send_pkt_poll_int (UWORD code, UWORD data);
uint8_t bif_send_pkt_poll_pkt (UWORD code, UWORD data, uint8_t *rcvData);

UBYTE bif_check_pkt (uint16_t timing[], UBYTE *data);

void bif_shutdown (void);
void bif_reset_delay(void);	// 1 ms
void bif_guard_delay(void);	// 5 Tau

uint16_t bif_search_id (uint8_t one_first, uint8_t *prefix, uint16_t pref_size, 
						BIF_UID *id_ptr, uint16_t maxNumOfBits);
#endif	/* _BIF_DRIVER_H_ */
