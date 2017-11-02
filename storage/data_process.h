
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
    REALTIME_STREAM, 					//ʵʱ��
    SNAPSHOT, 							//ץ��ͼ��
    TRAFFIC_SIGNAL, 					//��ͨ�ź�
    TRAFFIC_RECORDS_MOTOR_VEHICLES, 	//������ͨ�м�¼
    TRAFFIC_RECORDS_OTHERS, 			//���˻�ǻ�����ͨ�м�¼
    VIOLATION_RECORDS_MOTOR_VEHICLE, 	//������Υ����¼
    VIOLATION_RECORDS_OTHERS, 			//���˻�ǻ�����Υ����¼
    EVENT_ALARM, 						//�¼�����
    TRAFFIC_FLOW, 						//��ͨ��
};


#define IMAGE_INFO_SIZE 	56 			//ͼ����Ϣ��С
#define IMAGE_PADDING_SIZE 	512 		//ͼ������С

#define MQ_TEXT_BUF_SIZE 	2048 		//MQ�ı���������С

#define STORAGE_PATH_DEPTH 	6 			//�洢·�����
#define FILE_NAME_MAX_LEN 	64 			//ͼƬ����󳤶�

#define EP_TIME_STR_LEN 	24 			//ʱ���ַ�������

#define EP_MANUFACTURER 	"IPNC" 	//�豸����  //"bitcom"  "IPNC"
#define EP_DATA_SOURCE 		"1" 		//����Դ�ǵ羯

#define EP_DATA_SOURCE_HISENSE "9"         //Υ��������Դ_����ƽ̨
#define DATA_SOURCE_HISENSE "4"         //Υ��������Դ_����ƽ̨

#define DATA_SOURCE_PASSCAR "2"        //����Դ�ǿ���
#define DATA_SOURCE_PARK    "4"        //����Դ�ǲ���
#define DATA_SOURCE_ILLEGAL "5"        //����Դ��Υͣ
#define EP_DEV_NAME 		"VD_2014" //�豸����
#define EP_SNAP_TYPE 		"1" 		//ץ�����ͣ�0-�� 1-ͼƬ 2-¼��
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
�ֶ�����20131014����ʵʱ�洢�ֶκ�

�������ݿ⣺6
������	29
Υ����	37
�¼���	19
��ͨ����23
*/

/* ����ͨ�м�¼�����������ǻ����������˶�ʹ�øñ� */
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
	int 	record_type; 		/* ���ݿ����� */
	char 	time[32]; 			/* �������ݿ��ʱ�� */
	char 	record_path[128]; 	/* �洢��·����ȫ·�������ļ����� */
	int 	flag_send;			/* ������־: 0������������1��ftp��Ҫ���� 2, aliyun��Ҫ����*/
	int 	flag_store;			/* �洢��־: 0������ʵʱ�洢��1��ʵʱ�洢 */
} DB_File; 						/* ���ݿ�ʹ�õ�ͨ�м�¼�ṹ�� */


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
int db_count_records(const char *db_name, char *sql_cond,pthread_mutex_t *mutex_db_records);//���������ݼ�¼�����޹�
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
