
#ifndef _PARK_RECORDS_PROCESS_H_
#define _PARK_RECORDS_PROCESS_H_


#include "upload.h"
#include "../../../ipnc_mcfw/mcfw/interfaces/link_api/jpeg_enc_info.h"
#include "Msg_Def.h"


/* �������ݿ�����ݱ�*/
#define SQL_CREATE_TABLE_PARK_RECORDS \
	"CREATE TABLE park_records(ID INTEGER PRIMARY KEY,\
plate_num VARCHAR(32),\
plate_type INTEGER,\
point_id VARCHAR(32),\
point_name VARCHAR(100),\
dev_id VARCHAR(32),\
lane_num INTEGER,\
speed INTEGER,\
time VARCHAR(32),\
collection_agencies VARCHAR(32),\
direction INTEGER,\
image_path1 VARCHAR(128),\
image_path2 VARCHAR(128),\
partition_path VARCHAR(64),\
color INTEGER,\
vehicle_logo INTEGER,\
objective_type INTEGER,\
coordinate_x1 INTEGER,\
coordinate_y1 INTEGER,\
width1 INTEGER,\
height1 INTEGER,\
coordinate_x2 INTEGER,\
coordinate_y2 INTEGER,\
width2 INTEGER,\
height2 INTEGER,\
plate_color INTEGER,\
description VARCHAR(128),\
ftp_user VARCHAR(128),\
ftp_passwd VARCHAR(128),\
ftp_ip VARCHAR(128),\
ftp_port INTEGER,\
flag_send INTEGER,\
flag_store INTEGER,\
vehicle_type INTEGER,\
confidence1 INTEGER,\
confidence2 INTEGER,\
objectState INTEGER,\
status INTEGER,\
image_name1 VARCHAR(128),\
image_name2 VARCHAR(128));"

#define NUM_COLUMN_TRAFFIC_RECORDS 30 	//�ֶ���Ŀ����Ҫ��SQL_CREATE_ �걣��һ�¡������ж����ݿ��б���ֶ����Ƿ������汾������ֶ���ƥ��

#define PARK_RECORDS_FTP_DIR 	"Park"

#define portnumber_berth 3333

#define PARK_PIC_NUM 2


typedef struct
{
	int 	ID;
	char 	plate_num[32]; 				/* ���ƺ��� */
	int 	plate_type; 				/* �������� */
	char 	point_id[32]; 				/* �ɼ����� */
	char 	point_name[100]; 			/* �ɼ������� */
	char 	dev_id[32]; 				/* �豸��� */
	int 	lane_num; 					/* ������ */
	int 	speed; 						/* ���� */
	char 	time[32]; 					/* ץ��ʱ�� */
	char 	collection_agencies[32]; 	/* �ɼ����ش��� */
	int 	direction; 					/* ������ */
	char 	image_path[2][256]; 			/* ͼ�����·�� */
	char    image_name[2][256];         // ͼƬ����            add by shp 2015/04/24
	char    aliyun_image_path[2][256];  // aliyunͼƬ���·��  add by shp 2015/04/24
	char 	partition_path[64]; 		// �洢�ĸ�·��
	int 	color; 						/* ������ɫ */
	int 	vehicle_logo; 				/* ���� */
	int 	objective_type; 			/* Ŀ������ */
	int 	coordinate_x[2]; 				/* ����X���� */
	int 	coordinate_y[2]; 				/* ����Y���� */
	int 	width[2]; 						/* ������ */
	int 	height[2]; 					/* ����߶� */
//	int 	pic_flag[2]; 					/* �����ڵڼ���ͼƬ�� */
	int 	plate_color; 				/* ������ɫ */
	char 	description[128]; 			/* ͨ������ */
	char 	ftp_user[128]; 				/* FTP�û��� */
	char 	ftp_passwd[128]; 			/* FTP���� */
	char 	ftp_ip[128]; 				/* FTP������IP��ַ */
	int 	ftp_port; 					/* FTP�������˿� */
	int 	flag_send;				/* ������־: ʹ�õ�3λ:
		��0λ��ʾ��Ϣ:0,����Ҫ������Ϣ��1����Ҫ������Ϣ��
		��1λ��ʾͼƬ:0,����Ҫ����ͼƬ��1����Ҫ����ͼƬ�� */
	int 	flag_store;				/* �洢��־: 0:����ʵʱ�洢��1��ʵʱ�洢 */
	int 	vehicle_type; 				/* ���� */
	int   confidence[2];						//���Ŷ�
	int 	objectState;                /*ʻ��ʻ�뿪״̬(1:ʻ�� 0:ʻ��)*/
	int  	status;             //ʻ��ʻ��״̬��Ϣ(ʻ��:0--����ʻ�룬1--���淶ͣ��
								//ʻ��: 0--����ʻ�� 1--�ڵ�����ʻ�� ʱ���쳣: ������ʶ���㷨Ҳ��֪����������ʻ��ʻ��ʱ���ʱ���ø�״̬)
								//4--����ʻ�룬�ڵ��������������ʻ���
} DB_ParkRecord; 					/* ���ݿ�ʹ�õ�ͨ�м�¼�ṹ�� */


struct Park_record
{
	DB_ParkRecord db_park_record;
	EP_PicInfo pic_info[PARK_PIC_NUM];
};

#ifdef  __cplusplus
extern "C"
{
#endif


int process_park_records(const SystemVpss_SimcopJpegInfo *info);

/****************������ͼƬ****************/
int analyze_park_record_picture(
     EP_PicInfo pic_info[], const SystemVpss_SimcopJpegInfo *info);
int send_park_records_image_buf(EP_PicInfo pic_info[],int pic_num);
int send_park_records_image_aliyun(EP_PicInfo pic_info[], int pic_num);
//�������ش洢
int save_park_record_image(
    const EP_PicInfo pic_info[], const char *park_stroge_path);

/****************��������Ϣ****************/
int analyze_park_record_info_motor_vehicle(
    DB_ParkRecord *db_park_record, const ParkVehicleInfo *result,
    const EP_PicInfo pic_info[], const char *partition_path);
int send_park_record_info(const DB_ParkRecord *db_park_record);

int format_mq_text_park_record(
     char *mq_text, const DB_ParkRecord *record);
int format_mq_text_park_record_motor_vehicle(
    char *mq_text, const DB_ParkRecord *record);
int format_mq_text_park_record_others(
    char *mq_text, const DB_ParkRecord *record);

//�����洢
int save_park_record_info(
    const DB_ParkRecord *db_park_record,
    const EP_PicInfo pic_info[], const char *park_stroge_path);

int db_write_park_records(char *db_traffic_name, DB_ParkRecord *records,
                             pthread_mutex_t *mutex_db_records);

int db_format_insert_sql_park_records(char *sql, DB_ParkRecord *records);



/****************�����������������Ϣ****************/
int park_enter_process_fun(DB_ParkRecord db_park_record);
int park_leave_process_fun(DB_ParkRecord db_park_record);
int	park_light_process_fun(Lighting_park* lightInfo);



int process_park_alarm(MSGDATACMEM *msg_alg_result_info);
int process_park_light(MSGDATACMEM *msg_alg_result_info);
int process_park_leave_info(MSGDATACMEM *msg_alg_result_info);
int process_park_status_info(MSGDATACMEM *msg_alg_result_info);
int process_snap_return_info(MSGDATACMEM *msg_alg_result_info);


int db_read_park_record(const char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records, char * sql_cond);

int db_unformat_read_sql_park_records(
    char *azResult[], DB_ParkRecord *traffic_record);


/****************������غ���****************/
int aliyun_reupload_info(
    DB_ParkRecord* db_park_record, EP_PicInfo pic_info[], str_aliyun_image *as_aliyun_image);
int send_park_records_image_file(
    DB_ParkRecord* db_park_record, EP_PicInfo pic_info[], str_aliyun_image *as_aliyun_image);
int db_delete_park_records(char *db_name, void *records,
                              pthread_mutex_t *mutex_db_records);




/****************��λ״̬��غ���****************/
//int status_berth(const Lighting_park *lightInfo);
void *status_berth(void *args);



void light_set_curr_status(const Lighting_park *light);
Lighting_park* light_get_curr_status(void);
int32_t light_ctrl_put(const Lighting_park *light);
#ifdef  __cplusplus
}
#endif


#endif 	/* _TRAFFIC_RECORDS_PROCESS_H_ */
