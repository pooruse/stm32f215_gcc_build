#include "igs_marquee_driver.h"
#include <string.h>
#include "igs_gpio.h"

static uint8_t get_parity(uint32_t data, uint32_t len);
static void byte_out(uint8_t clk_id, uint8_t data_id, uint32_t data, uint32_t length, uint8_t inverse);

static void MBI6803_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len);

static void MBI6024_init(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint32_t len);
static void MBI6024_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len);

static void MBI5026_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len, uint8_t inverse);
/**
 * @brief marquee_polling 
 *    control clk and data output,
 *  send marquee data out
 */
static void byte_out(uint8_t clk_id, uint8_t data_id, uint32_t data, uint32_t length, uint8_t inverse)
{
	int32_t i;
	uint32_t tmp;
	
	for(i=(length-1);i>=0;i--)
	{
		if(inverse == 1)
		{
			tmp = ~data;
		}
		else
		{
			tmp = data;
		}
		
		tmp = tmp >> i;
		igs_gpio_set_out_real(data_id, (tmp & 0x1));
		
		
		if(inverse == 1)
		{
			igs_gpio_set_out_real(clk_id, CLK_INACTIVE);
			igs_gpio_set_out_real(clk_id, CLK_ACTIVE);
		}
		else
		{
			igs_gpio_set_out_real(clk_id, CLK_ACTIVE);
			igs_gpio_set_out_real(clk_id, CLK_INACTIVE);
		}
	}
}

/**
 *  @brief get_parity
 */
static uint8_t get_parity(uint32_t data, uint32_t len)
{
	uint32_t tmp;
	uint32_t i;
	uint32_t parity;
	tmp = data;
	
	parity = 0;
	for(i=0;i<len;i++)
	{
		tmp = data & (1 << i);
		parity ^= (tmp == (1<<i));
	}
	return parity;
}
	
/**
 *  @brief MBI6803_set
 */
static void MBI6803_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len)
{
	uint32_t i;
	
	//head
	byte_out(clk_id, data_id,	0, 32, 0);
	
	//data
	
	for(i=0;i< len/3; i++)
	{
		byte_out(clk_id,	data_id,	0x1, 1, 0);
		byte_out(clk_id,	data_id, (data[i*3+2])>>3,	5, 0); // B
		byte_out(clk_id,	data_id, (data[i*3+1])>>3,	5, 0); // G
		byte_out(clk_id,	data_id, (data[i*3+0])>>3,	5, 0); // R
	}
	byte_out(clk_id,	data_id,	0, len/3, 0);
}
	
/**
 *  @brief MBI6024_init
 */
static void MBI6024_init(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint32_t len)
{
	uint32_t i;
	uint32_t chip_num;
	uint32_t head = 0x37;
	uint32_t address = 0 ;
	uint8_t parity;
	
	if(len != 0)
	{
		chip_num = (len-1)/12;
	}
	
	parity = 0;
	parity |= get_parity(chip_num, 10) << 0;
	parity |= get_parity(address, 10) << 1;
	parity |= get_parity(head, 6) << 2;
	parity |= get_parity(parity,3) << 3;
	
	//do not need initial
	byte_out(clk_id, data_id,	head, 6, 0); //6 bit configuration 0b110111
	byte_out(clk_id, data_id,	parity, 4, 0); //parity check disable
	byte_out(clk_id, data_id,	address, 10, 0); // address always 0
	byte_out(clk_id, data_id,	chip_num, 10, 0); //number of chips
	
	for(i=0;i<chip_num;i++)
	{
		//GCLK 24MHz
		//enable dot correction
		//reset PWM count
		//PWM Automatic synchronization
		//inverse from CKI to CKO
		//parity check disable
		byte_out(clk_id, data_id,	0x27F	, 10, 0); //CH1 0b 10 0111 1111
		byte_out(clk_id, data_id,	0x27F, 10, 0); //CF1 double check 0b10 0111 1111
		byte_out(clk_id, data_id,	0x7, 10, 0); //CH2
	}

	
}
	
/**
 *  @brief MBI6024_set
 */
static void MBI6024_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len)
{
	
	uint32_t i,j;
	uint32_t chip_num;
	uint32_t head = 0x2B;
	uint32_t address = 0;
	uint8_t parity;
	
	if(len != 0)
	{
		chip_num = (len-1)/12;
	}
	
	parity = 0;
	parity |= get_parity(chip_num, 10) << 0;
	parity |= get_parity(address, 10) << 1;
	parity |= get_parity(head, 6) << 2;
	parity |= get_parity(parity,3) << 3;
	
	byte_out(clk_id, data_id,	head, 6, 0); // 10-bit  Gray Scale data 0b 10 1011
	byte_out(clk_id, data_id,	parity, 4, 0); // parity check disable
	byte_out(clk_id, data_id,	address, 10, 0); // address always 0
	byte_out(clk_id, data_id,	chip_num, 10, 0); //number of chips

	for(i=0;i<(chip_num+1);i++)
	{
		for(j=0;j<12;j++)
		{
			if( (i*12+j) >= len )
			{
				byte_out(clk_id, data_id,	0, 10, 0);
			}
			else
			{
				byte_out(clk_id, data_id,	(data[i*12+j]<<2), 10, 0);
			}
		}
	}
 }

 /**
 *  @brief MBI6803_set
 */
static void MBI5026_set(uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len, uint8_t inverse)
{
	uint32_t i;
	
	for(i=0;i< len; i++)
	{
		byte_out(clk_id,	data_id, (data[i]),	8, inverse);
	}
	
	if(inverse == 1)
	{
		igs_gpio_set_out_real(latch_id, 0);
		igs_gpio_set_out_real(latch_id, 1);
	}
	else
	{
		igs_gpio_set_out_real(latch_id, 1);
		igs_gpio_set_out_real(latch_id, 0);
	}

}
 
	
/**
 *  @brief marquee_chip_init
 */
void marquee_chip_init(uint8_t chip_id, uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint32_t len)
{
	switch(chip_id)
	{
		case 1:	MBI6024_init(clk_id, data_id, latch_id, len); break;
		default: break;
	};
	igs_gpio_set_out_real(data_id, 0);
}

/**
 *  @brief marquee_chip_set
 */
void marquee_chip_set(uint8_t chip_id, uint8_t clk_id, uint8_t data_id, uint8_t latch_id, uint8_t *data, uint32_t len)
{
	switch(chip_id)
	{
		case 0: MBI6803_set(clk_id, data_id, latch_id, data, len); break;
		case 1:	MBI6024_set(clk_id, data_id, latch_id, data, len); break;
		case 2: MBI5026_set(clk_id, data_id, latch_id, data, len, 0); break;
		case 3: MBI5026_set(clk_id, data_id, latch_id, data, len, 1); break;
		default: break;
	};
	igs_gpio_set_out_real(data_id, 0);
}
