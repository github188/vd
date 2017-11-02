/**
 * @file platform_thread.cpp
 * @brief load platform_set.cfg and start platforms.
 * @author felixdu
 * @date 2017-02-09
 * @version 0.1
 * <pre><b>copyright: bitcom</b></pre>
 * <pre><b>email: </b>durd_bitcom@163.com</pre>
 * <pre><b>All rights reserved.</b></pre>
 */
#include <fstream>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#include "Msg_Def.h"
#include "platform.h"
#include "platform_thread.h"

#include "park_platform.h"
#include "ve/ve.h"

#include "json/json.h"
#include "commonfuncs.h"
#include "logger/log.h"
#include "vd_msg_queue.h"

using namespace std;

list<CPlatform*> g_platform_list;
EDeviceType g_device_type;

#define PLATFORM_SET_CFG "/mnt/nand/platform_set.cfg"
#define TAKE_A_REST() do{ usleep(10 * 1000); }while(0);

extern void *tvpsystem_pthread(void * arg);

/**
 * @class CPlatFormFactory
 * @brief Factory to create multi platform class.
 *
 * create platform classes according to platform_type.
 */
class CPlatFormFactory
{
public:
	static CPlatform * creator(string platform_type,
			                   EDeviceType device_type = EDeviceType::PARK)
	{
		switch(device_type) {
		case EDeviceType::PARK:
			if(platform_type == "zehin")
				return new CParkZehin();
			else if(platform_type == "bitcom")
				return new CParkBitcom();
			else if(platform_type == "songli")
				return new CParkSongli();
			else if(platform_type == "http")
				return new CParkHTTP();
			else if(platform_type == "ktetc")
				return new CParkKtetc();
	#if 0
		case EDeviceType::VP:
			if(platform_type == "uniview")
				return new CVPUniview();
			else if(platform_type == "baokang")
				return new CVPBaokang();
			else if(platform_type == "bitcom")
				return new CVPBitcom();
		case EDeviceType::VM:
			break;
	#endif
		default:
			return NULL;

		}
	}

	static CPlatform * creator(EPlatformType platform_type,
			                   EDeviceType device_type = EDeviceType::PARK)
	{
		switch (device_type) {
		case EDeviceType::PARK:
			switch (platform_type) {
			case EPlatformType::PARK_ZEHIN:
				return new CParkZehin();
			case EPlatformType::PARK_BITCOM:
				return new CParkBitcom();
			case EPlatformType::PARK_SONGLI:
				return new CParkSongli();
			case EPlatformType::PARK_HTTP:
				return new CParkHTTP();
            default:
                return NULL;
			}
		#if 0
		case EDeviceType::VP:
			switch (platform_type) {
			case EPlatformType::VP_UNIVIEW:
				return new CVPUniview();
			case EPlatformType::VP_BAOKANG:
				return new CVPBaokang();
			case EPlatformType::VP_BITCOM:
				return new CVPBitcom()
			}
		case EDeviceType::VM:
			return NULL;
		#endif
		default:
			return NULL;
		}
	}
};

// {
//     "platform_type": "zehin",
//     "enable": "yes", / "no"
//     "attritube": {
//         "center_ip": "xxx.xxx.xxx.xxx",
//         "cmd_port": "5678",
//         "device_id": "123456789",
//         "heartbeat_port": "45001",
//         "sf_ip": "xxx.xxx.xxx.xxx",
//         "sf_port": "1234"
//     }
// }
int set_platform_info_from_json(const Json::Value root)
{
	string pt_type_str = root["platform_type"].asString();

	auto item = find_if(g_platform_list.begin(),
			         g_platform_list.end(),
					 [&pt_type_str](CPlatform* &l)
					 {
						 return l->get_platform_type_str() == pt_type_str;
					 });
	if(item != g_platform_list.end())
	{
		(*item)->set_platform_info(root);
	}
	else
	{
		CPlatform *p = CPlatFormFactory::creator(pt_type_str, g_device_type);
		if(p == NULL)
			return -1;

		p->set_platform_info(root);
		g_platform_list.push_back(p);
		DEBUG("g_platform_list %s", p->get_platform_type_str().c_str());
	}
	return 0;
}

/**
 * @brief set platform info
 *
 * get the platform info from string which can be read from config file or get
 * from web, then if it is new, add into global g_platform_list, otherwise,
 * change items.
 *
 * @param[in] json_str - json string
 *            {
 *                "platform_type": "zehin",
 *                "enable": "yes", / "no"
 *                "attritube": {
 *                    "center_ip": "xxx.xxx.xxx.xxx",
 *                    "cmd_port": "5678",
 *                    "device_id": "123456789",
 *                    "heartbeat_port": "45001",
 *                    "sf_ip": "xxx.xxx.xxx.xxx",
 *                    "sf_port": "1234"
 *                }
 *            }
 * @return 0
 */
int set_platform_info_from_json(const string & json_str)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(json_str, root);

	set_platform_info_from_json(root);
	return 0;
}

/**
 * @brief get the configuration from config file
 * @param filename - the config file name
 */
int load_configuration(const char* filename)
{
	Json::Reader reader;
	Json::Value root;

	if(NULL == filename)
	{
		ERROR("Platform set file name is null.");
		return -1;
	}

	ifstream is;
	is.open(filename, ios::binary);
	if(!reader.parse(is, root))
	{
		ERROR("Parse Json failed.");
		is.close();
		return -1;
	}

	// set the global device type according the configuration.
	string device_type_str = root["device_type"].asString();
	if(device_type_str == "PARK")
		g_device_type = EDeviceType::PARK;
	else if(device_type_str == "VP")
		g_device_type = EDeviceType::VP;
	else if(device_type_str == "VM")
		g_device_type = EDeviceType::VM;
	else
		ERROR("DEVICE ERROR %s", device_type_str.c_str());

	int platform_count = root["platform_list"].size();
	for(int i = 0; i < platform_count; i++)
	{
		Json::Value val = root["platform_list"][i];
		set_platform_info_from_json(val);
	}
	return 0;
}

/**
 * @brief dump the configuration from ram to flash
 */
int dump_configuration(const char* filename)
{
	Json::Value root;
	Json::Value platform_list;

	string device_type;
	if(g_device_type == EDeviceType::PARK)
		device_type = "PARK";
	else if(g_device_type == EDeviceType::VP)
		device_type = "VP";
	else if(g_device_type == EDeviceType::VM)
		device_type = "VM";

	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it)
		platform_list.append((*it)->get_platform_info_json());

	root["device_type"] = device_type;
	root["platform_list"] = platform_list;

	Json::StyledWriter writer;
	std::string strWrite = writer.write(root);
	std::ofstream ofs;
	ofs.open(filename);
	ofs << strWrite;
	ofs.close();

	return 0;
}

/**
 * @brief what you want to do before start the platform.
 */
static void init(void)
{
	switch (DEV_TYPE) {
		case 1: // PARK
			park_init();
			break;
		case 2:
			break;
		case 3:
			break;
		case 4: // VP
			break;
#if (DEV_TYPE == 5)
		case 5: // Alleyway
			if (ve_init() != 0) {
				CRIT("Initialize ve failed!");
			}
			break;
#endif
		default:
			INFO("DEV_TYPE %d is invalid", DEV_TYPE);
			break;
	}
}

/**
 * @brief what you want to do when the platforms is running.
 *
 * one case to sleep(forever) to keep the thread running.
 */
static void monitor()
{
	switch (DEV_TYPE) {
		case 1: // PARK
			park_monitor();
			break;
		case 2:
			break;
		case 3:
			break;
		case 4: // VP
	#if 0
			ve_monitor();
	#endif
			break;
		case 5: // Alleyway
			break;
		default:
			INFO("DEV_TYPE %d is invalid", DEV_TYPE);
			break;
	}
}

/**
 * @brief the entrance for platfrom_thread
 *
 * - load platform configuration
 * - start platforms
 * - waitting message from web about configuratioin changement or push the
 * - platforms to web.
 */
void *platform_thread(void *arg)
{
#if ((DEV_TYPE == 1) || (DEV_TYPE == 5))
	init();

	// load configuration from file and create instance if necessary.
	load_configuration(PLATFORM_SET_CFG);

	// start platform thread.
	for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if ((*it)->is_enable()) {
            (*it)->run();
        }
	}

	str_platform_info_msg platform_info_msg;
	while(true)
	{
		// add a break for this loop to prevet the high cpu load.
		TAKE_A_REST();
		monitor();

		memset(&platform_info_msg, 0, sizeof(str_platform_info_msg));
		int ret = msgrcv(MSG_PLATFORM_INFO_ID,
			   &platform_info_msg,
			   sizeof(str_platform_info_msg) - sizeof(long),
			   0,
			   IPC_NOWAIT);

		if(-1 == ret)
			continue;

		Json::Reader reader;
		Json::Value root;
		reader.parse(platform_info_msg.info.data, root);

		if(root["cmd"].asString() == "get_platform_info")
		{
			Json::Value root;
			Json::Value platform_list;

			string device_type;
			if(g_device_type == EDeviceType::PARK)
				device_type = "PARK";
			else if(g_device_type == EDeviceType::VP)
				device_type = "VP";
			else if(g_device_type == EDeviceType::VM)
				device_type = "VM";

			for(auto it = g_platform_list.begin();
				it != g_platform_list.end();
				++it)
			{
				platform_list.append((*it)->get_platform_info_json());
			}

			root["cmd"] = "get_platform_info";
			root["device_type"] = device_type;
			root["platform_list"] = platform_list;

			//Json::StyledWriter writer;
			Json::FastWriter writer;
			std::string strWrite = writer.write(root);

			DEBUG("get_platform_info resp %s", strWrite.c_str());
			// return the json configuration back to boa.
			int id = Msg_Init(WEB_VD_MSG_KEY);
			str_intelligent_param msg = {0};
			msg.msg_type = INTELLIGENT_PARAM_GET;
			strncpy(msg.des, "vd_to_web", strlen("vd_to_web"));
			strcpy(msg.buf, strWrite.c_str());
			msgsnd(id, &msg, sizeof(str_intelligent_param) - sizeof(long), 0);
		}
		else if(root["cmd"].asString() == "set_platform_info")
		{
			string pt_type_str = root["platform_type"].asString();
			for(unsigned int i = 0; i < root["platform_list"].size(); i++)
			{
				set_platform_info_from_json(root["platform_list"][i]);
			}

			for(auto it = g_platform_list.begin();
				it != g_platform_list.end();
				++it)
			{
                if ((*it)->is_enable() && (*it)->is_running() == false) {
                    (*it)->run();
                } else if ((*it)->is_enable() == false && (*it)->is_running() == true) {
                    (*it)->stop();
                }
			}
			dump_configuration(PLATFORM_SET_CFG);

/* **************************************************** */
#if 1
			Json::Value root;
			Json::Value platform_list;

			string device_type;
			if(g_device_type == EDeviceType::PARK)
				device_type = "PARK";
			else if(g_device_type == EDeviceType::VP)
				device_type = "VP";
			else if(g_device_type == EDeviceType::VM)
				device_type = "VM";

			for(auto it = g_platform_list.begin();
				it != g_platform_list.end();
				++it)
			{
				platform_list.append((*it)->get_platform_info_json());
			}

			root["cmd"] = "get_platform_info";
			root["device_type"] = device_type;
			root["platform_list"] = platform_list;

			//Json::StyledWriter writer;
			Json::FastWriter writer;
			std::string strWrite = writer.write(root);

			// return the json configuration back to boa.
			int id = Msg_Init(WEB_VD_MSG_KEY);
			str_intelligent_param msg = {0};
			msg.msg_type = INTELLIGENT_PARAM_GET;
			strncpy(msg.des, "vd_to_web", strlen("vd_to_web"));
			strcpy(msg.buf, strWrite.c_str());
			msgsnd(id, &msg, sizeof(str_intelligent_param) - sizeof(long), 0);
#endif
/* **************************************************** */
		}
	}
#elif (DEV_TYPE == 4)
	tvpsystem_pthread(NULL);
#endif
	return 0;
}
