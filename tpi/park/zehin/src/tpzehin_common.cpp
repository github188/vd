/*
 * This file includes the common function for zehin platform.
 * which maybe used in any files.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "park_zehin.h"
#include "arm_config.h"
#include "Msg_Def.h"
#include "tpzehin_common.h"
#include "../lib/libsignalling/vPaas_DataType.h"
#include "../lib/libsignalling/vPaasT_SDK.h"

#if 0 /* 刚开始 所有的web配置都是通过这个获取的，后来改成从class里获取 */
extern PLATFORM_SET msgbuf_paltform;
#endif
extern list<CPlatform*> g_platform_list;

/**
 * Name:        get_zehin_platform_instance
 * Description: get the zehin instance from g_platform_list;
 *              this is a HACK method because the previous function is not
 *              an member funtcion of CPlatform so get the instance from
 *              list each time.
 */
static CPlatform* get_zehin_platform_instance()
{
	auto it = g_platform_list.begin();
	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it)
	{
		if(EPlatformType::PARK_ZEHIN == (*it)->get_platform_type())
			return *it;
	}
	return NULL;
}

/**
 * Name:        tpzehin_provisioned
 * Description: only check if the switch is on,
 *              used for dsp to decide whether send to zehin.
 *              check if online(vPaasT_GetIsOnLine) is used when send to zehin
 * Return: bool true switch on
 *              false switch off
 */
bool tpzehin_provisioned(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return false;

	if(((zehin_platform_info_t *)(instance->get_platform_info()))->enable)
#else
    // check if configured to run from web
    if(msgbuf_paltform.pf_switch.park_switch == P_on)
#endif
        return true;
    else
        return false;
}

/**
 * Name:        tpzehin_is_online
 * Discription: wrapper for vPaasT_GetIsOnLine
 */
bool tpzehin_is_online(void)
{
    return vPaasT_GetIsOnLine();
}

/**
 * Name:        tpzehin_get_spot_id
 * Discription: return the spot_id
 */
char* tpzehin_get_spot_id(void)
{
#if 0
	CPlatform* instance = get_zehin_platform_instance();

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->device_id.c_str();
#else
    extern ARM_config g_arm_config; //arm参数结构体全局变量
    return g_arm_config.basic_param.spot_id;
#endif
}

/**
 * Name:        tpzehin_get_center_ip
 * Discription: return the zehin_center_ip
 */
const char* tpzehin_get_center_ip(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return NULL;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->center_ip.c_str();
#else
    return msgbuf_paltform.Msg_buf.Zehin_Park_Mag.CenterIp;
#endif
}

/**
 * Name:        tpzehin_get_cmd_port
 * Discription: return the zehin_cmd_port
 */
int tpzehin_get_cmd_port(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->cmd_port;
#else
    return msgbuf_paltform.Msg_buf.Zehin_Park_Mag.CmdPort;
#endif
}

/**
 * Name:        tpzehin_get_heartbeat_port
 * Discription: return the zehin_heartbeat_port
 */
int tpzehin_get_heartbeat_port(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->heartbeat_port;
#else
    return msgbuf_paltform.Msg_buf.Zehin_Park_Mag.HbPort;
#endif
}

/**
 * Name:        tpzehin_get_sf_ip
 * Discription: return the zehin_sf_ip
 */
const char* tpzehin_get_sf_ip(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return NULL;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->sf_ip.c_str();
#else
    return msgbuf_paltform.Msg_buf.Zehin_Park_Mag.SFIp;
#endif
}

/**
 * Name:        tpzehin_get_sf_port
 * Discription: return the zehin_sf_port
 */
int tpzehin_get_sf_port(void)
{
#if 1
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->sf_port;
#else
    return msgbuf_paltform.Msg_buf.Zehin_Park_Mag.SFPort;
#endif
}

/**
 * Name:        tpzehin_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool tpzehin_retransmition_enable(void)
{
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return false;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->retransmition_enable;
}

/**
 * Name:        tpzehin_send_status_when_retransmition
 * Discription: return the bool value of send_status_when_retransmition
 */
bool tpzehin_send_status_when_retransmition(void)
{
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return false;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->send_status_when_retransmition;
}

/**
 * Name:        tpzehin_space_status_enable
 * Discription: return the bool value of space_status_enable
 */
bool tpzehin_space_status_enable(void)
{
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->space_status_enable;
}

/**
 * Name:        tpzehin_light_ctl_enable
 * Discription: if true, light controll instruction from platform make sense
 */
bool tpzehin_light_ctl_enable(void)
{
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->light_ctl_enable;
}

/**
 * Name:        tpzehin_rotate_count
 * Discription: return the rotate_count
 */
int tpzehin_rotate_count(void)
{
	CPlatform* instance = get_zehin_platform_instance();
	if(NULL == instance)
		return -1;

	return ((zehin_platform_info_t *)(instance->get_platform_info()))->rotate_count;
}

