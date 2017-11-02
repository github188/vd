#include <string>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <signal.h>
#include <unistd.h>
#include <json/json.h>

#include "commontypes.h"
#include "commonfuncs.h"
#include "logger/log.h"
#include "ep_type.h"
#include "data_process.h"

#include "park_songli.h"
#include "songli_descending.h"
#include "songli_db_api.h"
#include "spsystem_api.h"
#include "park_file_handler.h"
#include "refhandle.h"

bool g_songli_platform_enable = false;
bool g_songli_pic_use_aliyun = false;
bool g_songli_upload_light_control = false;

template <typename TYPE, void (TYPE::*run_thread)()>
void * start_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->run_thread();
    return NULL;
}

template <typename TYPE, void (TYPE::*hb_thread)()>
void * park_songli_hb_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->hb_thread();
    return NULL;
}

template <typename TYPE, void (TYPE::*retrans_thread)()>
void * park_songli_retrans_thread(void* param)
{
    TYPE* This = (TYPE*)param;
    This->retrans_thread();
    return NULL;
}

CParkSongli::CParkSongli(void)
{
    pthread_mutex_init(&upload_mutex, NULL);
    pthread_mutex_init(&down_mutex, NULL);
    pthread_rwlock_init(&status_rwlock, NULL);

    m_mq_park_upload = NULL;
    m_mq_park_down = NULL;

	this->m_platform_type = EPlatformType::PARK_SONGLI;
    set_status(SONGLI_STATUS::ENABLE);
}

CParkSongli::~CParkSongli(void)
{
    if (get_upload_mq() != NULL) {
        get_upload_mq()->close();
        delete get_upload_mq();
    }
    if (get_down_mq() != NULL) {
        get_down_mq()->close();
        delete get_down_mq();
    }
}

int CParkSongli::songli_database_init()
{
	create_multi_dir("/home/records/park/songli/database/");
	songli_db_open("/home/records/park/songli/database/spsongli.db");

	//songli_db_create_configparam_tb(spzehin_db);
	//songli_db_create_lastmsg_tb(spzehin_db);
	songli_db_create_msgreget_tb(songli_db);
	//songli_db_create_alarmreget_tb(spzehin_db);
	songli_db_create_picreget_tb(songli_db);

	return 0;
}
void CParkSongli::run_thread(void)
{
    int ret = 0;
    set_thread("songli_pthread");

    songli_database_init();

    ret = pthread_create(&songli_hb_tid, NULL,
            park_songli_hb_thread<CParkSongli, &CParkSongli::hb_thread>, this);
    if (ret != 0) {
        ERROR("create park_songli_hb_thread failed, ret = %d", ret);
        return;
    }

    pthread_t songli_retrans_tid;
    ret = pthread_create(&songli_retrans_tid, NULL,
            park_songli_retrans_thread<CParkSongli, &CParkSongli::retrans_thread>, this);
    if (ret != 0) {
        ERROR("create park_songli_retrans_thread_thread failed, ret = %d", ret);
        return;
    }

	// main loop
	str_park_msg park_msg;
    while(true) {
        memset(&park_msg, 0x00, sizeof(str_park_msg));
        msgrcv(MSG_PARK_ID, &park_msg, sizeof(str_park_msg) - sizeof(long),
                                                   MSG_PARK_SONGLI_TYPE, 0);

        INFO("songli receive message type = %d.", park_msg.data.songli.type);

        switch(park_msg.data.songli.type)
        {
        case MSG_PARK_ALARM:
            alarm_handler((ParkAlarmInfo*)(park_msg.data.songli.data));
			break;
		case MSG_PARK_RECORD:
            record_handler((struct Park_record*)(park_msg.data.songli.data));
			break;
        case MSG_PARK_INFO:
            info_handler((struct Park_record*)(park_msg.data.songli.data));
            break;
        case MSG_PARK_LIGHT:
            light_handler((Lighting_park*)(park_msg.data.songli.data));
            break;
		default:
			break;
		}
    }
    return;
}

int CParkSongli::run(void)
{
    set_status(SONGLI_STATUS::READY);

    pthread_create(&tid, NULL,
            start_thread<CParkSongli, &CParkSongli::run_thread>, this);
	return 0;
}

int CParkSongli::stop(void)
{
    set_status(SONGLI_STATUS::STOPPED);
    pthread_cancel(tid);
    pthread_cancel(songli_hb_tid);
    pthread_cancel(songli_retrans_tid);
	return 0;
}

int CParkSongli::subscribe(void)
{
	this->m_platform_info.enable = true;
    if (get_status() < SONGLI_STATUS::ENABLE) {
        set_status(SONGLI_STATUS::ENABLE);
    }
	return 0;
}

int CParkSongli::unsubscribe(void)
{
	this->m_platform_info.enable = false;
    if ((get_status() > SONGLI_STATUS::DISABLE) &&
        (get_status() != SONGLI_STATUS::STOPPED)) {
           stop();
        }
    set_status(SONGLI_STATUS::DISABLE);
	return 0;
}

bool CParkSongli::subscribed(void)
{
	return (this->m_platform_info.enable);
}

int CParkSongli::notify(int type, void* data)
{
	str_park_msg park_msg = {0};
	switch(type) {
		case MSG_PARK_ALARM: {
			ParkAlarmInfo *parkAlarmInfo = (ParkAlarmInfo*)data;
            if (NULL == parkAlarmInfo) {
                ERROR("malloc parkAlarmInfo failed.");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.songli.type = MSG_PARK_ALARM;
            park_msg.data.songli.data = parkAlarmInfo;
            park_msg.data.songli.lens = sizeof(ParkAlarmInfo);
            break;
		}
		case MSG_PARK_LIGHT: {
			Lighting_park *light = (Lighting_park*)data;
            if (NULL == light) {
                ERROR("malloc parkAlarmInfo failed.");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.songli.type = MSG_PARK_LIGHT;
            park_msg.data.songli.data = light;
            park_msg.data.songli.lens = sizeof(Lighting_park);
            break;
		}
		case MSG_PARK_INFO:
		{
			struct Park_record *park_record = (struct Park_record *)data;
            if (NULL == park_record) {
                ERROR("malloc park_record failed.");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.songli.type = MSG_PARK_INFO;
            park_msg.data.songli.data = park_record;
            park_msg.data.songli.lens = sizeof(struct Park_record);
			break;
		}
		case MSG_PARK_RECORD:
		{
			struct Park_record *park_record = (struct Park_record *)data;
            if (NULL == park_record) {
                ERROR("malloc park_record failed.");
                return -1;
            }

            _RefHandleRetain(data);
            park_msg.data.songli.type = MSG_PARK_RECORD;
            park_msg.data.songli.data = park_record;
            park_msg.data.songli.lens = sizeof(struct Park_record);
			break;
		}
		default:
		{
			DEBUG("SONGLI no match handler for %d", type);
			return -1;
		}

	}
	vmq_sendto_songli(park_msg);
	return 0;
}


/*
 * 功能：MQ会话线程.
 * 参数：
 * 返回：
 */
void CParkSongli::start_mq_songli(void)
{
	std::string uri_broker;
	std::string uri_dest;

	char s_ip[128] = {0};

	/* ############  MQ参数配置       ################ */
	sprintf((char *) s_ip, "tcp://%d.%d.%d.%d:%d",
	        g_arm_config.basic_param.mq_param.ip[0],
	        g_arm_config.basic_param.mq_param.ip[1],
	        g_arm_config.basic_param.mq_param.ip[2],
	        g_arm_config.basic_param.mq_param.ip[3],
	        g_arm_config.basic_param.mq_param.port);

	uri_broker += "failover://(";
	uri_broker += s_ip;
	uri_broker += "?wireFormat=openwire";

	/* The maximum inactivity duration (before which the socket is considered
     * dead) in milliseconds. On some platforms it can take a long time for a
     * socket to appear to die, so we allow the broker to kill connections if
     * they are inactive for a period of time. Use by some transports to enable
     * a keep alive heart beat feature. Set to a value <= 0 to disable
     * inactivity monitoring.
     */
    /* 失效检测时间. 现在此参数还无法配置成功. */
	uri_broker += "&wireFormat.MaxInactivityDuration=30000";
    /*失效重连间隔时间.现在此参数还无法配置成功. */
	uri_broker += "&wireFormat.MaxInactivityDurationInitalDelay=10002";
	uri_broker += "&soKeepAlive=true";
	uri_broker += "&transport.useAsyncSend=true";
	uri_broker += "&transport.useInactivityMonitor=true";
	uri_broker += "&keepAliveResponseRequired=false";
	uri_broker += ")";

	/* ###############/Failover Transport Options/###############*/
	/* If a send is blocked waiting on a failed connection to reconnect how
     * long should it wait before failing the send, default is forever (-1).
     */
	uri_broker += "?timeout=10001";
    /* How long to wait if the initial attempt to
     * connect to the broker fails.
     */
	uri_broker += "&initialReconnectDelay=10";
    /* Maximum time that the transport waits before trying to
     * connect to the Broker again.
     */
	uri_broker += "&maxReconnectDelay=20003";
    /* Max number of times to attempt to reconnect before
     * failing the transport, default is forever (0).
     */
	uri_broker += "&maxReconnectAttempts=0";
    /* async */
	uri_broker += "&connection.useAsyncSend=true";
	//uri_broker += "&connection.alwaysSyncSend=false";

	/* Time to wait on Message Sends for a Response, default value of zero
     * indicates to wait forever.  Waiting forever allows the broker to have
     * flow control over messages coming from this client if it is a fast
     * producer or there is no consumer such that the broker would run out of
     * memory if it did not slow down the producer.
     */
	uri_broker += "&connection.sendTimeout=10002";

	/* The amount of time to wait for a response from the broker when shutting
     * down. Normally we want a response to indicate that the client has been
     * disconnected cleanly, but we don't want to wait forever, however if you
     * do, set this to zero.
     */
	uri_broker += "&connection.closeTimeout=10002";
	uri_broker += "";

	/* uri_broker += "&soKeepAlive=true"; */
	/* uri_broker += "";//")";//)";//+")"; */

    extern void mq_lib_init(void);
	mq_lib_init();

	/* ###########/泊车记录上传会话(跟松立平台的数据交互)/########### */
	uri_dest = URI_PARK_UPLOAD;

	/* 其他MQ 会话默认使用Topic，泊车上传使用Queue */
	set_upload_mq(new class_mq_producer(uri_broker, uri_dest,
            "松立平台MQ会话",false));

    uri_dest = URI_PARK_DOWN;
    set_down_mq(new class_mq_consumer(uri_broker, uri_dest,
            "PARK PLATFORM CONTROL PIPE", recv_msg_park_down));

	/* ###########/泊车参数上传会话/########### */
    /* If the mq can't run successfully, this thread would block here. */
	while (get_upload_mq()->run() != 1)
	{
		get_upload_mq()->close();
		sleep(5);
	}

	while (get_down_mq()->run() != FLG_MQ_TRUE)
	{
		get_down_mq()->close();
		sleep(5);
	}

    set_status(SONGLI_STATUS::START);
    return;
}

void CParkSongli::register2platform(void)
{
    extern SET_NET_PARAM g_set_net_param;
	NET_PARAM netparam;
	memcpy((char*) &netparam, (char*) &(g_set_net_param.m_NetParam),
			sizeof(NET_PARAM));
	char ipAddr[16];
	char collector[32];
	sprintf(ipAddr,"%d.%d.%d.%d", netparam.m_IP[0], netparam.m_IP[1],
			                      netparam.m_IP[2], netparam.m_IP[3]);

	snprintf(collector, EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);

	Json::Value child;
	Json::Value root;
	Json::FastWriter fast_writer;

	child["category"] = 4;                   // 1.电警  2.卡口  3.违停  4.泊车
	child["collector"] =  collector;         // 采集机关代码
	child["direction"] =  (int)EP_DIRECTION; //行驶方向
	child["ipAddr"] = ipAddr;                //设备IP
	child["version"] = g_SoftwareVersion;

	root["cmdCode"] = 1;
	root["devId"] =  g_SerialNum;            //产品序列号
	root["psId"] = g_arm_config.basic_param.spot_id; //泊位编号
	root["data"] = child;
	root["maker"] = "bitcom";

	std::string result_json = fast_writer.write(root);

    set_status(SONGLI_STATUS::REGISTERING);

    while(send_msg_2_songli_platform(result_json) != 0) {
        DEBUG("FELIXDU register to songli platform failed.");
        sleep(60);
    }
    set_status(SONGLI_STATUS::REGISTERED);
    INFO("FELIXDU songli register successfully.");
}

/**
 * @brief 3rd-platform record platform heartbeat
 * @param arg
 * @return
 */
void CParkSongli::songli_hb(void)
{
	char dataTime[32];
	memset(dataTime,0,sizeof(dataTime));

	while(true) {
		time_t seconds_time = time(NULL);
        struct tm time_now;
		localtime_r(&seconds_time,&time_now);
		snprintf(dataTime, sizeof(dataTime), "%04d-%02d-%02d %02d:%02d:%02d",
				 time_now.tm_year + 1900, time_now.tm_mon + 1, time_now.tm_mday,
				 time_now.tm_hour, time_now.tm_min, time_now.tm_sec);

		Json::Value child;
		Json::Value root;
		Json::FastWriter fast_writer;

		child["dataTime"] =  dataTime;

		root["cmdCode"] = 2;
		root["devId"] = g_SerialNum; //设备编号
		root["psId"] = g_arm_config.basic_param.spot_id;
		root["data"] = child;
		root["maker"] = "bitcom";

		std::string result = fast_writer.write(root);
        int ret = send_msg_2_songli_platform(result);
        if (ret == 0) {
            DEBUG("FELIXDU send songli hb success.");
        }
        else {
            DEBUG("FELIXDU send songli hb failed.");
        }
		usleep(60 * 1000 * 1000);
	}
}

/**
 * @brief hb_thread
 * it include start the mq including the producer and consumer;
 * register to the platform;
 * start the heartbeat timer to send heartbeat message.
 */
void CParkSongli::hb_thread(void)
{
	set_thread("songli_hb");
	start_mq_songli();
	register2platform();
    songli_hb();
}

/**
 * @brief retrans_thread
 */
void CParkSongli::retrans_thread(void)
{
    set_thread("songli_retrans");

    char* msg = (char*)malloc(sizeof(char) * 1024);
    if (NULL == msg) {
        ERROR("alloc retrans msg failed.");
        return;
    }

    size_t pic_buffer_size = sizeof(unsigned char) * 1024 * 500;
    unsigned char *pic_buf = (unsigned char*)malloc(pic_buffer_size);
    if (NULL == pic_buf) {
        ERROR("malloc pic_buf failed.");
        free(msg);
        return;
    }

    while (true) {
        usleep(1000 * 1000 * 10);

        if (!retrans_enable())
            continue;

        /*
         * record retrans.
         */
        do {
            int msg_num = 0;
            if ((msg_num = songli_db_count_msgreget_tb(songli_db)) <= 0)
                break; /* if there is no msg to reget, goto pic reget */

            memset(msg, 0x00, sizeof(char) * 1024);
            int msg_id = songli_db_select_msgreget_tb(songli_db, msg);
            INFO("songli retrans msgid = %d", msg_id);

            if (msg_id < 0)
                break; /* if there is no msg to reget, goto pic reget */

            std::string result = msg;
            int ret = park_send_record_songli(result);
            if (ret == 0) /* retrans successful. */ {
               songli_db_delete_msgreget_tb(songli_db, msg_id);
            }
        }
        while(0);

        /*
         * picture retrans.
         */
        int pic_num = 0;
        songli_picreget_tb picreget_tb;
        if ((pic_num = songli_db_count_picreget_tb(songli_db)) > 0) {
            int pic_id = songli_db_select_picreget_tb(songli_db, &picreget_tb);
            INFO("songli retrans pic_num = %d picid = %d", pic_num, pic_id);

            if (pic_id < 0)
                continue;

            char* pic_name = picreget_tb.pic_name;
            char* pic_path = picreget_tb.pic_path;
            size_t pic_size = picreget_tb.pic_size;
            memset(pic_buf, 0x00, pic_buffer_size);

            int pic_len = park_read_picture(pic_name, pic_buf, pic_size);
            if (pic_len < 0) {
                /* read the picture failed, delete the picreget from table. */
                songli_db_delete_picreget_tb(songli_db, pic_id);

                /* update spsystem picture */
                int ret = spsystem_check_picture_exist(spsystem_db, pic_name);
                if (ret > 0) {
                    spsystem_update_picture_table(spsystem_db, pic_name,
                                                  1, PARK_SONGLI);
                }
                continue;
            }

            EP_PicInfo picinfo;
            memset(&picinfo, 0x00, sizeof(EP_PicInfo));

            if (m_platform_info.pic_src == SONGLI_PIC_SRC::FTP) {
                sscanf(pic_path, "%[^/]/%[^/]/%[^/]/%[^/]/%[^/]/%[^/]",
                       picinfo.path[0], picinfo.path[1], picinfo.path[2],
                       picinfo.path[3], picinfo.path[4], picinfo.path[5]);
            } else if (m_platform_info.pic_src == SONGLI_PIC_SRC::ALIYUN) {
                strcat(picinfo.aliname, pic_path);
                strcat(picinfo.aliname, pic_name);
            }

            strcpy(picinfo.name, pic_name);
            picinfo.buf = pic_buf;
            picinfo.size = pic_size;

            int ret = park_send_picture_songli(&picinfo, 1);
            if (0 == ret) {
                songli_db_delete_picreget_tb(songli_db, pic_id);
                spsystem_update_picture_table(spsystem_db, pic_name,
                                              1, PARK_SONGLI);
                INFO("Park songli retrans picture successful.");
            } else {
            DEBUG("FELIXDU");
                INFO("Park songli retrans picture failed.");
            }
        }
    }

    free(msg);
    msg = NULL;
    free(pic_buf);
    pic_buf = NULL;
}

int CParkSongli::alarm_handler(ParkAlarmInfo* alarm)
{
    if (NULL == alarm) {
        ERROR("alarm is null.");
        _RefHandleRelease((void**)&alarm, NULL, NULL);
        return -1;
    }

    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    char alarm_time[32] = {0};
    struct tm park_alarm_time;
    localtime_r((const time_t*)&(alarm->alarmTime), &park_alarm_time);

    snprintf(alarm_time, sizeof(struct tm), "%04d-%02d-%02d %02d:%02d:%02d",
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
    send_msg_2_songli_platform(result_json);
    _RefHandleRelease((void**)&alarm, NULL, NULL);
    return 0;
}

int CParkSongli::record_handler(struct Park_record* record)
{
    if (NULL == record) {
        ERROR("NULL is data!");
        _RefHandleRelease((void**)&record, NULL, NULL);
        return -1;
    }

    DB_ParkRecord *db_park_record = &(record->db_park_record);
    if (NULL == db_park_record) {
        ERROR("NULL is db_park_record!");
        _RefHandleRelease((void**)&record, NULL, NULL);
        return -1;
    }

    char plateFeature[512];
    char photo[256];
    char photo1[256];
    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    sprintf(plateFeature, "%d/%d/%d/%d/%d/%d/%d/%d",
            db_park_record->coordinate_x[0],
            db_park_record->coordinate_y[0],
            db_park_record->width[0],
            db_park_record->height[0],
            db_park_record->coordinate_x[1],
            db_park_record->coordinate_y[1],
            db_park_record->width[1],
            db_park_record->height[1]);

    if(this->m_platform_info.pic_src == SONGLI_PIC_SRC::ALIYUN) {
        sprintf(photo, "%s", db_park_record->aliyun_image_path[0]);
        sprintf(photo1, "%s", db_park_record->aliyun_image_path[1]);
    } else {
        sprintf(photo,"ftp://%s:%s@%s:%d/%s%s",
                db_park_record->ftp_user,
                db_park_record->ftp_passwd,
                db_park_record->ftp_ip,
                db_park_record->ftp_port,
                db_park_record->image_path[0],
                db_park_record->image_name[0]);
        sprintf(photo1,"ftp://%s:%s@%s:%d/%s%s",
                db_park_record->ftp_user,
                db_park_record->ftp_passwd,
                db_park_record->ftp_ip,
                db_park_record->ftp_port,
                db_park_record->image_path[1],
                db_park_record->image_name[1]);
    }

    child["numplate"] = db_park_record->plate_num; //车牌号
    child["status"] = db_park_record->status;
    child["confidence"] = db_park_record->confidence[0]; //置信度

    char plate_color[3] = {0};
    snprintf(plate_color, 3, "%02d", db_park_record->plate_color);

    /*
     * 车牌颜色
     * 符合《GA 24.7-2005 机动车登记信息代码 第7部分：号牌种类代码》编码规范
     * 00	未识别
     * 01	大型汽车 黄底黑字
     * 02	小型汽车 蓝底白字
     * ...
     * 99	其他
     */

    child["color"] = plate_color;
    child["plateFeature"] = plateFeature;    /* 车牌特征信息 车牌宽度,长度等 */
    child["photo"] = photo;                  /* 图片路径 */
    child["photo1"] = photo1;                /* 图片路径 */
    child["dataTime"] = db_park_record->time; /* 车辆驶入或者驶出时间 */

    if (1 == db_park_record->objectState)
        root["cmdCode"] = 3;
    else if (0 == db_park_record->objectState)
        root["cmdCode"] = 4;

    root["devId"] = g_SerialNum;             /* 设备编号 */
    root["psId"] = db_park_record->point_id;  /* 泊位号 */
    root["data"] = child;

    std::string result = fast_writer.write(root);

    char *content;
    int len = result.size() * 2;
    content = (char*) malloc(len);
    memset(content, 0x00, len);

    convert_enc("GBK", "UTF-8", result.data(), result.size(), content, len);
    result = content;

    if (park_send_record_songli(result) == -1) {
        INFO("Send record failed. save into databse.");
        size_t msg_tb_size = sizeof(songli_msgreget_tb);
        songli_msgreget_tb *msg = (songli_msgreget_tb*)malloc(msg_tb_size);
        if (NULL == msg) {
            ERROR("malloc msg failed.");
            _RefHandleRelease((void**)&record, NULL, NULL);
            return -1;
        }

        if (strlen(content) + 1 > MSG_LENGTH) {
            ERROR("content size exceed %d %d", strlen(content) + 1, MSG_LENGTH);
        }

        memcpy(msg->message, content, sizeof(msg->message));
        songli_db_insert_msgreget_tb(songli_db, *msg);
        free(msg);
        msg = NULL;

        /* if the record message send failed, save both two images. */
        EP_PicInfo *picinfo = record->pic_info;
        save_picture4retrans(picinfo, 2);
    } else {
        INFO("Send record successfully. begin to send image.");
        EP_PicInfo *picinfo = record->pic_info;
        int ret = park_send_picture_songli(picinfo, 2);
        if (ret < 0) {
            INFO("Send image failed. save into databse.");
            if (retrans_enable()) {
                if ((ret == -11) || (ret == -1)) {
                    save_picture4retrans(picinfo, 2);
                } else if (ret == -12) { /* only the secord pic. */
                    save_picture4retrans(picinfo + 1, 1);
                }
            }
        }
    }
    free(content);
    DEBUG("_____________________________________");
    _RefHandleRelease((void**)&record, NULL, NULL);
    return 0;
}

int CParkSongli::info_handler(struct Park_record* record)
{
    _RefHandleRelease((void**)&record, NULL, NULL);
    return 0;
}

int CParkSongli::light_handler(Lighting_park *light_info)
{
    Json::Value child;
    Json::Value root;
    Json::FastWriter fast_writer;

    unsigned char light_flag = 0;
    unsigned char *p = (unsigned char*)(light_info);
    for(int i = 0; i < 6; i++) {
        light_flag |= (((*p) > 0) ? 1 : 0) << (6 - i - 1);
        p++;
    }

    map<unsigned char, string> map_light;
    map_light[0x20] = "g";
    map_light[0x30] = "gf";
    map_light[0x08] = "r";
    map_light[0x0C] = "rf";
    map_light[0x02] = "b1";
    map_light[0x03] = "bf1";

    if(map_light[light_flag].empty()) {
        INFO("This light action don't need to sendto platform");
        return 0;
    }

    child["light"] = map_light[light_flag];

    time_t now;
    struct tm *time_now;

    time(&now);
    time_now = localtime(&now);

    char time_format[20] = {0};
    sprintf(time_format, "%04d-%02d-%02d %02d:%02d:%02d",
            time_now->tm_year + 1900, time_now->tm_mon + 1,
            time_now->tm_mday, time_now->tm_hour,
            time_now->tm_min, time_now->tm_sec);

    child["stateTime"] = time_format;

    root["cmdCode"] = 11;
    root["devId"] = g_SerialNum;         //设备编号
    root["psId"] = EP_POINT_ID;         //泊位号
    root["data"] = child;

    std::string result = fast_writer.write(root);

    send_msg_2_songli_platform(result);
    _RefHandleRelease((void**)&light_info, NULL, NULL);
    return 0;
}

int CParkSongli::park_send_record_songli(string &result)
{
    return send_msg_2_songli_platform(result);
}

/**
 * @brief send_msg_2_songli_platform
 * @param msg: text to send
 * @return 0 OK -1 fail
 */
int CParkSongli::send_msg_2_songli_platform(string &msg)
{
    INFO("[songli-info]:%s processid = %d thread_id = %d",
            msg.c_str(), getpid(), pthread_self());

	extern SemHandl_t park_sem;
    SemWait(park_sem);

	do {
		if (NULL == get_upload_mq()) {
            DEBUG("NULL is get_upload_mq");
            break;
		}

		auto mq_instance = get_upload_mq();
		if (FLG_MQ_FALSE == mq_instance->get_mq_conn_state()) {
			if (mq_instance->run() != 1) {
				mq_instance->close();
                DEBUG("run mq failed.");
                break;
			}
		}

        if (FLG_MQ_FALSE == mq_instance->send_msg_text_string(msg)) {
            INFO("send message to songli platform failed.");
            break;
        }

		mq_instance->close();
        SemRelease(park_sem);
        return 0;
	} while(0);
    SemRelease(park_sem);
	return -1;
}

int CParkSongli::save_picture4retrans(EP_PicInfo pic_info[], const int pic_num)
{
    songli_picreget_tb picreget_tb = {0};
    for (int i = 0; i < pic_num; i++) {

        /*
         * If there are more than rotate_count pictures in databse, delete the
         * oldest one then save the newer one.
         */
        int rotate_count = m_platform_info.rotate_count;
        if (songli_db_count_picreget_tb(songli_db) >= rotate_count) {

            int ret = songli_db_select_picreget_tb(songli_db, &picreget_tb);
            if (ret > 0) {

                INFO("Delete record picture %s up to limit %d",
                        picreget_tb.pic_name, rotate_count);
                songli_db_delete_picreget_tb(songli_db, ret);

                /* update spsystem picture */
                ret = spsystem_check_picture_exist(spsystem_db,
                        picreget_tb.pic_name);
                if (ret > 0) {
                    spsystem_update_picture_table(spsystem_db,
                            picreget_tb.pic_name, 1, PARK_SONGLI);
                }
            }
        }

        EP_PicInfo * pic = &(pic_info[i]);
        memset(&picreget_tb, 0x00, sizeof(songli_picreget_tb));
        picreget_tb.pic_size = pic->size;

        for(int j = 0; j < EP_FTP_URL_LEVEL.levelNum; j++) {
            if (m_platform_info.pic_src == SONGLI_PIC_SRC::FTP) {
                strcat(picreget_tb.pic_path, pic->path[j]);
                strcat(picreget_tb.pic_path, "/");
            } else if (m_platform_info.pic_src == SONGLI_PIC_SRC::ALIYUN) {
                strcat(picreget_tb.pic_path, pic->alipath[j]);
            }
        }

        sprintf(picreget_tb.pic_name, "%s", pic->name);

        DEBUG("saved image for retrans %s", picreget_tb.pic_name);
        int ret = songli_db_insert_picreget_tb(songli_db, picreget_tb);
        if (ret == 0) {
            ret = spsystem_check_picture_exist(spsystem_db, pic->name);
            if (ret > 0) {
                spsystem_update_picture_table(spsystem_db, pic->name,
                                              0, PARK_SONGLI);
            } else {
                char values[12][128] = {{0}};
                strcpy(values[1], pic->name);
                strcpy(values[3], "1");
                strcpy(values[4], "1");
                strcpy(values[5], "0");
                strcpy(values[6], "1");

                spsystem_insert_picture_table(spsystem_db, values);
                park_save_picture(pic->name, (unsigned char*)pic->buf, pic->size);
            }
        }
    }
    return 0;
}

int CParkSongli::park_send_picture_songli(EP_PicInfo pic_info[],
                                          const int pic_num)
{
    if ((NULL == pic_info) || (pic_num <= 0))
        return -1;

    int ret = 0;
    switch (this->m_platform_info.pic_src) {
        case SONGLI_PIC_SRC::FTP:
            ret = send_park_records_image_buf(pic_info, pic_num);
            break;
        case SONGLI_PIC_SRC::ALIYUN:
            ret = send_park_records_image_aliyun(pic_info, pic_num);
            break;
        default:
            break;
    }
    return ret;
}

int CParkSongli::set_platform_info(void *p)
{
	memcpy(&m_platform_info, p, sizeof(songli_platform_info_t));
	return 0;
}

int CParkSongli::set_platform_info(const Json::Value & root)
{
	if (root["platform_type"].asString() != "songli")
	{
		ERROR("json configuration %s is not for songli platform.",
				root["platform_type"].asString().c_str());
		return -1;
	}

	m_platform_info.enable =
		root["enable"].asString() == "yes" ? true : false;
    m_platform_info.retransmition_enable =
		root["attribute"]["retransmition_enable"].asString() == "yes" ? true : false;
    m_platform_info.rotate_count =
		atoi(root["attribute"]["rotate_count"].asString().c_str());

	if(root["attribute"]["upload_light_action"].asString() == "yes")
	{
		m_platform_info.upload_light_action = true;
		g_songli_upload_light_control = true;
	}
	else if(root["attribute"]["upload_light_action"].asString() == "no")
	{
		m_platform_info.upload_light_action = false;
		g_songli_upload_light_control = false;
	}

	if(root["attribute"]["pic_src"].asString() == "ftp")
	{
		m_platform_info.pic_src = SONGLI_PIC_SRC::FTP;
		g_songli_pic_use_aliyun = false;
	}
	else if(root["attribute"]["pic_src"].asString() == "aliyun")
	{
		m_platform_info.pic_src = SONGLI_PIC_SRC::ALIYUN;
		g_songli_pic_use_aliyun = true;
	}

	return 0;
}

int CParkSongli::set_platform_info(const string & json_str)
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

void * CParkSongli::get_platform_info(void)
{
	return &(this->m_platform_info);
}

// return the json string of the configuration.
string CParkSongli::get_platform_info_str(void)
{
	Json::Value root = get_platform_info_json();
	Json::FastWriter writer;

	return writer.write(root);
}

// return the json string of the configuration.
Json::Value CParkSongli::get_platform_info_json(void)
{
	Json::Value root;
	Json::Value attribute;

	songli_platform_info_t *platform_info =
		(songli_platform_info_t*)get_platform_info();

	char rotate_count[4] = {0};
	sprintf(rotate_count, "%d", platform_info->rotate_count);

	root["platform_type"] = "songli";
	root["enable"] = platform_info->enable ? "yes" : "no";

	attribute["pic_src"] = (platform_info->pic_src == SONGLI_PIC_SRC::FTP) ? "ftp" : "aliyun";
	attribute["retransmition_enable"] = platform_info->retransmition_enable ? "yes" : "no";
	attribute["upload_light_action"] = platform_info->upload_light_action ? "yes" : "no";
	attribute["rotate_count"] = rotate_count;
	root["attribute"] = attribute;
	return root;
}
