#ifndef __TPZEHIN_COMMON_H__
#define __TPZEHIN_COMMON_H__

#include "park_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Name:        tpzehin_provisioned
 * Description: only check if the switch is on,
 *              used for dsp to decide whether send to zehin.
 *              check if online(vPaasT_GetIsOnLine) is used when send to zehin
 * Return: bool true switch on
 *              false switch off
 */
bool tpzehin_provisioned(void);

/*
 * Name:        tpzehin_is_online
 * Discription: wrapper for vPaasT_GetIsOnLine
 */
bool tpzehin_is_online(void);

/*
 * Name:        tpzehin_get_spot_id
 * Discription: return the spot_id
 */
char* tpzehin_get_spot_id(void);

/*
 * Name:        tpzehin_get_center_ip
 * Discription: return the zehin_center_ip
 */
const char* tpzehin_get_center_ip(void);

/*
 * Name:        tpzehin_get_cmd_port
 * Discription: return the zehin_cmd_port
 */
int tpzehin_get_cmd_port(void);

/*
 * Name:        tpzehin_get_heartbeat_port
 * Discription: return the zehin_heartbeat_port
 */
int tpzehin_get_heartbeat_port(void);

/*
 * Name:        tpzehin_get_sf_ip
 * Discription: return the zehin_sf_ip
 */
const char* tpzehin_get_sf_ip(void);

/*
 * Name:        tpzehin_get_sf_port
 * Discription: return the zehin_sf_port
 */
int tpzehin_get_sf_port(void);

/*
 * Name:        tpzehin_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool tpzehin_retransmition_enable(void);

/*
 * Name:        tpzehin_send_status_when_retransmition
 * Discription: return the bool value of send_status_when_retransmition
 */
bool tpzehin_send_status_when_retransmition(void);

/*
 * Name:        tpzehin_rotate_count
 * Discription: return the rotate_count
 */
int tpzehin_rotate_count(void);

/*
 * Name:        tpzehin_space_status_enable
 * Discription: return the space_status_enable
 */
bool tpzehin_space_status_enable(void);

/**
 * Name:        tpzehin_light_ctl_enable
 * Discription: if true, light controll instruction from platform make sense
 */
bool tpzehin_light_ctl_enable(void);

/*
 * Name:        tpzehin_get_cmd_info
 * Description: the wrapper for popen
 * Paramaters:  cmd -- buffer for command
 *              result -- buffer for result
 * Return:      0 OK
 *              -1 fail
 */
int tpzehin_get_cmd_info(const char *cmd, char *result);

#ifdef __cplusplus
}
#endif

#endif
