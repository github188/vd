
#ifndef _VIOLATION_RECORDS_PROCESS_H_
#define _VIOLATION_RECORDS_PROCESS_H_


#include "upload.h"


/* ����������Υ����¼�� */
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
	int 	plate_type; 				/* �������� */
	char 	plate_num[32]; 				/* ���ƺ��� */
	char 	time[32]; 					/* �ɼ�ʱ�� */
	int 	violation_type; 			/* Υ������ */
	char 	point_id[32]; 				/* �ɼ����� */
	char 	point_name[100]; 			/* �ɼ������� */
	char 	collection_agencies[32]; 	/* �ɼ����ش��� */
	char 	dev_id[32]; 				/* �豸��� */
	int 	direction; 					/* ������ */
	int 	lane_num; 					/* ������ */
	char 	red_lamp_start_time[32]; 	/* �����ʼʱ�� */
	int 	red_lamp_keep_time; 		/* ��Ƴ���ʱ�� */
	char 	image_path[4][256]; 		/* ͼ�����·�� */
	char 	video_path[256]; 			/* ��Ƶ���·�� */
	char 	partition_path[64]; 		/* �洢�ķ�������·�� */
	int 	speed; 						/* ���� */
	int 	coordinate_x; 				/* ����X���� */
	int 	coordinate_y; 				/* ����Y���� */
	int 	width; 						/* ������ */
	int 	height; 					/* ����߶� */
	int 	pic_flag; 					/* �����ڵڼ���ͼƬ�� */
	int 	plate_color; 				/* ������ɫ */
	char 	description[128]; 			/* Υ������ */
	char 	ftp_user[128]; 				/* FTP�û��� */
	char 	ftp_passwd[128]; 			/* FTP���� */
	char 	ftp_ip[128]; 				/* FTP������IP��ַ */
	int 	ftp_port; 					/* FTP�������˿� */
	char 	video_ftp_user[128]; 		/* ��ƵFTP�û��� */
	char 	video_ftp_passwd[128]; 		/* ��ƵFTP���� */
	char 	video_ftp_ip[128]; 			/* ��ƵFTP������IP��ַ */
	int 	video_ftp_port; 			/* ��ƵFTP�������˿� */
	int 	flag_send;					/* ������־: ʹ�õ�3λ: 
		��0λ��ʾ��Ϣ:0,����Ҫ������Ϣ��1����Ҫ������Ϣ��  
		��1λ��ʾͼƬ:0,����Ҫ����ͼƬ��1����Ҫ����ͼƬ�� 
		��2λ��ʾΥ����Ƶ:0,����Ҫ����Υ����Ƶ��1����Ҫ����Υ����Ƶ��   */
	int 	flag_store;				/* �洢��־: 0:����ʵʱ�洢��1��ʵʱ�洢 */	
	int 	encode_type; 				/* ͼ���������.   3��1, 4��1, */
	int 	vehicle_type; 				/* ���� */
	int 	color; 						/* ������ɫ */
	int 	vehicle_logo; 				/* ���� */
} DB_ViolationRecordsMotorVehicle; 		/* ���ݿ�ʹ�õĻ�����Υ����¼�ṹ�� */


/* TODO �������˻�ǻ�����Υ����¼�� */
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
