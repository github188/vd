/**
 * @file park_lock_cram.h
 * @brief this file include the park lock control functions for park lock
 *        from CrAM IoT Technology Co.,Ltd. including protocal analyze, park
 *        lock control etc.
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-05-18
 */
#ifndef __PARK_LOCK_CRAM_H__
#define __PARK_LOCK_CRAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "park_lock_control.h"
#include "park_timer.h"

typedef void* (*park_lock_cram_resp_cb)(void*);

enum PARK_LOCK_CRAM_RESPONSE
{
    PARK_LOCK_CRAM_RESP_INVALID = 0,
    PARK_LOCK_CRAM_RESP_ACK,
    PARK_LOCK_CRAM_RESP_STATE,
    PARK_LOCK_CRAM_RESP_REFRESH_BEGIN,
    PARK_LOCK_CRAM_RESP_REFRESH_FINISH,
    PARK_LOCK_CRAM_RESP_REFRESH_ERROR,
    PARK_LOCK_CRAM_RESP_LOCK_LIST,
    PARK_LOCK_CRAM_RESP_MAX
};

typedef struct _park_lock_cram
{
    char mac[17];
    int gw_fd; /* the socket fd of its gateway */

    timer_t watchdog_timer;
    pthread_t recv_thread;
    park_lock_operation_t operation;
}park_lock_cram;

typedef struct _cram_ctrl_t
{
    int type;
    union {
        int beep_time;
    };
}cram_ctrl_t;

int park_lock_cram_init(const char* gateway_ip,
        const int gateway_port, const char* lock_mac);
#ifdef __cplusplus
}
#endif
#endif
