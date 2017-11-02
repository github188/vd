#ifndef __ZEHIN_TIMER_H__
#define __ZEHIN_TIMER_H__

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*timer_timeout)(union sigval sig);

void park_create_timer(timer_t *timer_id, timer_timeout func, int data);
void park_set_timer(timer_t *timer_id, int time_msec);
void park_delete_timer(timer_t *timer_id);

#ifdef __cplusplus
}
#endif

#endif
