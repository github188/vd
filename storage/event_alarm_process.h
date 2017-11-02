
#ifndef _EVENT_ALARM_PROCESS_H_
#define _EVENT_ALARM_PROCESS_H_

#include "upload.h"


/* 创建事件表 */
#define SQL_CREATE_TABLE_EVENT_ALARM \
	"CREATE TABLE event_alarm(ID INTEGER PRIMARY KEY,\
type INTEGER,\
description VARCHAR(128),\
name VARCHAR(64),\
point_id VARCHAR(32),\
point_name VARCHAR(100),\
image_path1 VARCHAR(128),\
image_path2 VARCHAR(128),\
image_path3 VARCHAR(128),\
image_path4 VARCHAR(128),\
partition_path VARCHAR(64),\
dev_id VARCHAR(32),\
time VARCHAR(32),\
ftp_user VARCHAR(32),\
ftp_passwd VARCHAR(32),\
ftp_ip VARCHAR(128),\
ftp_port INTEGER,\
flag_send INTEGER,\
flag_store INTEGER);"
#define NUM_COLUMN_EVENT_ALARM 19

#define EVENT_ALARM_IMAGE_NUM 	2 					/* 事件报警使用第3张图片 */
#define EVENT_ALARM_FTP_DIR 	"event_alarm" 		/* 事件报警的FTP文件夹 */

typedef struct
{
	int 	ID;
	int 	type; 					/* 事件类型 */
	char 	description[128]; 		/* 事件描述 */
	char 	name[64]; 				/* 事件名称 */
	char 	point_id[32]; 			/* 监测点编号 */
	char 	point_name[100]; 		/* 监测点名称 */
	char 	image_path[4][256]; 	/* 图像相对路径 */
	char 	partition_path[256];	/* 存储的分区挂载路径 */
	char 	dev_id[32]; 			/* 设备编号 */
	char 	time[32]; 				/* 检测时间 */
	char 	ftp_user[32]; 			/* FTP用户名 */
	char 	ftp_passwd[32]; 		/* FTP密码 */
	char 	ftp_ip[128]; 			/* FTP服务器IP地址 */
	int 	ftp_port; 				/* FTP服务器端口 */
	int 	flag_send;				/* 续传标志: 使用低3位: 
		第0位表示消息:0,不需要续传消息；1，需要续传消息；  
		第1位表示图片:0,不需要续传图片；1，需要续传图片； */
	int 	flag_store;				/* 存储标志: 0:不是实时存储，1，实时存储 */		
} DB_EventAlarm; 					/* 数据库使用的事件报警结构体 */


int process_event_alarm(const void *image_info);
int analyze_event_alarm_picture(EP_PicInfo pic_info[], const void *image_info);
int analyze_event_alarm_info(
    DB_EventAlarm *db_event_alarm, const void *image_info,
    const EP_PicInfo pic_info[], const char *partition_path);
int format_mq_text_event_alarm(char *mq_text, const DB_EventAlarm *event_alarm);
int db_write_event_alarm(char *db_name, DB_EventAlarm *event, pthread_mutex_t *mutex_db_records);
int db_format_insert_sql_event_alarm(char *sql, DB_EventAlarm *event);
int send_event_alarm_info(void *db_event_alarm);
int save_event_alarm_info(const DB_EventAlarm *db_event_records,
                          const void *image_info, const char *partition_path);
int send_event_alarm_info_history(void *db_event_alarm, int dest_mq, int num_record);
int send_event_alarm_image_buf(
    const EP_PicInfo pic_info[], unsigned int pic_num);
int save_event_alarm_image_buf(
    const EP_PicInfo pic_info[], unsigned int pic_num,
    const char *partition_path);
int send_event_alarm_image_file(DB_EventAlarm* db_record,
                                EP_PicInfo pic_info[], int pic_num);
int db_unformat_read_sql_event_alarm(char *azResult[],  DB_EventAlarm *event);
int db_read_event_records(char *db_name, void *records, pthread_mutex_t *mutex_db_records, char * sql_cond);
int db_delete_event_records(char *db_name, void *records, pthread_mutex_t *mutex_db_records);


#endif 	/* _EVENT_ALARM_PROCESS_H_ */
