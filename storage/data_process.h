
#ifndef _DATA_PROCESS_H_
#define _DATA_PROCESS_H_

#include <pthread.h>

#include "Appro_interface.h"
#include "upload.h"
#include "arm_config.h"
#include "ep_type.h"
#include "disk_mng.h"


extern ARM_config 		g_arm_config;
extern SET_NET_PARAM 	g_set_net_param;


enum
{
    REALTIME_STREAM, 					//实时流
    SNAPSHOT, 							//抓拍图像
    TRAFFIC_SIGNAL, 					//交通信号
    TRAFFIC_RECORDS_MOTOR_VEHICLES, 	//机动车通行记录
    TRAFFIC_RECORDS_OTHERS, 			//行人或非机动车通行记录
    VIOLATION_RECORDS_MOTOR_VEHICLE, 	//机动车违法记录
    VIOLATION_RECORDS_OTHERS, 			//行人或非机动车违法记录
    EVENT_ALARM, 						//事件报警
    TRAFFIC_FLOW, 						//交通流
};


#define IMAGE_INFO_SIZE 	56 			//图像信息大小
#define IMAGE_PADDING_SIZE 	512 		//图像填充大小

#define MQ_TEXT_BUF_SIZE 	2048 		//MQ文本缓冲区大小

#define STORAGE_PATH_DEPTH 	6 			//存储路径深度
#define FILE_NAME_MAX_LEN 	64 			//图片名最大长度

#define EP_TIME_STR_LEN 	24 			//时间字符串长度

#define EP_MANUFACTURER 	"IPNC" 	//设备厂商  //"bitcom"  "IPNC"
#define EP_DATA_SOURCE 		"1" 		//数据源是电警

#define EP_DATA_SOURCE_HISENSE "9"         //违法数据来源_海信平台
#define DATA_SOURCE_HISENSE "4"         //违法数据来源_海信平台

#define DATA_SOURCE_PASSCAR "2"        //数据源是卡口
#define DATA_SOURCE_PARK    "4"        //数据源是泊车
#define DATA_SOURCE_ILLEGAL "5"        //数据源是违停
#define EP_DEV_NAME 		"VD_2014" //设备名称
#define EP_SNAP_TYPE 		"1" 		//抓拍类型（0-无 1-图片 2-录像）
#define EP_DEV_ID 			g_set_net_param.m_NetParam.m_DeviceID
#define EP_SECTION_ID 		g_arm_config.basic_param.road_id
#define EP_SECTION_NAME 	g_arm_config.basic_param.spot
#define EP_POINT_ID 		g_arm_config.basic_param.spot_id
#define EP_POINT_NAME 		g_arm_config.basic_param.spot
#define EP_EXP_DEV_ID 		g_arm_config.basic_param.exp_device_id
#define EP_DIRECTION 		g_arm_config.basic_param.direction
#define EP_TRAFFIC_FTP 		g_arm_config.basic_param.ftp_param_pass_car
#define EP_VIOLATION_FTP 	g_arm_config.basic_param.ftp_param_illegal
#define EP_VIDEO_FTP 		g_arm_config.basic_param.ftp_param_h264
#define EP_COLLECTION_AGENCIES_SIZE	\
	(g_arm_config.basic_param.collect_actor_size + 1)
#define EP_FTP_URL_LEVEL 	g_set_net_param.m_NetParam.ftp_url_level
#define EP_UPLOAD_CFG 		g_arm_config.basic_param.data_save.ftp_data_config
#define EP_DISK_SAVE_CFG 	g_arm_config.basic_param.data_save.disk_wri_data
#define EP_REUPLOAD_CFG \
	g_arm_config.basic_param.data_save.resume_upload_data


/*
字段数：20131014，加实时存储字段后：

索引数据库：6
过车：	29
违法：	37
事件：	19
交通流：23
*/

/* 创建通行记录表，机动车、非机动车和行人都使用该表 */
#define SQL_CREATE_TABLE_DB_FILES \
	"CREATE TABLE DB_files(ID INTEGER PRIMARY KEY,\
record_type INTEGER,\
time VARCHAR(32),\
record_path VARCHAR(128),\
flag_send INTEGER,\
flag_store INTEGER);"
#define NUM_COLUMN_DB_FILES 6


typedef struct
{
	int 	ID;
	int 	record_type; 		/* 数据库类型 */
	char 	time[32]; 			/* 建立数据库的时间 */
	char 	record_path[128]; 	/* 存储的路径（全路径，含文件名） */
	int 	flag_send;			/* 续传标志: 0，不需续传；1，ftp需要续传 2, aliyun需要续传*/
	int 	flag_store;			/* 存储标志: 0，不是实时存储；1，实时存储 */
} DB_File; 						/* 数据库使用的通行记录结构体 */


#ifdef  __cplusplus
extern "C"
{
#endif

unsigned long long get_base_time(void);
int get_pic_info_lock(
    AV_DATA *av_data, VD_PicInfo *pic_info, unsigned int pic_id);
int get_pic_info_unlock(AV_DATA *av_data, unsigned int pic_id);

void init_data_process();
//int data_process(const void *image_info, const void *video_info);

int file_save(const char *dir_disk, const char dir[][80],
              const char *name, const void *buf, size_t buf_size);
int file_save_append(const char *dir_disk, const char dir[][80],
                     const char *name, const void *buf, size_t buf_size);
int dir_create(char *dir );

void start_db_upload_handler_thr();
void *db_upload_Thr(void *arg);

int db_unformat_read_sql_db_files( char *azResult[], DB_File *buf);
int db_read_DB_file(const char *db_name, DB_File  *db_file, pthread_mutex_t *mutex_db_files, char * sql_cond);
int db_column_num_DB_file(const char *db_name, void *records, pthread_mutex_t *mutex_db_records);
int db_count_records(const char *db_name, char *sql_cond,pthread_mutex_t *mutex_db_records);//函数与数据记录内容无关
int db_update_records(const char *db_name, char *sql_cond, pthread_mutex_t *mutex_db_records);
int check_DB_File_columns(const char *db_name, int num_column_now, pthread_mutex_t *mutex_db_files);
int db_delete_DB_file(const char *db_name, DB_File  *db_file, pthread_mutex_t *mutex_db_files);
int db_write_DB_file(const char *db_name, DB_File  *db_file, pthread_mutex_t *mutex_db_files);


int get_record_count(TYPE_HISTORY_RECORD *type_history_record);
int start_upload_history_record(TYPE_HISTORY_RECORD *type_history_record);
int stop_upload_history_record();


int get_hour_start(const char *time_start, char *history_time_start);
int get_hour_end(const char *time_end, char *history_time_end);

int move_record_to_trash(const char *partition_path, const char *file_path);
int32_t db_record_clr(const char *path, pthread_mutex_t *mutex);

#ifdef  __cplusplus
}
#endif


#endif 	/* _DATA_PROCESS_H_ */
