/**
 * @file igs_ikv.c
 * @author Sheng Wen Peng
 * @date 25 Nov 2014
 * @brief IKV Device IGS driver
 */

#include <string.h>
#include "igs_ikv.h"
#include "driver_usart.h"
#include "driver_led.h"
#include "driver_timer.h"
#include "driver_bifgpio.h"

#include "optiga.h"
#include "optiga_curve.h"
#include "optiga_host.h" 

#include "display_utils.h"

#include "igs_task.h"
#include "igs_command.h"
#include "igs_protocol.h"
#include "igs_queue.h"

#define IGS_IKV_MAC "SanWebbsun" // now no use
#define OPTIGA_NVM_SIZE_IN_BYTES 448
#define OPTIGA_NVM_LOCK_PERM 1
#define OPTIGA_PAGE_SIZE 8

/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_IKV_INIT 0x0
#define COMMAND_IKV_CHALLENGE 0x1
#define COMMAND_IKV_WRITE 0x2
#define COMMAND_IKV_READ 0x3
#define COMMAND_IKV_LOCK 0x4
#define COMMAND_IKV_STATE 0x5

/** 
 *  @brief ikv status defines
 */
#define IGS_IKV_WRITE_OK 0x1
#define IGS_IKV_LOCK_OK 0x2
#define IGS_IKV_ERROR 0x03
#define IGS_IKV_LOCK_FAIL 0x04

/** 
 *  @brief ikv self test define
 */
//#define IKV_SELF_TEST

//BUG: IF THE TAU SET TO 10 OR BIGGER, BIF WILL COMMUNICATE FAIL!!
//CALL CHRIS 23643228#55 (IKV-TECH)
#define DEFAULT_IGS_IKV_TAU_VALUE 4
#define MAX_DEF_CAPABILITY_NUM   10
#define null_capa                {0,0,0}

#define IGS_IKV_NVM_RW_SIZE 16

/** @brief ikv init device function  */
static uint8_t init_device (void);


#ifdef IKV_SELF_TEST

/** @brief ikv host init function  */
static void init_host (optiga_session *s, uint16_t rand, optiga_uid *id);

/** @brief ikv self test functions  */
static void ikv_self_test_challenge_add(void);
static void ikv_self_test_challenge(void);

#endif


static void ikv_command(uint8_t *buf, uint32_t len);

/** @brief ikv init functions  */
static void ikv_command_init(void);

/** @brief ikv challenge functions  */
static void ikv_command_challenge(uint8_t *buf, uint32_t len);
static void ikv_command_challenge_polling(void);

/** @brief ikv nvm write functions  */
static void ikv_command_nvm_write(uint8_t *buf, uint32_t len);
static void ikv_command_nvm_write_polling(void);

/** @brief ikv nvm read functions  */
static void ikv_command_nvm_read(uint8_t *buf, uint32_t len);
static void ikv_command_nvm_read_polling(void);

/** @brief debug show functions  */
static void debuf_led_toggle(void);

static optiga_session auth_session;
static optiga_uid chip_id;

uint8_t ikv_device_IsEnable = 0;
uint16_t        pid, vid;
uint16_t        lock_ofs;
static UBYTE    chip_odc[OPTIGA_ODC_BYTE_LEN];
static UBYTE    response[22];
static UBYTE    mac_data[10];
static uint8_t  dataBuf[IGS_IKV_NVM_RW_SIZE]; //use for nvm read/write

/** @brief ikv operation variables */
static uint16_t ikv_rand;


/** @brief ikv polling registers */
static uint8_t       nvmAddr;
static uint32_t      optiga_polling_result;
static uint32_t      optiga_polling_cmd, optiga_polling_state;

#ifdef IKV_SELF_TEST
/** @brief igs ikv self test variable */
static uint32_t testChallengeCount=0;
#endif

/** @brief igs ikv debug led variable */
static uint32_t led_task_id;

/** @brief igs ikv command state */ 
enum ikvCommandState_t
{
	IKV_COMMAND_STATE_IDLE, /**< device is idle */
	IKV_COMMAND_STATE_POLLING, /**< device is busy */
	IKV_COMMAND_STATE_ERROR, /**< device is error */
}ikvCommandState = IKV_COMMAND_STATE_IDLE; 

/**
 * @breif debuf_led_toggle
 *    for debug blink
 */
static void debuf_led_toggle()
{
	DRIVER_LED_Toggle();
}

/**
 * @brief poll_result
 *   used in init_device function, polling the reselt is OK or not
 * is simple version of igs_ikv_polling function
 * @param cmd 
 *   return data, get the command now we used
 * @param state
 *    return state, get the IKV chip state now
 * @return
 */
static void poll_result (uint32_t *cmd, uint32_t *state)
{
	uint32_t i = 0;
	uint32_t result;
	do {
		result = optiga_async_pooling_bif();
		*cmd = result >> 16;
		*state = result & 0xFFFF;
		i++;

		/**
		 *  @brief ikv init functions
		 *
		 *     we list the do while loop process execute times below		
		 *     optiga_async_init needs 84 times
		 *     optiga_async_get_vendor_info 1 time
		 *     optiga_async_get_odc 79 times

		 *     if retry times is over 1000, 
		 *     we can consider ikv is communication time out
		 */
		if(i > 1000)
		{
			return;
		}
	} while(*state == OPTIGA_ASYNC_STATE_WAITING);
}

/**
 * @brief init_device
 *   initialize ikv device. if define IKV_SELF_TEST,
 * it will initial host device too
 * @param
 * @return
 */
static uint8_t init_device()
{
	uint16_t      rand;
	uint32_t			cmd, state;


#ifdef IKV_SELF_TEST
	uint32_t i;
#endif

	//disable blink function
	igs_task_set_task(IGS_TASK_ROUTINE_POLLING,led_task_id,0);

	/* get chip_id & rand */
	optiga_async_init(&chip_id, &rand);
	poll_result(&cmd, &state);
	if (state == OPTIGA_ASYNC_STATE_OK) 
	{
#ifdef IKV_SELF_TEST
		printf ("... OPTIGA_CHIP_ID ... \r\n");
		printf ("--> UID(79:0) = [");
		for (i = 0; i < sizeof(BIF_UID); i++) 
		{
			printf ("%02X", chip_id.uid_raw.bn[sizeof(BIF_UID)-1-i]);
		}
		printf ("]\r\n");	
		printf("rand_data = [0x%04X]\r\n", rand);
		init_host (&auth_session, rand, &chip_id);
#endif
		DRIVER_LED_On();
	} 
	else if (state == OPTIGA_ASYNC_STATE_ERROR) 
	{
		DRIVER_LED_Off();
		return 0;
	}

	/* get vid, pid */
	optiga_async_get_vendor_info(&pid, &vid);
	poll_result(&cmd, &state);
	if (state == OPTIGA_ASYNC_STATE_OK) 
	{
#ifdef IKV_SELF_TEST
		// init optiga host
		printf ("--> PID = [0x%04X]  VID = [0x%04X]\r\n", pid, vid);
		if (!optiga_curve_init(pid, vid)) 
		{
			return 0;
		}
		else 
		{
			init_host (&auth_session, rand, &chip_id);
		}
#endif

	} 
	else 
	{
		igs_task_set_task(IGS_TASK_ROUTINE_POLLING,led_task_id,100);
		return 0;
	}


	/* get odc */
	optiga_async_get_odc (chip_odc, sizeof(chip_odc));
	poll_result(&cmd, &state);
	if (state == OPTIGA_ASYNC_STATE_OK) 
	{

#ifdef IKV_SELF_TEST
		printf ("... raw ODC in chip registers ...\r\n");
		for(i = 0; i < OPTIGA_ODC_BYTE_LEN; i++) 
		{
			if(i % 16 == 0) 
			{
				printf("\r\n%02Xh: ", i);
			}
			printf("%02X ", chip_odc[i]);
		}
		printf("\r\n");
		// verify odc
		if (optiga_hst_verify_odc (&auth_session, chip_odc) == FALSE) 
		{
			printf ("[Error] failed in verifying ODC ... \r\n");
			return 0;
		} 
		else
		{
			printf("--> ODC verification passed ... \r\n");
		}
#endif

	} 
	else 
	{
		igs_task_set_task(IGS_TASK_ROUTINE_POLLING,led_task_id,300);
		return 0;
	}


	return 1;
}

#ifdef IKV_SELF_TEST
/**
 * @brief init_host
 *    ikv host device init
 * @param s 
 *    this function will fill up session data you put in
 * @param rand
 *    user input rand value, it will be used to generate curve data for ECC
 * @param id
 *    chip id, it is generate when the ikv initialize
 */
static void init_host (optiga_session *s, uint16_t rand, optiga_uid *id)
{
	// initialize host session!!
	optiga_hst_rand_init (rand);
	optiga_hst_session_init (s, id);
}

#endif

/**
 * @brief igs_ikv_init
 *    igs ikv initial function
 * @param
 * @return
 */
void igs_ikv_init()
{

#ifdef IKV_SELF_TEST
	DRIVER_USART_Config();
#endif

	DRIVER_LED_Config();
	DRIVER_TIMER_Config();
	DRIVER_BIF_Config();

	//led blink default disable
	led_task_id = igs_task_add(IGS_TASK_ROUTINE_POLLING, 0,debuf_led_toggle);

	if(!optiga_set_bif_tau(DEFAULT_IGS_IKV_TAU_VALUE)) 
	{
		//set tau error
	}
	init_device();


	igs_protocol_command_register(COMMAND_IKV,ikv_command);

#ifdef IKV_SELF_TEST
	igs_task_add(IGS_TASK_ROUTINE_POLLING, 2000,ikv_self_test_challenge_add);
	igs_task_add(IGS_TASK_POLLING, 1,ikv_self_test_challenge);
#endif

	igs_task_add(IGS_TASK_ROUTINE_POLLING, 2,igs_ikv_polling);
	igs_task_add(IGS_TASK_POLLING, 1,ikv_command_challenge_polling);
	igs_task_add(IGS_TASK_POLLING, 1,ikv_command_nvm_write_polling);
	igs_task_add(IGS_TASK_POLLING, 1,ikv_command_nvm_read_polling);
}

#ifdef IKV_SELF_TEST
/**
 * @brief ikv_self_test_challenge_add
 *    you can use this function to trigger igs ikv self test mechanism
 * @param
 * @return
 */
static void ikv_self_test_challenge_add()
{
	testChallengeCount++;
}

/**
 * @brief ikv_self_test_challenge
 *    this function check if any new challenge request from itself
 * @param
 * @return
 *
 */
static void ikv_self_test_challenge()
{
	if(ikvCommandState != IKV_COMMAND_STATE_IDLE)
	{
		return;
	}

	if(testChallengeCount > 0)
	{
		memcpy(mac_data,IGS_IKV_MAC,sizeof(mac_data));
		/* generate challenge */
		optiga_hst_gen_challenge (&auth_session);
		display_challenge(&auth_session);
		/* send challenge and process response */
		optiga_async_process_challenge_response
			(
			 (uint8_t*)&(auth_session.challenge),   OPTIGA_CHALLENGE_LEN,
			 response,                              sizeof(response), 
			 mac_data,                              sizeof(mac_data)
			);

		testChallengeCount--;
		ikvCommandState = IKV_COMMAND_STATE_POLLING;
	}
}
#endif

/**
 * @brief ikv_command
 *    ikv command handler
 * @param buf
 * @param len
 * @return none 
 */
static void ikv_command(uint8_t *buf, uint32_t len)
{
	int ret;

	switch(buf[0])
	{
		case COMMAND_IKV_INIT:
			ikv_device_IsEnable = buf[1];
			if(ikv_device_IsEnable == 0) break;
			ikv_command_init();
			break;
		case COMMAND_IKV_CHALLENGE:
			if(ikv_device_IsEnable == 0) break;
			ikv_command_challenge(&buf[1],OPTIGA_CHALLENGE_LEN);
			break;
		case COMMAND_IKV_WRITE:
			if(ikv_device_IsEnable == 0) break;
			ikv_command_nvm_write(&buf[1],IGS_IKV_NVM_RW_SIZE+1);
			break;
		case COMMAND_IKV_READ:
			if(ikv_device_IsEnable == 0) break;
			ikv_command_nvm_read(&buf[1],1);
			break;
		case COMMAND_IKV_LOCK:
			if(ikv_device_IsEnable == 0) break;

			ret = optiga_update_lockofs(OPTIGA_NVM_SIZE_IN_BYTES-1, OPTIGA_NVM_LOCK_PERM);

			igs_command_add_dat(COMMAND_IKV_STATE);
			igs_command_add_dat(((ret==2)?IGS_IKV_LOCK_OK:IGS_IKV_LOCK_FAIL));
			igs_command_insert(COMMAND_IKV);
			break;
		default: break;
	}
}

/**
 * @brief ikv_command_init
 *    for ikv data return to main chip including chip id, odc and vid/pid
 * @param buf
 * @param len
 * @return none 
 */
static void ikv_command_init()
{
	//chip_id.uid_raw.bn, odc, pid, vid
	optiga_async_init(&chip_id, &ikv_rand);
	ikvCommandState = IKV_COMMAND_STATE_POLLING;

}

/**
 * @brief ikv_command_challenge
 *    for receive igs ikv challenge command from main chip (ex. M01)
 * @param buf
 *    contains 22 bytes challenge data and 10 bytes mac data
 * @param len
 *    buf length
 */
static void ikv_command_challenge(uint8_t *buf, uint32_t len)
{
	igs_queue_push(IGS_QUEUE_IKV_CHALLENGE,buf,len);
}

/**
 * @brief ikv_command_challenge_polling
 *    check ikv device state and whethere any challenge command exist
 * @param
 * @return
 */ 
static void ikv_command_challenge_polling()
{
	int32_t ret;
	uint8_t *buf;

	// check ikv device state
	if(ikvCommandState != IKV_COMMAND_STATE_IDLE)
	{
		return;
	}

	//check whether the command challenge exists
	ret = igs_queue_pop(IGS_QUEUE_IKV_CHALLENGE,&buf);
	if(ret < 0)
	{
		return;
	}

	//prepare response and MAC
	memcpy((uint8_t*)&(auth_session.challenge),&buf[0],OPTIGA_CHALLENGE_LEN);
	memcpy(mac_data,IGS_IKV_MAC,sizeof(mac_data));

	//trigger challenge process
	optiga_async_process_challenge_response
		(
		 (uint8_t*)&(auth_session.challenge),   OPTIGA_CHALLENGE_LEN,
		 response,                              sizeof(response), 
		 mac_data,                              sizeof(mac_data)
		);

	//igs ikv state polling
	ikvCommandState = IKV_COMMAND_STATE_POLLING;

}

/**
 * @brief ikv_command_nvm_write
 *   execute commnad nvm write
 * @param buf
 *   write data from main chip
 * @param len
 *   buf length
 */
static void ikv_command_nvm_write(uint8_t *buf, uint32_t len)
{
	igs_queue_push(IGS_QUEUE_IKV_NVM_WRITE,buf,len);
}

/**
 * @brief ikv_command_nvm_write_polling
 *    check if exist any nvm write commnad, it will trigger ikv to write nvm
 * @param
 * @return
 */
static void ikv_command_nvm_write_polling()
{
	int32_t ret;
	uint8_t *buf;

	// check ikv device state
	if(ikvCommandState != IKV_COMMAND_STATE_IDLE)
	{
		return;
	}

	//check whether the command nvm write exists
	ret = igs_queue_pop(IGS_QUEUE_IKV_NVM_WRITE,&buf);
	if(ret < 0)
	{
		return;
	}

	//clear the nvm write data buf
	memcpy(dataBuf,&buf[1],IGS_IKV_NVM_RW_SIZE);

	//trigger ikv to execute ikv write command
	optiga_async_write_nvm (buf[0]*IGS_IKV_NVM_RW_SIZE, IGS_IKV_NVM_RW_SIZE, dataBuf);

	//ikv state polling
	ikvCommandState = IKV_COMMAND_STATE_POLLING;

}

/**
 * @brief ikv_command_nvm_read
 *    set ikv_command_vnm_read_f to 1
 * @param buf 
 *    just for the protocol function point interface.
 *    useless here
 * @param len
 *    just for the protocol function point interface.
 *    useless here
 * @return
 *
 */
static void ikv_command_nvm_read(uint8_t *buf, uint32_t len)
{
	igs_queue_push(IGS_QUEUE_IKV_NVM_READ,buf,1);
}

/**
 * @brief ikv_command_nvm_read_polling
 *    check the ikv_command_nvm_read_f. If it is 1 and ikv device is idle,
 * start the ikv nvm read process
 * @param
 * @return
 *
 */
static void ikv_command_nvm_read_polling(void)
{
	int32_t ret;
	uint8_t *buf;
	//check ikv device state
	if(ikvCommandState != IKV_COMMAND_STATE_IDLE)
	{
		return;
	}

	//check the ikv_command-nvm_read_f flag
	ret = igs_queue_pop(IGS_QUEUE_IKV_NVM_READ,&buf);
	if(ret < 0)
	{
		return;
	}

	//default address is 0 (page = 0, offset = 0)
	//ikv address contains 55 pages. Each page contains 8 bytes

	nvmAddr = buf[0];
	optiga_async_read_nvm (nvmAddr*IGS_IKV_NVM_RW_SIZE, IGS_IKV_NVM_RW_SIZE, dataBuf);


	//set read flag and ikv state
	ikvCommandState = IKV_COMMAND_STATE_POLLING;
}

/**
 * @brief igs_ikv_polling
 *    igs ikv polling function. it should be put in main function and always be polling
 * @param
 * @return
 */
void igs_ikv_polling()
{
	/* polling command state */
	if(ikvCommandState != IKV_COMMAND_STATE_POLLING)
	{
		return;
	}

	//get polling result, this function can't be interrupt by any interrputs
	//except TIM7 (ikv default timer)
	optiga_polling_result = optiga_async_pooling_bif();

	optiga_polling_cmd = optiga_polling_result >> 16;
	optiga_polling_state = optiga_polling_result & 0xFFFF;

	// command is still not completed 
	if (optiga_polling_state == OPTIGA_ASYNC_STATE_WAITING) 
	{
		return;
	}

	ikvCommandState = IKV_COMMAND_STATE_IDLE;

	// ikv communication error
	if(optiga_polling_state == OPTIGA_ASYNC_STATE_ERROR)
	{
		igs_command_add_dat(COMMAND_IKV_STATE);
		igs_command_add_dat(IGS_IKV_ERROR);
		igs_command_insert(COMMAND_IKV);

		DRIVER_LED_Off();

		return;
	}


	// command functions
	switch(optiga_polling_cmd) 
	{
		case OPTIGA_ASYNC_CMD_INIT: 
			{
				lock_ofs = optiga_get_lockofs();

				igs_command_add_dat(COMMAND_IKV_INIT);
				igs_command_add_dat(pid & 0xFF);
				igs_command_add_dat((pid>>8) & 0xFF);
				igs_command_add_dat(vid & 0xFF);
				igs_command_add_dat((vid>>8) & 0xFF);
				igs_command_add_buf(chip_id.uid_raw.bn,10);
				igs_command_add_buf(chip_odc,OPTIGA_ODC_BYTE_LEN);

				// lock_ofs state
				if(lock_ofs == 0xFFFF || lock_ofs == 0)
				{
					//unlock or bif communication error
					igs_command_add_dat(0x0);
				}
				else
				{
					//lock
					igs_command_add_dat(0x1);
				}

				igs_command_insert(COMMAND_IKV);
				igs_task_set_task(IGS_TASK_ROUTINE_POLLING,led_task_id,0);
				DRIVER_LED_On();
				break;
			}
		case OPTIGA_ASYNC_CMD_GET_VENDOR_INFO: 
			{
				break;
			}
		case OPTIGA_ASYNC_CMD_GET_ODC: 
			{
				break;
			}

		case OPTIGA_ASYNC_CMD_GET_SPECIAL_COUNTERS: 
			{
				break;
			}
		case OPTIGA_ASYNC_CMD_GET_LOCKOFS: 
			{
				break;
			}
		case OPTIGA_ASYNC_CMD_PROCESS_CHALLENGE_RESPONSE: 
			{				

				igs_command_add_dat(COMMAND_IKV_CHALLENGE);
				igs_command_add_buf(response,sizeof(response));
				igs_command_add_buf(mac_data,sizeof(mac_data));
				igs_command_insert(COMMAND_IKV);

#ifdef IKV_SELF_TEST
				if (optiga_hst_verify_response (&auth_session, response, mac_data) == TRUE)
				{
					printf ("--> Response validation ok ...\n\r");      
				} 
				else 
				{ 
					printf ("[Error] failed to verify response ...\n\r");
				}	
#endif
				break;
			}
		case OPTIGA_ASYNC_CMD_PROCESS_MAC: 
			{
				break;
			}
		case OPTIGA_ASYNC_CMD_WRITE_NVM: 
			{
				igs_command_add_dat(COMMAND_IKV_STATE);
				igs_command_add_dat(IGS_IKV_WRITE_OK);
				igs_command_insert(COMMAND_IKV);
				break;
			}
		case OPTIGA_ASYNC_CMD_READ_NVM: 
			{
				igs_command_add_dat(COMMAND_IKV_READ);
				igs_command_add_dat(nvmAddr);
				igs_command_add_buf(dataBuf,IGS_IKV_NVM_RW_SIZE);
				igs_command_insert(COMMAND_IKV);
				break;
			}
		default:break;
	}
}
