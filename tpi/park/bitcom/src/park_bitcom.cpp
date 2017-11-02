#include <cstdio>
#include <cstdlib>
#include <json/json.h>
#include <string.h>

#include "vd_msg_queue.h"
#include "logger/log.h"
#include "tpbitcom_pthread.h"
#include "tpbitcom_records.h"
#include "park_records_process.h"
#include "park_bitcom.h"
#include "refhandle.h"

CParkBitcom::CParkBitcom(void)
{
	this->m_platform_type = EPlatformType::PARK_BITCOM;
}

int CParkBitcom::run(void)
{
	static bool first_time = true;
	if(first_time)
	{
		pthread_t bitcom;
		if(pthread_create(&bitcom, NULL, tpbitcom_pthread, NULL))
			ERROR("Create tpi tpbitcom_pthread failed!");
		this->m_running = true;
		first_time = false;
	}
	return 0;
}

int CParkBitcom::stop(void)
{
	// For bitcom platform, no more configuration changed.
	// simply keep the threads and just set the flag.
	this->m_running = false;

	return 0;
}

int CParkBitcom::subscribe(void)
{
	this->m_platform_info.enable = true;
	return 0;
}

int CParkBitcom::unsubscribe(void)
{
	this->m_platform_info.enable = false;
	return 0;
}

bool CParkBitcom::subscribed(void)
{
	return (this->m_platform_info.enable && this->m_running);
}

int CParkBitcom::notify(int type, void *data)
{
	str_park_msg lstr_park_msg = {0};
	switch(type)
	{
#if 0 // there is no implement of alarm for bitcom platform now.
		case MSG_PARK_ALARM:
		{
			break;
		}
#endif
		case MSG_PARK_INFO:
		{
            struct Park_record *record = (struct Park_record*)data;
            if (NULL == record) {
                return -1;
            }

            _RefHandleRetain(data);

			gstr_tpbitcom_records.pic_flag = 0;
            memcpy(&gstr_tpbitcom_records.field, &(record->db_park_record),
					                     sizeof(DB_ParkRecord));
            lstr_park_msg.data.bitcom.type = MSG_PARK_INFO;
			break;
		}
#if 0
		case MSG_PARK_STATUS:
		{
			break;
		}
#endif
		case MSG_PARK_RECORD:
		{
			struct Park_record *park_record = (struct Park_record *)data;
            if (NULL == park_record) {
                ERROR("NULL is park_record.");
                return -1;
            }

            _RefHandleRetain(data);

            memcpy(&gstr_tpbitcom_records.field,
					&(park_record->db_park_record), sizeof(DB_ParkRecord));

			for(int i = 0; i < PARK_PIC_NUM; i++)
			{
				gstr_tpbitcom_records.picture_size[i] = park_record->pic_info[i].size;
				memcpy(gstr_tpbitcom_records.picture[i],
						park_record->pic_info[i].buf, gstr_tpbitcom_records.picture_size[i]);
				memcpy(&gstr_tpbitcom_records.picture_info[i],
						&(park_record->pic_info[i]), sizeof(EP_PicInfo));
			}
			gstr_tpbitcom_records.pic_flag = 1;
            lstr_park_msg.data.bitcom.type = MSG_PARK_RECORD;
			break;
		}
		default:
		{
			DEBUG("BITCOM no match handler for %d", type);
			return -1;
		}
	}
	vmq_sendto_tpbitcom(lstr_park_msg);

    /**
     * For bitcom platform, now copy all the message into gstr_tpbitcom_records,
     * it will no need data, so release it directly.
     */
    _RefHandleRelease((void**)&data, NULL, NULL);
	return 0;
}

int CParkBitcom::set_platform_info(void *p)
{
	memcpy(&m_platform_info, p, sizeof(bitcom_platform_info_t));
	return 0;
}

int CParkBitcom::set_platform_info(const Json::Value & root)
{
	if (root["platform_type"].asString() != "bitcom")
	{
		ERROR("json configuration %s is not for bitcom platform.",
				root["platform_type"].asString().c_str());
		return -1;
	}

	bool new_state = root["enable"].asString() == "yes" ? true : false;

	m_platform_info.enable = new_state;
    m_platform_info.retransmition_enable = root["attribute"]["retransmition_enable"].asString() == "yes" ? true : false;
    m_platform_info.rotate_count = atoi(root["attribute"]["rotate_count"].asString().c_str());

	return 0;
}

int CParkBitcom::set_platform_info(const string & json_str)
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

void * CParkBitcom::get_platform_info(void)
{
	return &(this->m_platform_info);
}

// return the json string of the configuration.
string CParkBitcom::get_platform_info_str(void)
{
	Json::Value root = get_platform_info_json();
	Json::FastWriter writer;
	return writer.write(root);
}

// return the json string of the configuration.
Json::Value CParkBitcom::get_platform_info_json(void)
{
	Json::Value root;
	Json::Value attribute;

	bitcom_platform_info_t *platform_info =
		(bitcom_platform_info_t*)get_platform_info();

	char rotate_count[4] = {0};
	sprintf(rotate_count, "%d", platform_info->rotate_count);

	root["platform_type"] = "bitcom";
	root["enable"] = platform_info->enable ? "yes" : "no";

	attribute["retransmition_enable"] = platform_info->retransmition_enable ? "yes" : "no";
	attribute["rotate_count"] = rotate_count;
	root["attribute"] = attribute;

	return root;
}
