#include "stm32f2xx.h"
#include <stdio.h>
#include "optiga_host.h"

void display_welcome_msg (void)
{
  printf ("\r\n");
  printf ("************************************************\r\n");	
  printf ("*    Welcome to STM32 OPTIGA Test Platform     *\r\n");	
#ifdef __IAR_SYSTEMS_ICC__
  printf ("*                    Compiler: IAR System ICC  *\r\n");
#endif
#ifdef __arm__
  printf ("*                    Compiler: uVision armcc   *\r\n");
#endif
  printf ("*                    Copyright 2014(R) by IKV  *\r\n");
  printf ("*===============================================\r\n");		
}

					  
void display_config (void)
{
  printf ("*         [Platform Information]               *\r\n");
  printf ("*          MCU:      STM32F207ZG               *\r\n");
  printf ("*          Timer:    TIM7 1us                  *\r\n");
  printf ("*          BIF_GPIO: PA4                       *\r\n");
  printf ("*          EVB_LED:  PA6                       *\r\n");
  printf ("*          Tau:     %3d ticks                  *\r\n", bif_get_tau());
  printf ("************************************************\r\n");		    
}
					  
void display_chip_id (BIF_UID *id)
{
  int i;

  printf ("... OPTIGA_CHIP_ID ... \r\n");
  printf ("--> UID(79:0) = [");
  for (i = 0; i < sizeof(BIF_UID); i++) {
    printf ("%02X", id->uid_raw.bn[sizeof(BIF_UID)-1-i]);
  }
  printf ("]\r\n");	
}

void display_otp_status (uint16_t lockofs, uint16_t ls, uint16_t ts)
{
  printf ("... OPTIGA_OTP_STATUS ...\r\n");
  printf ("--> LOCK_OFFSET=0x%04X\r\n", lockofs); 
  printf ("--> LIFESPAN_CTR=0x%04X\r\n", ls);
  printf ("--> TRANSIT_CTR=0x%04X\r\n", ts);
}

void display_byte_array(uint8_t *data, uint8_t length)
{
  int i;
  for (i = 0; i < length; i++) {
    if(i % 16 == 0) {
      printf("\r\n    %3d: ", i);
    }
    printf("%02X ", data[i]);
  }
  printf("\r\n");
}

void display_odc(optiga_odc *cert_ptr)
{
  printf ("\r\n... ODC content ... \r\n");
  /* 1. Publuc Key */
  printf ("--> Chip public key (22B)");
  display_byte_array((uint8_t*)cert_ptr->chip_pub_key, sizeof(cert_ptr->chip_pub_key));
  /* 2. Signature s-value */
  printf ("--> Signature (s-value/24B)");
  display_byte_array((uint8_t*)cert_ptr->sig.s_value, sizeof(cert_ptr->sig.s_value));
  /* 3. Signature r-value */
  printf ("--> Signature (s-value/24B)");
  display_byte_array((uint8_t*)cert_ptr->sig.r_value, sizeof(cert_ptr->sig.r_value));
  printf("\r\n");
}

void display_challenge(optiga_session *auth_session)
{
  printf ("\r\n... Host Random (Lambda) and Challenge ... \r\n");
  /* 1. Host Random (Lambda) */
  printf ("--> Host Random (Lambda)");
  display_byte_array((uint8_t*)auth_session->lambda, sizeof(auth_session->lambda));
  /* 2. Host Challenge */
  printf ("--> Host Challenge");
  display_byte_array((uint8_t*)auth_session->challenge, sizeof(auth_session->challenge));
  
  printf("\r\n");
}
void display_response(UBYTE *response, UBYTE *mac_data)
{
  printf ("\r\n... Optiga Response and MAC ... \r\n");
  /* 1. Optiga Response */
  printf ("--> Optiga Response");
  display_byte_array((uint8_t*)response, 22);
  /* 2. Response-MAC */
  printf ("-->  Response-MAC");
  display_byte_array((uint8_t*)mac_data, 10);

  printf("\r\n");  
}

void display_nvmdata (uint8_t *data, int pageSize, int nPages, int startPage, int currPage) {
  int i, j;
  for (i = 0; i < nPages; i++) {
    if(i + startPage == currPage) {
      printf("+[%03d] ", i + startPage);
    } else {
      printf(" [%03d] ", i + startPage);
    }
    
    for(j = 0; j < pageSize; j++) {
      printf("%02X ", data[i * pageSize + j]);
    }
    printf("\r\n");
  }
}

void display_async_menu(void)
{
  printf ("**********************************************************\r\n");
  printf ("* Select command for testing, available commands:        *\r\n");
  printf ("**********************************************************\r\n");
  printf ("  (0) optiga_async_init\r\n");    
  printf ("  (1) optiga_async_get_vendor_info\r\n");
  printf ("  (2) optiga_async_get_odc\r\n");
  printf ("  (3) optiga_async_get_special_counters\r\n");
  printf ("  (4) optiga_async_get_lockofs\r\n");
  printf ("  (5) optiga_async_process_challenge_response\r\n");
  printf ("  (6) optiga_async_process_mac\r\n");
  printf ("  (7) optiga_async_write_nvm\r\n");
  printf ("  (8) optiga_async_read_nvm\r\n");
  printf ("  (9) optiga_async_reset_bif\r\n");
  printf ("  (a) set tau for bif driver\r\n");
  printf ("===> "); 
}
