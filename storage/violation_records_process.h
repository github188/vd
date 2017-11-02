
#ifndef _VIOLATION_RECORDS_PROCESS_H_
#define _VIOLATION_RECORDS_PROCESS_H_


#include "upload.h"


/* 创建机动车违法记录表 */
#define SQL_CREATE_TABLE_VIOLATION_RECORDS_MOTOR_VEHICLE \
	"CREATE TABLE violation_records_motor_vehicle(ID INTEGER PRIMARY KEY,\
plate_type INTEGER,\
plate_num VARCHAR(32),\
time VARCHAR(32),\
violation_type INTEGER,\
point_id VARCHAR(32),\
point_name VARCHAR(100),\
collection_agencies VARCHAR(32),\
dev_id VARCHAR(32),\
direction INTEGER,\
lane_num INTEGER,\
red_lamp_start_time TIMESTAMP,\
red_lamp_keep_time INTEGER,\
image_path1 VARCHAR(128),\
image_path2 VARCHAR(128),\
image_path3 VARCHAR(128),\
image_path4 VARCHAR(128),\
video_path VARCHAR(128),\
partition_path VARCHAR(64),\
speed INTEGER,\
coordinate_x INTEGER,\
coordinate_y INTEGER,\
width INTEGER,\
height INTEGER,\
pic_flag INTEGER,\
plate_color INTEGER,\
description VARCHAR(128),\
ftp_user VARCHAR(128),\
ftp_passwd VARCHAR(128),\
ftp_ip VARCHAR(128),\
ftp_port INTEGER,\
video_ftp_user VARCHAR(128),\
video_ftp_passwd VARCHAR(128),\
video_ftp_ip VARCHAR(128),\
video_ftp_port INTEGER,\
flag_send INTEGER,\
flag_store INTEGER,\
encode_type INTEGER,\
vehicle_type INTEGER,\
color INTEGER,\
vehicle_logo INTEGER);"
#define NUM_COLUMN_VIOLATION_RECORDS 39

#define DB_VIOLATION_RECORDS_MOTOR_VEHICLE_NAME \
	"violation_records_motor_vehicle.db"
#define VIOLATION_RECORDS_FTP_DIR 	"weifa"

typedef struct
{
	int 	ID;
	int 	plate_type; 				/* 号牌类型 */
	char 	plate_num[32]; 				/* 车牌号码 */
	char 	time[32]; 					/* 采集时间 */
	int 	violation_type; 			/* 违法类型 */
	char 	point_id[32]; 				/* 采集点编号 */
	char 	point_name[100]; 			/* 采集点名称 */
	char 	collection_agencies[32]; 	/* 采集机关代号 */
	char 	dev_id[32]; 				/* 设备编号 */
	int 	direction; 					/* 方向编号 */
	int 	lane_num; 					/* 车道号 */
	char 	red_lamp_start_time[32]; 	/* 红灯起始时间 */
	int 	red_lamp_keep_time; 		/* 红灯持续时间 */
	char 	image_path[4][256]; 		/* 图像相对路径 */
	char 	video_path[256]; 			/* 视频相对路径 */
	char 	partition_path[64]; 		/* 存储的分区挂载路径 */
	int 	speed; 						/* 车速 */
	int 	coordinate_x; 				/* 区域X坐标 */
	int 	coordinate_y; 				/* 区域Y坐标 */
	int 	width; 						/* 区域宽度 */
	int 	height; 					/* 区域高度 */
	int 	pic_flag; 					/* 车牌在第几张图片上 */
	int 	plate_color; 				/* 车牌颜色 */
	char 	description[128]; 			/* 违法描述 */
	char 	ftp_user[128]; 				/* FTP用户名 */
	char 	ftp_passwd[128]; 			/* FTP密码 */
	char 	ftp_ip[128]; 				/* FTP服务器IP地址 */
	int 	ftp_port; 					/* FTP服务器端口 */
	char 	video_ftp_user[128]; 		/* 视频FTP用户名 */
	char 	video_ftp_passwd[128]; 		/* 视频FTP密码 */
	char 	video_ftp_ip[128]; 			/* 视频FTP服务器IP地址 */
	int 	video_ftp_port; 			/* 视频FTP服务器端口 */
	int 	flag_send;					/* 续传标志: 使用低3位: 
		第0位表示消息:0,不需要续传消息；1，需要续传消息；  
		第1位表示图片:0,不需要续传图片；1，需要续传图片； 
		第2位表示违法视频:0,不需要续传违法视频；1，需要续传违法视频；   */
	int 	flag_store;				/* 存储标志: 0:不是实时存储，1，实时存储 */	
	int 	encode_type; 				/* 图像编码类型.   3合1, 4合1, */
	int 	vehicle_type; 				/* 车型 */
	int 	color; 						/* 车身颜色 */
	int 	vehicle_logo; 				/* 车标 */
} DB_ViolationRecordsMotorVehicle; 		/* 数据库使用的机动车违法记录结构体 */


/* TODO 创建行人或非机动车违法记录表 */
#define SQL_CREATE_TABLE_VIOLATION_RECORDS_OTHERS \
	"CREATE TABLE violation_records_others(ID INTEGER PRIMARY KEY,\
);"


int process_violation_records_motor_vehicle(
    const void *image_info, const void *video_info);
int analyze_violation_records_picture(
    EP_PicInfo pic_info[], const void *image_info);
int analyze_violation_records_motor_vehicle_info(
    DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const void *video_info,
    const EP_PicInfo pic_info[], const EP_VidInfo *vid_info,
    const char *partition_path);
int db_write_violation_records_others(char *db_violation_name, DB_ViolationRecordsMotorVehicle *records);
int db_write_violation_records_motor_vehicle(char *db_name, void *records, pthread_mutex_t *mutex_db_records);
int db_read_violation_records_motor_vehicle(char *db_name, void *records, pthread_mutex_t *mutex_db_records, char * sql_cond);
int db_delete_violation_records_motor_vehicle(char *db_name, void *records, pthread_mutex_t *mutex_db_records);
int db_format_insert_sql_violation_records_motor_vehicle(char *sql, void *buf);
int db_format_insert_sql_violation_records_others(char *sql, void *buf);
int db_unformat_read_sql_violation_records_motor_vehicle( char *azResult[], DB_ViolationRecordsMotorVehicle *buf);
int format_mq_text_violation_records_motor_vehicle(char *mq_text,
        DB_ViolationRecordsMotorVehicle *records);
int process_violation_records_others(
    const void *image_info, const void *video_info);
int send_violation_records_motor_vehicle_info(void *records);
int send_violation_records_motor_vehicle_info_history(void *records, int dest_mq, int num_record);
int send_violation_records_image_buf(EP_PicInfo pic_info[], int num_pic);
int send_violation_records_image_file(
    DB_ViolationRecordsMotorVehicle* db_violation_record,
    EP_PicInfo pic_info[], int num_pic);
int send_violation_records_video_buf(EP_VidInfo *vid_info);
int send_violation_records_video_file(
    DB_ViolationRecordsMotorVehicle* db_violation_record, EP_VidInfo *vid_info);
int analyze_violation_records_video(
    EP_VidInfo *vid_info, const void *image_info, const void *video_info);
int save_violation_records_image_buf(
    const EP_PicInfo pic_info[],
    const void *image_info, const char *partition_path);
int save_violation_records_video_buf(
    const EP_VidInfo *vid_info, const char *partition_path);
int save_violation_records_motor_vehicle_info(
    const DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const char *partition_path);

int process_vp_records(
    const void *image_info, const void *video_info);
int get_violation_code_index_PD(int type, int *code);
int analyze_PD_records_picture(
    EP_PicInfo pic_info[], const void *image_info);
int analyze_PD_records_motor_vehicle_info(
    DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const void *video_info,
    const EP_PicInfo pic_info[], const EP_VidInfo *vid_info,
    const char *partition_path);

int format_mq_text_vp_records_hisense(char *mq_text_hisense,
        DB_ViolationRecordsMotorVehicle *records);

int mq_send_vp_records_hisense(char *msg);

#endif 	/* _VIOLATION_RECORDS_PROCESS_H_ */
