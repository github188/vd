#include "json/json.h"
#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sqlite3.h>
#include <cstring>
#include <string>

#include "logger/log.h"
#include "http_common.h"
#include "http_pthread.h"
#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "dsp_config.h"
#include "global.h"
#include "data_process.h"

#include "park_util.h"
#include "park_timer.h"
#include "park_file_handler.h"

#include "interface_alg.h"
#include "http_db_api.h"
#include "http_descending.h"
#include "spsystem_api.h"

#include "mycurl.h"
#include "refhandle.h"


extern char g_SerialNum[50];
extern char g_SoftwareVersion[50];

static timer_t g_http_hb_timer = NULL;

static void* http_response_cb(void* args)
{
	if (NULL == args) {
		ERROR("NULL == args");
		return NULL;
	}

	struct curl_http_args_st *p = (struct curl_http_args_st*)args;
	std::string url {p->url};
	if (url.find("heartBeat") != std::string::npos) {
		http_recv_msg_park_down(p->resp_data);
	}
}

static int http_database_init()
{
	create_multi_dir("/home/records/park/http/database/");
	http_db_open("/home/records/park/http/database/sphttp.db");

	//http_db_create_configparam_tb(spzehin_db);
	//http_db_create_lastmsg_tb(spzehin_db);
	http_db_create_msgreget_tb(http_db);
	//http_db_create_alarmreget_tb(spzehin_db);
	http_db_create_picreget_tb(http_db);

	return 0;
}

static int http_save_picture4retrans(EP_PicInfo pic_info[], const int pic_num);

static int send_record2platform(const char* url, void* body,
        const size_t lens, const char* space_code, http_resp_cb cb = 0)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
    curl_args.curl_method = CURL_METHOD_POST;
    curl_args.url = (char*)url;
    curl_args.post_data = (char*)body;
    curl_args.post_lens = lens;
    return curl_http_post(&curl_args, 2, (void*)space_code, cb);
}

static int send_info2platform(const char* url, const char* body,
        const size_t lens, const char* space_code, http_resp_cb cb = 0)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
    curl_args.curl_method = CURL_METHOD_POST;
    curl_args.url = (char*)url;
    curl_args.post_data = (char*)body;
    curl_args.post_lens = lens;
    return curl_http_post(&curl_args, 1, (void*)space_code, cb);
}

/*
 * Name: http_record_handler
 */
static int http_record_handler(struct Park_record* data)
{
    if (NULL == data) {
        ERROR("NULL is data!");
        _RefHandleRelease((void**)&data, NULL, NULL);
        return -1;
    }

    DB_ParkRecord *db_park_record = &(data->db_park_record);
    if (NULL == db_park_record) {
        ERROR("NULL is db_park_record!");
        _RefHandleRelease((void**)&data, NULL, NULL);
        return -1;
    }

    EP_PicInfo *picinfo = NULL;
    if (data->pic_info[0].name[0] != '\0') {
        picinfo = data->pic_info;
    }

    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    char plateFeature[128] = {0};
    sprintf(plateFeature, "%d/%d/%d/%d/%d/%d/%d/%d",
                          db_park_record->coordinate_x[0],
                          db_park_record->coordinate_y[0],
                          db_park_record->width[0],
                          db_park_record->height[0],
                          db_park_record->coordinate_x[1],
                          db_park_record->coordinate_y[1],
                          db_park_record->width[1],
                          db_park_record->height[1]);

    child["numplate"] = db_park_record->plate_num;		 //车牌号
    child["status"] = db_park_record->status;
    child["confidence"] = db_park_record->confidence[0]; //置信度

    char plate_color[3] = {0};
    sprintf(plate_color, "%02d", db_park_record->plate_color);
    child["color"] = plate_color; //车牌颜色

    child["plateFeature"] = plateFeature;			//车牌特征信息 车牌宽度,长度

    if (picinfo != NULL) {
        child["photo"] = picinfo[0].name;		//图片路径
        child["photo1"] = picinfo[1].name;		//图片路径
    } else {
        child["photo"] = "";		//图片路径
        child["photo1"] = "";		//图片路径
    }

    child["dataTime"] = db_park_record->time;	    //车辆驶入或者驶出时间

	if (db_park_record->objectState == 1) // in
        root["cmdCode"] = 3;
    else if (db_park_record->objectState == 0) //out
        root["cmdCode"] = 4;
    root["devId"] = g_SerialNum;         //设备编号
    root["psId"] = db_park_record->point_id;         //泊位号
    root["data"] = child;

    std::string result_json = fast_writer.write(root);

    int len;
    char *content;
    len = result_json.size() * 2;
    content = (char*) malloc(len);
    memset(content, 0x00, len);

    convert_enc("GBK", "UTF-8", result_json.data(),
			result_json.size(),content, len);

    body_data_t body_data = {0};
    body_data.json_data = content;
    body_data.json_lens = strlen(content);
    body_data.pic = data->pic_info;

    char url[256] = "";
	if (db_park_record->objectState == 1) // in
        sprintf(url, "%s%s", http_get_url(), "/api/v1/park/driveIn");
    else if (db_park_record->objectState == 0) //out
        sprintf(url, "%s%s", http_get_url(), "/api/v1/park/driveOut");

#if 0
	if (!http_is_online()) {
        INFO("http offline don't send record");
        size_t msg_tb_size = sizeof(http_msgreget_tb);
        http_msgreget_tb *msg = (http_msgreget_tb*)malloc(msg_tb_size);
        if (NULL == msg) {
            ERROR("malloc msg failed.");
            _RefHandleRelease((void**)&data, NULL, NULL);
            return -1;
        }

        if (strlen(content) + 1 > MSG_LENGTH) {
            ERROR("content size exceed %d %d", strlen(content) + 1, MSG_LENGTH);
        }

        memcpy(msg->message, content, sizeof(msg->message));
        http_db_insert_msgreget_tb(http_db, *msg);
        free(msg);
        msg = NULL;

        /* save picture for retrans. */
        if (picinfo != NULL) {
            http_save_picture4retrans(picinfo, 2);
        }

		free(content);
		DEBUG("___________________________________");
		_RefHandleRelease((void**)&data, NULL, NULL);
		return 0;
	}
#endif

    if (send_record2platform(url, &body_data,
				sizeof(body_data_t), db_park_record->point_id) == -1) {
        INFO("Send record failed. save into database.");
        size_t msg_tb_size = sizeof(http_msgreget_tb);
        http_msgreget_tb *msg = (http_msgreget_tb*)malloc(msg_tb_size);
        if (NULL == msg) {
            ERROR("malloc msg failed.");
            _RefHandleRelease((void**)&data, NULL, NULL);
            return -1;
        }

        if (strlen(content) + 1 > MSG_LENGTH) {
            ERROR("content size exceed %d %d", strlen(content) + 1, MSG_LENGTH);
        }

        memcpy(msg->message, content, sizeof(msg->message));
        http_db_insert_msgreget_tb(http_db, *msg);
        free(msg);
        msg = NULL;

        /* save picture for retrans. */
        if (picinfo != NULL) {
            http_save_picture4retrans(picinfo, 2);
        }
    }

    free(content);
    DEBUG("___________________________________");
    _RefHandleRelease((void**)&data, NULL, NULL);
    return 0;
}

static int http_save_picture4retrans(EP_PicInfo pic_info[], const int pic_num)
{
    http_picreget_tb picreget_tb = {0};
    for (int i = 0; i < pic_num; i++) {

        /*
         * If there are more than rotate_count pictures in databse, delete the
         * oldest one then save the newer one.
         */
        if (http_db_count_picreget_tb(http_db) >= http_rotate_count()) {

            int pic_id = http_db_select_picreget_tb(http_db, &picreget_tb);
            if (pic_id > 0) {
                INFO("Delete record picture %s up to limit %d",
                        picreget_tb.pic_name, http_rotate_count());
                http_db_delete_picreget_tb(http_db, pic_id);

                /* update spsyatem picture */
                int ret = spsystem_check_picture_exist(spsystem_db,
                        picreget_tb.pic_name);
                if (ret > 0) {
                    spsystem_update_picture_table(spsystem_db,
                            picreget_tb.pic_name, 1, PARK_HTTP);
                }
            }
        }

        EP_PicInfo * pic = &(pic_info[i]);
        memset(&picreget_tb, 0x00, sizeof(http_picreget_tb));
        picreget_tb.pic_size = pic->size;
        sprintf(picreget_tb.pic_name, "%s", pic->name);

        DEBUG("saved image for retrans %s", pic->name);
        int ret = http_db_insert_picreget_tb(http_db, picreget_tb);
        if (ret == 0) {
            ret = spsystem_check_picture_exist(spsystem_db, pic->name);
            if(ret > 0) {
                spsystem_update_picture_table(spsystem_db, pic->name,
                        0, PARK_HTTP);
            } else {
                char values[12][128] = {{0}};
                strcpy(values[1], pic->name);
                strcpy(values[3], "1");
                strcpy(values[4], "1");
                strcpy(values[5], "1");
                strcpy(values[6], "0");

                spsystem_insert_picture_table(spsystem_db, values);
                park_save_picture(pic->name, (unsigned char*)pic->buf, pic->size);
            }
        }
    }
    return 0;
}

static int http_info_handler(struct Park_record* data)
{
    http_record_handler(data);
    return 0;
}


/*
 * Name: http_alarm_handler
 * Description:
 */
static int http_alarm_handler(ParkAlarmInfo* alarm)
{
    if (NULL == alarm) {
        ERROR("alarm is null.");
        _RefHandleRelease((void**)alarm, NULL, NULL);
        return -1;
    }
    /* construct json string */
    /* construct common header */

    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    char alarm_time[32] = {0};
    struct tm park_alarm_time;
    localtime_r((const time_t*)&(alarm->alarmTime), &park_alarm_time);

    snprintf(alarm_time, sizeof(alarm_time), "%04d-%02d-%02d %02d:%02d:%02d",
             park_alarm_time.tm_year + 1900, park_alarm_time.tm_mon + 1,
             park_alarm_time.tm_mday, park_alarm_time.tm_hour,
             park_alarm_time.tm_min, park_alarm_time.tm_sec);

    child["category"] = alarm->category;
    child["level"] = alarm->level;
    child["alarmTime"] = alarm_time;

    root["cmdCode"] = 5;
    root["devId"] = g_SerialNum; //产品序列号
    root["psId"] = g_arm_config.basic_param.spot_id; //泊位编号
    root["data"] = child;

    std::string result_json = fast_writer.write(root);

#if 0
	if (!http_is_online()) {
		INFO("http is offline, alarm abort.");
		return 0;
	}
#endif

    char url[256] = "";
    sprintf(url, "%s%s", http_get_url(), "/api/v1/park/alarm");
    send_info2platform(url, result_json.c_str(),
            result_json.length() + 1, g_arm_config.basic_param.spot_id);
    _RefHandleRelease((void**)&alarm, NULL, NULL);
    return 0;
}

void park_http_register()
{
    /* construct json string */
    /* construct common header */
	char ipAddr[16];
	char collector[32];

	NET_PARAM netparam;
	memcpy((char*) &netparam,
		   (char*) &(g_set_net_param.m_NetParam),
			sizeof(NET_PARAM));
	sprintf(ipAddr,"%d.%d.%d.%d", netparam.m_IP[0], netparam.m_IP[1],
                                  netparam.m_IP[2], netparam.m_IP[3]);
	snprintf(collector, EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);

	Json::Value child;
	Json::Value root;
	Json::FastWriter fast_writer;

	child["category"] = 1;
	child["collector"] =  collector;
	child["direction"] =  (int)EP_DIRECTION;
	child["ipAddr"] = ipAddr;
	child["version"] = g_SoftwareVersion;

	root["cmdCode"] = 1;
	root["devId"] =  g_SerialNum; //产品序列号
	root["psId"] = g_arm_config.basic_param.spot_id;
	root["data"] = child;
	root["maker"] = "bitcom";

	std::string result_json = fast_writer.write(root);
    INFO("register http platform. %s", result_json.c_str());

    /* construct url */
    char url[256] = "";
    sprintf(url, "%s%s", http_get_url(), "/api/v1/park/register");
    while (send_info2platform(url, result_json.c_str(),
                result_json.length() + 1, g_arm_config.basic_param.spot_id)) {
        ERROR("register park http platform failed.");
        sleep(60);
    }
    set_http_online(true);
}

static void park_http_hb_cb(union sigval sig)
{
	Json::Value child;
	Json::Value root;
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

	child["dataTime"] = time_format;

	root["cmdCode"] = 2;
	root["devId"] =  g_SerialNum; //产品序列号
	root["psId"] = g_arm_config.basic_param.spot_id;
	root["data"] = child;
	root["maker"] = "bitcom";

	std::string result_json = fast_writer.write(root);
    INFO("send http heartbeat %s", result_json.c_str());
    char url[256] = "";
    sprintf(url, "%s%s", http_get_url(), "/api/v1/park/heartBeat");
    send_info2platform(url, result_json.c_str(), result_json.length() + 1,
			g_arm_config.basic_param.spot_id, http_response_cb);
	//start the timer again.
	park_set_timer(&g_http_hb_timer, http_hb_interval() * 1000);
}

/*
 * Name       : park_http_hb_pthread
 * Discription: The hb thread for http post platform.
 */
void* park_http_hb_pthread(void*)
{
    set_thread("http_hb");
    park_http_register(); /* maybe blocked until register successful. */
    park_create_timer(&g_http_hb_timer, park_http_hb_cb, (int)0);
    park_set_timer(&g_http_hb_timer, 200);
    return NULL;
}

/*
 * Name       : park_http_retrans_pthread
 * Discription: The hb thread for http post platform.
 */
void* park_http_retrans_pthread(void*)
{
    set_thread("http_retrans");

    char* msg = (char*)malloc(sizeof(char) * 1024);
    if (NULL == msg) {
        ERROR("alloc retrans msg failed.");
        return NULL;
    }

    size_t pic_buffer_size = sizeof(unsigned char) * 1024 * 500;
    unsigned char *pic_buf = (unsigned char*)malloc(pic_buffer_size * 2);
    if (NULL == pic_buf) {
        ERROR("malloc pic_buf failed.");
        free(msg);
        return NULL;
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

        if (!http_retransmition_enable())
            continue;

        int msg_num = 0;
        if ((msg_num = http_db_count_msgreget_tb(http_db)) <= 0)
            continue;

        memset(msg, 0x00, sizeof(char) * 1024);
        int msg_id = http_db_select_msgreget_tb(http_db, msg);
        INFO("http retrans msgid = %d", msg_id);

        if (msg_id < 0)
            continue;

        /* get the picture name from msg */
        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(msg, root))
        {
            ERROR("Parse json failed. msg = %s", msg);
            return NULL;
        }

        body_data_t body_data = {0};
        body_data.json_data = msg;
        body_data.json_lens = strlen(msg);

        for (auto i = 0; i < 2; i++) {
            string pic_name = "";

            if (i == 0)
                pic_name = root["data"]["photo"].asString();
            else if (i == 1)
                pic_name = root["data"]["photo1"].asString();

            size_t pic_len = 0;

            if (pic_name != "") {
                const char* picname = pic_name.c_str();
                pic_len = park_read_picture(picname, (unsigned char*)(picinfo[i].buf), 0);
                if (pic_len < 0) {
                    /* read the picture failed, delete the picreget from table. */
                    http_db_delete_picreget_tb(http_db, picname);

                    /* update spsystem picture */
                    int ret = spsystem_check_picture_exist(spsystem_db, picname);
                    if (ret > 0) {
                        spsystem_update_picture_table(spsystem_db, picname,
                                1, PARK_HTTP);
                    }
                    pic_name = "";
                }
            }
            memset(picinfo[i].name, 0x00, sizeof(picinfo[i].name));
            memcpy(picinfo[i].name, pic_name.c_str(), pic_name.size());
            picinfo[i].size = pic_len;
        }

        body_data.pic = picinfo;

        char url[256] = "";
        if (root["cmdCode"].asInt() == 3) // in
            sprintf(url, "%s%s", http_get_url(), "/api/v1/park/driveIn");
        else if (root["cmdCode"].asInt() == 4) //out
            sprintf(url, "%s%s", http_get_url(), "/api/v1/park/driveOut");

        if (send_record2platform(url, &body_data, sizeof(body_data_t),
					root["psId"].asString().c_str()) == 0) {
            http_db_delete_msgreget_tb(http_db, msg_id);

            int ret = 0;
            for (auto i = 0; i < 2; i++) {
                if (picinfo[0].size > 0) {
                    ret = http_db_delete_picreget_tb(http_db, picinfo[i].name);
                    /* update spsystem picture */
                    ret = spsystem_check_picture_exist(spsystem_db, picinfo[i].name);
                    if (ret > 0) {
                        spsystem_update_picture_table(spsystem_db, picinfo[i].name,
                                1, PARK_HTTP);
                    }
                }
            }
        }
    }

    free(msg);
    msg = NULL;
    free(pic_buf);
    pic_buf = NULL;
    return NULL;
}

/*
 * Name       : http pthread function
 * Discription: The main thread for http post platform.
 */
void* park_http_platform_pthread(void* arg)
{
	int ret = 0;
    set_thread("park_http");

    http_database_init();
    pthread_t http_hb_tid;
    ret = pthread_create(&http_hb_tid, NULL, park_http_hb_pthread, NULL);
    if (ret != 0) {
        ERROR("create park_http_hb_pthread failed. ret = %d", ret);
        return NULL;
    }

    pthread_t http_retrans_tid;
    ret = pthread_create(&http_retrans_tid, NULL,
                         park_http_retrans_pthread, NULL);
    if (ret != 0) {
        ERROR("create park_http_retrnas_pthread failed. ret = %d", ret);
        return NULL;
    }

	str_park_msg park_msg;
    while(true) {
        memset(&park_msg, 0x00, sizeof(str_park_msg));
        ret = msgrcv(MSG_PARK_ID, &park_msg,
                     sizeof(str_park_msg) - sizeof(long),
                     MSG_PARK_HTTP_TYPE, 0);
		if(ret < 0) {
			ERROR("%s: msgrcv error ",__func__);
			usleep(10000);
			continue;
		}

        INFO("HTTP recv message %d", park_msg.data.http.type);

        void* data = park_msg.data.http.data;

#if 0
        if (!http_is_online()) {
            INFO("http receive msg %d but offline.", park_msg.data.http.type);
            _RefHandleRelease((void**)&data, NULL, NULL);
            continue;
        }
#endif

        switch(park_msg.data.http.type) {
        case MSG_PARK_ALARM:
            http_alarm_handler((ParkAlarmInfo*)data);
            break;
        case MSG_PARK_RECORD:
            http_record_handler((struct Park_record*)data);
            break;
        case MSG_PARK_INFO:
            http_info_handler((struct Park_record*)data);
            break;
        default:
            ERROR("type = %d is invalid.", park_msg.data.http.type);
            break;
        }
    }
    return NULL;
}
