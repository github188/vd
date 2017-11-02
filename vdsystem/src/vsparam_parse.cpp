#include <stdio.h>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <json/json.h>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <fstream>
#include "interface.h"
#include "dsp_config.h"
#include "commonfuncs.h"
#include "messagefile.h"
#include "xmlCreater.h"
#include "xmlParser.h"
#include "Msg_Def.h"
#include "logger/log.h"
#include "ve/config/ve_cfg.h"
#include "ve/config/cam_param.h"

using namespace std;

int alleway_jsparam_encode();
int park_jsparam_encode();

extern ARM_config g_arm_config; //arm参数结构体全局变量
extern SET_NET_PARAM g_set_net_param;

Json::Value read_root;

void split(const string& src, const string& separator, vector<string>& dest)
{
    string str = src;
    string substring;
    string::size_type start = 0, index;

    do
    {
        index = str.find_first_of(separator, start);
        if (index != string::npos)
        {
            substring = str.substr(start, index - start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
            if (start == string::npos) return;
        }
    }while(index != string::npos);

    //the last token
    substring = str.substr(start);
    dest.push_back(substring);
}

//park_jsparam_decode
int park_jsparam_decode()
{
	//PARK
	ARM_config arm_config;
	memcpy(&arm_config, &g_arm_config, sizeof(ARM_config));

	VDConfigData dsp_config;
	dsp_config.flag_func = 1; // 1 indicate it's park device.

	Json::Value jsv_content;
	jsv_content = read_root["content"];

	//graphics
	Json::Value jsv_graphics = jsv_content["graphics"];
	Json::Value jsv_graphics_param = jsv_graphics["param"];

	for(unsigned int i = 0; i < jsv_graphics["dest"].size(); i++)
	{
		string dst = jsv_graphics["dest"][i].asString();

		if(dst == "arm")
		{
		}
		if(dst == "dsp")
		{
			Detectarea_info &info = dsp_config.vdConfig.vdCS_config.detectarea_info;

			Json::Value jsv_leftline_info = jsv_graphics_param["leftLineInfo"];
			Json::Value jsv_leftline_start = jsv_leftline_info["startPoint"];
			Json::Value jsv_leftline_end = jsv_leftline_info["endPoint"];

			info.leftLineInfo.startPoint.x = jsv_leftline_start["x"].asInt();
			info.leftLineInfo.startPoint.y = jsv_leftline_start["y"].asInt();
			info.leftLineInfo.endPoint.x = jsv_leftline_end["x"].asInt();
			info.leftLineInfo.endPoint.y = jsv_leftline_end["y"].asInt();

			Json::Value jsv_detectarea = jsv_graphics_param["detectArea"];
			if (!jsv_detectarea.isNull()) {
				INFO("Get dsp detect area param.");
				info.detect_area.x = jsv_detectarea["x"].asInt();
				info.detect_area.y = jsv_detectarea["y"].asInt();
				info.detect_area.width = jsv_detectarea["w"].asInt();
				info.detect_area.height = jsv_detectarea["h"].asInt();
				info.detect_area.left = jsv_detectarea["x"].asInt();
				info.detect_area.right = jsv_detectarea["x"].asInt() +
					                     jsv_detectarea["w"].asInt() - 1;
				info.detect_area.top = jsv_detectarea["y"].asInt();
				info.detect_area.bottom = jsv_detectarea["y"].asInt() +
					                      jsv_detectarea["h"].asInt() - 1;
			}

			Json::Value jsv_polygon = jsv_graphics_param["polygon"];
			if (!jsv_polygon.isNull()) {
				INFO("Get dsp polygon param");

				info.parking_area.num = jsv_polygon["pointCount"].asInt();
				INFO("The num of point is %d.", info.parking_area.num);

				Json::Value jsv_point_arr = jsv_polygon["pointArr"];
				if (info.parking_area.num == (short)jsv_point_arr.size()) {
					INFO("Array size chk passed!");

					for (int i = 0; i < info.parking_area.num; ++i) {
						info.parking_area.point[i].x = jsv_point_arr[i]["x"].asInt();
						info.parking_area.point[i].y = jsv_point_arr[i]["y"].asInt();
						INFO("point[%d]: %d, %d", i,
							 info.parking_area.point[i].x,
							 info.parking_area.point[i].y);
					}
				}
			}
		}
	}

	//basicParam
	Json::Value jsv_basic = jsv_content["basicParam"];
	Json::Value jsv_basic_param = jsv_basic["param"];
	for(unsigned int i = 0; i <jsv_basic["dest"].size(); i++)
	{
		string dst = jsv_basic["dest"][i].asString();

     	if(dst == "arm")
		{
			stringstream arm_pointcode(jsv_basic_param["pointcode"].asString());
			arm_pointcode >> arm_config.basic_param.spot_id;
			stringstream arm_pointname(jsv_basic_param["pointname"].asString());
			arm_pointname >> arm_config.basic_param.spot;

			char tmp[100] = {0};
			convert_enc("UTF-8", "GBK", arm_config.basic_param.spot, 32, tmp, 64);
			strcpy(arm_config.basic_param.spot, tmp);
			stringstream ifdevicecode(jsv_basic_param["ifdevicecode"].asString());
			ifdevicecode >> arm_config.basic_param.exp_device_id;
			//stringstream devicecode(jsv_basic_param["devicecode"].asString());
			//devicecode >> arm_config.basic_param.device_id;
			arm_config.basic_param.direction = jsv_basic_param["direction"].asInt();
			arm_config.basic_param.log_level = jsv_basic_param["loglevel"].asInt();

			strcpy(g_set_net_param.m_NetParam.m_DeviceID,
					jsv_basic_param["devicecode"].asString().c_str());

			Json::Value jsv_mq_server = jsv_basic_param["mq_server"];

			vector<string> ip_list;
			split(jsv_mq_server["ip"].asString(), ".", ip_list);
			g_set_net_param.m_NetParam.m_MQ_IP[0] = atoi(ip_list[0].c_str());
			g_set_net_param.m_NetParam.m_MQ_IP[1] = atoi(ip_list[1].c_str());
			g_set_net_param.m_NetParam.m_MQ_IP[2] = atoi(ip_list[2].c_str());
			g_set_net_param.m_NetParam.m_MQ_IP[3] = atoi(ip_list[3].c_str());
			g_set_net_param.m_NetParam.m_MQ_PORT = jsv_mq_server["port"].asInt();

			Json::Value jsv_record_ftp = jsv_basic_param["record_ftp_server"];
			Json::Value jsv_record_mq = jsv_basic_param["mq_server1"];

			ip_list.clear();
			split(jsv_record_ftp["ip"].asString(), ".", ip_list);
			arm_config.basic_param.ftp_param_pass_car.ip[0] = atoi(ip_list[0].c_str());
			arm_config.basic_param.ftp_param_pass_car.ip[1] = atoi(ip_list[1].c_str());
			arm_config.basic_param.ftp_param_pass_car.ip[2] = atoi(ip_list[2].c_str());
			arm_config.basic_param.ftp_param_pass_car.ip[3] = atoi(ip_list[3].c_str());
			arm_config.basic_param.ftp_param_pass_car.port = jsv_record_ftp["port"].asInt();
			strncpy(arm_config.basic_param.ftp_param_pass_car.user,
					jsv_record_ftp["user"].asString().c_str(),
					sizeof(arm_config.basic_param.ftp_param_pass_car.user));
			strncpy(arm_config.basic_param.ftp_param_pass_car.passwd,
					jsv_record_ftp["password"].asString().c_str(),
					sizeof(arm_config.basic_param.ftp_param_pass_car.passwd));
			arm_config.basic_param.ftp_param_pass_car.allow_anonymous = jsv_record_ftp["anonymous"].asInt();

			g_set_net_param.m_NetParam.ftp_url_level.levelNum = jsv_record_ftp["ftp_path_layer_count"].asInt();
			for(int i = 0; i < g_set_net_param.m_NetParam.ftp_url_level.levelNum; i++)
				g_set_net_param.m_NetParam.ftp_url_level.urlLevel[i] = jsv_record_ftp["ftp_path_layer"][i].asInt();

			ip_list.clear();
			split(jsv_record_mq["ip"].asString(), ".", ip_list);
			arm_config.basic_param.mq_param.ip[0] = atoi(ip_list[0].c_str());
			arm_config.basic_param.mq_param.ip[1] = atoi(ip_list[1].c_str());
			arm_config.basic_param.mq_param.ip[2] = atoi(ip_list[2].c_str());
			arm_config.basic_param.mq_param.ip[3] = atoi(ip_list[3].c_str());
			arm_config.basic_param.mq_param.port = jsv_record_mq["port"].asInt();

			Json::Value jsv_aliyun = jsv_basic_param["aliyun_server"];
			strncpy(arm_config.basic_param.oss_aliyun_param.username,
					jsv_aliyun["user"].asString().c_str(),
					sizeof(arm_config.basic_param.oss_aliyun_param.username));
			strncpy(arm_config.basic_param.oss_aliyun_param.passwd,
					jsv_aliyun["password"].asString().c_str(),
					sizeof(arm_config.basic_param.oss_aliyun_param.passwd));
			strncpy(arm_config.basic_param.oss_aliyun_param.url,
					jsv_aliyun["url"].asString().c_str(),
					sizeof(arm_config.basic_param.oss_aliyun_param.url));
			strncpy(arm_config.basic_param.oss_aliyun_param.bucket_name,
					jsv_aliyun["bucket"].asString().c_str(),
					sizeof(arm_config.basic_param.oss_aliyun_param.bucket_name));
		}
		if(dst == "dsp")
		{
			stringstream dsp_pointcode(jsv_basic_param["pointcode"].asString());
			dsp_pointcode >> dsp_config.vdConfig.vdCS_config.device_config.spotID;
			stringstream dsp_pointname(jsv_basic_param["pointname"].asString());
			dsp_pointname >> dsp_config.vdConfig.vdCS_config.device_config.strSpotName;
			char tmp[100] = {0};
			convert_enc("UTF-8", "GBK", dsp_config.vdConfig.vdCS_config.device_config.strSpotName, 32, tmp, 64);
			strcpy(dsp_config.vdConfig.vdCS_config.device_config.strSpotName, tmp);

			stringstream dsp_devicecode(jsv_basic_param["devicecode"].asString());
			dsp_devicecode >> dsp_config.vdConfig.vdCS_config.device_config.deviceID;
			dsp_config.vdConfig.vdCS_config.device_config.direction = jsv_basic_param["direction"].asInt();
		}
	}

	//cameraParam
	Json::Value jsv_camera = jsv_content["cameraParam"];
	Json::Value jsv_camera_param = jsv_camera["param"];
	for(unsigned int i = 0; i < jsv_camera["dest"].size(); i++)
	{
		string dst = jsv_camera["dest"][i].asString();
		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_quality = jsv_camera_param["picQuality"].asInt();
			Json::Value jsv_resolution = jsv_camera_param["picResolution"];
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_width = jsv_resolution["width"].asInt();
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_height = jsv_resolution["height"].asInt();
		}
	}

	//resumeParam
	Json::Value jsv_resume = jsv_content["resumeParam"];
	Json::Value jsv_resume_param = jsv_resume["param"];

	for(unsigned int i = 0; i < jsv_resume["dest"].size(); i++)
	{
		string dst = jsv_resume["dest"][i].asString();
		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{

		}
	}

	//vehicleOverlay
	Json::Value jsv_overlay = jsv_content["vehicleOverlay"];
	Json::Value jsv_overlay_param = jsv_overlay["param"];
	for(unsigned int i = 0; i < jsv_overlay["dest"].size(); i++)
	{
		string dst = jsv_overlay["dest"][i].asString();

		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{
			Overlay_vehicle &overlay_vehicle = dsp_config.vdConfig.vdCS_config.overlay_vehicle;
			overlay_vehicle.is_overlay_vehicle = jsv_overlay_param["isOverlay"].asInt();
			overlay_vehicle.font_size = jsv_overlay_param["fontSize"].asInt();
			overlay_vehicle.is_time = jsv_overlay_param["isOverlayTime"].asInt();
			overlay_vehicle.is_spot_name = jsv_overlay_param["isPointName"].asInt();
			overlay_vehicle.is_device_numble = jsv_overlay_param["isDeviceCode"].asInt();
			overlay_vehicle.is_direction = jsv_overlay_param["isDirection"].asInt();
			overlay_vehicle.is_plate_numble = jsv_overlay_param["isCarno"].asInt();
			overlay_vehicle.is_plate_color = jsv_overlay_param["isPlatecolor"].asInt();
			overlay_vehicle.is_plate_type = jsv_overlay_param["isLicenseType"].asInt();
			overlay_vehicle.is_vehicle_type = jsv_overlay_param["isVehicleModel"].asInt();
			overlay_vehicle.is_vehicle_color = jsv_overlay_param["isCarcolor"].asInt();
			overlay_vehicle.is_vehicle_logo = jsv_overlay_param["isCarlogo"].asInt();
			overlay_vehicle.is_check_code = jsv_overlay_param["isSecurityCode"].asInt();
			Json::Value jsv_color = jsv_overlay_param["color"];
			overlay_vehicle.color.r = jsv_color["r"].asInt();
			overlay_vehicle.color.g = jsv_color["g"].asInt();
			overlay_vehicle.color.b = jsv_color["b"].asInt();
		}
	}

#if 1
    /*{
     *   "global" : {
     *      "city" : "青岛",
     *      "province" : "山东"
     *   },
     *   "device": {
     *        "parklock": {
     *            "enable": true,
     *            "type": 2,
     *            "attribute": {
     *                "gw_ip": "192.168.99.101",
     *                "gw_port": 8008,
     *                "mac": "AABBCCDDEEFFGG"
     *            }
     *        }
     *   }
     *}
     */
    /* construct the json message send to dsp as advanced param. */
    Json::Value jsv_advanced = jsv_content["advanced_parameter"];
    Json::Value jsv_advanced_param = jsv_advanced["param"];
    Json::Value jsv_install_province = jsv_advanced_param["install_city"]["province"];
    Json::Value jsv_install_city = jsv_advanced_param["install_city"]["city"];

    Json::Value root;
    Json::Value global;

    global["province"] = jsv_install_province;
    global["city"] = jsv_install_city;

    root["global"] = global;

    Json::FastWriter writer;
    const char* param_str = writer.write(root).c_str();

    /* convert utf-8 to gbk & send to dsp */
    char* buf_gbk = (char*)malloc(sizeof(char) * strlen(param_str) + 1);
    if (buf_gbk == NULL) {
        ERROR("malloc buf_gbk failed.");
        return -1;
    }

    ssize_t length = convert_enc_s("UTF-8", "GBK", param_str,
            strlen(param_str) + 1, buf_gbk, strlen(param_str) + 1);
    DEBUG("send msg to dsp %s", buf_gbk);
    send_dsp_msg((void*)buf_gbk, length + 1, 12);
    free(buf_gbk);

    /* the content above only handle global and sendto dsp,
     * following begin to handle device
     */

    Json::Value device;
    Json::Value parklock = jsv_advanced_param["parklock"];

    device["parklock"] = parklock;
    root["device"] = device;

    /* dump to file */
	Json::StyledWriter s_writer;
	std::string strWrite = s_writer.write(root);
	std::ofstream ofs;
	ofs.open("/mnt/nand/park.conf");
	ofs << strWrite;
	ofs.close();
#endif

	//Create arm_config.xml dsp_config.xml
	write_config_file((char*) SERVER_CONFIG_FILE);
	char tmp_path[128] = {0};
	sprintf(tmp_path, "%s.bak", (char *)ARM_PARAM_FILE_PATH);
	create_xml_file((char *) tmp_path, NULL, NULL, &arm_config);
	remove(ARM_PARAM_FILE_PATH);
	rename(tmp_path, (char *)ARM_PARAM_FILE_PATH);
	parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
	memset(tmp_path, 0, sizeof(tmp_path));
	sprintf(tmp_path, "%s.bak", (char *)DSP_PARAM_FILE_PATH);
	create_xml_file((char *) tmp_path, &dsp_config, NULL, NULL);
	remove(DSP_PARAM_FILE_PATH);
	rename(tmp_path, (char *)DSP_PARAM_FILE_PATH);
	send_config_to_dsp();
	send_arm_cfg();

	/*
	 * Perform multiple passes sync command to ensure successful
	 * writes to disk
	 */
	sync();
	sync();
	sync();

	park_jsparam_encode();

	return 0;
}
//alleway_jsdecode
int alleway_jsparam_decode()
{
	int  li_i = 0;
	char lc_tmp_path[PATH_MAX + 1] = {0};
	char gbk_buf[2048];
	ARM_config   arm_config;
	VDConfigData dsp_config;
	Json::Value jsv_content;
	Json::Value jsv_level1;
	Json::Value jsv_level2;
	Json::Value jsv_level3;
	Json::Value jsv_level4;
	Json::Value jsv_level5;

	//VE
	dsp_config.flag_func = 5;
	memcpy(&arm_config, &g_arm_config, sizeof(ARM_config));

	jsv_content = read_root["content"];

	//graphics
	jsv_level1 = jsv_content["graphics"];
	jsv_level2 = jsv_level1["param"];
	for(li_i=0; li_i<(int32_t)jsv_level1["dest"].size(); li_i++)
	{
		string dst = jsv_level1["dest"][li_i].asString();

		if(dst=="arm")
		{
		}
		if(dst=="dsp")
		{
			jsv_level3 = jsv_level2["leftLineInfo"];
			jsv_level4 = jsv_level3["startPoint"];
			dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.x = jsv_level4["x"].asInt();
			dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.y = jsv_level4["y"].asInt();
			jsv_level4 = jsv_level3["endPoint"];
			dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.x = jsv_level4["x"].asInt();
			dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.y = jsv_level4["y"].asInt();

			jsv_level3 = jsv_level2["detectArea"];
			if (!jsv_level3.isNull()) {
				INFO("Get dsp detect area param.");
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x = jsv_level3["x"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y = jsv_level3["y"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.width = jsv_level3["w"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.height = jsv_level3["h"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.left = jsv_level3["x"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.right = jsv_level3["x"].asInt() + jsv_level3["w"].asInt() - 1;
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.top = jsv_level3["y"].asInt();
				dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.bottom = jsv_level3["y"].asInt() + jsv_level3["h"].asInt() - 1;
			}

			jsv_level3 = jsv_level2["polygon"];
			if (!jsv_level3.isNull()) {
				INFO("Get dsp polygon param");

				dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num = jsv_level3["pointCount"].asInt();
				INFO("The num of point is %d.", dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num);

				jsv_level4 = jsv_level3["pointArr"];
				if (dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num == (short)jsv_level4.size()) {
					INFO("Array size chk passed!");

					for (int32_t i = 0; i < dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num; ++i) {
						dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].x = jsv_level4[i]["x"].asInt();
						dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].y = jsv_level4[i]["y"].asInt();
						INFO("point[%d]: %d, %d",
							 i,
							 dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].x,
							 dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].y);
					}
				}

			}
		}
	}

	//basicParam
	jsv_level1 = jsv_content["basicParam"];
	jsv_level2 = jsv_level1["param"];
	for(li_i=0; li_i<(int32_t)jsv_level1["dest"].size(); li_i++)
	{
		string dst = jsv_level1["dest"][li_i].asString();

     	if(dst == "arm")
		{
			stringstream arm_pointcode(jsv_level2["pointcode"].asString());
			arm_pointcode >> arm_config.basic_param.spot_id;
			/*
			 * Point Name
			 */
			string spot = jsv_level2["pointname"].asString();
			ssize_t l = convert_enc_s("UTF-8", "GBK",
									  spot.c_str(), spot.size(),
									  arm_config.basic_param.spot,
									  sizeof(arm_config.basic_param.spot) - 1);
			arm_config.basic_param.spot[l > 0 ? l : 0] = 0;

			stringstream arm_devicecode(jsv_level2["ifdevicecode"].asString());
			arm_devicecode >> arm_config.basic_param.exp_device_id;
			arm_config.basic_param.direction = jsv_level2["direction"].asInt();
			arm_config.basic_param.log_level = jsv_level2["loglevel"].asInt();

			strcpy(g_set_net_param.m_NetParam.m_DeviceID,
					jsv_level2["devicecode"].asString().c_str());
		}
		if(dst == "dsp")
		{
			stringstream dsp_pointcode(jsv_level2["pointcode"].asString());
			dsp_pointcode >> dsp_config.vdConfig.vdCS_config.device_config.spotID;
			/*
			 * Point name
			 */
			string spot = jsv_level2["pointname"].asString();
			ssize_t l = convert_enc_s("UTF-8", "GBK",
									  spot.c_str(), spot.size(),
									  dsp_config.vdConfig.vdCS_config.device_config.strSpotName,
									  sizeof(dsp_config.vdConfig.vdCS_config.device_config.strSpotName) - 1);
			dsp_config.vdConfig.vdCS_config.device_config.strSpotName[l > 0 ? l : 0] = 0;

			stringstream dsp_devicecode(jsv_level2["devicecode"].asString());
			dsp_devicecode >> dsp_config.vdConfig.vdCS_config.device_config.deviceID;
			dsp_config.vdConfig.vdCS_config.device_config.direction = jsv_level2["direction"].asInt();
		}
	}

	//cameraParam
	jsv_level1 = jsv_content["cameraParam"];
	jsv_level2 = jsv_level1["param"];
	for(li_i=0; li_i<(int32_t)jsv_level1["dest"].size(); li_i++)
	{
		string dst = jsv_level1["dest"][li_i].asString();
		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_quality = jsv_level2["picQuality"].asInt();
			jsv_level3 = jsv_level2["picResolution"];
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_width = jsv_level3["width"].asInt();
			dsp_config.vdConfig.vdCS_config.camera_arithm.image_height = jsv_level3["height"].asInt();
		}
	}

	//resumeParam
	jsv_level1 = jsv_content["resumeParam"];
	jsv_level2 = jsv_level1["param"];
	for(li_i=0; li_i<(int32_t)jsv_level1["dest"].size(); li_i++)
	{
		string dst = jsv_level1["dest"][li_i].asString();
		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{

		}
	}

	//vehicleOverlay
	jsv_level1 = jsv_content["vehicleOverlay"];
	jsv_level2 = jsv_level1["param"];
	for(li_i=0; li_i<(int32_t)jsv_level1["dest"].size(); li_i++)
	{
		string dst = jsv_level1["dest"][li_i].asString();

		if(dst == "arm")
		{

		}
		if(dst == "dsp")
		{
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_overlay_vehicle = jsv_level2["isOverlay"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.font_size = jsv_level2["fontSize"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_time = jsv_level2["isOverlayTime"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_spot_name = jsv_level2["isPointName"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_device_numble = jsv_level2["isDeviceCode"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_direction = jsv_level2["isDirection"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_numble = jsv_level2["isCarno"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_color = jsv_level2["isPlatecolor"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_type = jsv_level2["isLicenseType"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_type = jsv_level2["isVehicleModel"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_color = jsv_level2["isCarcolor"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_logo = jsv_level2["isCarlogo"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_check_code = jsv_level2["isSecurityCode"].asInt();
			jsv_level3 = jsv_level2["color"];
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.r = jsv_level3["r"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.g = jsv_level3["g"].asInt();
			dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.b = jsv_level3["b"].asInt();
		}
	}

	//Create arm_config.xml dsp_config.xml
	write_config_file((char*) SERVER_CONFIG_FILE);
	sprintf(lc_tmp_path, "%s.bak", (char *)ARM_PARAM_FILE_PATH);
	create_xml_file((char *) lc_tmp_path, NULL, NULL, &arm_config);
	remove(ARM_PARAM_FILE_PATH);
	rename(lc_tmp_path, (char *)ARM_PARAM_FILE_PATH);
	parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
	memset(lc_tmp_path, 0, sizeof(lc_tmp_path));
	sprintf(lc_tmp_path, "%s.bak", (char *)DSP_PARAM_FILE_PATH);
	create_xml_file((char *) lc_tmp_path, &dsp_config, NULL, NULL);
	remove(DSP_PARAM_FILE_PATH);
	rename(lc_tmp_path, (char *)DSP_PARAM_FILE_PATH);
	send_config_to_dsp();
	send_arm_cfg();

	/*
	 * Configures from ve.conf
	 */
	jsv_level1 = jsv_content["advanced_parameter"];
	jsv_level2 = jsv_level1["param"];
	if (!jsv_level2.isNull()) {
		ve_cfg_t cfg;
		ve_cfg_get(&cfg);

		global_cfg_t *global = &cfg.global;
		dsp_cfg_t *dsp = &cfg.dsp;
		dev_cfg_t *dev = &cfg.dev;

		jsv_level3 = jsv_level2["install_city"];
		if (!jsv_level3.isNull()) {
			ssize_t l = convert_enc_s("UTF-8", "GBK",
									  jsv_level3["province"].asString().c_str(),
									  jsv_level3["province"].asString().size(),
									  gbk_buf, sizeof(gbk_buf) - 1);
			gbk_buf[l > 0 ? l : 0] = 0;
			snprintf(global->prov, sizeof(global->prov), "%s", gbk_buf);

			l = convert_enc_s("UTF-8", "GBK",
							  jsv_level3["city"].asString().c_str(),
							  jsv_level3["city"].asString().size(),
							  gbk_buf, sizeof(gbk_buf) - 1);
			gbk_buf[l > 0 ? l : 0] = 0;
			snprintf(global->city, sizeof(global->city), "%s", gbk_buf);
		}

		global->ve_mode = (9 == get_direction()) ? VE_MODE_IN : VE_MODE_OUT;
		global->detect_mode = jsv_level2["coilControlMode"].asInt();
		global->rg_mode = jsv_level2["roadBrakeLiftMode"].asInt();
		dev->light.white_bright = jsv_level2["whiteLightMaxRate"].asInt();
		dev->light.white_mode = jsv_level2["whiteLightMode"].asInt();
		dev->led.model = jsv_level2["ledModel"].asInt();
		dev->audio.volume = jsv_level2["volume"].asInt();
		dev->audio.model = jsv_level2["audioCard"].asInt();
		ssize_t l = convert_enc_s("UTF-8", "GBK",
					jsv_level2["ledUserDefineDisplay"].asString().c_str(),
					jsv_level2["ledUserDefineDisplay"].asString().size(),
					dev->led.user_content,
					sizeof(dev->led.user_content) - 1);
		dev->led.user_content[l > 0 ? l : 0] = 0;

		if ((!jsv_level2["ledWidth"].isNull())
			&& (jsv_level2["ledWidth"].type() == Json::intValue)) {
			dev->led.width = jsv_level2["ledWidth"].asInt();
		}

		if ((!jsv_level2["ledHeight"].isNull())
			&& (jsv_level2["ledHeight"].type() == Json::intValue)) {
			dev->led.height = jsv_level2["ledHeight"].asInt();
		}

		if ((!jsv_level2["ledLattice"].isNull())
			&& (jsv_level2["ledLattice"].type() == Json::intValue)) {
			dev->led.lattice = jsv_level2["ledLattice"].asInt();
		}

		if ((!jsv_level2["whiteListMatch"].isNull())
			&& (jsv_level2["whiteListMatch"].type() == Json::intValue)) {
			dsp->white_list_match = jsv_level2["whiteListMatch"].asInt();
		}

		if ((!jsv_level2["captureMode"].isNull())
			&& (jsv_level2["captureMode"].type() == Json::intValue)) {
			dsp->capture_mode = jsv_level2["captureMode"].asInt();
		}

		if ((!jsv_level2["nonMotorMode"].isNull())
			&& (jsv_level2["nonMotorMode"].type() == Json::intValue)) {
			dsp->non_motor_mode = jsv_level2["nonMotorMode"].asInt();
		}

		ve_cfg_set(&cfg);
		ve_cfg_dump();
		ve_cfg_send_to_dsp();
	}

	/*
	 * Perform multiple passes sync command to ensure successful
	 * writes to disk
	 */
	sync();
	sync();
	sync();

	alleway_jsparam_encode();

	return 0;
}

//intelligentinfo jsencode
int park_jsparam_encode()
{
	VDConfigData dsp_config;
	ARM_config   arm_config;
	char lc_tmp[64] = {0};
	Json::FastWriter jswriter;
	Json::Value write_root;
	Json::Value jsv_level1;
	Json::Value jsv_level2;
	Json::Value jsv_level3;
	Json::Value jsv_level4;
	Json::Value jsv_level5;
	int web_to_vd = Msg_Init(WEB_VD_MSG_KEY);
	str_intelligent_param lstr_intelligent_param;

	memcpy(&arm_config, &g_arm_config, sizeof(ARM_config));
	parse_xml_doc((const char*)DSP_PARAM_FILE_PATH, &dsp_config, NULL, NULL);

	//graphics
	jsv_level2["dest"].append("dsp");
	jsv_level5["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.x;
   	jsv_level5["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.y;
	jsv_level4["startPoint"] = jsv_level5;
	jsv_level5.clear();
	jsv_level5["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.x;
   	jsv_level5["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.y;
	jsv_level4["endPoint"] = jsv_level5;
	jsv_level3["leftLineInfo"] = jsv_level4;
	jsv_level4.clear();

	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.left;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.top;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.width = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.right - dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x + 1;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.height = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.bottom - dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y + 1;

	jsv_level4["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x;
	jsv_level4["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y;
	jsv_level4["w"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.width;
	jsv_level4["h"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.height;
	jsv_level3["detectArea"] = jsv_level4;
	jsv_level4.clear();

	/*
	 * the polygon area
	 */
	Json::Value polygon;
	Json::Value point_tab;
	Json::Value point;

	for (int32_t i = 0; i < dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num; ++i) {
		point["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].x;
		point["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].y;
		point_tab.append(point);
		point.clear();
	}
	polygon["pointArr"] = point_tab;
	polygon["pointCount"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num;

	jsv_level3["polygon"] = polygon;

	jsv_level2["param"] = jsv_level3;
	jsv_level1["graphics"] = jsv_level2;

	//basicparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("arm");
	jsv_level2["dest"].append("dsp");
	jsv_level3["pointcode"] = arm_config.basic_param.spot_id;
	convert_enc("GBK", "UTF-8", arm_config.basic_param.spot, 32, lc_tmp, 64);
	jsv_level3["pointname"] = lc_tmp;
	jsv_level3["devicecode"] = g_set_net_param.m_NetParam.m_DeviceID;
	jsv_level3["ifdevicecode"] = arm_config.basic_param.exp_device_id;
	jsv_level3["direction"] = arm_config.basic_param.direction;
	jsv_level3["loglevel"] = arm_config.basic_param.log_level;

	Json::Value jsv_config_mq;
	Json::Value jsv_record_ftp;
	Json::Value jsv_record_mq;
	Json::Value jsv_aliyun;

	char tmp[16] = "";
	snprintf(tmp, 16, "%d.%d.%d.%d",
			 g_set_net_param.m_NetParam.m_MQ_IP[0],
			 g_set_net_param.m_NetParam.m_MQ_IP[1],
			 g_set_net_param.m_NetParam.m_MQ_IP[2],
			 g_set_net_param.m_NetParam.m_MQ_IP[3]);
	jsv_config_mq["ip"] = tmp;
	jsv_config_mq["port"] = g_set_net_param.m_NetParam.m_MQ_PORT;

	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, 16, "%d.%d.%d.%d",
			 arm_config.basic_param.ftp_param_pass_car.ip[0],
			 arm_config.basic_param.ftp_param_pass_car.ip[1],
			 arm_config.basic_param.ftp_param_pass_car.ip[2],
			 arm_config.basic_param.ftp_param_pass_car.ip[3]);
	jsv_record_ftp["ip"] = tmp;
	jsv_record_ftp["port"] = arm_config.basic_param.ftp_param_pass_car.port;
	jsv_record_ftp["user"] = arm_config.basic_param.ftp_param_pass_car.user;
	jsv_record_ftp["password"] = arm_config.basic_param.ftp_param_pass_car.passwd;
	jsv_record_ftp["anonymous"] = arm_config.basic_param.ftp_param_pass_car.allow_anonymous;
	jsv_record_ftp["ftp_path_layer_count"] = g_set_net_param.m_NetParam.ftp_url_level.levelNum;
	for(int i = 0; i < g_set_net_param.m_NetParam.ftp_url_level.levelNum; i++)
		jsv_record_ftp["ftp_path_layer"].append(g_set_net_param.m_NetParam.ftp_url_level.urlLevel[i]);

	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, 16, "%d.%d.%d.%d",
			 arm_config.basic_param.mq_param.ip[0],
			 arm_config.basic_param.mq_param.ip[1],
			 arm_config.basic_param.mq_param.ip[2],
			 arm_config.basic_param.mq_param.ip[3]);

	jsv_record_mq["ip"] = tmp;
	jsv_record_mq["port"] = arm_config.basic_param.mq_param.port;

	jsv_aliyun["user"] = arm_config.basic_param.oss_aliyun_param.username;
	jsv_aliyun["password"] = arm_config.basic_param.oss_aliyun_param.passwd;
	jsv_aliyun["url"] = arm_config.basic_param.oss_aliyun_param.url;
	jsv_aliyun["bucket"] = arm_config.basic_param.oss_aliyun_param.bucket_name;

	jsv_level3["mq_server"] = jsv_config_mq;
	jsv_level3["record_ftp_server"] = jsv_record_ftp;
	jsv_level3["mq_server1"] = jsv_record_mq;
	jsv_level3["aliyun_server"] = jsv_aliyun;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["basicParam"] = jsv_level2;

	//cameraparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("dsp");
	jsv_level4["width"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_width;
	jsv_level4["height"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_height;
	jsv_level3["picResolution"] = jsv_level4;
	jsv_level3["picQuality"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_quality;
	jsv_level3["picSizeUpperLimit"] = 0;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["cameraParam"] = jsv_level2;

	//resumeparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("arm");
	jsv_level3["isBrokenResume"] = 1;
	jsv_level3["loopCoverThreshold"] = 7;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["resumeParam"] = jsv_level2;

	//vehiclevoerlay
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("dsp");
	jsv_level4["r"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.r;
	jsv_level4["g"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.g;
	jsv_level4["b"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.b;
	jsv_level3["isOverlay"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_overlay_vehicle;
	jsv_level3["color"] = jsv_level4;
	jsv_level3["fontSize"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.font_size;
	jsv_level3["isOverlayTime"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_time;
	jsv_level3["isPointName"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_spot_name;
	jsv_level3["isDeviceCode"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_device_numble;
	jsv_level3["isDirection"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_direction;
	jsv_level3["isCarno"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_numble;
	jsv_level3["isPlatecolor"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_color;
	jsv_level3["isLicenseType"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_type;
	jsv_level3["isVehicleModel"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_type;
	jsv_level3["isCarcolor"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_color;
	jsv_level3["isCarlogo"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_logo;
	jsv_level3["isSecurityCode"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_check_code;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["vehicleOverlay"] = jsv_level2;

#if 1
	Json::Reader reader;
	Json::Value root;

	ifstream is;
	is.open("/mnt/nand/park.conf", ios::in);
	if(!reader.parse(is, root))
	{
		ERROR("Parse Json failed.");
		is.close();
	}

    Json::Value install_city;
    install_city["province"] = root["global"]["province"].asString();
    install_city["city"] = root["global"]["city"].asString();

    Json::Value parklock;
    parklock = root["device"]["parklock"];

    jsv_level2.clear();
    jsv_level2["param"]["install_city"] = install_city;
    jsv_level2["param"]["parklock"] = parklock;
    jsv_level1["advanced_parameter"] = jsv_level2;
#endif

	write_root["content"] = jsv_level1;
	write_root["devicetype"] = 1;
	write_root["version"] = 1;
	write_root["method"] = "get";
	std::string debug_json = jswriter.write(write_root);

	TRACE_LOG_SYSTEM("[debug_json]:%s", debug_json.c_str());

	lstr_intelligent_param.msg_type = INTELLIGENT_PARAM_GET;
	strncpy(lstr_intelligent_param.des, "vd_to_web", strlen("vd_to_web"));
	strcpy(lstr_intelligent_param.buf, debug_json.data());

	fprintf(stderr, "*************vd send web parmam to boa!*****************\n");

	msgsnd(web_to_vd, &lstr_intelligent_param, sizeof(lstr_intelligent_param) - sizeof(long), 0);

	return 0;
}
//intelligentinfo jsencode
int alleway_jsparam_encode()
{
	VDConfigData dsp_config;
	ARM_config   arm_config;
	char lc_tmp[64] = {0};
	char utf8_buf[1024];
	ssize_t len;
	Json::FastWriter jswriter;
	Json::Value write_root;
	Json::Value jsv_level1;
	Json::Value jsv_level2;
	Json::Value jsv_level3;
	Json::Value jsv_level4;
	Json::Value jsv_level5;
	int web_to_vd = Msg_Init(WEB_VD_MSG_KEY);
	str_intelligent_param lstr_intelligent_param;

	memcpy(&arm_config, &g_arm_config, sizeof(ARM_config));
	parse_xml_doc((const char*)DSP_PARAM_FILE_PATH, &dsp_config, NULL, NULL);

	//graphics
	jsv_level2["dest"].append("dsp");
	jsv_level5["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.x;
   	jsv_level5["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.startPoint.y;
	jsv_level4["startPoint"] = jsv_level5;
	jsv_level5.clear();
	jsv_level5["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.x;
   	jsv_level5["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.leftLineInfo.endPoint.y;
	jsv_level4["endPoint"] = jsv_level5;
	jsv_level3["leftLineInfo"] = jsv_level4;
	jsv_level4.clear();

	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.left;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.top;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.width = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.right - dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x + 1;
	dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.height = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.bottom - dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y + 1;

	jsv_level4["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.x;
	jsv_level4["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.y;
	jsv_level4["w"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.width;
	jsv_level4["h"] = dsp_config.vdConfig.vdCS_config.detectarea_info.detect_area.height;
	jsv_level3["detectArea"] = jsv_level4;
	jsv_level4.clear();

	/*
	 * the polygon area
	 */
	Json::Value polygon;
	Json::Value point_tab;
	Json::Value point;

	for (int32_t i = 0; i < dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num; ++i) {
		point["x"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].x;
		point["y"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.point[i].y;
		point_tab.append(point);
		point.clear();
	}
	polygon["pointArr"] = point_tab;
	polygon["pointCount"] = dsp_config.vdConfig.vdCS_config.detectarea_info.parking_area.num;

	jsv_level3["polygon"] = polygon;

	jsv_level2["param"] = jsv_level3;
	jsv_level1["graphics"] = jsv_level2;

	//basicparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("arm");
	jsv_level2["dest"].append("dsp");
	jsv_level3["pointcode"] = arm_config.basic_param.spot_id;
	convert_enc("GBK", "UTF-8", arm_config.basic_param.spot, 32, lc_tmp, 64);
	jsv_level3["pointname"] = lc_tmp;
	jsv_level3["devicecode"] = g_set_net_param.m_NetParam.m_DeviceID;
	jsv_level3["ifdevicecode"] = arm_config.basic_param.exp_device_id;
	jsv_level3["direction"] = arm_config.basic_param.direction;
	jsv_level3["loglevel"] = arm_config.basic_param.log_level;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["basicParam"] = jsv_level2;

	//cameraparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("dsp");
	jsv_level4["width"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_width;
	jsv_level4["height"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_height;
	jsv_level3["picResolution"] = jsv_level4;
	jsv_level3["picQuality"] = dsp_config.vdConfig.vdCS_config.camera_arithm.image_quality;
	jsv_level3["picSizeUpperLimit"] = 0;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["cameraParam"] = jsv_level2;

	//resumeparam
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("arm");
	jsv_level3["isBrokenResume"] = 1;
	jsv_level3["loopCoverThreshold"] = 7;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["resumeParam"] = jsv_level2;

	//vehiclevoerlay
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();
	jsv_level2["dest"].append("dsp");
	jsv_level4["r"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.r;
	jsv_level4["g"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.g;
	jsv_level4["b"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.color.b;
	jsv_level3["isOverlay"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_overlay_vehicle;
	jsv_level3["color"] = jsv_level4;
	jsv_level3["fontSize"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.font_size;
	jsv_level3["isOverlayTime"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_time;
	jsv_level3["isPointName"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_spot_name;
	jsv_level3["isDeviceCode"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_device_numble;
	jsv_level3["isDirection"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_direction;
	jsv_level3["isCarno"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_numble;
	jsv_level3["isPlatecolor"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_color;
	jsv_level3["isLicenseType"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_plate_type;
	jsv_level3["isVehicleModel"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_type;
	jsv_level3["isCarcolor"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_color;
	jsv_level3["isCarlogo"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_vehicle_logo;
	jsv_level3["isSecurityCode"] = dsp_config.vdConfig.vdCS_config.overlay_vehicle.is_check_code;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["vehicleOverlay"] = jsv_level2;

	//ve.conf
	jsv_level2.clear();
	jsv_level3.clear();
	jsv_level4.clear();
	jsv_level5.clear();

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	jsv_level2["dest"].append("dsp");
	jsv_level2["dest"].append("arm");

	len = convert_enc_s("GBK", "UTF-8",
						cfg.global.prov, strlen(cfg.global.prov),
						utf8_buf, sizeof(utf8_buf) - 1);
	utf8_buf[len > 0 ? len : 0] = 0;
	jsv_level4["province"] = utf8_buf;

	len = convert_enc_s("GBK", "UTF-8",
						cfg.global.city, strlen(cfg.global.city),
						utf8_buf, sizeof(utf8_buf) - 1);
	utf8_buf[len > 0 ? len : 0] = 0;

	jsv_level4["city"] = utf8_buf;

	jsv_level3["install_city"] = jsv_level4;
	jsv_level3["coilControlMode"] = cfg.global.detect_mode;
	jsv_level3["roadBrakeLiftMode"] = cfg.global.rg_mode;
	jsv_level3["whiteLightMaxRate"] = cfg.dev.light.white_bright;
	jsv_level3["whiteLightMode"] = cfg.dev.light.white_mode;
	jsv_level3["volume"] = cfg.dev.audio.volume;
	jsv_level3["audioCard"] = cfg.dev.audio.model;
	jsv_level3["ledModel"] = cfg.dev.led.model;
	ssize_t l = convert_enc_s("GBK", "UTF-8", cfg.dev.led.user_content,
							  strlen(cfg.dev.led.user_content),
							  utf8_buf, sizeof(utf8_buf) - 1);
	utf8_buf[l > 0 ? l : 0] = 0;
	jsv_level3["ledUserDefineDisplay"] = utf8_buf;
	jsv_level3["ledWidth"] = cfg.dev.led.width;
	jsv_level3["ledLattice"] = cfg.dev.led.lattice;
	jsv_level3["ledHeight"] = cfg.dev.led.height;
	jsv_level3["whiteListMatch"] = cfg.dsp.white_list_match;
	jsv_level3["nonMotorMode"] = cfg.dsp.non_motor_mode;
	jsv_level3["captureMode"] = cfg.dsp.capture_mode;
	jsv_level2["param"] = jsv_level3;
	jsv_level1["advanced_parameter"] = jsv_level2;

	write_root["content"] = jsv_level1;
	write_root["devicetype"] = 5;
	write_root["version"] = 1;
	write_root["method"] = "get";
	std::string debug_json = jswriter.write(write_root);

	INFO("[debug_json]:%s", debug_json.data());

	lstr_intelligent_param.msg_type = INTELLIGENT_PARAM_GET;
	strncpy(lstr_intelligent_param.des, "vd_to_web", strlen("vd_to_web"));
	strcpy(lstr_intelligent_param.buf, debug_json.data());

	fprintf(stderr, "*************vd send web parmam to boa!*****************\n");

	msgsnd(web_to_vd, &lstr_intelligent_param, sizeof(lstr_intelligent_param) - sizeof(long), 0);

	return 0;
}

int intelligent_jsparam_encode(char* data)
{
	int dev_type = 0;
	Json::Reader reader;

	INFO("intellignet json: %s", data);

	if(reader.parse(data, read_root))
	{
		dev_type = read_root["devicetype"].asInt();

		switch(dev_type)
		{
			case 1:
				park_jsparam_encode();
				break;
			case 5:
				alleway_jsparam_encode();
				break;
		}
	}
	return 0;
}

//intelligentinfo jsdecode
int intelligent_jsdecode(char *data)
{
	int dev_type = 0;
	Json::Reader reader;

	DEBUG("Json from boa: %s", data);

	if(reader.parse(data, read_root))
	{
		dev_type = read_root["devicetype"].asInt();

		switch(dev_type)
		{
			case 1:
				park_jsparam_decode();
				break;
			case 5:
				alleway_jsparam_decode();
				break;
		}
	}

//	alleway_jsparam_encode();

	return 0;
}

