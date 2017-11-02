#ifndef __HTTP_COMMON_H__
#define __HTTP_COMMON_H__

#include "park_records_process.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _body_data {
    const char* json_data;
    size_t json_lens;
    const EP_PicInfo *pic;
}body_data_t;

typedef struct {
    ParkAlarmInfo *alarm;
}http_alarm;

/*
 * Name:        http_provisioned
 * Description: only check if the switch is on,
 *              used for dsp to decide whether send to http.
 * Return: bool true switch on
 *              false switch off
 */
bool http_provisioned(void);

bool set_http_online(bool status);

bool http_is_online(void);

/*
 * Name:        http_get_url
 * Discription: return the http_url
 */
const char* http_get_url(void);

/*
 * Name:        http_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool http_retransmition_enable(void);

/*
 * Name:        http_rotate_count
 * Discription: return the rotate_count
 */
int http_rotate_count(void);
int http_hb_interval(void);

#ifdef __cplusplus
}
#endif
#endif
