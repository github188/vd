#ifndef __XTIMER_H
#define __XTIMER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "types.h"

C_CODE_BEGIN

/* Timer handle */
typedef timer_t xtimer_t;
/* Timer callback function */
typedef void (*xtimer_callback)(union sigval sig);

/**
 * xtimer_create - Create a timer
 *
 * @author cyj (2016/5/6)
 *
 * @param tmr - the timer handle
 * @param cb - timer callback function
 * @param arg - the timer callback arg
 *
 * @return int_fast32_t On success return 0, on failure -1 is
 *  	   returned
 */
int_fast32_t xtimer_create(xtimer_t *tmr, xtimer_callback cb, void *arg);

/**
 * xtimer_settime - Set time for timer
 *
 * @author cyj (2016/5/6)
 *
 * @param tmr - the timer handle
 * @param delay - time to first run callback, unit: ms
 * @param intv - callback run interval, unit: ms
 *
 * @return int_fast32_t On success return 0, on failure -1 is
 *  	   returned
 */
int_fast32_t xtimer_settime(xtimer_t *tmr, int_fast32_t delay,
							int_fast32_t intv);

/**
 * xtimer_del - Delete a timer
 *
 * @author cyj (2016/5/6)
 *
 * @param tmr - the timer handle
 *
 * @return int_fast32_t On success return 0, on failure -1 is
 *  	   returned
 */
int_fast32_t xtimer_del(xtimer_t *tmr);


C_CODE_END


#endif
