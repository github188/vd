/*
 * This file includes the common function for http platform.
 * which maybe used in any files.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "park_http.h"
#include "arm_config.h"
#include "Msg_Def.h"
#include "http_common.h"

extern list<CPlatform*> g_platform_list;

static bool g_http_is_online = false;

/*
 * Name:        get_http_platform_instance
 * Description: get the http instance from g_platform_list;
 *              this is a HACK method because the previous function is not
 *              an member funtcion of CPlatform so get the instance from
 *              list each time.
 */
static CPlatform* get_http_platform_instance()
{
	auto it = g_platform_list.begin();
	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
		if(EPlatformType::PARK_HTTP == (*it)->get_platform_type())
			return *it;
	}
	return NULL;
}

/*
 * Name:        http_provisioned
 * Description: only check if the switch is on,
 *              used for dsp to decide whether send to http.
 * Return: bool true switch on
 *              false switch off
 */
bool http_provisioned(void)
{
	CPlatform* instance = get_http_platform_instance();
	if(NULL == instance)
		return false;

	if(((http_platform_info_t *)(instance->get_platform_info()))->enable)
        return true;
    else
        return false;
}

bool set_http_online(bool status)
{
    g_http_is_online = status;
    return g_http_is_online;
}

bool http_is_online(void)
{
    return g_http_is_online;
}

/*
 * Name:        http_get_url
 * Discription: return the http_url
 */
const char* http_get_url(void)
{
	CPlatform* instance = get_http_platform_instance();
	if(NULL == instance)
		return NULL;

	return ((http_platform_info_t *)(instance->get_platform_info()))->url.c_str();
}

/*
 * Name:        http_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool http_retransmition_enable(void)
{
	CPlatform* instance = get_http_platform_instance();
	if(NULL == instance)
		return false;

	return ((http_platform_info_t *)(instance->get_platform_info()))->retransmition_enable;
}

/*
 * Name:        http_rotate_count
 * Discription: return the rotate_count
 */
int http_rotate_count(void)
{
	CPlatform* instance = get_http_platform_instance();
	if(NULL == instance)
		return -1;

	return ((http_platform_info_t *)(instance->get_platform_info()))->rotate_count;
}

int http_hb_interval(void)
{
	CPlatform* instance = get_http_platform_instance();
	if(NULL == instance)
		return -1;

	return ((http_platform_info_t *)(instance->get_platform_info()))->hb_interval;
}
