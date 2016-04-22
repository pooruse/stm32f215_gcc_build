#include "bif_driver.h"
#include "bif_protocol.h"

// function lists:
//
// void bif_treat_invert_flag(U_BIF_WORD *sp_DataWord)
// void bif_check_bit_inversion(U_BIF_WORD * uwp_Word)
// void bif_set_parity_bits(U_BIF_WORD *sp_DataWord)
// void bif_check_parity_bit(U_BIF_WORD * sp_DataWord)
//

void bif_treat_invert_flag(U_BIF_WORD *sp_DataWord)
{
	uint8_t ubBits;
	uint8_t ubCount = 0;
	UWORD uwTarget = sp_DataWord->sBifInvRelevant.DATA;

	// loop through data bits to count for number of one.
	for(ubBits = 0; ubBits < 14; ubBits++)
	{
		if(uwTarget & 0x1)
			ubCount++;
		uwTarget >>= 1;
	}

	// if number of one more than half then carry out bit 
	// inversion handling.
	if(ubCount > 7)
	{
		sp_DataWord->sBifInvRelevant.DATA = ~sp_DataWord->sBifInvRelevant.DATA;
		sp_DataWord->sBifInvRelevant.INV = 1;
	}
}

void bif_set_parity_bits(U_BIF_WORD *sp_DataWord)
{
	UWORD ubCount = 0;
	
	// parity bit 0 setting
	// check all bits that covered by parity bit 0
	// if total bit set is even number then parity bit 0 shall be set as zero,
	// otherwise set parity bit 0 as one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D4) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D1) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D0) ? 1 : 0;
	
	// total bit set that covered by parity bit 0 is even number
	if (ubCount%2 == 0)
	{
		sp_DataWord->sBifBits.P0 = 0;
	}
	else // total bit set that covered by parity bit 0 is odd number
	{
		sp_DataWord->sBifBits.P0 = 1;
	}
	
	// parity bit 1 setting
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 1
	// if total bit set is even number then parity bit 1 shall be set as zero,
	// otherwise set parity bit 1 as one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D5) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D2) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D0) ? 1 : 0;
	
	// total bit set that covered by parity bit 1 is even number
	if (ubCount%2 == 0)
	{
		sp_DataWord->sBifBits.P1 = 0;
	}
	else // total bit set that covered by parity bit 1 is odd number
	{
		sp_DataWord->sBifBits.P1 = 1;
	}	
	
	// parity bit 2 setting
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 2
	// if total bit set is even number then parity bit 2 shall be set as zero,
	// otherwise set parity bit 2 as one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D7) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D2) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D1) ? 1 : 0;
	
	// total bit set that covered by parity bit 2 is even number
	if (ubCount%2 == 0)
	{
		sp_DataWord->sBifBits.P2 = 0;
	}
	else // total bit set that covered by parity bit 2 is odd number
	{
		sp_DataWord->sBifBits.P2 = 1;
	}
	
	// parity bit 3 setting.
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 3.
	// if total bit set is even number then parity bit 3 shall be set as zero,
	// otherwise set parity bit 3 as one.
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D7) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D5) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D4) ? 1 : 0;

	// total bit set that covered by parity bit 3 is even number
	if (ubCount%2 == 0)
	{
		sp_DataWord->sBifBits.P3 = 0;
	}
	else // total bit set that covered by parity bit 3 is even number
	{
		sp_DataWord->sBifBits.P3 = 1;
	}
}

int bif_check_parity_bit(U_BIF_WORD * sp_DataWord)
{
	UBYTE ubCount = 0;

	// parity bit 0 setting
	// check all bits that covered by parity bit 0
	// if total bit set is even number then parity bit 0 shall be zero,
	// otherwise parity bit 0 shall be one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D4) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D1) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D0) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.P0) ? 1 : 0;
	
	// total bit set that covered by parity bit 0 is even number
	if (ubCount%2 != 0)
	{
		return -1;
	}
	
	// parity bit 1 setting
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 1
	// if total bit set is even number then parity bit 1 shall be set as zero,
	// otherwise set parity bit 1 as one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D5) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D2) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D0) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.P1) ? 1 : 0;
	
	// total bit set that covered by parity bit 1 is even number
	if (ubCount%2 != 0)
	{
		return -2;
	}
	
	// parity bit 2 setting
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 2
	// if total bit set is even number then parity bit 2 shall be set as zero,
	// otherwise set parity bit 2 as one
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D7) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D3) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D2) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D1) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.P2) ? 1 : 0;

	// total bit set that covered by parity bit 2 is even number
	if (ubCount%2 != 0)
	{
		return -3;
	}
	
	// parity bit 3 setting.
	// reset ubCount counter
	ubCount = 0;
	
	// check all bits that covered by parity bit 3.
	// if total bit set is even number then parity bit 3 shall be set as zero,
	// otherwise set parity bit 3 as one.
	ubCount += (sp_DataWord->sBifBits._BCF) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D9) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D8) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D7) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D6) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D5) ? 1 : 0;
	ubCount += (sp_DataWord->sBifBits.D4) ? 1 : 0;	
	ubCount += (sp_DataWord->sBifBits.P3) ? 1 : 0;	

	// total bit set that covered by parity bit 3 is even number
	if (ubCount%2 != 0)
	{
		return -4;
	}
	
	return 0;
}

int bif_check_bit_inversion(U_BIF_WORD * uwp_Word)
{
	UBYTE ubCount = 0;
	
	ubCount += (uwp_Word->sBifBits.D9) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D8) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D7) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D6) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D5) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D4) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.P3) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D3) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D2) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.D1) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.P2) ? 1 : 0;	
	ubCount += (uwp_Word->sBifBits.D0) ? 1 : 0;
	ubCount += (uwp_Word->sBifBits.P1) ? 1 : 0;	
	ubCount += (uwp_Word->sBifBits.P0) ? 1 : 0;
	
	// check to make sure logical ones are less than half.
	return (ubCount<=7);
}


