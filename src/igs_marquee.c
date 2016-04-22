/**
 * @file igs_marquee.c
 * @author Sheng Wen Peng
 * @date 20 Jan 2014
 * @brief IGS marquee driver
 * 
 *    pattern: 
 *    group: 

 */

#include <stdio.h>
#include "igs_marquee.h"
#include "igs_marquee_driver.h"
#include "igs_malloc.h"
#include "igs_gpio.h"
#include "igs_task.h"
#include "igs_protocol.h"

#define NUM_OF_MARQUEE 4

#define MARQUEE_REPEAT_LEN 85
/** 
 *  @brief spi subpackage command list
 */
#define COMMAND_MARQUEE_INIT 0x0
#define COMMAND_MARQUEE_GO 0x1
#define COMMAND_MARQUEE_SET_PATTERN 0x2
#define COMMAND_MARQUEE_SET_GROUP 0x3
#define COMMAND_MARQUEE_DIR_INIT 0x4
#define COMMAND_MARQUEE_DIR_SET 0x5
#define COMMAND_MARQUEE_DIR_SET_REPEAT 0x6

#define EQUAL 0
#define LESS -1
#define GREATER 1

#define MARQUEE_RUN 1
#define MARQUEE_STOP 0

#define MARQUEE_ELAPSTED_TIME 2

/**
 * @brief 
 *    spi data lenght = 256-head = 249
 *    (R, G, P) = 3
 *    249 = 3, 3 * 63
 */
#define MAX_LEN_OF_PATTERN 63 // = 249 


/**
 *  @brief struct pattern_t
 *     store RGB byte data in pattern
 */ 
struct pattern_t
{
	uint32_t device_id;
	uint32_t pattern_id;
	uint8_t len;
	uint8_t *data;
	struct pattern_t *next; /**< store all patterns in a large linking list */
};

/**
 *  @brief struct pattern_list_t
 *    store pattern in the liking list. when the marquee is work, 
 *  we can get pattern orderly through this structrue
 */
struct pattern_list_t
{
	uint8_t len;
	uint8_t *data;
	struct pattern_list_t *next; /**< store the pattern play order here */
};

/**
 *  @brief struct group_t
 *     store different combination of pattern
 */
struct group_t
{
	uint8_t IsEnable;
	uint32_t device_id;
	uint32_t group_id;
	uint8_t num_of_pattern;
	struct pattern_list_t *pattern_head;
	struct group_t *next; /**< store all groups in a large linking list */
};

/**
 *  @brief struct marquee_device_t
 */
struct marquee_device_t
{
	uint8_t IsEnable;
	uint8_t IsInit;
	uint8_t chip_type;
	uint8_t output_data_id;
	uint8_t output_clk_id;
	uint8_t output_latch_id;
	
	uint32_t change_time;
	uint32_t change_count;
	uint8_t IsSingleRun;
	uint8_t IsRunning;
	struct pattern_list_t *pattern_head; /**< group pattern head */
	struct pattern_list_t *pattern_now;  /**< store the pattern need to paly next time */
}marquee_device[NUM_OF_MARQUEE];


static struct pattern_t *pattern_top = NULL;
static struct group_t *group_top = NULL;
static uint8_t repeat_buf[MARQUEE_REPEAT_LEN*3];

static int32_t compare_pattern(void *_a, void *_b);
static int32_t compare_group(void *_a, void *_b);

static void pattern_insert(struct pattern_t *obj);
static struct pattern_t * pattern_search(uint32_t device_id, uint32_t pattern_id);
static void group_insert(struct group_t *obj);
static struct group_t * group_search(uint32_t device_id, uint32_t group_id);

static void marquee_polling(void);
static void marquee_commnad(uint8_t *buf, uint32_t len);
static void marquee_command_init(uint8_t *buf, uint32_t len);
static void marquee_command_go(uint8_t *buf, uint32_t len);
static void marquee_command_set_pattern(uint8_t *buf, uint32_t len);
static void marquee_command_set_group(uint8_t *buf, uint32_t len);
static void marquee_command_dir_marquee_init(uint8_t *buf, uint32_t len);
static void marquee_command_dir_marquee_set(uint8_t *buf, uint32_t len);
static void marquee_command_dir_marquee_set_repeat(uint8_t *buf, uint32_t len);

/**
 *  @brief compare_pattern
 *  @param a
 *		comparator
 *  @param b 
 *    comparand
 *  @return
 *    1 represent a > b
 *    0 represent a = b
 *   -1 represent a < b
 */
static int32_t compare_pattern(void *_a, void *_b)
{
	struct pattern_t *a,*b;
	a = (struct pattern_t *)_a;
	b = (struct pattern_t *)_b;
	if(a->device_id > b->device_id)
	{
		// a > b
		return GREATER;
	}
	else if(a->device_id < b->device_id)
	{
		// a < b
		return LESS;
	}

	if(a->pattern_id > b->pattern_id)
	{
		// a > b
		return GREATER;
	}
	else if(a->pattern_id < b->pattern_id)
	{
		// a < b
		return LESS;
	}
	else
	{
		// a = b
		return EQUAL;
	}
}

/**
 *  @brief compare_group
 *  @param a
 *		comparator
 *  @param b 
 *    comparand
 *  @return
 *    1 represent a > b
 *    0 represent a = b
 *   -1 represent a < b
 */
static int32_t compare_group(void *_a, void *_b)
{
	
	struct group_t *a,*b;
	a = (struct group_t *)_a;
	b = (struct group_t *)_b;
	
	if(a->device_id > b->device_id)
	{
		// a > b
		return GREATER;
	}
	else if(a->device_id < b->device_id)
	{
		// a < b
		return LESS;
	}

	if(a->group_id > b->group_id)
	{
		// a > b
		return GREATER;
	}
	else if(a->group_id < b->group_id)
	{
		// a < b
		return LESS;
	}
	else
	{
		// a = b
		return EQUAL;
	}
}

/**
 *  @brief pattern_insert
 *     insert pattern into the data struct
 *  @param obj
 *     pattern you want to insert
 *  @return none
 */
static void pattern_insert(struct pattern_t *obj)
{
	struct pattern_t *tmp;
	if(pattern_top == NULL)
	{
		pattern_top = obj;
		return;
	}
	
	tmp = pattern_top;
	while(tmp->next != NULL)
	{
		tmp = tmp->next;
	}
	tmp-> next = obj;
}

/**
 *  @brief pattern_search
 *     find specify pattern from the group memory struct
 *  @param device_id
 *  @param pattern_id
 *  @return 
 *     NULL-> search failed
 *     or struct pattern_t point
 */
static struct pattern_t * pattern_search(uint32_t device_id, uint32_t pattern_id)
{
	struct pattern_t *tmp1,tmp2;
	int32_t ret;
	
	if(pattern_top == NULL)
	{
		return NULL;
	}
	
	tmp1 = pattern_top;
	tmp2.device_id = device_id;
	tmp2.pattern_id = pattern_id;
	
	while(1)
	{
		ret = compare_pattern((void *)tmp1, (void *)&tmp2);
		if(ret == EQUAL)
		{
			return tmp1;
		}
		
		if(tmp1->next == NULL)
		{
			return NULL;
		}
		
		tmp1 = tmp1->next;
	}
}

/**
 *  @brief group_insert
 *     insert group into the data struct
 *  @param obj
 *     group you want to insert
 *  @return none
 */
static void group_insert(struct group_t *obj)
{
	struct group_t *tmp;
	if(group_top == NULL)
	{
		group_top = obj;
		return;
	}
	
	tmp = group_top;
	while(tmp->next != NULL)
	{
		tmp = tmp->next;
	}
	tmp-> next = obj;
}

/**
 *  @brief group_search
 *     find specify group from the group memory struct
 *  @param device_id
 *  @param group_id
 *  @return 
 *     NULL-> search failed
 *     or struct group_t point
 */
static struct group_t * group_search(uint32_t device_id, uint32_t group_id)
{
	struct group_t *tmp1,tmp2;
	int32_t ret;
	
	if(group_top == NULL)
	{
		return NULL;
	}
	
	tmp1 = group_top;
	tmp2.device_id = device_id;
	tmp2.group_id = group_id;
	
	while(1)
	{
		ret = compare_group((void *)tmp1, (void *)&tmp2);
		if(ret == EQUAL)
		{
			return tmp1;
		}
		
		if(tmp1->next == NULL)
		{
			return NULL;
		}
		
		tmp1 = tmp1->next;
	}
}


/**
 * @brief marquee_polling 
 */
static void marquee_polling(void)
{
	uint32_t i;
	uint8_t stagger_flag;
	struct pattern_list_t *pattern_list;
	
	stagger_flag = 0;
	for(i=0;i<NUM_OF_MARQUEE;i++)
	{
		if(marquee_device[i].IsEnable == 1)
		{
			if(marquee_device[i].IsRunning)
			{
				marquee_device[i].change_count+=MARQUEE_ELAPSTED_TIME;
				if(stagger_flag == 1) continue;
				
				if(marquee_device[i].change_count >= marquee_device[i].change_time)
				{
					pattern_list = marquee_device[i].pattern_now;
					
					if(marquee_device[i].IsInit == 0)
					{
						marquee_device[i].IsInit = 1;
						
						marquee_chip_init(
							marquee_device[i].chip_type, 
							marquee_device[i].output_clk_id, 
							marquee_device[i].output_data_id, 
							marquee_device[i].output_latch_id,
							pattern_list->len
						);
						marquee_device[i].change_count = 0;
						stagger_flag = 1;
						continue;
					}
					
					
					marquee_chip_set(
						marquee_device[i].chip_type, 
						marquee_device[i].output_clk_id, 
						marquee_device[i].output_data_id,
						marquee_device[i].output_latch_id,
						pattern_list->data,
						pattern_list->len
					);
					//
					//  next pattern exist?
					//
					if( (pattern_list -> next) == NULL)
					{
						if(marquee_device[i].IsSingleRun == 1)
						{
							marquee_device[i].IsRunning = MARQUEE_STOP;
						}
						else
						{
							marquee_device[i].IsRunning = MARQUEE_RUN;
							marquee_device[i].change_count = 0;
							marquee_device[i].pattern_now = marquee_device[i].pattern_head;
						}
					}
					else
					{
						marquee_device[i].pattern_now = pattern_list->next;
						marquee_device[i].change_count = 0;
					}
					stagger_flag = 1;
					continue;
				}
			}
		}
	}
}

/**
 * @brief marquee_commnad
 *    
 * @param buf
 *   format:  buf[0] sub command
 *            buf[1] command content1
 *            buf[2] command content2
 *            buf[3]         .
 *            buf[4]         .
 *            buf[5]         .
 * @param len
 *    buf length
 *  @return
 */
static void marquee_commnad(uint8_t *buf, uint32_t len)
{
	switch(buf[0])
	{
		case COMMAND_MARQUEE_INIT:
			marquee_command_init(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_GO:
			marquee_command_go(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_SET_PATTERN:
			marquee_command_set_pattern(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_SET_GROUP:
			marquee_command_set_group(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_DIR_INIT:
			marquee_command_dir_marquee_init(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_DIR_SET:
			marquee_command_dir_marquee_set(&buf[1],len-1);
			break;
		case COMMAND_MARQUEE_DIR_SET_REPEAT:
			marquee_command_dir_marquee_set_repeat(&buf[1],len-1);
			break;
		default: break;
	}
}

/**
 * @brief marquee_command_init
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] IsEnable
 *            buf[2] chip_type
 *            buf[3] output_clk_id
 *            buf[4] output_data_id
 *            buf[5] output_latch_id
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_init(uint8_t *buf, uint32_t len)
{
	uint8_t device_id;
	uint8_t IsEnable;
	uint8_t chip_type;
	uint8_t output_clk_id;
	uint8_t output_data_id;
	uint8_t output_latch_id;
	
	
	device_id = buf[0];
	IsEnable = buf[1];
	chip_type = buf[2];
	output_clk_id = buf[3];
	output_data_id = buf[4];
	output_latch_id = buf[5];
	
	if(device_id < NUM_OF_MARQUEE)
	{
		marquee_device[device_id].IsEnable = IsEnable;
		marquee_device[device_id].chip_type = chip_type;
		marquee_device[device_id].IsInit = 0;
		if(IsEnable == 1)
		{
			igs_gpio_set_output_mask(output_clk_id ,DISABLE_GPIO_MASK);
			igs_gpio_set_output_mask(output_data_id ,DISABLE_GPIO_MASK);
			igs_gpio_set_output_mask(output_latch_id ,DISABLE_GPIO_MASK);
			
			igs_gpio_set_out_real(output_clk_id, CLK_INACTIVE);
			igs_gpio_set_out_real(output_data_id, 0);
			igs_gpio_set_out_real(output_latch_id, LATCH_INACTIVE);
			
			marquee_device[device_id].output_clk_id = output_clk_id;
			marquee_device[device_id].output_data_id = output_data_id;
			marquee_device[device_id].output_latch_id = output_latch_id;
			marquee_device[device_id].IsRunning = MARQUEE_STOP;
		}
		else
		{
			igs_gpio_set_output_mask(marquee_device[device_id].output_clk_id ,ENABLE_GPIO_MASK);
			igs_gpio_set_output_mask(marquee_device[device_id].output_data_id ,ENABLE_GPIO_MASK);
			igs_gpio_set_output_mask(marquee_device[device_id].output_latch_id ,ENABLE_GPIO_MASK);
		}
		
	}
}

/**
 * @brief marquee_command_go
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] group_id
 *            buf[2] change time
 *            buf[3] IsSingleRun
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_go(uint8_t *buf, uint32_t len)
{
	uint8_t device_id;
	uint8_t group_id;
	uint8_t change_time;
	uint8_t IsSingleRun;
	struct group_t *group;
	
	device_id = buf[0];
	group_id = buf[1];
	change_time = buf[2];
	IsSingleRun = buf[3];
	
	if(device_id < NUM_OF_MARQUEE)
	{
		group = group_search(device_id, group_id);
		
		if(group == NULL || group->IsEnable == 0)
		{
			marquee_device[device_id].IsRunning = MARQUEE_STOP;
			return;
		}
		
		if(change_time == 0)
		{
			change_time = 90;
		}
		else if(change_time < 50)
		{
			change_time = 50;
		}
		else if( (change_time%2) != 0 ) 
		{
			change_time += 1;
		}
		
		marquee_device[device_id].IsSingleRun = IsSingleRun;
		marquee_device[device_id].change_time = change_time;
		marquee_device[device_id].change_count = 0;
		marquee_device[device_id].pattern_head = group->pattern_head;
		marquee_device[device_id].pattern_now = group->pattern_head;
		marquee_device[device_id].IsRunning = MARQUEE_RUN;
	}
}
	
/**
 * @brief marquee_command_set_pattern
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] pattern_id
 *            buf[2] led length
 *            buf[3] R
 *            buf[4] G
 *            buf[5] B
 *            buf[6] R 
 *                 .
 *                 .
 *                 .
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_set_pattern(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t device_id, pattern_id;
	uint8_t data_length;
	struct pattern_t *pattern;
	
	device_id = buf[0];
	pattern_id = buf[1];
	data_length = len-3;

	pattern = (struct pattern_t *)igs_malloc(sizeof(struct pattern_t));
	if(pattern == NULL)
	{
		return;
	}
	
	pattern->data = igs_malloc(data_length);
	if(pattern->data == NULL)
	{
		return;
	}
	
	for(i=0;i<data_length;i++)
	{
		pattern->data[i] = buf[i+3];
	}
	
	pattern->next = NULL;
	pattern->len = data_length;
	pattern->device_id = device_id;
	pattern->pattern_id = pattern_id;
	pattern_insert(pattern);
	
}

/**
 * @brief marquee_command_set_group
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] group_id
 *            buf[2] num_of_pattern
 *            buf[3] pattern 1
 *            buf[4] pattern 2
 *                 .
 *                 .
 *                 .
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_set_group(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t device_id, group_id, pattern_id;
	uint8_t num_of_pattern;
	struct pattern_t *pattern;
	struct group_t *group;
	struct pattern_list_t *pattern_list, **tmp_pattern_list;
	
	device_id = buf[0];
	group_id = buf[1];
	num_of_pattern = buf[2];
	
	group = (struct group_t *)igs_malloc(sizeof(struct group_t));
	if(group == NULL)
	{
		return;
	}
	group->IsEnable = 1;
	group->group_id = group_id;
	group->device_id = device_id;
	group->num_of_pattern = num_of_pattern;
	tmp_pattern_list = &(group->pattern_head);
	
	for(i=0;i<num_of_pattern;i++)
	{
		pattern_id = buf[i+3];
		pattern = pattern_search(device_id, pattern_id);
		if(pattern == NULL)
		{
			return;
		}
		
		pattern_list = (struct pattern_list_t *)igs_malloc(sizeof(struct pattern_list_t));
		if(pattern_list == NULL)
		{
			return;
		}
		
		pattern_list->len = pattern->len;
		pattern_list->data = pattern->data;
		*tmp_pattern_list = pattern_list;
		tmp_pattern_list = &(pattern_list->next);
	}
	
	group_insert(group);
	
}

/**
 * @brief marquee_command_dir_marquee_set
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] data[0]
 *            buf[2] data[1]
 *            buf[3] data[2]
 *            buf[4] data[3]
 *                 .
 *                 .
 *                 .
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_dir_marquee_set(uint8_t *buf, uint32_t len)
{
	uint8_t device_id;
	device_id = buf[0];
	if(device_id < NUM_OF_MARQUEE)
	{
		marquee_device[device_id].IsRunning = MARQUEE_STOP;
		
		marquee_chip_set(
			marquee_device[device_id].chip_type, 
			marquee_device[device_id].output_clk_id,
			marquee_device[device_id].output_data_id,
			marquee_device[device_id].output_latch_id,
			&buf[1],
			len-1
		);
	}
}

/**
 * @brief marquee_command_dir_marquee_init
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] marquee device length
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_dir_marquee_init(uint8_t *buf, uint32_t len)
{
	uint8_t device_id,marquee_length;
	device_id = buf[0];
	marquee_length = buf[1];
	if(device_id < NUM_OF_MARQUEE)
	{
		marquee_device[device_id].IsRunning = MARQUEE_STOP;
		
		marquee_chip_init(
			marquee_device[device_id].chip_type, 
			marquee_device[device_id].output_clk_id,
			marquee_device[device_id].output_data_id,
			marquee_device[device_id].output_latch_id,
			marquee_length
		);
	}
}

/**
 * @brief marquee_command_dir_marquee_set_repeat
 *    
 * @param buf
 *   format:  buf[0] device_id
 *            buf[1] repeat times
 *            buf[2] data[0]
 *            buf[3] data[1]
 *            buf[4] data[2]
 *                 .
 *                 .
 *                 .
 * @param len
 *    buf length
 *  @return
 */
static void marquee_command_dir_marquee_set_repeat(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t id;
	uint8_t repeat_times;
	id = buf[0];
	
	if(id < NUM_OF_MARQUEE)
	{
		repeat_times = buf[1];
		if(repeat_times > MARQUEE_REPEAT_LEN)
		{
			repeat_times = MARQUEE_REPEAT_LEN;
		}
		
		for(i=0;i<repeat_times;i++)
		{
			repeat_buf[i*3] = buf[2];
			repeat_buf[i*3+1] = buf[3];
			repeat_buf[i*3+2] = buf[4];
		}
		
		marquee_device[id].IsRunning = MARQUEE_STOP;
		
		marquee_chip_set(
			marquee_device[id].chip_type, 
			marquee_device[id].output_clk_id,
			marquee_device[id].output_data_id,
			marquee_device[id].output_latch_id,
			repeat_buf,
			repeat_times*3
		);
	}
}

/**
 *  @brief igs_marquee_init
 */
void igs_marquee_init()
{
	igs_protocol_command_register(COMMAND_MARQUEE,marquee_commnad);
	igs_task_add(IGS_TASK_ROUTINE_POLLING, MARQUEE_ELAPSTED_TIME,marquee_polling);
}
