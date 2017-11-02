#include "logger/log.h"
#include "ftp.h"
#include "data_process.h"
#include "park_records_process.h"
#include "park_util.h"
#include "capture_picture.h"
#include "songli_descending.h"
#include "json/json.h"
#include <sstream>
#include "park_songli.h"
#include "light_ctl_api.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "sem_util.h"
#ifdef __cplusplus
}
#endif

#define PARK_CMD_CODE_LIGHT_CONTROL (0x0B)
#define PARK_CMD_CODE_RESERVE       (0x0C)
#define PARK_CMD_CODE_PIC_CAPTURE   (0x0D)

#define PARK_RESERVE_ACTION_RESERVE (0x01)
#define PARK_RESERVE_ACTION_CANCEL  (0x02)
#define PARK_RESERVE_ACTION_ARRIVE  (0x03)

template <typename T>
static const string num2str(const T t)
{
	stringstream ss;
	string s;

	ss << t;
	ss >> s;
	return s;
}

static int deal_msg_park_down_light_control(string& light_action);
static int deal_msg_park_down_reserve(const int schedule_action);
static int deal_msg_park_down_pic_capture(void);

static int construct_reserve_response(const int schedule_action,
		                              const int schedule_state,
									  string &resp);
static int construct_pic_capture_response(const string& url, string& resp);
static int send_picture_2_songli_platform(const unsigned char* pic_data,
		                                  const unsigned int pic_size);
static int send_msg_2_songli_platform(const char* msg);

/*
 * Name:        recv_msg_park_down
 * Description: the callback handler for class_mq_consumer
 *              the portal fun to handler MQ received messages.
 * Paramaters:  const Message *raw_message: Message
 * Return:      void
 */
void recv_msg_park_down(const Message *raw_message)
{
	DEBUG("Receive message from songli.");
	if (NULL == raw_message) {
		ERROR("NULL == raw_message");
		return;
	}

	extern char g_SerialNum[50];
	/* const char* dev_id_remote = read_root["devId"].asString().c_str(); */
	/* const char* dev_id_local = g_set_net_param.m_NetParam.m_DeviceID; */
	const char* dev_id_remote = raw_message->getCMSCorrelationID().c_str();
	const char* dev_id_local = g_SerialNum;
	if ((NULL == dev_id_remote) || (NULL == dev_id_local)) {
		ERROR("NULL == dev_id_remote or NULL == dev_id_local");
		return;
	}

	if (strcmp(dev_id_local, dev_id_remote) != 0) {
		DEBUG("收到MQ消息非本设备!,本设备id = %s, MQ_id = %s",
				dev_id_local, dev_id_remote);
		return;
	}

	const TextMessage* msg = dynamic_cast<const TextMessage*> (raw_message);
	if (NULL == msg) {
		ERROR("NULL == msg");
		return;
	}

	string message = msg->getText();
	INFO("recv PARK DOWN MESSAGE %s", message.c_str());

	Json::Value read_root;
	Json::Reader reader;

	if(!reader.parse(message, read_root)) {
		ERROR("Parse received message failed.");
		return;
	}

	if(PARK_CMD_CODE_LIGHT_CONTROL == read_root["cmdCode"].asInt()) {
		string light_action = read_root["data"]["light"].asString();
		if(!light_action.empty()) {
			deal_msg_park_down_light_control(light_action);
		}
	} else if (PARK_CMD_CODE_RESERVE == read_root["cmdCode"].asInt()) {
		string schedule_str = read_root["data"]["scheduleAction"].asString();
		if (!schedule_str.empty()) {
			int schedule_action = atoi(schedule_str.c_str());
			deal_msg_park_down_reserve(schedule_action);
		}
	} else if (PARK_CMD_CODE_PIC_CAPTURE == read_root["cmdCode"].asInt()) {
		deal_msg_park_down_pic_capture();
	} else {
		ERROR("cmdCode parse failed.");
	}
}

static int deal_msg_park_down_light_control(string& light_action)
{
	/*
	 * {
	 * “light”: “r”,
	 * “stateTime”: “2014-11-11 08:20:10”
	 * }
	 */
	INFO("deal_msg_park_down_light_control");
	Lighting_park lightInfo = {0};

	// r    停车状态       红色
	// g    空车位状态     绿色
	// b1   不规范停车状态 蓝色
	// b2   泊位占用状态   蓝色
	// rf   正在驶离状态   红闪
	// gf   正在驶入状态   绿闪
	// bf1  遮挡相机视频   蓝闪
	// bf2  遮挡车牌       蓝闪
	// c    泊位已缴费     青色

	if (light_action == "r") {
		lightInfo.red = 255;
	} else if (light_action == "g") {
		lightInfo.green = 255;
	} else if (light_action == "b1") {
		lightInfo.blue = 255;
	} else if (light_action == "b2") {
		lightInfo.blue = 255;
	} else if (light_action == "rf") {
		lightInfo.red = 255;
		lightInfo.red_interval = 5;
	} else if (light_action == "gf") {
		lightInfo.green = 255;
		lightInfo.green_interval = 5;
	} else if (light_action == "bf1") {
		lightInfo.blue = 255;
		lightInfo.blue_interval = 5;
	} else if (light_action == "bf2") {
		lightInfo.blue = 255;
		lightInfo.blue_interval = 5;
	} else if (light_action == "c") {
		lightInfo.green = 0;
		lightInfo.blue = 0;
		lightInfo.green = 255;
		lightInfo.blue = 255;
	}

	light_ctl(LIGHT_ACTION_START, LIGHT_PRIO_PLATFORM,
            lightInfo.red, lightInfo.red_interval,
            lightInfo.green, lightInfo.green_interval,
            lightInfo.blue, lightInfo.blue_interval,
            lightInfo.white);
	return 0;
}

static int deal_msg_park_down_reserve(const int schedule_action)
{
	int ret = 0;

	switch (schedule_action) {
	case PARK_RESERVE_ACTION_RESERVE:
		ret = park_lock_lock();
		break;
	/* both Cancel and arrive unlock the parking lock. */
	case PARK_RESERVE_ACTION_CANCEL:
	case PARK_RESERVE_ACTION_ARRIVE:
		ret = park_lock_unlock();
		break;
	default:
		INFO("SONGLI ERR scheduleState = %d", schedule_action);
		// response to platform
		ret = -1;
		break;
	}

	string resp = "";
	ret = construct_reserve_response(schedule_action, ret, resp);
	return send_msg_2_songli_platform(resp.c_str());
}

static int deal_msg_park_down_pic_capture(void)
{
	int ret = 0;
	data_transfer_cb cb_handler = send_picture_2_songli_platform;
	ret = do_cap_pic_upload(cb_handler);
	return ret;
}

/*
 * Name:        send_picture_2_songli_platform
 * Description: the callback function invoked at deal_msg_park_down_pic_capture
 *				& do_cap_pic_upload used to send capture picture to songli
 *				platform.
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
static int send_picture_2_songli_platform(const unsigned char* pic_data,
		                                  const unsigned int pic_size)
{
	if ((NULL == pic_data) || (pic_size <= 0)) {
		ERROR("bad paramater.");
		return -1;
	}

	int ret = 0;

#if 1 /* hardcode code, may be provisioned from configuration. */
	FTP_URL_Level snap_ftp_url_level;
	snap_ftp_url_level.levelNum = 3;
	snap_ftp_url_level.urlLevel[0] = YEAR_MONTH;
	snap_ftp_url_level.urlLevel[1] = DAY;
	snap_ftp_url_level.urlLevel[2] = EVENT_NAME;
#endif

	EP_PicInfo pic_info;
	memset(&pic_info, 0, sizeof(EP_PicInfo));
	pic_info.buf = (void*)pic_data;
	pic_info.size = pic_size;

    struct timeval tv = {0};
    struct tm *timenow = NULL;

    gettimeofday(&tv, NULL);
    timenow = localtime(&tv.tv_sec);

    pic_info.tm.year    = timenow->tm_year + 1900;
    pic_info.tm.month   = timenow->tm_mon + 1;
    pic_info.tm.day     = timenow->tm_mday;
    pic_info.tm.hour    = timenow->tm_hour;
    pic_info.tm.minute  = timenow->tm_min;
    pic_info.tm.second  = timenow->tm_sec;
    pic_info.tm.msecond = tv.tv_usec / 1000;

	for (int j = 0; j < snap_ftp_url_level.levelNum; j++)
	{
		switch (snap_ftp_url_level.urlLevel[j])
		{
		case SPORT_ID: 		//地点编号
			sprintf(pic_info.path[j], "%s", EP_POINT_ID);
			sprintf(pic_info.alipath[j], "%s", EP_POINT_ID);
			break;
		case DEV_ID: 		//设备编号
			sprintf(pic_info.path[j], "%s", EP_DEV_ID);
			sprintf(pic_info.alipath[j], "%s", EP_DEV_ID);
			break;
		case YEAR_MONTH: 	//年/月
			sprintf(pic_info.path[j], "%04d%02d",
					pic_info.tm.year,pic_info.tm.month);
			sprintf(pic_info.alipath[j], "%04d%02d",
					pic_info.tm.year,pic_info.tm.month);
			break;
		case DAY: 			//日
			sprintf(pic_info.path[j], "%02d",pic_info.tm.day);
			sprintf(pic_info.alipath[j], "%02d",pic_info.tm.day);
			break;
		case EVENT_NAME: 	//事件类型
			sprintf(pic_info.path[j], "%s", "snap");
			sprintf(pic_info.alipath[j], "%s", "snap");
			break;
		case HOUR: 			//时
			sprintf(pic_info.path[j], "%02d",	pic_info.tm.hour);
			sprintf(pic_info.alipath[j], "%02d",	pic_info.tm.hour);
			break;
		case FACTORY_NAME: 	//厂商名称
			sprintf(pic_info.path[j], "%s", EP_MANUFACTURER);
			sprintf(pic_info.alipath[j], "%s", EP_MANUFACTURER);
			break;
		default:
			break;
		}
	}

	//文件名以泊位号+时间命名
	snprintf(pic_info.name, sizeof(pic_info.name),
			"%s%04d%02d%02d%02d%02d%02d%03d.jpg",
			g_arm_config.basic_param.spot_id,
			pic_info.tm.year,
			pic_info.tm.month,
			pic_info.tm.day,
			pic_info.tm.hour,
			pic_info.tm.minute,
			pic_info.tm.second,
			pic_info.tm.msecond
			);

	// if use oss, send picture to aliyun oss, else send to ftp.
	string ali_url = "";
	string ftp_url = "";
	extern bool g_songli_pic_use_aliyun;
	if(g_songli_pic_use_aliyun) {
        for (auto i = 0; i < 10; i++) {
            if (pic_info.alipath[i] == '\0') {
                break;
            }
            strcat(pic_info.aliname, pic_info.alipath[i]);
            strcat(pic_info.aliname, "/");
        }
        strcat(pic_info.aliname, pic_info.name);
		if (send_park_records_image_aliyun(&pic_info, 1) < 0) {
			ERROR("SEND IMAGE TO ALIYUN FAILED.");
		}
	} else {
		auto ftp = get_ftp_chanel(FTP_CHANNEL_PASSCAR);
		auto config = ftp->get_config();
		char ip_str[16] = "";
		sprintf(ip_str, "%d.%d.%d.%d", config->ip[0], config->ip[1],
									   config->ip[2], config->ip[3]);
		ftp_url = "ftp://" + string(config->user) + ":"
						   + string(config->passwd) + "@"
						   + string(ip_str) + ":"
						   + num2str(config->port);
		for (int i = 0; i < 10; i++) {
			if (pic_info.path[i][0] == '\0') {
				break;
			} else {
				ftp_url += "/" + string(pic_info.path[i]);
			}
		}
		ftp_url += "/" + string(pic_info.name);
		if (send_park_records_image_buf(&pic_info, 1) < 0) {
			ERROR("SEND IMAGE TO FTP FAILED.");
		}
	}

	// after send the picture data to oss or ftp, send message to platform to
	// indicate the certain picture.
	string resp = "";
	if (g_songli_pic_use_aliyun) {
		construct_pic_capture_response(ali_url, resp);
	} else {
		construct_pic_capture_response(ftp_url, resp);
	}
	ret = send_msg_2_songli_platform(resp.c_str());

	return ret;
}

/*
 * Name:		send_msg_2_songli_platform
 * Paramaters: const char* msg: text to send
 * Return:      0 OK
 *              -1 fail
 */
static int send_msg_2_songli_platform(const char* msg)
{
	if (NULL == msg) {
		ERROR("NULL == msg");
		return -1;
	}

    CParkSongli* instance = NULL;
    extern list<CPlatform*> g_platform_list;
	auto it = g_platform_list.begin();
	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it)
	{
        if(EPlatformType::PARK_SONGLI == (*it)->get_platform_type()) {
            instance = (CParkSongli*)(*it);
            break;
        }
	}

    string m = msg;
    instance->send_msg_2_songli_platform(m);
	return 0;
}

static int construct_reserve_response(const int schedule_action,
		                              const int schedule_state,
									  string& resp)
{
    // construct response data to platform
    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    child["scheduleAction"] = num2str(schedule_action);
    child["scheduleState"]  = (schedule_state == 0) ? "1" : "0"; // 1 OK 0 Fail

    root["cmdCode"] = PARK_CMD_CODE_RESERVE;
    root["devId"] = g_SerialNum;         //设备编号
    root["psId"] = EP_POINT_ID;         //泊位号
    root["data"] = child;

    resp = fast_writer.write(root);
	return 0;
}

static int construct_pic_capture_response(const string& url, string& resp)
{
	if (url.empty()) {
		ERROR("url is empty.");
		return -1;
	}

    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

	child["picUrl"] = url;

    root["cmdCode"] = PARK_CMD_CODE_PIC_CAPTURE;
    root["devId"] = g_SerialNum;         //设备编号
    root["psId"] = EP_POINT_ID;         //泊位号
    root["data"] = child;

    resp = fast_writer.write(root);
	return 0;
}
