#ifndef __BAOKANG_TIMER_H__
#define __BAOKANG_TIMER_H__

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*timer_timeout)(union sigval sig);

void baokang_create_timer(timer_t *timer_id, timer_timeout func, int data);
void baokang_set_timer(timer_t *timer_id, int time_msec);
void baokang_delete_timer(timer_t *timer_id);

#ifdef __cplusplus
}
#endif

#endif
