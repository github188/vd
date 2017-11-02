#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "storage_common.h"
#include "data_process.h"
#include "database_mng.h"
#include "park_records_process.h"
#include "ftp.h"

#include "platform.h"
#include "logger/log.h"

#include "oss_interface.h"
#include "vd_msg_queue.h"
#include "tpzehin_common.h"
#include "tpzehin_records.h"
#include "tpbitcom_records.h"
#include "spsystem_api.h"

#include "refhandle.h"

#include "ve/dev/light.h"

#include "park_file_handler.h"
#include "park_status.h"
#include "light_ctl_api.h"

#define BEIJING_KTETC

bool tpzehin_provisioned(void);

static const char * alarm_str_map[] = {"",
                                       "[1] 不规范停车",
                                       "[2] 视频遮挡",
                                       "[3] 车位非法占用",
                                       "[4] 车牌置信度低",
                                       "[5] 遮挡号牌"};

/**
 * @brief get the message's virtual address.
 *
 * @param msg_alg_result_info - msg from dsp.
 *
 * @return the virtual address.
 */
static char* get_dsp_result_addr(MSGDATACMEM *msg_alg_result_info)
{
    if (NULL == msg_alg_result_info) {
        ERROR("msg_alg_result_info == NULL.");
        return NULL;
    }

    /* get the dsp result addr virtual */
    void *result_addr_vir = get_dsp_result_cmem_pointer();

    if (result_addr_vir == NULL) {
        ERROR("Alg result virtual address is NULL\n");
        return NULL;
    }

    /* get the dsp result addr phy */
    unsigned long result_addr_phy = get_dsp_result_addr_phy();

    /* the offset of message from the beginner of dsp result. */
    off_t offset = msg_alg_result_info->addr_phy - result_addr_phy;

    /* get the virtual addr of message. */
    return (char*)result_addr_vir + offset;
}

/*******************************************************************************
 * 函数名:process_park_alarm
 * 功  能: 处理泊车报警信息
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
 ******************************************************************************/
int process_park_alarm(MSGDATACMEM *msg_alg_result_info)
{
    char* result_addr = get_dsp_result_addr(msg_alg_result_info);
    if (NULL == result_addr) {
        return -1;
    }

    ParkAlarmInfo *alarm = (ParkAlarmInfo*)_RefHandleCreate(MSG_PARK_ALARM,
            sizeof(ParkAlarmInfo));
    if (NULL == alarm) {
        ERROR("malloc alarm failed.");
        return -1;
    }

    memcpy(alarm, result_addr, sizeof(ParkAlarmInfo));

    INFO("ALARM %s", alarm_str_map[alarm->category]);
    for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if((*it)->subscribed()) {
            (*it)->notify(MSG_PARK_ALARM, alarm);
        }
    }
    _RefHandleRelease((void**)&alarm, NULL, NULL);
    return 0;
}

/*******************************************************************************
 * 函数名:process_park_light
 * 功  能: 处理泊车灯控信息
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
 ******************************************************************************/
int process_park_light(MSGDATACMEM *msg_alg_result_info)
{
    char* result_addr = get_dsp_result_addr(msg_alg_result_info);
    if (NULL == result_addr) {
        return -1;
    }

    Lighting_park *light = (Lighting_park*)_RefHandleCreate(MSG_PARK_LIGHT,
            sizeof(Lighting_park));
    if (NULL == light) {
        ERROR("malloc light failed.");
        return -1;
    }

    memcpy(light, result_addr, sizeof(Lighting_park));

    INFO("LIGHT --- green [%d] interval [%d] "
            "red [%d] interval [%d] "
            "blue [%d] interval [%d] white [%d]",
            light->green, light->green_interval,
            light->red, light->red_interval,
            light->blue, light->blue_interval,
            light->white);

#if (5 == DEV_TYPE)
    light_ctrl_t ctrl;

    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.info[LIGHT_ID_GREEN].value = light->green;
    ctrl.info[LIGHT_ID_GREEN].intv = light->green_interval;
    setbit(ctrl.mask, LIGHT_ID_GREEN);

    ctrl.info[LIGHT_ID_RED].value = light->red;
    ctrl.info[LIGHT_ID_RED].intv = light->red_interval;
    setbit(ctrl.mask, LIGHT_ID_RED);

    ve_light_ctrl_put(&ctrl);
#else
    light_ctl(LIGHT_ACTION_START, LIGHT_PRIO_VD,
              light->red, light->red_interval,
              light->green, light->green_interval,
              light->blue, light->blue_interval,
              light->white);

    for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if((*it)->subscribed()) {
            (*it)->notify(MSG_PARK_LIGHT, light);
        }
    }
#endif
    _RefHandleRelease((void**)&light, NULL, NULL);
    return 0;
}

/*******************************************************************************
 * 函数名:process_park_leave_info
 * 功  能: 处理泊车补发的驶离信息
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
 ******************************************************************************/
int process_park_leave_info(MSGDATACMEM *msg_alg_result_info)
{
    char* result_addr = get_dsp_result_addr(msg_alg_result_info);
    if (NULL == result_addr) {
        return -1;
    }

    static ParkVehicleInfo parkVehicleInfo;
    memcpy(&parkVehicleInfo, result_addr, sizeof(ParkVehicleInfo));

    struct Park_record *park_record = (struct Park_record*)_RefHandleCreate(
            MSG_PARK_INFO, sizeof(struct Park_record));
    if (NULL == park_record) {
        ERROR("malloc park_record failed.");
        return -1;
    }

    memset(park_record, 0x00, sizeof(struct Park_record));

    /* analyze_park_record_info_motor_vehicle
     * will use pic_info[0].tm as db_park_record's time.
     * leave info without picture, but construct the pictime.
     */
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);
    park_record->pic_info[0].tm.year    = timenow->tm_year + 1900;
    park_record->pic_info[0].tm.month   = timenow->tm_mon + 1;
    park_record->pic_info[0].tm.day     = timenow->tm_mday;
    park_record->pic_info[0].tm.hour    = timenow->tm_hour;
    park_record->pic_info[0].tm.minute  = timenow->tm_min;
    park_record->pic_info[0].tm.second  = timenow->tm_sec;
    park_record->pic_info[0].tm.msecond = 0;

    parkVehicleInfo.picNum = 0;
    analyze_park_record_info_motor_vehicle(&(park_record->db_park_record),
            &parkVehicleInfo, park_record->pic_info, NULL);

    for (auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if ((*it)->subscribed()) {
            (*it)->notify(MSG_PARK_INFO, park_record);
        }
    }

#ifndef BEIJING_KTETC
	DB_ParkRecord &record = park_record->db_park_record;
    parkStatus::push_record_2_history_list(record.objectState,
                                           record.plate_num,
                                           record.time,
                                           NULL,
                                           NULL,
                                           NULL);

    // insert into the park system database
    char lc_values[12][128] = {{0}};

    sprintf(lc_values[1], "%s", record.plate_num);
    sprintf(lc_values[2], "%d", record.status);
    sprintf(lc_values[3], "%d", record.objectState);
    sprintf(lc_values[4], "%s", record.time);
    spsystem_delete_lastmsg_table(spsystem_db);
    spsystem_insert_lastmsg_table(spsystem_db, lc_values);
#endif

    _RefHandleRelease((void**)&park_record, NULL, NULL);
    return 0;
}

int process_park_status_info(MSGDATACMEM *msg_alg_result_info)
{
    char* result_addr = get_dsp_result_addr(msg_alg_result_info);
    if (NULL == result_addr) {
        return -1;
    }

    StatusPark *status = (StatusPark*)_RefHandleCreate(MSG_PARK_STATUS,
            sizeof(StatusPark));
    if (NULL == status) {
        ERROR("malloc status failed.");
        return -1;
    }

    memcpy(status, result_addr, sizeof(StatusPark));

    for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if((*it)->subscribed()) {
            (*it)->notify(MSG_PARK_STATUS, status);
        }
    }
    _RefHandleRelease((void**)&status, NULL, NULL);
    return 0;
}

int process_snap_return_info(MSGDATACMEM *msg_alg_result_info)
{
    char* result_addr = get_dsp_result_addr(msg_alg_result_info);
    if (NULL == result_addr) {
        return -1;
    }

    SnapRequestReturn *snap_request_return = (SnapRequestReturn*)
        _RefHandleCreate(MSG_PARK_SNAP, sizeof(SnapRequestReturn));
    memcpy(snap_request_return, result_addr, sizeof(SnapRequestReturn));

    str_park_msg lstr_park_msg = {0};
    INFO("ZEHIN send snap response.");
    lstr_park_msg.data.zehin.type = MSG_PARK_SNAP;
    lstr_park_msg.data.zehin.lens = sizeof(SnapRequestReturn);
    lstr_park_msg.data.zehin.data = (char*)snap_request_return;

    if (tpzehin_provisioned()) {
        _RefHandleRetain(snap_request_return);
        vmq_sendto_tpzehin(lstr_park_msg);
    }
    _RefHandleRelease((void**)&snap_request_return, NULL, NULL);
    return 0;
}

/*******************************************************************************
 * 函数名: process_park_record
 * 功  能: 处理泊车记录
 * 参  数: result，要处理的结果
 * 返回值: 成功，返回0；出错，返回-1
 ******************************************************************************/
int process_park_records(const SystemVpss_SimcopJpegInfo *info)
{
    int ret = 0;

    if (NULL == info) {
        ERROR("NULL is info.");
        return -1;
    }

    const ParkVehicleInfo *result = NULL;
    result = (ParkVehicleInfo *) &(info->algResultInfo.AlgResultInfo);
    if (NULL == result) {
        ERROR("NULL is result.");
        return -1;
    }

    char park_storage_path[MOUNT_POINT_LEN] = {0};

    get_park_storage_path(park_storage_path);

    /**
     * 在这里malloc出一块内存，notify里面如果需要处理这条消息，就会增加这块内存
     * 的引用计数，notify里面增加的引用计数直到该平台处理完毕该消息才释放，而这
     * 里的release只是临时decrease reference count. it won't realease the block
     * at all.
     */
    struct Park_record *park_record = NULL;
    const size_t size = sizeof(struct Park_record);
    park_record = (struct Park_record*)_RefHandleCreate(MSG_PARK_RECORD, size);
    if (NULL == park_record) {
        ERROR("malloc park_record failed.");
        return -1;
    }

    memset(park_record, 0x00, sizeof(struct Park_record));

    ret = analyze_park_record_picture(park_record->pic_info, info);
	if(ret < 0) {
        return -1;
	}

	EP_PicInfo *pic_info = park_record->pic_info;
    analyze_park_record_info_motor_vehicle(&(park_record->db_park_record),
										   result,
										   pic_info,
										   park_storage_path);

    for(auto it = g_platform_list.begin(); it != g_platform_list.end(); ++it) {
        if((*it)->subscribed()) {
            (*it)->notify(MSG_PARK_RECORD, park_record);
        }
    }

#ifndef BEIJING_KTETC
	DB_ParkRecord &record = park_record->db_park_record;
    parkStatus::push_record_2_history_list(record.objectState,
                                           record.plate_num,
                                           record.time,
                                           pic_info[0].name,
                                           pic_info[1].name,
                                           NULL);
    for (int i = 0; i < 2; i++) {
        char pic_path[128] = {0};
        sprintf(pic_path, "/var/www/parkjson/%s", pic_info[i].name);
        park_save_picture(pic_path,
				(unsigned char*)(pic_info[i].buf), pic_info[i].size);
    }

    // insert into the park system database
    char lc_values[12][128] = {{0}};
    sprintf(lc_values[1], "%s", record.plate_num);
    sprintf(lc_values[2], "%d", record.status);
    sprintf(lc_values[3], "%d", record.objectState);
    sprintf(lc_values[4], "%s", record.time);
    spsystem_delete_lastmsg_table(spsystem_db);
    spsystem_insert_lastmsg_table(spsystem_db, lc_values);
#endif
    _RefHandleRelease((void**)&park_record, NULL, NULL);
    return 0;
}

/*******************************************************************************
 * 函数名: analyze_park_record_picture
 * 功  能: 解析机动车通行记录图片
 * 参  数: pic_info，解析出来的图片信息；lane_num，车道号
 * 返回值: 成功，返回0；失败，返回-1
 ******************************************************************************/
int analyze_park_record_picture(EP_PicInfo pic_info[],
        const SystemVpss_SimcopJpegInfo *info)
{
    const ParkVehicleInfo *result =
        (ParkVehicleInfo *) &(info->algResultInfo.AlgResultInfo);

    const char pic_num = result->picNum;

    struct tm time_tmp;

    /* 路径按照用户的配置进行设置 */
    int li_aliyun_count = 0;
    int li_aliyun_i = 0;
    int j = 0;
    unsigned int li_pic_count = 0;

    for(auto i = 0; i < pic_num; i++)
    {
        li_aliyun_count = 0;
        localtime_r((time_t *)(&result->picInfo_park[i].pic_time), &time_tmp);
        pic_info[i].tm.year    = time_tmp.tm_year + 1900;
        pic_info[i].tm.month   = time_tmp.tm_mon + 1;
        pic_info[i].tm.day     = time_tmp.tm_mday;
        pic_info[i].tm.hour    = time_tmp.tm_hour;
        pic_info[i].tm.minute  = time_tmp.tm_min;
        pic_info[i].tm.second  = time_tmp.tm_sec;
        pic_info[i].tm.msecond = result->picInfo_park[i].pic_ms;

        for (j = 0; j < EP_FTP_URL_LEVEL.levelNum; j++)
        {
            switch (EP_FTP_URL_LEVEL.urlLevel[j])
            {
                case SPORT_ID: 		//地点编号
                    sprintf(pic_info[i].path[j], "%s", EP_POINT_ID);
                    sprintf(pic_info[i].alipath[j], "%s", EP_POINT_ID);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case DEV_ID: 		//设备编号
                    sprintf(pic_info[i].path[j], "%s", EP_DEV_ID);
                    sprintf(pic_info[i].alipath[j], "%s", EP_DEV_ID);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case YEAR_MONTH: 	//年/月
                    sprintf(pic_info[i].path[j], "%04d%02d",
                            pic_info[i].tm.year,pic_info[i].tm.month);
                    sprintf(pic_info[i].alipath[j], "%04d%02d",
                            pic_info[i].tm.year,pic_info[i].tm.month);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case DAY: 			//日
                    sprintf(pic_info[i].path[j], "%02d",pic_info[i].tm.day);
                    sprintf(pic_info[i].alipath[j], "%02d",pic_info[i].tm.day);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case EVENT_NAME: 	//事件类型
                    sprintf(pic_info[i].path[j], "%s", PARK_RECORDS_FTP_DIR);
                    sprintf(pic_info[i].alipath[j], "%s", PARK_RECORDS_FTP_DIR);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case HOUR: 			//时
                    sprintf(pic_info[i].path[j], "%02d",	pic_info[i].tm.hour);
                    sprintf(pic_info[i].alipath[j], "%02d",	pic_info[i].tm.hour);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                case FACTORY_NAME: 	//厂商名称
                    sprintf(pic_info[i].path[j], "%s", EP_MANUFACTURER);
                    sprintf(pic_info[i].alipath[j], "%s", EP_MANUFACTURER);
                    strcat(pic_info[i].alipath[j], "/");
                    break;
                default:
                    break;
            }
        }

        if(0 == i)
        {
            //文件名以泊位号+时间+驶入驶离状态命名
            snprintf(pic_info[i].name, sizeof(pic_info->name),
                    "%s%04d%02d%02d%02d%02d%02d%03d%d.jpg",
                    g_arm_config.basic_param.spot_id,
                    pic_info[i].tm.year,
                    pic_info[i].tm.month,
                    pic_info[i].tm.day,
                    pic_info[i].tm.hour,
                    pic_info[i].tm.minute,
                    pic_info[i].tm.second,
                    pic_info[i].tm.msecond,
                    result->objectState
                    );

            strncpy(pic_info[i].alipath[j++],
                    pic_info[i].name,
                    strlen(pic_info[i].name));
        }
        else if(1 == i)
        {
            snprintf(pic_info[i].name, sizeof(pic_info->name),
                    "%s%04d%02d%02d%02d%02d%02d%03d%c%d.jpg",
                    g_arm_config.basic_param.spot_id,
                    pic_info[i].tm.year,
                    pic_info[i].tm.month,
                    pic_info[i].tm.day,
                    pic_info[i].tm.hour,
                    pic_info[i].tm.minute,
                    pic_info[i].tm.second,
                    pic_info[i].tm.msecond,
                    'b',
                    result->objectState
                    );

            strncpy(pic_info[i].alipath[j++],
                    pic_info[i].name,
                    strlen(pic_info[i].name));
        }

        for(li_aliyun_i=0; li_aliyun_i<j; li_aliyun_i++)
        {
            strcat(pic_info[i].aliname, pic_info[i].alipath[li_aliyun_i]);
        }

        //获取jpeg大缓冲区的首地址
        char *buf_jpeg_start = (char *)((unsigned int)info -((SystemVpss_SimcopJpegInfo *)info)->jpeg_bufid * SIZE_JPEG_BUFFER);

        for(int num_offset_jpeg = NUM_JPEG_SPLIT_BUFFER;
                num_offset_jpeg < NUM_JPEG_BUFFER_TOTAL;
                num_offset_jpeg++)
        {
            SystemVpss_SimcopJpegInfo * jpeg_info =
                (SystemVpss_SimcopJpegInfo *)
                (buf_jpeg_start + num_offset_jpeg * SIZE_JPEG_BUFFER);

            //避免驶入驶离图片寻找到同一张jpeg，通过驶入驶离状态来进一步的区分
            ParkVehicleInfo *park_record_buf =
                (ParkVehicleInfo *) &(jpeg_info->algResultInfo.AlgResultInfo);
            if((result->picInfo_park[i].pic_id == jpeg_info->jpeg_numid)
                    &&(result->objectState==park_record_buf->objectState)
                    &&(result->picInfo_park[i].xPos==park_record_buf->picInfo_park[i].xPos)
                    &&(result->picInfo_park[i].yPos==park_record_buf->picInfo_park[i].yPos)
              )
            {
                li_pic_count++;
                pic_info[i].buf = (void *)((char *)jpeg_info + SIZE_JPEG_INFO);

                /* 获取图片缓存的大小 */
                pic_info[i].size = jpeg_info->jpeg_buf_size;
                break;
            }
        }
    }

    if (li_pic_count != pic_num) {
        ERROR("%s", "picture's ID is not found ");
        return -1;//找不到jpeg，不进行记录发送
    }

    return 0;
}


/*******************************************************************************
 * 函数名: analyze_park_record_info_motor_vehicle
 * 功  能: 解析机动车通行记录信息
 * 参  数: db_park_record，解析的结果；
 *         image_info，图像信息；pic_info，图片文件信息
 * 返回值: 成功，返回0
 ******************************************************************************/
int analyze_park_record_info_motor_vehicle(DB_ParkRecord *db_park_record,
                                           const ParkVehicleInfo *result,
                                           const EP_PicInfo pic_info[],
                                           const char *partition_path)
{
    const char pic_num = result->picNum;
    memset(db_park_record, 0, sizeof(DB_ParkRecord));

    snprintf(db_park_record->plate_num,
            sizeof(db_park_record->plate_num),
            "%s", result->strResult);

    db_park_record->objectState = result->objectState;

    snprintf(db_park_record->point_id,
            sizeof(db_park_record->point_id),
            "%s", EP_POINT_ID);

    snprintf(db_park_record->point_name,
            sizeof(db_park_record->point_name),
            "%s", EP_POINT_NAME);

    snprintf(db_park_record->dev_id,
            sizeof(db_park_record->dev_id),
            "%s", EP_EXP_DEV_ID);

    /* for drive in, use the last picture time as record time;
     * for drive out, use the first picture time as record time.
     */
    if (db_park_record->objectState == 1) { // drive in
        snprintf(db_park_record->time,
                sizeof(db_park_record->time),
                "%04d-%02d-%02d %02d:%02d:%02d",
                pic_info[1].tm.year, pic_info[1].tm.month,
                pic_info[1].tm.day, pic_info[1].tm.hour,
                pic_info[1].tm.minute, pic_info[1].tm.second);
    } else if (db_park_record->objectState == 0) { // drive out
        snprintf(db_park_record->time,
                sizeof(db_park_record->time),
                "%04d-%02d-%02d %02d:%02d:%02d",
                pic_info[0].tm.year, pic_info[0].tm.month,
                pic_info[0].tm.day, pic_info[0].tm.hour,
                pic_info[0].tm.minute, pic_info[0].tm.second);
    } else {
        ERROR("BAD OBJECT STATE %d", db_park_record->objectState);
        return -1;
    }

    snprintf(db_park_record->collection_agencies,
            EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);

    db_park_record->direction = EP_DIRECTION;
    db_park_record->objective_type 	= 1; /* 1表示机动车 */

    DEBUG("db_park_record->objectState=%d, pic_num=%d, time=%s",
          db_park_record->objectState, pic_num, db_park_record->time);

    if (pic_num > 0) {
        db_park_record->plate_type = result->plateType;
        db_park_record->lane_num = result->berthNum;
        db_park_record->speed = result->speed;

        for (auto i = 0; i < pic_num; i++)
        {
            get_ftp_path(EP_FTP_URL_LEVEL,
                    pic_info[i].tm.year,
                    pic_info[i].tm.month,
                    pic_info[i].tm.day,
                    pic_info[i].tm.hour,
                    (char*) PARK_RECORDS_FTP_DIR,
                    db_park_record->image_path[i]);

            size_t strlen_length = strlen(pic_info[i].aliname);
            size_t size = strlen_length < 256 ? strlen_length : 255;
            strncpy(db_park_record->aliyun_image_path[i], pic_info[i].aliname, size);
            db_park_record->aliyun_image_path[i][size] = '\0';

            strlen_length = strlen(pic_info[i].name);
            size = strlen_length < 256 ? strlen_length : 255;
            strncpy(db_park_record->image_name[i], pic_info[i].name, size);
            db_park_record->image_name[i][size] = '\0';
        }

        snprintf(db_park_record->partition_path,
                sizeof(db_park_record->partition_path),
                "%s", partition_path);
        db_park_record->color 			= result->vehicleColor;
        db_park_record->vehicle_logo 	= result->vehicleLogo;

        for(unsigned int j = 0;j < pic_num;j++)
        {
            db_park_record->coordinate_x[j] 	= result->picInfo_park[j].xPos;
            db_park_record->coordinate_y[j] 	= result->picInfo_park[j].yPos;
            db_park_record->width[j] 		= result->picInfo_park[j].width;
            db_park_record->height[j] 		= result->picInfo_park[j].height;
        }

        db_park_record->plate_color = result->color;

        if(result->objectState == 1)
        {
            snprintf(db_park_record->description,
                    sizeof(db_park_record->description),
                    "图像编号: %d", result->picInfo_park[0].pic_id);
        }
        if(result->objectState == 0)
        {
            snprintf(db_park_record->description,
                    sizeof(db_park_record->description),
                    "图像编号: %d", result->picInfo_park[1].pic_id);
        }

        snprintf(db_park_record->ftp_user,
                sizeof(db_park_record->ftp_user),
                "%s", EP_TRAFFIC_FTP.user);

        snprintf(db_park_record->ftp_passwd,
                sizeof(db_park_record->ftp_passwd),
                "%s", EP_TRAFFIC_FTP.passwd);

        snprintf(db_park_record->ftp_ip,
                sizeof(db_park_record->ftp_ip),
                "%d.%d.%d.%d",
                EP_TRAFFIC_FTP.ip[0],
                EP_TRAFFIC_FTP.ip[1],
                EP_TRAFFIC_FTP.ip[2],
                EP_TRAFFIC_FTP.ip[3]);

        db_park_record->ftp_port = EP_TRAFFIC_FTP.port;

        db_park_record->vehicle_type = result->vehicleType;

        for(unsigned int k = 0;k < pic_num;k++)
        {
            db_park_record->confidence[k] = result->picInfo_park[k].confidence;
        }
        db_park_record->status = result->status;
    }
    else
    {
        if( (result->objectState==0) && (pic_num==0) )
        {
            //设置驶入驶离状态为：补发驶离
            INFO("%s: we add a park leave record", __func__);
            db_park_record->status = 4;
        }
        else
        {
            INFO("%s: status is abnormal ->objectState=%d, pic_num=%d",
                    __func__, db_park_record->objectState, pic_num);
        }
    }
    return 0;
}

/*******************************************************************************
 * 函数名:send_park_record_image_aliyun
 * 功  能: 发送缓存中的泊车记录图片
 * 参  数: pic_info，图片文件信息
 * 返回值: 成功，返回0；失败，返回-1
 ******************************************************************************/
int send_park_records_image_aliyun(EP_PicInfo pic_info[], int pic_num)
{
    for(int i = 0;i < pic_num;i++)
    {
        DEBUG("send image %s to oss.", pic_info[i].aliname);
        if (oss_put_file(pic_info[i].aliname,
                         (char *)pic_info[i].buf,
                         pic_info[i].size) != 0)
        {
            /* if pic_info[i==0] send failed, return -11;
             * else if pic_info[i==1] send failed, return -12
             */
            return -1 - 10 - i;
        }
    }

    return 0;
}

/*******************************************************************************
 * 函数名:send_park_record_image_buf
 * 功  能: 发送缓存中的泊车记录图片
 * 参  数: pic_info，图片文件信息
 * 返回值: 成功，返回0；失败，返回-1
 ******************************************************************************/
int send_park_records_image_buf(EP_PicInfo pic_info[],int pic_num)
{
    if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
    {
        return -1;
    }

    for(int i = 0;i < pic_num;i++)
    {
        if (ftp_send_pic_buf(&pic_info[i], FTP_CHANNEL_PASSCAR) < 0)
        {
            /* if pic_info[i==0] send failed, return -11;
             * else if pic_info[i==1] send failed, return -12
             */
            return -1 - 10 - i;
        }
    }
    return 0;
}

/*******************************************************************************
 * 函数名: send_park_record_info
 * 功  能: 发送通行记录信息
 * 参  数: db_park_records，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
 *******************************************************************************/
int send_park_record_info(const DB_ParkRecord *db_park_record)
{
    printf("In send_park_record_info\n");

    char mq_text[MQ_TEXT_BUF_SIZE] = {0};

    memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

    format_mq_text_park_record(mq_text, db_park_record);

    log_state("VD:","The park mq info : %s\n", mq_text);

    TRACE_LOG_PACK_PLATFORM("mq_text = %s", mq_text);

    strcpy(gstr_tpbitcom_records.message, mq_text);

    if (mq_get_status_park_record() < 0)
    {
        ERROR("get mq status failed! in send_park_record_info\n");
        return -1;
    }

    mq_send_park_record(mq_text);

    return 0;
}

/*******************************************************************************
 * 函数名: format_mq_text_park_record
 * 功  能: 格式化泊车记录MQ  文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；record，过车记录
 * 返回值: 字符串长度
 ******************************************************************************/
int format_mq_text_park_record(
        char *mq_text, const DB_ParkRecord *record)
{
    printf("In format_mq_text_park_record\n");

    if (record->objective_type == 1) 	//目标类型  1 : 机动车
    {
        return format_mq_text_park_record_motor_vehicle(mq_text, record);
    }
    else
    {
        printf("record->object_type is wrong!\n");
    }
    return 0;
}

/*******************************************************************************
 * 函数名: format_mq_text_park_record_motor_vehicle
 * 功  能: 格式化机动车交通通行记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；record，过车记录
 * 返回值: 字符串长度
 ******************************************************************************/
int format_mq_text_park_record_motor_vehicle(
        char *mq_text, const DB_ParkRecord *record)

{
    int len = 0;

    if((record->status==4)&&(record->objectState==0))//如果是补发驶离记录
    {

        len += sprintf(mq_text + len, "%s", DATA_SOURCE_PARK);
        /* 字段1 数据来源 */

        len += sprintf(mq_text + len, ",%s", record->plate_num);
        /* 字段2 车牌号码 */

        len += sprintf(mq_text + len, ",%02d", 2);//信息中没有，假定为2，小型车  record->plate_type);
        /* 字段3 号牌类型 */

        len += sprintf(mq_text + len, ",%s", record->point_id);
        /* 字段4 采集点编号 */

        len += sprintf(mq_text + len, ",%s", record->point_name);
        /* 字段5 采集点名称 */

        len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
        /* 字段6 抓拍类型 */

        len += sprintf(mq_text + len, ",%s", record->dev_id);
        /* 字段7 设备编号 */

        len += sprintf(mq_text + len, ",%d", 0);//信息中没有，假定为0车道  record->lane_num);
        /* 字段8 车道编号 */

        len += sprintf(mq_text + len, ",%d", 0);//信息中没有，假定为0  record->speed
        /* 字段9 车辆速度 */

        len += sprintf(mq_text + len, ",%s", record->time);
        /* 字段10 抓拍时间 */

        len += sprintf(mq_text + len, ",%s", record->collection_agencies);
        /* 字段11 采集机关编号 */

        len += sprintf(mq_text + len, ",%d", record->direction);
        /* 字段12 方向编号 */

        //        len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s%s",
        //    	    record->ftp_user, record->ftp_passwd,
        //    	    record->ftp_ip, record->ftp_port, record->image_path[0], record->image_name[0]);
        len += sprintf(mq_text + len, ",ftp:0");

        /* 字段13 第一张图片地址 */

        //        len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s%s",
        //    	    record->ftp_user, record->ftp_passwd,
        //    	    record->ftp_ip, record->ftp_port, record->image_path[1], record->image_name[1]);
        len += sprintf(mq_text + len, ",ftp:1");
        len += sprintf(mq_text + len, ",");
        /* 字段14、15 第二三张图片地址，目前没有 */

        len += sprintf(mq_text + len, ",%d", record->color);
        /* 字段16 车身颜色编号 */

        len += sprintf(mq_text + len, ",%d", 0);//record->vehicle_logo);
        /* 字段17 车标编号 */

        len += sprintf(mq_text + len, ",%d", record->objective_type);
        /* 字段18 目标类型 */

        //        len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d/%d/%d/%d",
        //    	    record->coordinate_x[0], record->coordinate_y[0],
        //    	    record->width[0], record->height[0],
        //    	    record->coordinate_x[1], record->coordinate_y[1],
        //    	    record->width[1], record->height[1]);
        len += sprintf(mq_text + len, ",0/0/0/0/0/0/0/0" );
        /* 字段19 车牌定位（X坐标、Y坐标、宽度、高度，各出现两次） */

        len += sprintf(mq_text + len, ",,,,,,,,,,,");
        /* 字段20到30 未用 */

        len += sprintf(mq_text + len, ",%d", record->plate_color);
        /* 字段31 车牌颜色 */

        len += sprintf(mq_text + len, ",");
        /* 字段32 未用 */

        len += sprintf(mq_text + len, ",%s", "");//record->description);
        /* 字段33 过车描述 */

        len += sprintf(mq_text + len, ",%d", 0);//record->vehicle_type);
        /* 字段34 车型 */

        len += sprintf(mq_text + len, ",%d", (int)",");
        /* 字段35 */

        len += sprintf(mq_text + len, ",%d", record->objectState);
        /* 字段36 驶入驶离信息(1:驶入 0:驶离) */

        len += sprintf(mq_text + len, ",%s", record->point_id);
        /* 字段37 泊位号() */

        len += sprintf(mq_text + len, ",%d", record->status);

    }
    else
    {
        len += sprintf(mq_text + len, "%s", DATA_SOURCE_PARK);
        /* 字段1 数据来源 */

        len += sprintf(mq_text + len, ",%s", record->plate_num);
        /* 字段2 车牌号码 */

        len += sprintf(mq_text + len, ",%02d", record->plate_type);
        /* 字段3 号牌类型 */

        len += sprintf(mq_text + len, ",%s", record->point_id);
        /* 字段4 采集点编号 */

        len += sprintf(mq_text + len, ",%s", record->point_name);
        /* 字段5 采集点名称 */

        len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
        /* 字段6 抓拍类型 */

        len += sprintf(mq_text + len, ",%s", record->dev_id);
        /* 字段7 设备编号 */

        len += sprintf(mq_text + len, ",%d", record->lane_num);
        /* 字段8 车道编号 */

        len += sprintf(mq_text + len, ",%d", record->speed);
        /* 字段9 车辆速度 */

        len += sprintf(mq_text + len, ",%s", record->time);
        /* 字段10 抓拍时间 */

        len += sprintf(mq_text + len, ",%s", record->collection_agencies);
        /* 字段11 采集机关编号 */

        len += sprintf(mq_text + len, ",%d", record->direction);
        /* 字段12 方向编号 */

        len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s%s",
                record->ftp_user, record->ftp_passwd,
                record->ftp_ip, record->ftp_port, record->image_path[0], record->image_name[0]);
        /* 字段13 第一张图片地址 */

        len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s%s",
                record->ftp_user, record->ftp_passwd,
                record->ftp_ip, record->ftp_port, record->image_path[1], record->image_name[1]);
        len += sprintf(mq_text + len, ",");
        /* 字段14、15 第二三张图片地址，目前没有 */

        len += sprintf(mq_text + len, ",%d", record->color);
        /* 字段16 车身颜色编号 */

        len += sprintf(mq_text + len, ",%d", record->vehicle_logo);
        /* 字段17 车标编号 */

        len += sprintf(mq_text + len, ",%d", record->objective_type);
        /* 字段18 目标类型 */

        len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d/%d/%d/%d",
                record->coordinate_x[0], record->coordinate_y[0],
                record->width[0], record->height[0],
                record->coordinate_x[1], record->coordinate_y[1],
                record->width[1], record->height[1]);
        /* 字段19 车牌定位（X坐标、Y坐标、宽度、高度、第几张图片） */

        len += sprintf(mq_text + len, ",,,,,,,,,,,");
        /* 字段20到30 未用 */

        len += sprintf(mq_text + len, ",%d", record->plate_color);
        /* 字段31 车牌颜色 */

        len += sprintf(mq_text + len, ",");
        /* 字段32 未用 */

        len += sprintf(mq_text + len, ",%s", record->description);
        /* 字段33 过车描述 */

        len += sprintf(mq_text + len, ",%d", record->vehicle_type);
        /* 字段34 车型 */

        len += sprintf(mq_text + len, ",%d", (int)",");
        /* 字段35 */

        len += sprintf(mq_text + len, ",%d", record->objectState);
        /* 字段36 驶入驶离信息(1:驶入 0:驶离) */

        len += sprintf(mq_text + len, ",%s", record->point_id);
        /* 字段37 泊位号() */

        len += sprintf(mq_text + len, ",%d", record->status);
    }
    return len;
}

/*******************************************************************************
 * 函数名: save_park_record_info
 * 功  能: 保存泊车记录
 * 参  数: db_park_record，通行记录信息
 * 返回值: 成功，返回0；失败，返回-1
 *******************************************************************************/
int save_park_record_image(
        const EP_PicInfo pic_info[], const char *park_storage_path)
{

    char record_path[32];
    sprintf(record_path, "%s/%s", park_storage_path, DISK_RECORD_PATH);
    int ret = dir_create(record_path);
    if (ret != 0)
    {
        printf("dir_create %s failed\n", record_path);
        return -1;
    }

    for(int i = 0;i < PARK_PIC_NUM; i++ )
    {
        ret = file_save(record_path, pic_info[i].path, pic_info[i].name, pic_info[i].buf,
                pic_info[i].size);
    }
    return ret;

}
#if 1

/*******************************************************************************
 * 函数名: save_park_record_info
 * 功  能: 保存泊车记录信息
 * 参  数: db_park_record，通行记录信息
 * 返回值: 成功，返回0；失败，返回-1
 *******************************************************************************/
int save_park_record_info(
        const DB_ParkRecord *db_park_record,
        const EP_PicInfo pic_info[], const char *park_storage_path)

{
    printf("In save_park_record_info\n");

    char db_park_name[PATH_MAX_LEN] = {0};//  例如: /mnt/sda1/DB/park_records_2013082109.db
    static DB_File db_file;
    int flag_record_in_DB_file=0;//是否需要记录到索引数据库


    char dir_temp[PATH_MAX_LEN];
    sprintf(dir_temp, "%s/%s", park_storage_path, DISK_DB_PATH);
    int ret = dir_create(dir_temp);
    if (ret != 0)
    {
        printf("dir_create %s failed\n", dir_temp);

        return -1;
    }

    strcat(dir_temp, "/park_record");
    ret = dir_create(dir_temp);
    if (ret != 0)
    {
        printf("dir_create %s failed\n", dir_temp);
        return -1;
    }

    if(db_park_record->objectState == 1) //驶入，采用第一张图片的时间作为数据库命名时间(驶入过程中的图片)
    {
        sprintf(db_park_name,"%s/%04d%02d%02d%02d.db",
                dir_temp,pic_info[0].tm.year,pic_info[0].tm.month,pic_info[0].tm.day,pic_info[0].tm.hour);
    }

    if(db_park_record->objectState == 0) //驶离,采用第二张图片作为数据库命名时间(驶离过程中的图片)
    {
        sprintf(db_park_name,"%s/%04d%02d%02d%02d.db",
                dir_temp,pic_info[1].tm.year,pic_info[1].tm.month,pic_info[1].tm.day,pic_info[1].tm.hour);
    }

    printf("db_park_name:%s\n",db_park_name);

    if (access(db_park_name, F_OK) != 0)
    {
        resume_print("The db_park is not exit\n");
        //数据库文件不存在，需要在索引数据库中增加相应的一条记录
        //数据库文件如果存在，说明在同一个小时内出现了多个记录进行存储
        flag_record_in_DB_file=1;
    }
    else
    {
        resume_print("The db_park is exit\n");
        flag_record_in_DB_file=0;
    }


    if (flag_record_in_DB_file==1)//保证，索引记录的删除，同时删对应交通数据库
    {
        resume_print("add to DB_files, db_park_name is %s\n", db_park_name);

        //写一条记录到数据库名管理
        db_file.record_type = 0;
        memset(db_file.record_path, 0, sizeof(db_file.record_path));
        memcpy(db_file.record_path, db_park_name, strlen(db_park_name));
        memset(db_file.time, 0, sizeof(db_file.time));
        memcpy(db_file.time, db_park_record->time, strlen(
                    db_park_record->time));
        db_file.flag_send = db_park_record->flag_send;
        db_file.flag_store= db_park_record->flag_store;

        ret = db_write_DB_file((char*)DB_NAME_PARK_RECORD, &db_file,
                &mutex_db_files_park);
        if (ret != 0)
        {
            printf("db_write_DB_file failed\n");
        }
        else
        {
            printf("db_write_DB_file %s in %s ok.\n", db_park_name,
                    (char*)DB_NAME_PARK_RECORD);
        }
    }

    ret = db_write_park_records(db_park_name,
            (DB_ParkRecord *) db_park_record,
            &mutex_db_records_park);

    if (ret != 0)
    {
        printf("db_write_park_records failed\n");
        return -1;
    }
    return ret;
}


/*******************************************************************************
 * 函数名: db_write_park_records
 * 功  能: 写泊车记录数据库
 * 参  数: records，交通通行记录
 * 返回值: 成功，返回0；失败，返回-1
 *******************************************************************************/
int db_write_park_records(char *db_park_name, DB_ParkRecord *records,
        pthread_mutex_t *mutex_db_records)
{
    printf("In db_write_park_records\n");
    char sql[SQL_BUF_SIZE];

    memset(sql, 0, SQL_BUF_SIZE);

    db_format_insert_sql_park_records(sql, records);

    printf("In db_write_park_records sql:%s\n",sql);
    pthread_mutex_lock(mutex_db_records);
    int ret = db_write(db_park_name, sql, SQL_CREATE_TABLE_PARK_RECORDS);
    pthread_mutex_unlock(mutex_db_records);

    if (ret != 0)
    {
        printf("db_write_park_records failed\n");
        return -1;
    }

    return 0;
}



/*******************************************************************************
 * 函数名: db_format_insert_sql_park_records
 * 功  能: 格式化交通通行记录表SQL插入语句
 * 参  数: sql，SQL缓存；records，通行记录
 * 返回值: SQL长度
 *******************************************************************************/
int db_format_insert_sql_park_records(char *sql, DB_ParkRecord *records)
{
    int len = 0;

    len += sprintf(sql, "INSERT INTO park_records VALUES(NULL");
    len += sprintf(sql + len, ",'%s'", records->plate_num);
    len += sprintf(sql + len, ",'%d'", records->plate_type);
    len += sprintf(sql + len, ",'%s'", records->point_id);
    len += sprintf(sql + len, ",'%s'", records->point_name);
    len += sprintf(sql + len, ",'%s'", records->dev_id);
    len += sprintf(sql + len, ",'%d'", records->lane_num);
    len += sprintf(sql + len, ",'%d'", records->speed);
    len += sprintf(sql + len, ",'%s'", records->time);
    len += sprintf(sql + len, ",'%s'", records->collection_agencies);
    len += sprintf(sql + len, ",'%d'", records->direction);
    len += sprintf(sql + len, ",'%s'", records->image_path[0]);
    len += sprintf(sql + len, ",'%s'", records->image_path[1]);

    len += sprintf(sql + len, ",'%s'", records->partition_path);
    len += sprintf(sql + len, ",'%d'", records->color);
    len += sprintf(sql + len, ",'%d'", records->vehicle_logo);
    len += sprintf(sql + len, ",'%d'", records->objective_type);

    len += sprintf(sql + len, ",'%d'", records->coordinate_x[0]);
    len += sprintf(sql + len, ",'%d'", records->coordinate_y[0]);
    len += sprintf(sql + len, ",'%d'", records->width[0]);
    len += sprintf(sql + len, ",'%d'", records->height[0]);

    len += sprintf(sql + len, ",'%d'", records->coordinate_x[1]);
    len += sprintf(sql + len, ",'%d'", records->coordinate_y[1]);
    len += sprintf(sql + len, ",'%d'", records->width[1]);
    len += sprintf(sql + len, ",'%d'", records->height[1]);
    //	len += sprintf(sql + len, ",'%d'", records->pic_flag);
    len += sprintf(sql + len, ",'%d'", records->plate_color);
    len += sprintf(sql + len, ",'%s'", records->description);
    len += sprintf(sql + len, ",'%s'", records->ftp_user);
    len += sprintf(sql + len, ",'%s'", records->ftp_passwd);
    len += sprintf(sql + len, ",'%s'", records->ftp_ip);
    len += sprintf(sql + len, ",'%d'", records->ftp_port);
    len += sprintf(sql + len, ",'%d'", records->flag_send);
    len += sprintf(sql + len, ",'%d'", records->flag_store);
    len += sprintf(sql + len, ",'%d'", records->vehicle_type);
    len += sprintf(sql + len, ",'%d'", records->confidence[0]);
    len += sprintf(sql + len, ",'%d'", records->confidence[1]);
    len += sprintf(sql + len, ",'%d'", records->objectState);
    len += sprintf(sql + len, ",'%d'", records->status);
    len += sprintf(sql + len, ",'%s'", records->image_name[0]);
    len += sprintf(sql + len, ",'%s'", records->image_name[1]);
    len += sprintf(sql + len, ");");

    return len;
}

#endif

#if 1
/*******************************************************************************
 * 函数名: db_read_park_record
 * 功  能: 读取数据库中的通行记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
 *******************************************************************************/
int db_read_park_record(const char *db_name, void *records,
        pthread_mutex_t *mutex_db_records, char * sql_cond)

{
    char *pzErrMsg = NULL;
    int rc;
    sqlite3 *db = NULL;
    //char sql[1024];
    int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
    char **azResult;
    static char plateNum[16]; //判断同一车牌传送的次数，防止删除数据库失败引起重复传送
    int samePlateCnt = 0; //续传时判断同一车牌传送的次数
    int ID_read=0;

    DB_ParkRecord * park_record = (DB_ParkRecord *) records;

    pthread_mutex_lock(mutex_db_records);

    rc = sqlite3_open(db_name, &db);//打开记录数据库，如果没有则创建

    if (rc != SQLITE_OK)
    {
        printf("Create database %s failed!\n", db_name);
        printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(mutex_db_records);
        return -1;
    }
    else
    {
        printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
    }

    rc = db_create_table(db, SQL_CREATE_TABLE_PARK_RECORDS);

    if (rc < 0)
    {
        printf("db_create_table failed\n");
        sqlite3_close(db);
        pthread_mutex_unlock(mutex_db_records);
        return -1;
    }

    //查询数据
    //memset(sql, 0, sizeof(sql));
    //sprintf(sql, "SELECT * FROM park_records limit 1");
    nrow = 0;
    rc = sqlite3_get_table(db, sql_cond, &azResult, &nrow, &ncolumn, &pzErrMsg);
    if (rc != SQLITE_OK || nrow == 0)
    {
        //		printf("Can't require data, Error Message: %s\n", pzErrMsg);
        //		printf("row:%d column=%d \n", nrow, ncolumn);
        sqlite3_free(pzErrMsg);
        sqlite3_free_table(azResult);
        sqlite3_close(db);
        pthread_mutex_unlock(mutex_db_records);

        return -1;
    }

    printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn][0]);

    sprintf(park_record->plate_num, "%s", azResult[ncolumn + 1]);
    printf("park_record->plate_num is %s\n", park_record->plate_num);

    if (strcmp(park_record->plate_num, plateNum) == 0)//判断车牌号是否重复
    {
        samePlateCnt++;
        if (samePlateCnt == 5)
        {
            printf("Can't del data %s\n", pzErrMsg);
            sqlite3_free(pzErrMsg);
            sqlite3_free_table(azResult);
            sqlite3_close(db);
            pthread_mutex_unlock(mutex_db_records);
            return -1;
        }
    }
    else
    {
        sprintf(plateNum, "%s", park_record->plate_num);
        samePlateCnt = 0;
    }

    ID_read= atoi(azResult[ncolumn]);

    db_unformat_read_sql_park_records(&(azResult[ncolumn]), park_record);

    printf("db_read_park_record  finish\n");

    sqlite3_free(pzErrMsg);

    sqlite3_free_table(azResult);

    sqlite3_close(db);
    pthread_mutex_unlock(mutex_db_records);

    return ID_read;
}

/*******************************************************************************
 * 函数名: db_unformat_read_sql_park_records
 * 功  能: 解析从机动车通行记录表中读出的数据
 * 参  数: azResult，数据库表中读出的数据缓存；buf，通行记录结构体指针
 * 返回值: 0正常，其他为异常
 ******************************************************************************/
int db_unformat_read_sql_park_records(
        char *azResult[], DB_ParkRecord *park_record)

{
    int ncolumn = 0;

    if (park_record == NULL)
    {
        printf("db_unformat_read_sql_park_records: park_record is NULL\n");
        return -1;
    }

    if (azResult == NULL)
    {
        printf("db_unformat_read_sql_park_records: azResult is NULL\n");
        return -1;
    }

    memset(park_record, 0, sizeof(DB_ParkRecord));

    park_record->ID= atoi(azResult[ncolumn + 0]);

    sprintf(park_record->plate_num, "%s", azResult[ncolumn + 1]);

    park_record->plate_type = atoi(azResult[ncolumn + 2]);

    sprintf(park_record->point_id, "%s", azResult[ncolumn + 3]);

    sprintf(park_record->point_name, "%s", azResult[ncolumn + 4]);

    sprintf(park_record->dev_id, "%s", azResult[ncolumn + 5]);

    park_record->lane_num = atoi(azResult[ncolumn + 6]);

    park_record->speed = atoi(azResult[ncolumn + 7]);

    sprintf(park_record->time, "%s", azResult[ncolumn + 8]);

    sprintf(park_record->collection_agencies, "%s", azResult[ncolumn + 9]);

    park_record->direction = atoi(azResult[ncolumn + 10]);

    sprintf(park_record->image_path[0], "%s", azResult[ncolumn + 11]);

    sprintf(park_record->image_path[1], "%s", azResult[ncolumn + 12]);

    sprintf(park_record->partition_path, "%s", azResult[ncolumn + 13]);

    park_record->color 			= atoi(azResult[ncolumn + 14]);

    park_record->vehicle_logo 	= atoi(azResult[ncolumn + 15]);

    park_record->objective_type 	= atoi(azResult[ncolumn + 16]);

    park_record->coordinate_x[0] 	= atoi(azResult[ncolumn + 17]);
    park_record->coordinate_y[0] 	= atoi(azResult[ncolumn + 18]);
    park_record->width[0] 			= atoi(azResult[ncolumn + 19]);
    park_record->height[0] 			= atoi(azResult[ncolumn + 20]);

    park_record->coordinate_x[1] 	= atoi(azResult[ncolumn + 21]);
    park_record->coordinate_y[1] 	= atoi(azResult[ncolumn + 22]);
    park_record->width[1] 			= atoi(azResult[ncolumn + 23]);
    park_record->height[1] 			= atoi(azResult[ncolumn + 24]);

    //	park_record->pic_flag 		= atoi(azResult[ncolumn + 20]);

    park_record->plate_color 	= atoi(azResult[ncolumn + 25]);

    sprintf(park_record->description, 	"%s", azResult[ncolumn + 26]);

    sprintf(park_record->ftp_user, 		"%s", azResult[ncolumn + 27]);

    sprintf(park_record->ftp_passwd, 	"%s", azResult[ncolumn + 28]);

    sprintf(park_record->ftp_ip, 		"%s", azResult[ncolumn + 29]);

    park_record->ftp_port 		= atoi(azResult[ncolumn + 30]);

    park_record->flag_send 		= atoi(azResult[ncolumn + 31]);

    park_record->flag_store 	= atoi(azResult[ncolumn + 32]);

    park_record->vehicle_type 	= atoi(azResult[ncolumn + 33]);

    park_record->confidence[0]= atoi(azResult[ncolumn + 34]);

    park_record->confidence[1]= atoi(azResult[ncolumn + 35]);

    park_record->objectState= atoi(azResult[ncolumn + 36]);

    park_record->status= atoi(azResult[ncolumn + 37]);
    sprintf(park_record->image_name[0], 		"%s", azResult[ncolumn + 38]);
    sprintf(park_record->image_name[1], 		"%s", azResult[ncolumn + 39]);

    return 0;
}


/********************************
  函数:aliyun_reupload_info
  功能:aliyun续传信息赋值
  返回值:0 成功 -1 失败
 ********************************/
int aliyun_reupload_info(DB_ParkRecord* db_park_record,
                         EP_PicInfo pic_info[],
                         str_aliyun_image *as_aliyun_image)
{
    char file_name_pic[NAME_MAX_LEN];
    char lc_image_path[PARK_PIC_NUM][128] = {{0}};
    int li_j = 0;
    int fd = 0;

    for(int i = 0; i < PARK_PIC_NUM; i++)
    {
        memset(file_name_pic,0,sizeof(file_name_pic));

        sprintf(file_name_pic, "%s/%s/%s",
                db_park_record->partition_path,
                DISK_RECORD_PATH,
                db_park_record->image_path[i]);

        if (access(file_name_pic, F_OK) < 0)
        {
            log_debug_storage("%s does not exist !\n", file_name_pic);
            return -1;
        }

        strcat(lc_image_path[i], file_name_pic);
        strcat(lc_image_path[i], db_park_record->image_name[i]);
        fd = open(lc_image_path[i], O_RDONLY);
        as_aliyun_image->size[i] = read(fd, as_aliyun_image->buf[i], 1024*1024);

        close(fd);

        for(li_j=0; li_j<6; li_j++)
        {
            if(strlen(pic_info[i].path[li_j]) > 0)
            {
                strcat(as_aliyun_image->name[i], pic_info[i].path[li_j]);
                strcat(as_aliyun_image->name[i], "/");
            }
        }

        strcat(as_aliyun_image->name[i], db_park_record->image_name[i]);
    }
    return 0;
}

/********************************
  函数:send_park_records_image_file
  功能:发送缓存到设备的泊车图片
  返回值:0 成功 -2 没有找到图片
 ********************************/
int send_park_records_image_file(DB_ParkRecord* db_park_record,
                                 EP_PicInfo pic_info[],
                                 str_aliyun_image *as_aliyun_image)
{

    char file_name_pic[NAME_MAX_LEN];
    char lc_image_path[PARK_PIC_NUM][128] = {{0}};
    int li_j = 0;
    int fd = 0;

    for(int i = 0; i < PARK_PIC_NUM; i++)
    {
        memset(file_name_pic,0,sizeof(file_name_pic));

        sprintf(file_name_pic, "%s/%s/%s",
                db_park_record->partition_path,
                DISK_RECORD_PATH,
                db_park_record->image_path[i]);

        if (access(file_name_pic, F_OK) < 0)
        {
            log_debug_storage("%s does not exist !\n", file_name_pic);
            return -2;
        }

        strcat(lc_image_path[i], file_name_pic);
        strcat(lc_image_path[i], db_park_record->image_name[i]);
        fd = open(lc_image_path[i], O_RDONLY);
        as_aliyun_image->size[i] = read(fd, as_aliyun_image->buf[i], 1024*1024);

        close(fd);

        //ftp and aliyun 续传信息赋值
        for(li_j=0; li_j<6; li_j++)
        {
            if(strlen(pic_info[i].path[li_j]) > 0)
            {
                strcat(as_aliyun_image->name[i], pic_info[i].path[li_j]);
                strcat(as_aliyun_image->name[i], "/");
            }
        }

        //strcat(pic_info[i].path[], as_aliyun_image->name[i]);
        strcat(as_aliyun_image->name[i], db_park_record->image_name[i]);

        TRACE_LOG_PACK_PLATFORM("file_name_pic=%s", file_name_pic);

        strcpy(pic_info[i].name, db_park_record->image_name[i]);
        strcat(file_name_pic, db_park_record->image_name[i]);

        //ftp 上传
        ftp_send_pic_file(file_name_pic,&pic_info[i],FTP_CHANNEL_PASSCAR_RESUMING); // FTP_CHANNEL_PASSCAR

    }

    return 0;
}


//删除指定ID 的一条记录
//删除指定ID 的记录－－对于非历史记录
//或清理续传标志--对于历史记录
int db_delete_park_records(char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records)
{
    char *pzErrMsg = NULL;
    int rc;
    sqlite3 *db = NULL;
    char sql[1024];

    DB_ParkRecord * park_record = (DB_ParkRecord *) records;

    pthread_mutex_lock(mutex_db_records);
    rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK)
    {
        printf("Create database %s failed!\n", db_name);
        printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
        sqlite3_close(db);
        pthread_mutex_unlock(mutex_db_records);
        return -1;
    }
    else
    {
        printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
    }
    /*
       rc = db_create_table(db, SQL_CREATE_TABLE_park_recordS);
       if (rc < 0)
       {
       printf("db_create_table failed\n");
       sqlite3_close(db);
       pthread_mutex_unlock(mutex_db_records);
       return -1;
       }
       */
    //查询数据
    memset(sql, 0, sizeof(sql));

    if (park_record->flag_store==1)
    {
        //清理续传标志
        sprintf(sql, "UPDATE park_records SET flag_send=0 WHERE ID = %d ;", park_record->ID);
    }
    else
    {
        //删除指定ID 的记录	//只有在一个数据库全部续传完成时，才能删除这条对应的记录
        sprintf(sql, "DELETE FROM park_records WHERE ID = %d ;", park_record->ID);
    }

    printf("the sql is %s\n", sql);
    rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
    if (rc != SQLITE_OK)
    {
        printf("Delete data failed, flag_store=%d,  Error Message: %s\n", park_record->flag_store, pzErrMsg);
    }

    sqlite3_free(pzErrMsg);

    //  表为空?
    //	remove(db_name);

    sqlite3_close(db);
    pthread_mutex_unlock(mutex_db_records);

    if (rc != SQLITE_OK)
    {
        return -1;
    }

    printf("db_delete_park_records  finish!\n");

    return 0;
}
#endif
