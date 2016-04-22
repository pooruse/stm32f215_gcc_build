void display_welcome_msg (void);					  
void display_config (void);					  
void display_chip_id (BIF_UID *id);
void display_otp_status (uint16_t lockofs, uint16_t ls, uint16_t ts);
void display_byte_array(uint8_t *data, uint8_t length);
void display_odc(optiga_odc *cert_ptr);
void display_challenge(optiga_session *auth_session);
void display_response(UBYTE *response, UBYTE *mac_data);
void display_nvmdata (uint8_t *data, int pageSize, int nPages, int startPage, int currPage);
void display_menu (void);
void display_async_menu(void);
