#include "logger/log.h"
#include "park_records_process.h"
#include "park_util.h"
#include "capture_picture.h"
#include "http_descending.h"
#include "json/json.h"
#include "park_http.h"
#include "light_ctl_api.h"

#include <sstream>
#include <cstring>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "sem_util.h"
#ifdef __cplusplus
}
#endif

constexpr static char PARK_CMD_CODE_LIGHT_CONTROL {0x0B};
constexpr static char PARK_CMD_CODE_RESERVE       {0x0C};
constexpr static char PARK_CMD_CODE_PIC_CAPTURE   {0x0D};

constexpr static char PARK_RESERVE_ACTION_RESERVE {0x01};
constexpr static char PARK_RESERVE_ACTION_CANCEL  {0x02};
constexpr static char PARK_RESERVE_ACTION_ARRIVE  {0x03};

extern char g_SerialNum[50];

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

/*
 * Name:        recv_msg_park_down
 * Description: the callback handler for class_mq_consumer
 *              the portal fun to handler MQ received messages.
 * Paramaters:  const Message *raw_message: Message
 * Return:      void
 */
void http_recv_msg_park_down(const char *raw_message)
{
	DEBUG("Receive message from http.");
	if (NULL == raw_message) {
		ERROR("NULL == raw_message");
		return;
	}

	Json::Value root;
	Json::Reader reader;

	if(!reader.parse(raw_message, root)) {
		ERROR("Parse received message failed.");
		return;
	}

	/* const char* dev_id_remote = read_root["devId"].asString().c_str(); */
	/* const char* dev_id_local = g_set_net_param.m_NetParam.m_DeviceID; */
	const char* dev_id_local = g_SerialNum;
	const char* dev_id_remote = root["devId"].asString().c_str();
	if ((NULL == dev_id_remote) || (NULL == dev_id_local)) {
		ERROR("NULL == dev_id_remote or NULL == dev_id_local");
		return;
	}

	if (strcmp(dev_id_local, dev_id_remote) != 0) {
		DEBUG("收到MQ消息非本设备!,本设备id = %s, MQ_id = %s",
				dev_id_local, dev_id_remote);
		return;
	}

	int cmdCode = root["cmdCode"].asInt();

	switch (cmdCode) {
	case PARK_CMD_CODE_LIGHT_CONTROL: {
		string light_action = root["data"]["light"].asString();
		if(!light_action.empty()) {
			deal_msg_park_down_light_control(light_action);
		}
		break;
	}
	case PARK_CMD_CODE_RESERVE: {
		string schedule_str = root["data"]["scheduleAction"].asString();
		if (!schedule_str.empty()) {
			int schedule_action = atoi(schedule_str.c_str());
			//deal_msg_park_down_reserve(schedule_action);
		}
		break;
	}
	case PARK_CMD_CODE_PIC_CAPTURE:
		//deal_msg_park_down_pic_capture();
		break;
	default:
		break;
	}
	return;
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

	light_ctl(LIGHT_ACTION_START, LIGHT_PRIO_VD,
            lightInfo.red, lightInfo.red_interval,
            lightInfo.green, lightInfo.green_interval,
            lightInfo.blue, lightInfo.blue_interval,
            lightInfo.white);
	return 0;
}
