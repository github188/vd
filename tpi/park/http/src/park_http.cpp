#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <json/json.h>
#include <string.h>

#include "vd_msg_queue.h"
#include "logger/log.h"

#include "park_http.h"
#include "http_pthread.h"
#include "refhandle.h"

CParkHTTP::CParkHTTP(void)
{
	this->m_platform_type = EPlatformType::PARK_HTTP;
}

int CParkHTTP::run(void)
{
	pthread_t http;
    if(pthread_create(&http, NULL, park_http_platform_pthread, NULL)) {
		ERROR("Create park_http_platform_pthread failed! errno = %d", errno);
        return -1;
    }
	this->m_running = true;
	return 0;
}

int CParkHTTP::stop(void)
{
	this->m_running = false;
	return 0;
}

int CParkHTTP::subscribe(void)
{
	this->m_platform_info.enable = true;
	return 0;
}

int CParkHTTP::unsubscribe(void)
{
	this->m_platform_info.enable = false;
	return 0;
}

bool CParkHTTP::subscribed(void)
{
	return (this->m_platform_info.enable && this->m_running);
}

int CParkHTTP::notify(int type, void* data)
{
	str_park_msg park_msg = {0};
	switch(type) {
    case MSG_PARK_ALARM: {
        ParkAlarmInfo *parkAlarmInfo = (ParkAlarmInfo*)data;
        if (NULL == parkAlarmInfo) {
            ERROR("parkAlarmInfo is NULL.");
            return -1;
        }

        _RefHandleRetain(data);

        park_msg.data.http.type = MSG_PARK_ALARM;
        park_msg.data.http.data = parkAlarmInfo;
        park_msg.data.http.lens = sizeof(ParkAlarmInfo);
        break;
    }
    case MSG_PARK_INFO: {
        struct Park_record *record = (struct Park_record*)data;
        if (NULL == record) {
            ERROR("Null is park_record.");
            return -1;
        }

        _RefHandleRetain(data);
        park_msg.data.http.type = MSG_PARK_INFO;
        park_msg.data.http.data = record;
        park_msg.data.http.lens = sizeof(struct Park_record);
        break;
    }
    case MSG_PARK_RECORD: {
        struct Park_record *record = (struct Park_record*)data;
        if (NULL == record) {
            ERROR("Null is park_record.");
            return -1;
        }

        _RefHandleRetain(data);
        park_msg.data.http.type = MSG_PARK_RECORD;
        park_msg.data.http.data = record;
        park_msg.data.http.lens = sizeof(struct Park_record);
        break;
    }
    default: {
        DEBUG("HTTP no match handler for %d", type);
        return -1;
    }
	}

	vmq_sendto_http(park_msg);
	return 0;
}

int CParkHTTP::set_platform_info(void *p)
{
	memcpy(&m_platform_info, p, sizeof(http_platform_info_t));
	return 0;
}

int CParkHTTP::set_platform_info(const Json::Value & root)
{
	if (root["platform_type"].asString() != "http")
	{
		ERROR("json configuration %s is not for http platform.",
				root["platform_type"].asString().c_str());
		return -1;
	}

    if (root["enable"].asString() != "")
        m_platform_info.enable = root["enable"].asString() == "yes" ? true : false;
    if (root["attribute"]["retransmition_enable"].asString() != "")
        m_platform_info.retransmition_enable = root["attribute"]["retransmition_enable"].asString() == "yes" ? true : false;
    if (root["attribute"]["rotate_count"].asString() != "")
        m_platform_info.rotate_count = atoi(root["attribute"]["rotate_count"].asString().c_str());
    if (root["attribute"]["device_id"].asString() != "")
        m_platform_info.device_id = root["attribute"]["device_id"].asString();
    if (root["attribute"]["password"].asString() != "")
        m_platform_info.device_id = root["attribute"]["password"].asString();
    if (root["attribute"]["url"].asString() != "")
        m_platform_info.url = root["attribute"]["url"].asString();
    if (root["attribute"]["hb_interval"].asInt() != 0)
        m_platform_info.hb_interval = root["attribute"]["hb_interval"].asInt();

	return 0;
}

int CParkHTTP::set_platform_info(const string & json_str)
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

void * CParkHTTP::get_platform_info(void)
{
	return &(this->m_platform_info);
}

// return the json string of the configuration.
string CParkHTTP::get_platform_info_str(void)
{
	Json::Value root = get_platform_info_json();
	Json::FastWriter writer;
	return writer.write(root);
}

// return the json string of the configuration.
Json::Value CParkHTTP::get_platform_info_json(void)
{
	Json::Value root;
	Json::Value attribute;

	char rotate_count[4] = {0};

	http_platform_info_t *platform_info =
		(http_platform_info_t*)get_platform_info();

	sprintf(rotate_count, "%d", platform_info->rotate_count);

	attribute["retransmition_enable"] = platform_info->retransmition_enable ? "yes" : "no";
	attribute["rotate_count"] = rotate_count;
	attribute["device_id"] = platform_info->device_id;
	attribute["url"] = platform_info->url;
	attribute["password"] = platform_info->password;
	attribute["hb_interval"] = platform_info->hb_interval;

	root["platform_type"] = "http";
	root["enable"] = platform_info->enable ? "yes" : "no";
	root["attribute"] = attribute;

	return root;
}
