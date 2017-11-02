#ifndef __VE_LED_H
#define __VE_LED_H

#include "types.h"
#include "list.h"
#include "sys/pool.h"
#include "dev/led/led.h"

C_CODE_BEGIN


typedef struct ve_led_screen{
	list_head_t link;
	/* New screens data */
	led_screen_t data;
}ve_led_screen_t;

/**
 * @brief Initialize memory for led screen
 * 
 * @author cyj (2016/7/18)
 * 
 * @param void 
 * 
 * @return int32_t On success return 0, otherwise return -1
 */
int32_t ve_led_ram_init(void);

/**
 * @brief Create thread for led screen manager
 * 
 * @author cyj (2016/7/18)
 * 
 * @param void 
 * 
 * @return int32_t On success return 0, otherwise some error 
 *  	   occcur
 */
int32_t ve_led_thread_init(void);

/**
 * @brief Lock or unlock the led screen
 *
 * @author chenyj (2017-03-31)
 *
 * @param lock True for lock, false for unlock
 */
void ve_led_set_lock(bool lock);
int_fast32_t led_add_lines(led_line_t **lines, int_fast32_t n);
int32_t ve_led_get_max_line_num(void);
void led_set_model(int32_t model);

C_CODE_END


#endif
