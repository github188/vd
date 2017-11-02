
#ifndef _EVENT_ALARM_PROCESS_H_
#define _EVENT_ALARM_PROCESS_H_

#include "upload.h"


/* �����¼��� */
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

#define EVENT_ALARM_IMAGE_NUM 	2 					/* �¼�����ʹ�õ�3��ͼƬ */
#define EVENT_ALARM_FTP_DIR 	"event_alarm" 		/* �¼�������FTP�ļ��� */

typedef struct
{
	int 	ID;
	int 	type; 					/* �¼����� */
	char 	description[128]; 		/* �¼����� */
	char 	name[64]; 				/* �¼����� */
	char 	point_id[32]; 			/* ������ */
	char 	point_name[100]; 		/* �������� */
	char 	image_path[4][256]; 	/* ͼ�����·�� */
	char 	partition_path[256];	/* �洢�ķ�������·�� */
	char 	dev_id[32]; 			/* �豸��� */
	char 	time[32]; 				/* ���ʱ�� */
	char 	ftp_user[32]; 			/* FTP�û��� */
	char 	ftp_passwd[32]; 		/* FTP���� */
	char 	ftp_ip[128]; 			/* FTP������IP��ַ */
	int 	ftp_port; 				/* FTP�������˿� */
	int 	flag_send;				/* ������־: ʹ�õ�3λ: 
		��0λ��ʾ��Ϣ:0,����Ҫ������Ϣ��1����Ҫ������Ϣ��  
		��1λ��ʾͼƬ:0,����Ҫ����ͼƬ��1����Ҫ����ͼƬ�� */
	int 	flag_store;				/* �洢��־: 0:����ʵʱ�洢��1��ʵʱ�洢 */		
} DB_EventAlarm; 					/* ���ݿ�ʹ�õ��¼������ṹ�� */


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
