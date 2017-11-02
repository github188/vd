#include "park_bitcom.h"
#include "tpbitcom_common.h"

extern list<CPlatform*> g_platform_list;

/*
 * Name:        get_bitcom_platform_instance
 * Description: get the bitcom instance from g_platform_list;
 *              this is a HACK method because the previous function is not
 *              an member funtcion of CPlatform so get the instance from
 *              list each time.
 */
static CPlatform* get_bitcom_platform_instance()
{
	auto it = g_platform_list.begin();
	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it)
	{
		if(EPlatformType::PARK_BITCOM == (*it)->get_platform_type())
			return *it;
	}
	return NULL;
}

/*
 * Name:        tpbitcom_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool tpbitcom_retransmition_enable(void)
{
	CPlatform* instance = get_bitcom_platform_instance();
	if(NULL == instance)
		return -1;

	return ((bitcom_platform_info_t *)(instance->get_platform_info()))->retransmition_enable;
}

/*
 * Name:        tpbitcom_rotate_count
 * Discription: return the rotate_count
 */
int tpbitcom_rotate_count(void)
{
	CPlatform* instance = get_bitcom_platform_instance();
	if(NULL == instance)
		return -1;

	return ((bitcom_platform_info_t *)(instance->get_platform_info()))->rotate_count;
}
