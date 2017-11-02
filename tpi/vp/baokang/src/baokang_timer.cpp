#include "baokang_timer.h"

timer_t heart_beat;

void baokang_create_timer(timer_t *timer_id, timer_timeout func, int data)
{
    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_int = data;
    sev.sigev_notify_function = func;
    sev.sigev_notify_attributes = NULL;

    /* create timer */
    if (timer_create (CLOCK_REALTIME, &sev, timer_id) == -1)
    {
        perror("timer_create, error");
        return;
    }
    return;
}

void baokang_set_timer(timer_t *timer_id, int time_msec)
{
    struct itimerspec its;

    /* Start the timer */
    its.it_value.tv_sec = time_msec / 1000;
    its.it_value.tv_nsec = (time_msec % 1000) * 1000000;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timer_settime (*timer_id, 0, &its, NULL) == -1)
    {
        perror("timer_settime error");
    }
    printf("call timer_settime reset timer done.\n");
    return;
}

void baokang_delete_timer(timer_t *timer_id)
{
    timer_delete(*timer_id);
}
