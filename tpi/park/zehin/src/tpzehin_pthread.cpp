#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sqlite3.h>
#include <string.h>
#include <curl/curl.h>

#include "logger/log.h"
#include "../inc/tpzehin_common.h"
#include "../inc/tpzehin_pthread.h"
#include "../inc/tpzehin_records.h"
#include "../lib/libsignalling/vPaas_DataType.h"
#include "../lib/libsignalling/vPaasT_SDK.h"
#include "sendFile.h"
#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "spsystem_api.h"
#include "spzehin_api.h"
#include "sendFile.h"
#include "json/json.h"
#include "dsp_config.h"
#include "global.h"
#include "refhandle.h"

#if 1
#include "park_util.h"
#endif

#include "../../../../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"

/* global variables */
str_tpzehin_login gstr_tpzehin_login;
bool g_light_controlled_by_platform = false;

/**<
 * 众恒平台login的时候，如果一次不能注册成功就会等待一段时间之后再次注册，直至
 * 注册成功才会走下面的逻辑，启用timer来注册。
 */
static timer_t g_login_timer = NULL;
static int send_picture_2_zehin_platform(const unsigned char* pic_data,
		                                 const unsigned int pic_size);

/**
 * 当晚上平台下发抓图指令时，有可能画面很暗，就将这个抓图指令告诉dsp，dsp会通过
 * 长曝光等方式提高画面亮度，然后再抓图。
 */
static void tpzehin_send_snap_req_2_dsp(void)
{
    //send ParkRecordInput to DSP

    SnapRequestInput snap_request_input;

    // 抓拍请求标识：1：抓拍请求；其他，无效
    snap_request_input.flag_snap = 1;
    snap_request_input.dev_type = DEV_TYPE;

    //INFO("==== dsp : snap_request_input.flag_snap=%d,dev_type=%d.\n",
    //        snap_request_input.flag_snap,
    //        snap_request_input.dev_type
    //);

    //1，算法配置参数；
    //2，隐形配置参数；
    //3，车牌库；
    //4，道闸控制输入参数；
    //5，电子车牌触发参数；
    //6，重启前记录回传；
    //7，arm参数；
    //8，抓拍请求
    send_dsp_msg(&snap_request_input, sizeof(SnapRequestInput), 8);
    //INFO("%s: after send_dsp_msg SnapRequestInput init \n", __func__);
}

/**
 * reserve handler, recv the reserve action and control the lock action.
 * for device, the reserve logic is only control the park lock.
 */
static int reserve_handler(Json::Value &read_root)
{
    // construct response data to platform
    Json::Value response;
    Json::FastWriter fast_writer;

    response["type"]           = read_root["type"].asString();
    response["leaguerId"]      = read_root["leaguerId"].asString();
    response["plateNo"]        = read_root["plateNo"].asString();
    response["spaceCode"]      = read_root["spaceCode"].asString();
    response["scheduleAction"] = read_root["scheduleAction"].asString();
    response["scheduleState"]  = "1"; // 1 OK 0 Fail

    // verify input info.
    if(strcmp(read_root["spaceCode"].asString().c_str(),
              tpzehin_get_spot_id()) != 0)
    {
        ERROR("Zehin %s don't match %s",
                read_root["spaceCode"].asString().c_str(),
                tpzehin_get_spot_id());

        // send the error response.
        response["scheduleState"] = "0";
        char* resp_str = (char*)(fast_writer.write(response).c_str());
        bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
        INFO("send reserve fail status = %d", result);
        return -1;
    }

    int Action = atoi(read_root["scheduleAction"].asString().c_str());
    switch (Action)
    {
        // all the down action share this.
        case 0: // cancel schedule down
        case 2: // arrive down
			park_lock_unlock();
            break;
        case 1: // schedule up
			park_lock_lock();
            break;
        default:
            INFO("Zehin ERR scheduleState = %d", Action);
            // response to platform
            response["scheduleState"] = "0";
            break;
    }

    // response to platform
    char* resp_str = (char*)(fast_writer.write(response).c_str());
    bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
    INFO("zehin callback response %s %d", resp_str, result);
    return 0;
}

static int send_picture_2_zehin_platform(const unsigned char* pic_data,
		                                 const unsigned int pic_size)
{
	int ret = -1;

	if ((NULL == pic_data) || (pic_size <= 0)) {
		ERROR("parameter parse failed.");
		return ret;
	}

    timeval tv = {0};
    struct tm *timenow = NULL;

    gettimeofday(&tv, NULL);
    timenow = localtime(&tv.tv_sec);

    char pic_name[32] = {0};
    sprintf(pic_name, "%s%02d%02d%02d%02d%02d.jpg",
            tpzehin_get_spot_id(),
            timenow->tm_mon + 1,
            timenow->tm_mday, timenow->tm_hour,
            timenow->tm_min, timenow->tm_sec);

    const char* url = tpzehin_get_sf_ip();
    char respurl[512] = {0};
    if ((strncmp(tpzehin_get_sf_ip(), "http://", 7) == 0) ||
        (strncmp(tpzehin_get_sf_ip(), "https://", 8) == 0)) {

		for(int i = 0; i < 3; i++) {
			if(!vPaasT_GetIsOnLine()) {
				ERROR("!!!!!Capture pic zehin is offline.");
				break;
			}

			int ret = SF_SendForPOSTData(url, pic_name, (const char*)pic_data,
					pic_size, respurl);

			if(ret == 0) {
				INFO("SF_Send picture OK!!!!!, error code = %d", ret);
				break;
			} else {
				CURLcode error_code = static_cast<CURLcode>(ret);
				const char* strerror = curl_easy_strerror(error_code);
				ERROR("SF_Send failed!!!!!, %d %s", error_code, strerror);
			}
		}
	} else {
#if 1 /************* XXX BELOW WILL BE ABANDONED **********/
		struct stFileInfo pic_info = {0};

		pic_info.iCamID = 111111;
		pic_info.iFileType = 0;
		pic_info.iCamIndex = 1;

		pic_info.stCreateDate.iYear    = timenow->tm_year + 1900;
		pic_info.stCreateDate.iMonth   = timenow->tm_mon + 1;
		pic_info.stCreateDate.iDay     = timenow->tm_mday;
		pic_info.stCreateDate.iHour    = timenow->tm_hour;
		pic_info.stCreateDate.iMinutes = timenow->tm_min;
		pic_info.stCreateDate.iSecond  = timenow->tm_sec;

		strcpy(pic_info.sFileName, pic_name);

		for(int i = 0; i < 3; i++) {
			if(!vPaasT_GetIsOnLine()) {
				ERROR("!!!!!Capture pic zehin is offline.");
				break;
			}

			ret = sf_send(gstr_tpzehin_login.sf.ip,
						  gstr_tpzehin_login.sf.port,
						  (char *)pic_data,
						  pic_size,
						  pic_info,
						  5,
						  respurl);

			if((ret == SF_SUCCESS) || (ret == SF_FILEEXIST)) {
				INFO("SF_Send picture OK!!!!!, error code = %d", ret);
				break;
			} else
				ERROR("SF_Send failed!!!!!, error code = %d", ret);
		}
#endif
	}

    //construct the response
    Json::Value response;
    response["type"]      = "2";
    response["spaceCode"] = tpzehin_get_spot_id();
    response["picUrl"]    = respurl;

    Json::FastWriter fast_writer;
    char* resp_str = (char*)(fast_writer.write(response).c_str());

	for (int i = 0; i < 3; i++) {
		bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
		INFO("send %s status = %d", resp_str, result);
		if (result == true)
			break;
	}
    return 0;
}


/**
 * Name: capture_pic_upload
 * Description: the state machine handler for pictue capture.
 *              it can describe below:
 *                                         ZEHIN_CAP_START
 *                                                |
 *              click picture capture ----------->|
 *                                                v
 *                                         ZEHIN_CAP_SEND2DSP
 *                                                |
 *              receive dsp mq ------------------>|
 *                                                v
 *                                         ZEHIN_CAP_FINISH = ZEHIN_CAP_START
 *
 * platform_type: 1 zehin
 *                2 songli
 */
static int capture_pic_upload(SnapRequestReturn* snap_rtn = NULL)
{
    static int cap_state = ZEHIN_CAP_START;
    switch(cap_state)
    {
        case ZEHIN_CAP_FINISH:
        case ZEHIN_CAP_START:
        {
            //send picture_capture info to dsp to adjust light.
            tpzehin_send_snap_req_2_dsp();
            cap_state = ZEHIN_CAP_SEND2DSP;
            break;
        }
        case ZEHIN_CAP_SEND2DSP:
        {
            if(NULL == snap_rtn)
            {
                // this may occur when click capture picture very frequently.
                // the first capture req send mq to dsp and set state to
                // ZEHIN_CAP_SEND2DSP then the second request coming.
                // NOW the solution is send mq to dsp to make it as same as
                // the first request.

                //send picture_capture info to dsp to adjust light.
                tpzehin_send_snap_req_2_dsp();
                break;
            }

            int delay_time = 0;
            if((snap_rtn->scene == 2) || (snap_rtn->scene == 3))
            {
                if(0 == snap_rtn->delay_ms)
                    delay_time = 1000;
                else
                    delay_time = snap_rtn->delay_ms;
            }

            _RefHandleRelease((void**)&snap_rtn, NULL, NULL);
            usleep(delay_time * 1000);

			data_transfer_cb cb_handler = send_picture_2_zehin_platform;
            do_cap_pic_upload(cb_handler);
            cap_state = ZEHIN_CAP_FINISH;
            break;
        }
    }
    return 0;
}

/**
 * Name: light_control
 * Description: receive light control command from platform
 */
static int light_control(Json::Value &read_root)
{
    INFO("light_control");

	if (!tpzehin_light_ctl_enable()) {
		INFO("received light ctl from platform but take NO action.");
		return 0;
	}

    do{
        char values[5][128] = {{0}};
        spsystem_select_lastmsg_table(spsystem_db, values);

        int objectstate = atoi(values[3]);
        if (objectstate != 1) {
            /**
             * last record type is not parking in,
             * so there is no car in this space.
             */
            INFO("There is no car in the space, take no action");
            break;
        }

#if 0
        char plate[64] = {0};
        convert_enc("GBK", "UTF-8", lc_values[1], 32, plate, 64);
        if (strcmp(plate, read_root["plateNo"].asString().c_str()) != 0) {
            INFO("plate number don't match. %s %s %s",lc_values[1], plate,
                  read_root["plateNo"].asString().c_str());
            break;
        }
#endif

        if (read_root["lightControl"] == "bf") {
            unsigned char red;
            unsigned char red_interval;
            unsigned char blue;
            unsigned char blue_interval;
            unsigned char green;
            unsigned char green_interval;
            unsigned char white;

            get_light_state(&red, &red_interval, &blue, &blue_interval,
                            &green, &green_interval, &white);

            unsigned char current_light_brightness = red | blue | green;

            light_ctl(LIGHT_ACTION_START, LIGHT_PRIO_PLATFORM,
                      0, 0, 0, 0, current_light_brightness, 5, 0);

            park_save_light_state();
            g_light_controlled_by_platform  = true;

        } else if (read_root["lightControl"] == "reset") {

            light_ctl(LIGHT_ACTION_STOP, LIGHT_PRIO_PLATFORM,
                      0, 0, 0, 0, 255, 5, 0);

            g_light_controlled_by_platform  = false;
            park_delete_light_state();
        } else {
            ERROR("Bad lightControl action.");
        }
    }while(0);

    Json::Value response;
    response["type"]      = "8";
    response["spaceCode"] = tpzehin_get_spot_id();
    response["plateNo"] = read_root["plateNo"];
    response["lightControl"] = read_root["lightControl"];
    response["lightState"] = read_root["lightControl"];

    Json::FastWriter fast_writer;
    char* resp_str = (char*)(fast_writer.write(response).c_str());

	for (int i = 0; i < 3; i++) {
		bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
		INFO("send %s status = %d", resp_str, result);
		if (result == true)
			break;
	}
    return 0;
}

static int get_cmd_info(const char *cmd, char *result)
{
    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL) {
        pclose(fp);
        return -1;
    }

    if (fread(result, 2048, 1, fp) <= 0) {
    //if (fgets(result, 2048, fp) == NULL) {
        pclose(fp);
        return -1;
    } else {
        int len = strlen(result);
        DEBUG("len = %d", len);
        int j = 0;
        for (j = 0; j < len; j++) {
            if (result[j] == '\n')
                result[j] = 0;
        }

        pclose(fp);
        return 0;
    }
}

/**
 * Name: run_command
 * Description: receive commands from zehin platform to control ipnc.
 */
static int run_command(Json::Value &read_root)
{
    INFO("run_command");
    do{
        const char* command = read_root["command"].asString().c_str();
        CRIT("receive command from zehin platform: %s", command);
        char result[2048] = {0};
        get_cmd_info(command, result);
        DEBUG("run command finished. %s", result);

        Json::Value response;
        response["type"]      = "9";
        response["spaceCode"] = tpzehin_get_spot_id();
        response["result"] = result;

        Json::FastWriter fast_writer;
        char* resp_str = (char*)(fast_writer.write(response).c_str());

        for (int i = 0; i < 3; i++) {
            bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
            INFO("send %s status = %d", resp_str, result);
            if (result == true)
                break;
        }
    }
    while(0);

    return 0;
}
/**
 * Name: tpzehin_cb_handler
 * Description: the callback handler for zehin custom data.
 */
int tpzehin_cb_handler(void* data, int length)
{
    char* custom_data = (char*)data;
    if((NULL == custom_data) || (0 == length))
    {
        ERROR("Zehin custom_data == NULL");
        return -1;
    }

    Json::Value read_root;
    Json::Reader reader;

    if(!reader.parse(custom_data, read_root))
    {
        ERROR("Zehin parse json failed. json = %s", custom_data);
        free(custom_data);
        return -1;
    }

    int type = atoi(read_root["type"].asString().c_str());
    switch (type)
    {
    case 1: /// reserve
        reserve_handler(read_root);
        break;
    case 2: /// picture capture
        capture_pic_upload(NULL);
        break;
    case 8: /// light control
        light_control(read_root);
        break;
    //case 4: /// command action.
    case 9: /// command action.
        run_command(read_root);
        break;
    default:
        ERROR("bad type. %d", type);
        break;
    }
    free(custom_data);
    return 0;
}

/**
 * Name: recv_cmd_func
 * Description: The callback function for zehin platform.
 *              send the data to tpzehin thread and handle
 *              it in the tpzehin main thread.
 */
int recv_cmd_func(char*sCmd,
                  void* lpStructParam,
                  int iStructLen,
                  void* lpXMLDataParam,
                  int iXMLDataLen,
                  char*sFromIP,
                  int iPort,
                  void* pCustomDate)
{
    DEBUG("_______Zehin callback recv cmd________ = %s", sCmd);

    if (strcmp(sCmd, "DATA_REAL_CUSTOMDATA_TO_CT") == 0) {
        str_park_msg park_msg = {0};
        park_msg.data.zehin.type = MSG_PARK_ZEHIN_CB;
        park_msg.data.zehin.lens = iStructLen;
        char* custom_data = (char*)malloc(iStructLen);
        if (NULL == custom_data) {
            ERROR("malloc failed.");
            return -1;
        }
        memcpy(custom_data, ((stZehinDataRealCustomDataToCT_T_ *)
					lpStructParam)->sCustomData, iStructLen);

        park_msg.data.zehin.data = custom_data;
        vmq_sendto_tpzehin(park_msg);
        /* NOTE this memory NOT free until handle finished. */

    } else if (strcmp(sCmd, "DATA_REAL_TIME_ALARM_EX_REP") == 0 ||
            strcmp(sCmd, "DATA_MULTI_REAL_TIME_EX_REP") == 0) {
        stZehinDataMultiRealTimeRep_T_ *struRep =
            (stZehinDataMultiRealTimeRep_T_*)lpStructParam;
        DEBUG("ZEHIN_SDK_LOG Sn = %s, Ret = %d, CustomData = %s, RecordID = %s",
                struRep->sTerminalSn, struRep->iRetCode,
                struRep->sCustomData, struRep->sRecordID);
    } else {
#if 1
        if(lpStructParam != NULL)
            INFO("lpStructParam = %s", lpStructParam);
        if(lpXMLDataParam != NULL)
            INFO("Zehin callback recv iXMLDataLen=%d, lpXMLDataParam = %s",
                    iXMLDataLen, (char*)lpXMLDataParam);
#endif
    }
    return 0;
}

/**
 * Name: tpzehin_login_timer_cb
 * Description: timer callback function for login.
 *              retry every 5 secords. this include
 *              login and set tag info etc.
 */
static void tpzehin_login_timer_cb(union sigval sig)
{
    const char* center_ip = tpzehin_get_center_ip();
    const unsigned short cmd_port = tpzehin_get_cmd_port();
	DEBUG("Zehin platform is logging in %s:%d", center_ip, cmd_port);

	static bool already_stun = false;

	if (!already_stun) {
		bool stun_ret = vPaasT_ConnetStunSer();
		if (!stun_ret) {
			char *reason = vPaasT_GetStunText();
			INFO("ConnectStun failed. %s", reason);
			vPaasT_Close();
			park_set_timer(&g_login_timer, 5000);
			return;
		}
		already_stun = true;
	}

	int login_status = vPaasT_Login(gstr_tpzehin_login.local.user_name,
					                gstr_tpzehin_login.local.user_stat,
					                gstr_tpzehin_login.local.type);
	if (login_status != 1000) {
		ERROR("Zehin login failed status = %d", login_status);
		//start the timer again.
		park_set_timer(&g_login_timer, 5000);
		return;
	}

	INFO("Zehin platform login successful.");

	vPaasT_SetAutoReLogin(true);
	//upload camera infomaiton to vPaas
	vPaasT_UploadCamInfo(gstr_tpzehin_login.camera);

	//upload device infomation to vPass
	vPaasT_UploaDevInfo(gstr_tpzehin_login.device);

	//upload index infomation to vPass
    const char* data_name_list[] = {"berth_num", "plate_num", "plate_color",
                              "status", "confidence", "eltype", "eltime",
                              "alarm_flag", "plate_position"};

    for (unsigned int i = 0;
         i < sizeof(data_name_list) / sizeof(data_name_list[0]);
         i++) {
        strcpy(gstr_tpzehin_login.index.sDataName, data_name_list[i]);
        gstr_tpzehin_login.index.iDataIndex = i + 1;
        gstr_tpzehin_login.index_num++;
        vPaasT_UploaDataPointInfo(gstr_tpzehin_login.index);
    }

	// set tag info
	stZehinSetTagInfo_T_ tag_info;
	memset(&tag_info, 0, sizeof(stZehinSetTagInfo_T_));
	tag_info.iActive = 1;
	strcpy(tag_info.sTerminalSn,  tpzehin_get_spot_id());
	strcpy(tag_info.sSerTag,      "teyiting");
	strcpy(tag_info.sUsrTag,      "teyiting");
	strcpy(tag_info.sLocationTag, "teyiting");
	vPaasT_SetTagInfo(tag_info);
}

/**
 * Name: tpzehin_login
 * Description: login
 */
static int tpzehin_login(void)
{
//	vPaasT_Init();

	vPaasT_SetCenterIP((char*)tpzehin_get_center_ip());
	vPaasT_SetCenterCmdPort(tpzehin_get_cmd_port());
	vPaasT_SetCenterHeartPort(tpzehin_get_heartbeat_port());

	vPaasT_SetCmdCallBack(recv_cmd_func, NULL);

	vPaasT_SetStunIP((char*)tpzehin_get_center_ip());

	INFO("Login: username:%s, userstat=%d, usertype=%d",
                     gstr_tpzehin_login.local.user_name,
                     gstr_tpzehin_login.local.user_stat,
                     gstr_tpzehin_login.local.type);

    /* start the timer to do login action,
     * retry every 5 seconds.
     */
    park_create_timer(&g_login_timer, tpzehin_login_timer_cb, (int)0);
    park_set_timer(&g_login_timer, 10);

	return 0;
}

//tpzehin init
static void tpzehin_login_init()
{
	memset(&gstr_tpzehin_login, 0, sizeof(str_tpzehin_login));

	//zehin center info
	strcpy(gstr_tpzehin_login.center.ip, tpzehin_get_center_ip());
	gstr_tpzehin_login.center.cmd_port = tpzehin_get_cmd_port();
	gstr_tpzehin_login.center.heart_port = tpzehin_get_heartbeat_port();
	strcpy(gstr_tpzehin_login.sf.ip, tpzehin_get_sf_ip());
	gstr_tpzehin_login.sf.port = tpzehin_get_sf_port();
	INFO("center_ip = %s cmd = %d hb = %d sf_ip = %s sf = %d",
          gstr_tpzehin_login.center.ip,
          gstr_tpzehin_login.center.cmd_port,
          gstr_tpzehin_login.center.heart_port,
          gstr_tpzehin_login.sf.ip,
          gstr_tpzehin_login.sf.port);

	//zehin local info
	strcpy(gstr_tpzehin_login.local.ip, "192.168.1.220");
	strcpy(gstr_tpzehin_login.local.user_name, tpzehin_get_spot_id());
	//strcpy(gstr_tpzehin_login.local.user_name,
	//msgbuf_paltform.Msg_buf.Zehin_Park_Mag.DeviceId);
	strcpy(gstr_tpzehin_login.local.user_passwd, "passwd");
	gstr_tpzehin_login.local.user_stat = 2;
	gstr_tpzehin_login.local.type = 1;

	//zehin devcie
	strcpy(gstr_tpzehin_login.device.sTerminalSn, tpzehin_get_spot_id());
	gstr_tpzehin_login.device.iDevIndex = 2;
	strcpy(gstr_tpzehin_login.device.sDevname, "bitcompark");

	//zehin index
	strcpy(gstr_tpzehin_login.index.sTerminalSn, tpzehin_get_spot_id());
	gstr_tpzehin_login.index.iDevIndex = 2;;
}

//init park zehin database
static int tpzehin_database_init()
{
	create_multi_dir("/home/records/park/zehin/database/");

	spzehin_open("/home/records/park/zehin/database/spzehin.db");
	spzehin_create_msgreget_table(spzehin_db);
	spzehin_create_alarmreget_table(spzehin_db);
	spzehin_create_picreget_table(spzehin_db);

	if(spzehin_check_table(spzehin_db, (char*)"msgreget", NULL, 10) < 0)
	{
		spzehin_drop_table(spzehin_db, (char*)"msgreget");
		spzehin_create_msgreget_table(spzehin_db);
		DEBUG("Drop table msgreget");
	}

	if(spzehin_check_table(spzehin_db, (char*)"alarmreget", NULL, 5) < 0)
	{
		spzehin_drop_table(spzehin_db, (char*)"alarmreget");
		spzehin_create_alarmreget_table(spzehin_db);
		DEBUG("Drop table alarmreget");
	}

	if(spzehin_check_table(spzehin_db, (char*)"picreget", NULL, 7) < 0)
	{
		spzehin_drop_table(spzehin_db, (char*)"picreget");
		spzehin_create_picreget_table(spzehin_db);
		DEBUG("Drop table picreget");
	}

	return 0;
}

/**
 * Name: zehin_startup
 * Description: start record/alarm/reget thread and
 *              start timer to login to platform.
 */
static int zehin_startup(void)
{
    int ret = 0;
    static bool initialized = false;

    if(initialized)
    {
        INFO("tpzehin platform already initialized.");
        return 0;
    }

    INFO("##### ZEHIN START UP. #####");

    pthread_t tpzehin_record_id;
    pthread_t tpzehin_alarm_id;
    pthread_t tpzehin_recordsreget_id;

    //create send park record pthread
    if((ret = pthread_create(&tpzehin_record_id, NULL,
                       tpzehin_record_pthread, NULL)) != 0)
    {
        ERROR("Create tpzehin_record_pthread failed. %d\n", ret);
    }

    //create send park alarm pthread
    if((ret = pthread_create(&tpzehin_alarm_id, NULL,
                       tpzehin_alarm_pthread, NULL)) != 0)
    {
        ERROR("Create tpi tpzehin_alarm_pthread failed. %d\n", ret);
    }

	if(tpzehin_retransmition_enable())
	{
		//create reget records pthread
		if((ret = pthread_create(&tpzehin_recordsreget_id, NULL,
						   tpzehin_recordsreget_pthread, NULL)) != 0)
		{
			ERROR("Create tpi tpzehin_recordsreget_pthread failed. %d\n", ret);
		}
	}

    //init park zehin database
    tpzehin_database_init();
    vPaasT_Init();

    //init login info
    tpzehin_login_init();

    //login zehin platform
    tpzehin_login();

    initialized = true;
    return 0;
}

/**
 * Name: tpzehin_conf_handler
 * Description: receive the platform provision message
 *              from web and turn on/off zehin platform
 * Note: this is not used yet.
 */
static int tpzehin_conf_handler(char* data, int length)
{
    if((NULL == data) || (0 == length))
    {
        ERROR("data is NULL\n");
        return -1;
    }

    Json::Value config;
    Json::Reader reader;

    if(!reader.parse(data, config))
    {
        ERROR("ZEHIN parse config json failed. json = %s\n", data);
        return -1;
    }

    if(config["dev_type"].asString() != "PARK")
    {
        ERROR("ZEHIN error device %s\n", config["dev_type"].asString().c_str());
        return -1;
    }

    if(config["platform_type"].asString() != "zehin")
    {
        ERROR("ZEHIN error platform %s\n",
                config["platform_type"].asString().c_str());
        return -1;
    }

    // turn on zehin platform
    if(config["switch"].asString() == "on")
    {
        INFO("ZEHIN LOGIN.\n");

        // this only make effect once.
        zehin_startup();

        // check if is same as previous configuration,
        // if so, do nothing, else logout and login with new configuration.
        bool conf_changed = false;
        while(0)
        {
            char* center_ip = vPaasT_GetCenterIP();
            if(strcmp(center_ip, tpzehin_get_center_ip()) != 0)
            {
                conf_changed = true;
                break;
            }

            int cmd_port = vPaasT_GetCenterCmdPort();
            if(cmd_port != tpzehin_get_cmd_port());
            {
                conf_changed = true;
                break;
            }

            int hb_port = vPaasT_GetCenterHeartPort();
            if(hb_port != tpzehin_get_heartbeat_port());
            {
                conf_changed = true;
                break;
            }

            // global variables from sendFile.cpp
            extern char g_sIP[16];
            char* sf_ip = g_sIP;
            if(strcmp(sf_ip, tpzehin_get_sf_ip()) != 0)
            {
                conf_changed = true;
                break;
            }

            extern int g_iPort;
            int sf_port = g_iPort;
            if(sf_port != tpzehin_get_sf_port());
            {
                conf_changed = true;
                break;
            }
        }

        if(conf_changed)
        {
            INFO("ZEHIN configuration changed, relogin.\n");

            //init login info
            tpzehin_login_init();

            //login zehin platform
            tpzehin_login();
        }
    }
    // turn off zehin platform
    else
    {
        INFO("ZEHIN LOGOUT.\n");
        vPaasT_Logout();
    }
    return 0;
}

/**
 * Name: tpzehin_status_handler
 * Description: construct the status json string and
 *              send to zehin platform if online.
 */
static int tpzehin_status_handler(StatusPark *status)
{
    if(NULL == status)
    {
        ERROR("ZEHIN NULL == status.\n");
        _RefHandleRelease((void**)&status, NULL, NULL);
        return -1;
    }

    static bool first_status = true;
    if (first_status) {
        first_status = false;

        /* this file exist indicate that the light before reboot is
         * controlled by platform, then restore this scenaio */
        if(park_light_state_exist()) {
            char * plate_in_status = status->plateNo;

            char values[5][128] = {{0}};
            spsystem_select_lastmsg_table(spsystem_db, values);

            char * plate_in_lastmsg = values[1];

            DEBUG("%s %s", plate_in_status, plate_in_lastmsg);
            if (strcmp(plate_in_status, plate_in_lastmsg) == 0) {
                unsigned char red = 0x00;
                unsigned char red_interval = 0x00;
                unsigned char blue = 0x00;
                unsigned char blue_interval = 0x00;
                unsigned char green = 0x00;
                unsigned char green_interval = 0x00;
                unsigned char white = 0x00;

                get_light_state(&red, &red_interval, &blue, &blue_interval,
                                &green, &green_interval, &white);

                unsigned char current_light_brightness = red | blue | green;

                light_ctl(LIGHT_ACTION_START, LIGHT_PRIO_PLATFORM,
                          0, 0, 0, 0, current_light_brightness, 5, 0);

                // park_save_light_state();
                g_light_controlled_by_platform  = true;
            } else {
                park_delete_light_state();
            }
        }
    }

	if(!tpzehin_space_status_enable())
	{
		INFO("SPACE_STATUS is NOT enable.");
        _RefHandleRelease((void**)&status, NULL, NULL);
		return 0;
	}

    Json::Value response;
    Json::FastWriter fast_writer;

	time_t now;
	struct tm *timenow;

	time(&now);
	timenow = localtime(&now);

    char time_format[20] = {0};
    sprintf(time_format, "%04d-%02d-%02d %02d:%02d:%02d",
            timenow->tm_year + 1900, timenow->tm_mon + 1,
            timenow->tm_mday, timenow->tm_hour,
            timenow->tm_min, timenow->tm_sec);

    char plate_no[64] = {0};
    convert_enc("GBK", "UTF-8", status->plateNo, 32, plate_no, 64);

    response["type"] = "3";
    response["spaceCode"] = tpzehin_get_spot_id();

	char car_statue_str[2] = "";
	snprintf(car_statue_str, 2, "%d", status->carState);
    response["carState"] = car_statue_str;

    response["plateNo"] = plate_no;
    response["currentTime"] = time_format;

    char* resp_str = (char*)(fast_writer.write(response).c_str());

    /**
     * epark platform use the status as heartbeat, so try best to send
     * it regardless of the info indicate if offline from vpaas heatbeat.
     * otherwise, use if(tpzehin_is_online()) instead of if (1).
     */
    if (1)
    {
		/* has NOT set the flag send status when retransmition AND
		 * retransmition enable AND
		 * there are msg in msgreget table -- don't send status.*/
        if (!tpzehin_send_status_when_retransmition() &&
			tpzehin_retransmition_enable() &&
            spzehin_count_msgreget_table(spzehin_db) > 0) {
                INFO("msgreget, don't send status.");
                return 0;
        }

        bool result = vPaasT_DataRealCustomDataToAPP(resp_str);
        _RefHandleRelease((void**)&status, NULL, NULL);
        if (!result) {
            ERROR("send status failed %s", resp_str);
            return -1;
        } else {
            INFO("send status success %s", resp_str);
            return 0;
        }
    }
    else
    {
        ERROR("zehin received spot status but device is offline, don't send.")
        return -1;
    }
}

/**
 * Name: tpzehin_record_handler
 * Description: send mq to record_handler thread.
 */
static int tpzehin_record_handler()
{
    int zehin_qid = msgget(0x7a656869, IPC_CREAT | 0666);
    zehin_msg_t zehin_msg = {0};
    zehin_msg.type = 200;
    zehin_msg.identity = 0; // TODO
    zehin_msg.with_picture = true;

	msgsnd(zehin_qid, &zehin_msg, sizeof(zehin_msg_t) - sizeof(long), IPC_NOWAIT);
    return 0;
}

/**
 * Name: tpzehin_info_handler
 * Description: send mq to record_handler thread.
 */
static int tpzehin_info_handler(void)
{
    int zehin_qid = msgget(0x7a656869, IPC_CREAT | 0666);
    zehin_msg_t zehin_msg = {0};
    zehin_msg.type = 200;
    zehin_msg.identity = 0; // TODO
    zehin_msg.with_picture = false;

	msgsnd(zehin_qid, &zehin_msg, sizeof(zehin_msg_t) - sizeof(long), IPC_NOWAIT);
    return 0;
}

/**
 * Name: tpzehin_alarm_handler
 * Description: send mq to alarm_handler thread.
 */
static int tpzehin_alarm_handler()
{
    int zehin_qid = msgget(0x7a656869, IPC_CREAT | 0666);
    zehin_msg_t zehin_msg = {0};
    zehin_msg.type = 100;
    zehin_msg.identity = 0; // TODO

	msgsnd(zehin_qid, &zehin_msg, sizeof(zehin_msg_t) - sizeof(long), IPC_NOWAIT);
    return 0;
}


/**
 * @brief zehin_record_test
 *
 * @param filename
 *
 * @return 0 OK otherwise fail
 */
static int zehin_record_test(const char* filename)
{
    if (NULL == filename) {
        ERROR("NULL == filename");
        return -1;
    }

    extern str_tpzehin_records gstr_tpzehin_records;
    str_tpzehin_records &records = gstr_tpzehin_records;

    memset(&records, 0x00, sizeof(str_tpzehin_records));

    FILE *fp = fopen(filename, "r");
    if (NULL == fp) {
        ERROR("open file %s failed.", filename);
        return -1;
    }

    char buff[512] = {0};
    fread(buff, 1, 512, fp);
    DEBUG("park_test %s", buff);

	pthread_mutex_lock(&g_zehin_record_mutex);
	char* p = strtok(buff, ",");
	strcpy(records.field.point_id, p);
	p = strtok(NULL, ",");
	strcpy(records.field.plate_num, p);
	p = strtok(NULL, ",");
	records.field.plate_color = atoi(p);
	p = strtok(NULL, ",");
	records.field.status = atoi(p);
	p = strtok(NULL, ",");
	records.field.confidence[0] = atoi(p);
	records.field.confidence[1] = atoi(p);
	p = strtok(NULL, ",");
	records.field.objectState = atoi(p);
	p = strtok(NULL, ",");
	strcpy(records.field.time, p);
	p = strtok(NULL, ",");
	strcpy(records.field.image_name[0], p);
	p = strtok(NULL, ",");
	strcpy(records.field.image_name[1], p);
	fclose(fp);

	FILE* pic0 = fopen(records.field.image_name[0], "r");
	records.picture_size[0] = fread(records.picture[0], 1, 1024*500, pic0);
	fclose(pic0);

	FILE* pic1 = fopen(records.field.image_name[0], "r");
	records.picture_size[1] = fread(records.picture[1], 1, 1024*500, pic1);
	fclose(pic1);

	pthread_mutex_unlock(&g_zehin_record_mutex);

    tpzehin_record_handler();
    return 0;
}

/**
 * Name       : tpzehin pthread function
 * Discription: The main thread for zehin platform.
 */
void* tpzehin_pthread(void* arg)
{
    set_thread("zehin_pthread");

	int msg_size = 0;

    if(tpzehin_provisioned()) {
        zehin_startup();
    }

	str_park_msg park_msg;
    while (true) {
        memset(&park_msg, 0x00, sizeof(str_park_msg));
        msg_size = msgrcv(MSG_PARK_ID, &park_msg,
                sizeof(str_park_msg) - sizeof(long), MSG_PARK_ZEHIN_TYPE, 0);
		if(msg_size < 0) {
			ERROR("%s: msgrcv error.");
			usleep(10000);
			continue;
		}

        int type = park_msg.data.zehin.type;
        char* data = park_msg.data.zehin.data;
        int lens = park_msg.data.zehin.lens;

        DEBUG("zehin receive %d message type = %d.", MSG_PARK_ID, type);

        switch (type) {
        case MSG_PARK_CONF:
            tpzehin_conf_handler(data, lens);
            break;
        case MSG_PARK_ALARM:
            tpzehin_alarm_handler();
            break;
        case MSG_PARK_RECORD:
            tpzehin_record_handler();
            break;
        case MSG_PARK_INFO:
            tpzehin_info_handler();
            break;
        case MSG_PARK_STATUS:
        {
            StatusPark *status = (StatusPark*)(data);
            tpzehin_status_handler(status);
            break;
        }
        case MSG_PARK_ZEHIN_CB:
        {
            /// now handle the cb in main thread, make it into a seperate thread
            /// if necessary.
            tpzehin_cb_handler(data, lens);
            break;
        }
        case MSG_PARK_SNAP:
        {
            capture_pic_upload((SnapRequestReturn*)&(data));
            break;
        }

        /* just for test */
        case 88:
        {
            int filename = (int)data;

            char name[16] = {0};
            sprintf(name, "%s_%d", "park_test", filename);
            zehin_record_test(name);
            break;
        }
        default:
            ERROR("type = %d is invalid.", type);
            break;
        }
    }
    return NULL;
}
