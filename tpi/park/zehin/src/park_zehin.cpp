#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <json/json.h>
#include <string.h>

#include "vd_msg_queue.h"
#include "logger/log.h"
#include "tpzehin_pthread.h"
#include "tpzehin_records.h"

#include "park_zehin.h"
#include "refhandle.h"
#include "light_ctl_api.h"

extern bool g_light_controlled_by_platform;

CParkZehin::CParkZehin(void)
{
	this->m_platform_type = EPlatformType::PARK_ZEHIN;

	m_platform_info.enable = false;
	m_platform_info.retransmition_enable = false;
	m_platform_info.send_status_when_retransmition = false;
	m_platform_info.space_status_enable = false;
	m_platform_info.light_ctl_enable = true; // default is true if it is not
											 // setted from configuration file
	m_platform_info.rotate_count = 0;
	m_platform_info.device_id = "";
	m_platform_info.center_ip = "";
	m_platform_info.sf_ip = "";
	m_platform_info.cmd_port = 0;
	m_platform_info.heartbeat_port = 0;
	m_platform_info.sf_port = 0;
}

int CParkZehin::run(void)
{
	pthread_t zehin;
	if(pthread_create(&zehin, NULL, tpzehin_pthread, NULL))
		ERROR("Create tpi tpzehin_pthread failed!");
	this->m_running = true;
	return 0;
}

int CParkZehin::stop(void)
{
	this->m_running = false;
	return 0;
}

int CParkZehin::subscribe(void)
{
	this->m_platform_info.enable = true;
	return 0;
}

int CParkZehin::unsubscribe(void)
{
	this->m_platform_info.enable = false;
	return 0;
}

bool CParkZehin::subscribed(void)
{
	return (this->m_platform_info.enable && this->m_running);
}

int CParkZehin::alarm_notify(void *data)
{
    if (NULL == data) {
        ERROR("NULL == data");
        return -1;
    }

    str_tpzehin_records &records = gstr_tpzehin_records;

    ParkAlarmInfo *parkAlarmInfo = (ParkAlarmInfo*)data;

    _RefHandleRetain(data);

    char alarm_time[32];
    struct tm park_alarm_time;
    localtime_r((const time_t*)&(parkAlarmInfo->alarmTime), &park_alarm_time);

    snprintf(alarm_time, sizeof(alarm_time), "%04d-%02d-%02d %02d:%02d:%02d",
             park_alarm_time.tm_year + 1900, park_alarm_time.tm_mon + 1,
             park_alarm_time.tm_mday, park_alarm_time.tm_hour,
             park_alarm_time.tm_min, park_alarm_time.tm_sec);

	pthread_mutex_lock(&g_zehin_alarm_mutex);
	records.alarm.category = parkAlarmInfo->category;
	records.alarm.level = parkAlarmInfo->level;
	strcpy(records.alarm.time, alarm_time);
	pthread_mutex_unlock(&g_zehin_alarm_mutex);

    /**
     * for zehin platform, already copy data into gstr_tpzehin_xxx,
     * no need data so release it.
     */
    _RefHandleRelease((void**)&data, NULL, NULL);
    return 0;
}

/**
 * @brief leave info
 *
 * @param data
 *
 * @return useless
 */
int CParkZehin::info_notify(void *data)
{
    if (NULL == data) {
        ERROR("NULL == data");
        return -1;
    }

    str_tpzehin_records &records = gstr_tpzehin_records;

    DB_ParkRecord *db_park_record = &(((struct Park_record*)data)->db_park_record);
    _RefHandleRetain(data);

	pthread_mutex_lock(&g_zehin_record_mutex);
	memcpy(&records.field, db_park_record, sizeof(DB_ParkRecord));
	pthread_mutex_unlock(&g_zehin_record_mutex);

	/* if light is controlled by platform for charges owed indication
	 * when this car leave, should restore the light. */
	if (g_light_controlled_by_platform) {
		light_ctl(LIGHT_ACTION_STOP, LIGHT_PRIO_PLATFORM, 0, 0, 0, 0, 255, 5, 0);
	}

    /**
     * for zehin platform, already copy data into gstr_tpzehin_xxx,
     * no need data so release it.
     */
    _RefHandleRelease((void**)&data, NULL, NULL);
    return 0;
}

int CParkZehin::record_notify(void *data)
{
    if (NULL == data) {
        ERROR("NULL == data");
        return -1;
    }

    str_tpzehin_records &records = gstr_tpzehin_records;

    struct Park_record *park_record = (struct Park_record *)data;
    _RefHandleRetain(data);

	pthread_mutex_lock(&g_zehin_record_mutex);
	memcpy(&(records.field), &(park_record->db_park_record),
			sizeof(DB_ParkRecord));
	for(int i = 0; i < PARK_PIC_NUM; i++)
	{
		records.picture_size[i] = park_record->pic_info[i].size;
		memcpy(records.picture[i], park_record->pic_info[i].buf,
			   records.picture_size[i]);
	}

	pthread_mutex_unlock(&g_zehin_record_mutex);

	/* if light is controlled by platform for charges owed indication
	 * when this car leave, should restore the light. */
	if (g_light_controlled_by_platform) {
		light_ctl(LIGHT_ACTION_STOP, LIGHT_PRIO_PLATFORM, 0, 0, 0, 0, 255, 5, 0);
	}

    /**
     * for zehin platform, already copy data into gstr_tpzehin_xxx,
     * no need data so release it.
     */
    _RefHandleRelease((void**)&data, NULL, NULL);
    return 0;
}

int CParkZehin::notify(int type, void* data)
{
	str_park_msg lstr_park_msg = {0};
	switch(type) {
		case MSG_PARK_ALARM: {
            alarm_notify(data);
            lstr_park_msg.data.zehin.type = MSG_PARK_ALARM;
			break;
		}
		case MSG_PARK_INFO: {
            info_notify(data);
            lstr_park_msg.data.zehin.type = MSG_PARK_INFO;
			break;
		}
		case MSG_PARK_STATUS: {
			StatusPark* statusPark = (StatusPark*)data;
            _RefHandleRetain(data);
			lstr_park_msg.data.zehin.type = MSG_PARK_STATUS;
			lstr_park_msg.data.zehin.lens = sizeof(StatusPark);
			lstr_park_msg.data.zehin.data = (char*)statusPark;
			break;
		}
		case MSG_PARK_RECORD: {
            record_notify(data);
            lstr_park_msg.data.zehin.type = MSG_PARK_RECORD;
            break;
		}
		default: {
			DEBUG("ZEHIN no match handler for %d", type);
			return -1;
		}
	}

	vmq_sendto_tpzehin(lstr_park_msg);
	return 0;
}

int CParkZehin::set_platform_info(void *p)
{
	memcpy(&m_platform_info, p, sizeof(zehin_platform_info_t));
	return 0;
}

int CParkZehin::set_platform_info(const Json::Value & root)
{
	if (root["platform_type"].asString() != "zehin")
	{
		ERROR("json configuration %s is not for zehin platform.",
				root["platform_type"].asString().c_str());
		return -1;
	}

	m_platform_info.enable = root["enable"].asString() == "yes" ? true : false;
	m_platform_info.retransmition_enable = root["attribute"]["retransmition_enable"].asString() == "yes" ? true : false;
	m_platform_info.send_status_when_retransmition = root["attribute"]["send_status_when_retransmition"].asString() == "yes" ? true : false;
	m_platform_info.space_status_enable = root["attribute"]["space_status_enable"].asString() == "yes" ? true : false;

	if (!root["attribute"]["light_ctl_enable"].isNull())
		m_platform_info.light_ctl_enable = root["attribute"]["light_ctl_enable"].asString() == "yes" ? true : false;

	m_platform_info.rotate_count = atoi(root["attribute"]["rotate_count"].asString().c_str());
	m_platform_info.device_id = root["attribute"]["device_id"].asString();
	m_platform_info.center_ip = root["attribute"]["center_ip"].asString();
	m_platform_info.sf_ip = root["attribute"]["sf_ip"].asString();
	m_platform_info.sf_port = atoi(root["attribute"]["sf_port"].asString().c_str());
	m_platform_info.cmd_port = atoi(root["attribute"]["cmd_port"].asString().c_str());
	m_platform_info.heartbeat_port = atoi(root["attribute"]["heartbeat_port"].asString().c_str());

	return 0;
}

int CParkZehin::set_platform_info(const string & json_str)
{
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(json_str, root))
	{
		ERROR("Parse json failed. json_str = %s", json_str.c_str());
		return -1;
	}

	set_platform_info(root);
	return 0;
}

void * CParkZehin::get_platform_info(void)
{
	return &(this->m_platform_info);
}

// return the json string of the configuration.
string CParkZehin::get_platform_info_str(void)
{
	Json::Value root = get_platform_info_json();
	Json::FastWriter writer;
	return writer.write(root);
}

// return the json string of the configuration.
Json::Value CParkZehin::get_platform_info_json(void)
{
	Json::Value root;
	Json::Value attribute;

	char center_port[6] = {0};
	char heartbeat_port[6] = {0};
	char sf_port[6] = {0};
	char rotate_count[4] = {0};

	zehin_platform_info_t *platform_info =
		(zehin_platform_info_t*)get_platform_info();

	sprintf(center_port, "%d", platform_info->cmd_port);
	sprintf(heartbeat_port, "%d", platform_info->heartbeat_port);
	sprintf(sf_port, "%d", platform_info->sf_port);
	sprintf(rotate_count, "%d", platform_info->rotate_count);

	attribute["retransmition_enable"] = platform_info->retransmition_enable ? "yes" : "no";
	attribute["send_status_when_retransmition"] = platform_info->send_status_when_retransmition ? "yes" : "no";
	attribute["rotate_count"] = rotate_count;
	attribute["space_status_enable"] = platform_info->space_status_enable ? "yes" : "no";
	attribute["light_ctl_enable"] = platform_info->light_ctl_enable ? "yes" : "no";
	attribute["device_id"] = platform_info->device_id;
	attribute["center_ip"] = platform_info->center_ip;
	attribute["sf_ip"] = platform_info->sf_ip;
	attribute["cmd_port"] = center_port;
	attribute["heartbeat_port"] = heartbeat_port;
	attribute["sf_port"] = sf_port;

	root["platform_type"] = "zehin";
	root["enable"] = platform_info->enable ? "yes" : "no";
	root["attribute"] = attribute;

	return root;
}
