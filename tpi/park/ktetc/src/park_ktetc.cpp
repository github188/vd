#include <errno.h>
#include <sstream>
#include <json/json.h>

#include "commonfuncs.h"
#include "logger/log.h"

#include "vd_msg_queue.h"
#include "park_ktetc.h"
#include "park_ktetc_db_api.h"
#include "spsystem_api.h"
#include "park_file_handler.h"
#include "refhandle.h"

#include "mycurl.h"
#include "http_common.h"
#include <curl/curl.h>

using namespace std;

static string itos(int value)
{
    stringstream sstr;
    sstr << value;
    if (sstr.fail())
        ERROR("[Error:] Format number to string error!");

    return sstr.str();
}

static int send_record2platform_c(const char* url, void* body,
        const size_t lens, const char* space_code)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
    curl_args.curl_method = CURL_METHOD_POST;
    curl_args.url = (char*)url;
    curl_args.post_data = (char*)body;
    curl_args.post_lens = lens;
    return curl_http_post(&curl_args, 2, (void*)space_code);
}

static int send_info2platform_c(const char* url, const char* body,
        const size_t lens, const char* space_code)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
    curl_args.curl_method = CURL_METHOD_POST;
    curl_args.url = (char*)url;
    curl_args.post_data = (char*)body;
    curl_args.post_lens = lens;
    return curl_http_post(&curl_args, 1, (void*)space_code);
}

template <typename TYPE, void (TYPE::*run_thread)()>
void * start_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->run_thread();
    return NULL;
}

template <typename TYPE, void (TYPE::*hb_thread)()>
void * park_ktetc_hb_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->hb_thread();
    return NULL;
}

template <typename TYPE, void (TYPE::*hb_thread)()>
void * park_ktetc_retrans_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->retrans_thread();
    return NULL;
}

CParkKtetc::CParkKtetc(void)
{
	this->m_platform_type = EPlatformType::PARK_KTETC;

	create_multi_dir("/mnt/mmc/ktetc/");
	park_ktetc_db_open(KTETC_DB_PATH);
	park_ktetc_db_create_tb(); /* if not exist */
}

CParkKtetc::~CParkKtetc(void)
{
	park_ktetc_db_close();
}

void CParkKtetc::run_thread(void)
{
    set_thread("ktetc");

    pthread_create(&m_hb_tid, NULL,
            park_ktetc_hb_thread<CParkKtetc, &CParkKtetc::hb_thread>, this);
    pthread_create(&m_retrans_tid, NULL,
            park_ktetc_retrans_thread<CParkKtetc, &CParkKtetc::retrans_thread>, this);

	// main loop
	str_park_msg park_msg;
	size_t msg_size = sizeof(str_park_msg) - sizeof(long);

    while (true) {
        memset(&park_msg, 0x00, sizeof(str_park_msg));
        msgrcv(MSG_PARK_ID, &park_msg, msg_size, MSG_PARK_KTETC_TYPE, 0);

        DEBUG("ktetc receive message type = %d.", park_msg.data.ktetc.type);

		const int msg_type = park_msg.data.ktetc.type;
		const void *msg_data = park_msg.data.ktetc.data;

        switch(msg_type) {
        case MSG_PARK_ALARM:
            alarm_handler((ParkAlarmInfo*)msg_data);
			break;
		case MSG_PARK_RECORD:
            record_handler((struct Park_record*)msg_data);
			break;
        case MSG_PARK_INFO:
            info_handler((struct Park_record*)msg_data);
            break;
		default:
			break;
		}
    }
    return;
}

int CParkKtetc::run(void)
{
    pthread_create(&tid, NULL,
            start_thread<CParkKtetc, &CParkKtetc::run_thread>, this);
	return 0;
}

int CParkKtetc::stop(void)
{
    pthread_cancel(tid);
    pthread_cancel(m_hb_tid);
	return 0;
}

int CParkKtetc::subscribe(void)
{
	this->m_platform_info.enable = true;
	return 0;
}

int CParkKtetc::unsubscribe(void)
{
	this->m_platform_info.enable = false;
	return 0;
}

bool CParkKtetc::subscribed(void)
{
	return (this->m_platform_info.enable);
}

int CParkKtetc::notify(int type, void* data)
{
	str_park_msg park_msg = {0};

	switch(type) {
		case MSG_PARK_ALARM: {
			ParkAlarmInfo *parkAlarmInfo = (ParkAlarmInfo*)data;
            if (NULL == parkAlarmInfo) {
                ERROR("NULL == parkAlarmInfo");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.ktetc.type = MSG_PARK_ALARM;
            park_msg.data.ktetc.data = parkAlarmInfo;
            park_msg.data.ktetc.lens = sizeof(ParkAlarmInfo);
            break;
		}
		case MSG_PARK_INFO:
		{
			struct Park_record *park_record = (struct Park_record *)data;
            if (NULL == park_record) {
                ERROR("NULL == park_record");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.ktetc.type = MSG_PARK_INFO;
            park_msg.data.ktetc.data = park_record;
            park_msg.data.ktetc.lens = sizeof(struct Park_record);
			break;
		}
		case MSG_PARK_RECORD:
		{
			struct Park_record *park_record = (struct Park_record *)data;
            if (NULL == park_record) {
                ERROR("NULL == park_record");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.ktetc.type = MSG_PARK_RECORD;
            park_msg.data.ktetc.data = park_record;
            park_msg.data.ktetc.lens = sizeof(struct Park_record);
			break;
		}
		default:
		{
			DEBUG("KTETC no match handler for %d", type);
			return -1;
		}

	}
	vmq_sendto_ktetc(park_msg);
	return 0;
}

int CParkKtetc::alarm_handler(ParkAlarmInfo* alarm)
{
    if (NULL == alarm) {
        ERROR("alarm is null.");
		return -1;
    }

	ktetc_alarm_t ktetc_alarm;
	bitcom2ktetc_alarm(alarm, &ktetc_alarm);
	save_alarm2database(&ktetc_alarm);
	send_alarm2platform(&ktetc_alarm);

    _RefHandleRelease((void**)&alarm, NULL, NULL);
    return 0;
}

int CParkKtetc::get_record_time(char *record_time)
{
	if (NULL == record_time) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

    timeval tv = {0};
    struct tm *timenow = NULL;

    gettimeofday(&tv, NULL);
    timenow = localtime(&tv.tv_sec);

	snprintf(record_time, KTETC_TIME_LENGTH, "%04d%02d%02d%02d%02d%02d",
			timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday,
			timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
	return 0;
}

int CParkKtetc::time_convert(unsigned int from, char *to)
{
	if (NULL == to) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

    struct tm t;
    localtime_r((const time_t*)&(from), &t);

    sprintf(to, "%04d%02d%02d%02d%02d%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	return 0;
}

int CParkKtetc::time_convert(const char *from, char *to)
{
	if ((NULL == from) || (NULL == to)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	struct tm t = {0};
	errno = 0;
	sscanf(from, "%4d-%2d-%2d %2d:%2d:%2d", &(t.tm_year), &(t.tm_mon),
			&(t.tm_mday), &(t.tm_hour), &(t.tm_min), &(t.tm_sec));

	if (errno != 0) {
		ERROR("sscanf %s", strerror(errno));
		return -errno;
	}

	errno = 0;
	snprintf(to, KTETC_TIME_LENGTH, "%04d%02d%02d%02d%02d%02d",
			t.tm_year, t.tm_mon,t.tm_mday,
			t.tm_hour, t.tm_min, t.tm_sec);

	if (errno != 0) {
		ERROR("sprintf %s", strerror(errno));
		return -errno;
	}

	return 0;
}

int CParkKtetc::construct_flowId(const char *time, char *out)
{
	if ((NULL == out) || (NULL == time)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	errno = 0;
	snprintf(out, KTETC_FLOWID_LENGTH, "%s%d%s%s",
			m_platform_info.comType.c_str(), KTETC_VIDEO, time, "001");

	if (errno != 0) {
		ERROR("sprintf %s", strerror(errno));
		return -errno;
	}

	return 0;
}

int CParkKtetc::construct_inTime(const struct Park_record *r, char *inTime)
{
	if ((NULL == r) || (NULL == inTime)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	const DB_ParkRecord *record = &(r->db_park_record);

	/* inTime only used for driveout. */
	if (record->objectState == KTETC_DRIVE_IN) {
		strcat(inTime, "0");
		return 0;
	}

	char lc_values[5][128] = {{0}};
	spsystem_select_lastmsg_table(spsystem_db, lc_values);

	char *in_plate_num = lc_values[1];
	//char *in_status = lc_values[2];
	char *in_objectState = lc_values[3];
	char *in_time = lc_values[4];

	if (strcmp(record->plate_num, in_plate_num) != 0) {
		strcat(inTime, "0");
		return 0;
	}

	if (atoi(in_objectState) != KTETC_DRIVE_IN) {
		strcat(inTime, "0");
		return 0;
	}

	time_convert(in_time, inTime);
	return 0;
}

/**
 * @brief bitcom2ktetc
 *
 * @param record Park_record
 * @param ktetc ktetc_t
 *
 * @return -EINVAL 0
 */
int CParkKtetc::bitcom2ktetc(const struct Park_record *record, ktetc_t *ktetc)
{
	if ((NULL == record) || (NULL == ktetc)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

    char record_time[KTETC_TIME_LENGTH] = {0};
	char time[KTETC_TIME_LENGTH] = {0};
	char inTime[KTETC_TIME_LENGTH] = {0};
	char flowId[KTETC_FLOWID_LENGTH] = {0};
    char plateFeature[128] = {0};

    sprintf(plateFeature, "%d/%d/%d/%d/%d/%d/%d/%d",
                          record->db_park_record.coordinate_x[0],
                          record->db_park_record.coordinate_y[0],
                          record->db_park_record.width[0],
                          record->db_park_record.height[0],
                          record->db_park_record.coordinate_x[1],
                          record->db_park_record.coordinate_y[1],
                          record->db_park_record.width[1],
                          record->db_park_record.height[1]);

	get_record_time(record_time);

	time_convert(record->db_park_record.time, time);

	construct_flowId(record_time, flowId);
	construct_inTime(record, inTime);

	char plate_num[32] = {0};
    convert_enc("GBK", "UTF-8", record->db_park_record.plate_num,
			strlen(record->db_park_record.plate_num), plate_num, 32);

    char plate_color[3] = {0};
    snprintf(plate_color, 3, "%02d", record->db_park_record.plate_color);

    //char vehColor[2] = {0};
    //snprintf(vehColor, 2, "%c", record->db_park_record.color);

	ktetc->comType = m_platform_info.comType;
	ktetc->IDdataTime = string(record_time);
	ktetc->flowId = string(flowId);
	ktetc->parkCode = m_platform_info.parkCode;
	ktetc->devCode = m_platform_info.devCode;
	ktetc->psCode = m_platform_info.psCode;
	ktetc->inOutState = record->db_park_record.objectState;
	ktetc->vehPlate = string(plate_num);
	ktetc->confidence = record->db_park_record.confidence[0];
	ktetc->plateColor = string(plate_color);
	//ktetc->vehColor = string(vehColor);
	ktetc->vehColor = "0";
	ktetc->vehType = record->db_park_record.vehicle_type;
	ktetc->Image1 = string(flowId) + "/" + "1.jpg";
	ktetc->Image2 = string(flowId) + "/" + "2.jpg";
	ktetc->Image3 = "0";
	ktetc->Image4 = "0";
	ktetc->FiledataTime = string(time);
	ktetc->inTime = string(inTime);

	ktetc->plateFeature = string(plateFeature);
	ktetc->pic_info = record->pic_info;

    // insert into the park system database
    char lc_values[12][128] = {{0}};

    sprintf(lc_values[1], "%s", (record->db_park_record).plate_num);
    sprintf(lc_values[2], "%d", (record->db_park_record).status);
    sprintf(lc_values[3], "%d", (record->db_park_record).objectState);
    sprintf(lc_values[4], "%s", (record->db_park_record).time);
    spsystem_delete_lastmsg_table(spsystem_db);
    spsystem_insert_lastmsg_table(spsystem_db, lc_values);
	return 0;
}

/**
 * @brief bitcom2ktetc
 *
 * @param record Park_record
 * @param ktetc ktetc_t
 *
 * @return -EINVAL 0
 */
int CParkKtetc::bitcom2ktetc_alarm(const ParkAlarmInfo *alarm,
		ktetc_alarm_t *ktetc_alarm)
{
	if ((NULL == alarm) || (NULL == ktetc_alarm)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

    char alarm_time[KTETC_TIME_LENGTH] = {0};
	char flowId[KTETC_FLOWID_LENGTH] = {0};

	time_convert(alarm->alarmTime, alarm_time);

	construct_flowId(alarm_time, flowId);

	ktetc_alarm->comType = m_platform_info.comType;
	ktetc_alarm->flowId = string(flowId);
	ktetc_alarm->parkCode = m_platform_info.parkCode;
	ktetc_alarm->devCode = m_platform_info.devCode;
	ktetc_alarm->psCode = m_platform_info.psCode;
	ktetc_alarm->alarmCode = alarm->category;
	ktetc_alarm->alarmTime = string(alarm_time);
	ktetc_alarm->alarmLevel = alarm->level;
	return 0;
}

int CParkKtetc::save_record2database(const ktetc_t *k)
{
	if (NULL == k) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	park_ktetc_db_insert_record(k);
	return 0;
}

int CParkKtetc::save_alarm2database(const ktetc_alarm_t *k)
{
	if (NULL == k) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	park_ktetc_db_insert_alarm(k);
	return 0;
}

int	CParkKtetc::save_record_pictures(const ktetc_t *k, const struct Park_record *r)
{
	if ((NULL == k) || (NULL == r)) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	for (int i = 0; i < 2; i++) {
		string pic_folder = string(KTETC_ABSOLUTE_PATH) + k->flowId + "/";
		create_multi_dir(pic_folder.c_str());
		string pic_name = pic_folder + itos(i + 1) + ".jpg";

		if ((r->pic_info[i].buf != NULL) && (r->pic_info[i].size != 0)) {
			park_save_picture(pic_name.c_str(),
					(unsigned char*)(r->pic_info[i].buf), r->pic_info[i].size);
		}
	}
	return 0;
}

int CParkKtetc::send_record2platform(const ktetc_t *k)
{
	if (NULL == k) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

    Json::Value root;
	Json::FastWriter fast_writer;

	if (k->inOutState == 1) // in
        root["cmdCode"] = 3;
    else if (k->inOutState == 0) //out
        root["cmdCode"] = 4;

	root["comType"] = k->comType;
	root["flowId"] = k->flowId;
	root["parkCode"] = k->parkCode;
	root["devCode"] = k->devCode;
	root["psCode"] = k->psCode;
	root["inOutState"] = k->inOutState;
	root["vehPlate"] = k->vehPlate;
	root["confidence"] = k->confidence;
	root["plateColor"] = k->plateColor;
	root["vehColor"] = k->vehColor;
	root["vehType"] = k->vehType;
	root["plateFeature"] = k->plateFeature;
	root["Image1"] = k->Image1;
	root["Image2"] = k->Image2;
	root["Image3"] = k->Image3;
	root["Image4"] = k->Image4;
	root["dataTime"] = k->FiledataTime;
	root["inTime"] = k->inTime;

    std::string result_json = fast_writer.write(root);

    body_data_t body_data = {0};
    body_data.json_data = result_json.c_str();
    body_data.json_lens = result_json.size();
    body_data.pic = k->pic_info;

	string url = "";
	if (k->inOutState == 1) // in
        url = m_platform_info.url.c_str() + string("/api/v1/park/driveIn");
    else if (k->inOutState == 0) //out
        url = m_platform_info.url.c_str() + string("/api/v1/park/driveOut");

	if (park_ktetc_db_count_retrans() > 0) {
        INFO("More msg in retrans db, save this.");
        park_ktetc_db_insert_retrans(result_json.c_str());
		return 0;
	}

	int ret = 0;
	for (auto i = 0; i < 3; i++) {
		ret = send_record2platform_c(url.c_str(),
			(void*)&body_data, sizeof(body_data_t), k->psCode.c_str());
		if (ret == 0) {
			break;
		}
	}

	if (ret == -1) {
        INFO("Send record failed. save into database.");
        park_ktetc_db_insert_retrans(result_json.c_str());
	}
	return 0;
}

int CParkKtetc::send_alarm2platform(const ktetc_alarm_t *k)
{
	if (NULL == k) {
		ERROR("Invalid argument");
		return -EINVAL;
	}

	Json::Value root;
	Json::FastWriter fast_writer;

	root["comType"] = k->comType;
	root["flowId"] = k->flowId;
	root["parkCode"] = k->parkCode;
	root["devCode"] = k->devCode;
	root["psCode"] = k->psCode;
	root["alarmCode"] = k->alarmCode;
	root["alarmTime"] = k->alarmTime;
	root["alarmLevel"] = k->alarmLevel;

    std::string result_json = fast_writer.write(root);

    char url[256] = "";
    sprintf(url, "%s%s", m_platform_info.url.c_str(), "/api/v1/park/alarm");

    int ret = 0;
	for (auto i = 0; i < 3; i++) {
		ret = send_info2platform_c(url, result_json.c_str(), result_json.size(),
			k->psCode.c_str());
		if (ret == 0) {
			break;
		}
	}

	return 0;
}

int CParkKtetc::record_handler(const struct Park_record* record)
{
	int ret = 0;
    if (NULL == record) {
        ERROR("NULL is data!");
		return -1;
    }

	ktetc_t ktetc;
    const DB_ParkRecord *db_park_record = &(record->db_park_record);
    if (NULL == db_park_record) {
        ERROR("NULL is db_park_record!");
		ret = -1;
		goto release;
    }

	ret = bitcom2ktetc(record, &ktetc);
	if (ret != 0) {
		ret = -1;
		goto release;
	}

	save_record2database(&ktetc);
	save_record_pictures(&ktetc, record);
	send_record2platform(&ktetc);

release:
    _RefHandleRelease((void**)&record, NULL, NULL);
    return ret;
}

int CParkKtetc::info_handler(struct Park_record* record)
{
	record_handler(record);
    return 0;
}

int CParkKtetc::set_platform_info(void *p)
{
	memcpy(&m_platform_info, p, sizeof(ktetc_platform_info_t));
	return 0;
}

int CParkKtetc::set_platform_info(const Json::Value & root)
{
	if (root["platform_type"].asString() != "ktetc")
	{
		ERROR("json configuration %s is not for ktetc platform.",
				root["platform_type"].asString().c_str());
		return -1;
	}

	m_platform_info.enable = root["enable"].asString() == "yes" ? true : false;
	m_platform_info.comType = root["attribute"]["comType"].asString();
	m_platform_info.parkCode = root["attribute"]["parkCode"].asString();
	m_platform_info.devCode = root["attribute"]["devCode"].asString();
	m_platform_info.psCode = root["attribute"]["psCode"].asString();
	m_platform_info.url = root["attribute"]["url"].asString();
	return 0;
}

int CParkKtetc::set_platform_info(const string & json_str)
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

void * CParkKtetc::get_platform_info(void)
{
	return &(this->m_platform_info);
}

// return the json string of the configuration.
string CParkKtetc::get_platform_info_str(void)
{
	Json::Value root = get_platform_info_json();
	Json::FastWriter writer;

	return writer.write(root);
}

// return the json string of the configuration.
Json::Value CParkKtetc::get_platform_info_json(void)
{
	Json::Value root;
	Json::Value attribute;

	ktetc_platform_info_t *platform_info =
		(ktetc_platform_info_t*)get_platform_info();

	root["platform_type"] = "ktetc";
	root["enable"] = platform_info->enable ? "yes" : "no";
	attribute["comType"] = platform_info->comType;
	attribute["parkCode"] = platform_info->parkCode;
	attribute["devCode"] = platform_info->devCode;
	attribute["psCode"] = platform_info->psCode;
	attribute["url"] = platform_info->url;
	root["attribute"] = attribute;

	return root;
}

void CParkKtetc::ktetc_register()
{
	Json::FastWriter fast_writer;
	Json::Value root;

	root["comType"] = m_platform_info.comType;
	root["parkCode"] = m_platform_info.parkCode;
	root["devCode"] = m_platform_info.devCode;
	root["psCode"] = m_platform_info.psCode;

	std::string json = fast_writer.write(root);
    INFO("register ktetc platform. %s", json.c_str());

    auto url = m_platform_info.url +  "/api/v1/park/register";
    while (send_info2platform_c(url.c_str(), json.c_str(),
                json.size(), m_platform_info.psCode.c_str())) {
        ERROR("register park ktetc platform failed.");
        sleep(60);
    }

    //set_http_online(true);
}

void CParkKtetc::hb_thread(void)
{
    set_thread("ktetc_hb");
    ktetc_register(); /* maybe blocked until register successful. */

	Json::Value root;
	Json::FastWriter fast_writer;

	root["comType"] = m_platform_info.comType;
	root["parkCode"] = m_platform_info.parkCode;
	root["devCode"] = m_platform_info.devCode;
	root["psCode"] = m_platform_info.psCode;

	std::string json = fast_writer.write(root);
    auto url = m_platform_info.url +  "/api/v1/park/heartBeat";

	while (true) {
		INFO("send ktetc heartbeat %s", json.c_str());
		send_info2platform_c(url.c_str(), json.c_str(),
				json.size(), m_platform_info.psCode.c_str());
		usleep(60 * 1000 * 1000);
	}
    return;
}

/*
 * Name       : retrans_pthread
 * Discription: The hb thread for ktetc post platform.
 */
void CParkKtetc::retrans_thread(void)
{
    set_thread("ktetc_retrans");

    char* msg = (char*)malloc(sizeof(char) * 1024);
    if (NULL == msg) {
        ERROR("alloc retrans msg failed.");
        return;
    }

    size_t pic_buffer_size = sizeof(unsigned char) * 1024 * 500;
    unsigned char *pic_buf = (unsigned char*)malloc(pic_buffer_size * 2);
    if (NULL == pic_buf) {
        ERROR("malloc pic_buf failed.");
        free(msg);
        return;
    }

    EP_PicInfo picinfo[2];
    memset(picinfo, 0x00, sizeof(EP_PicInfo) * 2);

    picinfo[0].buf = (void*)pic_buf;
    picinfo[1].buf = (void*)(pic_buf + pic_buffer_size);

    while (true) {
        usleep(1000 * 1000 * 10);
#if 0
        if (!http_is_online()) {
            continue;
        }
#endif

        if (!retransmition_enable())
            continue;

        int msg_num = 0;
        if ((msg_num = park_ktetc_db_count_retrans()) <= 0)
            continue;

        memset(msg, 0x00, sizeof(char) * 1024);
        int msg_id = park_ktetc_db_select_retrans(msg);
        INFO("ktetc retrans msgid = %d", msg_id);

        if (msg_id < 0)
            continue;

        /* get the picture name from msg */
        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(msg, root))
        {
            ERROR("Parse json failed. msg = %s", msg);
            return;
        }

        body_data_t body_data = {0};
        body_data.json_data = msg;
        body_data.json_lens = strlen(msg);

        for (auto i = 0; i < 2; i++) {
            string pic_name = "";

            if (i == 0)
                pic_name = root["Image1"].asString();
            else if (i == 1)
                pic_name = root["Image2"].asString();

            size_t pic_len = 0;

            if (pic_name != "") {
                string picname = string(KTETC_ABSOLUTE_PATH) + pic_name;
                pic_len = park_read_picture(picname.c_str(), (unsigned char*)(picinfo[i].buf), 0);
                if (pic_len < 0) {
                    pic_name = "";
                }
            }

            memset(picinfo[i].name, 0x00, sizeof(picinfo[i].name));
            memcpy(picinfo[i].name, pic_name.c_str(), pic_name.size());
            picinfo[i].size = pic_len;
        }

        body_data.pic = picinfo;

        char url[256] = "";
        if (root["inOutState"].asInt() == 1) // in
            sprintf(url, "%s%s", m_platform_info.url.c_str(), "/api/v1/park/driveIn");
        else if (root["inOutState"].asInt() == 0) //out
            sprintf(url, "%s%s", m_platform_info.url.c_str(), "/api/v1/park/driveOut");

		if (send_record2platform_c(url, &body_data, sizeof(body_data_t),
					m_platform_info.psCode.c_str()) == 0) {

            park_ktetc_db_delete_retrans(msg_id);
		}
    }

    free(msg);
    msg = NULL;
    free(pic_buf);
    pic_buf = NULL;
    return;
}
