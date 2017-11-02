#ifndef __VE_LED_PORTS_H
#define __VE_LED_PORTS_H

#include "types.h"
#include "dev/led/led.h"

/**
 * Refresh a new screen for fly eq20131
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_fly_eq20131_refresh(led_screen_t *screen);

/**
 * Clear screen for fly eq20131
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_fly_eq20131_clear(void);

/**
 * Refresh a new screen for degao
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int_fast32_t ve_degao_refresh(led_screen_t *screen);

/**
 *
 *
 * @author chenyj (2016/9/18)
 *
 * @param idx
 *
 * @return int_fast32_t
 */
int_fast32_t ve_degao_clear_line(int_fast32_t idx);

/**
 * Clear screen for listen a4
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t listen_clear(void);

/**
 * Clear a line for listen a4
 *
 * @author chenyj (2016-09-02)
 *
 * @param idx
 *
 * @return int_fast32_t
 */
int_fast32_t listen_clear_line(int_fast32_t idx);

/**
 * Refresh a new screen for listen a4
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t listen_refresh(led_screen_t *screen);

/**
 * Initialize listen a4 card
 *
 * @author cyj (2016/7/21)
 *
 * @param void
 *
 * @return int32_t
 */
int32_t listen_init(void);

#endif
