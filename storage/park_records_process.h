
#ifndef _PARK_RECORDS_PROCESS_H_
#define _PARK_RECORDS_PROCESS_H_


#include "upload.h"
#include "../../../ipnc_mcfw/mcfw/interfaces/link_api/jpeg_enc_info.h"
#include "Msg_Def.h"


/* 泊车数据库的数据表*/
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

#define NUM_COLUMN_TRAFFIC_RECORDS 30 	//字段数目，需要与SQL_CREATE_ 宏保持一致。用于判断数据库中表的字段数是否与程序版本处理的字段数匹配

#define PARK_RECORDS_FTP_DIR 	"Park"

#define portnumber_berth 3333

#define PARK_PIC_NUM 2


typedef struct
{
	int 	ID;
	char 	plate_num[32]; 				/* 车牌号码 */
	int 	plate_type; 				/* 号牌类型 */
	char 	point_id[32]; 				/* 采集点编号 */
	char 	point_name[100]; 			/* 采集点名称 */
	char 	dev_id[32]; 				/* 设备编号 */
	int 	lane_num; 					/* 车道号 */
	int 	speed; 						/* 车速 */
	char 	time[32]; 					/* 抓拍时间 */
	char 	collection_agencies[32]; 	/* 采集机关代号 */
	int 	direction; 					/* 方向编号 */
	char 	image_path[2][256]; 			/* 图像相对路径 */
	char    image_name[2][256];         // 图片名字            add by shp 2015/04/24
	char    aliyun_image_path[2][256];  // aliyun图片相对路径  add by shp 2015/04/24
	char 	partition_path[64]; 		// 存储的根路径
	int 	color; 						/* 车身颜色 */
	int 	vehicle_logo; 				/* 车标 */
	int 	objective_type; 			/* 目标类型 */
	int 	coordinate_x[2]; 				/* 区域X坐标 */
	int 	coordinate_y[2]; 				/* 区域Y坐标 */
	int 	width[2]; 						/* 区域宽度 */
	int 	height[2]; 					/* 区域高度 */
//	int 	pic_flag[2]; 					/* 车牌在第几张图片上 */
	int 	plate_color; 				/* 车牌颜色 */
	char 	description[128]; 			/* 通行描述 */
	char 	ftp_user[128]; 				/* FTP用户名 */
	char 	ftp_passwd[128]; 			/* FTP密码 */
	char 	ftp_ip[128]; 				/* FTP服务器IP地址 */
	int 	ftp_port; 					/* FTP服务器端口 */
	int 	flag_send;				/* 续传标志: 使用低3位:
		第0位表示消息:0,不需要续传消息；1，需要续传消息；
		第1位表示图片:0,不需要续传图片；1，需要续传图片； */
	int 	flag_store;				/* 存储标志: 0:不是实时存储，1，实时存储 */
	int 	vehicle_type; 				/* 车型 */
	int   confidence[2];						//置信度
	int 	objectState;                /*驶入驶离开状态(1:驶入 0:驶离)*/
	int  	status;             //驶入驶离状态信息(驶入:0--正常驶入，1--不规范停车
								//驶离: 0--正常驶离 1--遮挡车牌驶离 时间异常: 用来标识在算法也不知道车辆具体驶入驶出时间的时候，置该状态)
								//4--补发驶离，在掉电或重启过程中驶离的
} DB_ParkRecord; 					/* 数据库使用的通行记录结构体 */


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

/****************处理泊车图片****************/
int analyze_park_record_picture(
     EP_PicInfo pic_info[], const SystemVpss_SimcopJpegInfo *info);
int send_park_records_image_buf(EP_PicInfo pic_info[],int pic_num);
int send_park_records_image_aliyun(EP_PicInfo pic_info[], int pic_num);
//断网本地存储
int save_park_record_image(
    const EP_PicInfo pic_info[], const char *park_stroge_path);

/****************处理泊车信息****************/
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

//断网存储
int save_park_record_info(
    const DB_ParkRecord *db_park_record,
    const EP_PicInfo pic_info[], const char *park_stroge_path);

int db_write_park_records(char *db_traffic_name, DB_ParkRecord *records,
                             pthread_mutex_t *mutex_db_records);

int db_format_insert_sql_park_records(char *sql, DB_ParkRecord *records);



/****************处理中心软件泊车信息****************/
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


/****************续传相关函数****************/
int aliyun_reupload_info(
    DB_ParkRecord* db_park_record, EP_PicInfo pic_info[], str_aliyun_image *as_aliyun_image);
int send_park_records_image_file(
    DB_ParkRecord* db_park_record, EP_PicInfo pic_info[], str_aliyun_image *as_aliyun_image);
int db_delete_park_records(char *db_name, void *records,
                              pthread_mutex_t *mutex_db_records);




/****************泊位状态相关函数****************/
//int status_berth(const Lighting_park *lightInfo);
void *status_berth(void *args);



void light_set_curr_status(const Lighting_park *light);
Lighting_park* light_get_curr_status(void);
int32_t light_ctrl_put(const Lighting_park *light);
#ifdef  __cplusplus
}
#endif


#endif 	/* _TRAFFIC_RECORDS_PROCESS_H_ */
