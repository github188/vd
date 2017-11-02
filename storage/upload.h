
#ifndef _MQ_API_H_
#define _MQ_API_H_


#include "partition_func.h"
#include "disk_mng.h"
#include "dsp_config.h"


#define FTP_PATH_LEN 		80 		/* FTP路径长度最大值 */
#define FTP_PATH_DEPTH 		6 		/* FTP路径深度 */


typedef struct __PicTime
{
	int 	year; 		/* 公元后年份 */
	int 	month; 		/* 月份，1-12 */
	int 	day; 		/* 每月的日期，1-31 */
	int 	hour; 		/* 每天的小时数，0-23 */
	int 	minute; 	/* 每小时的分钟数，0-59 */
	int 	second; 	/* 每分钟的秒数，通常范围是0-59，闰秒时为60 */
	int 	msecond; 	/* 每秒的毫秒数，0-999 */
} PicTime;


typedef struct __VD_PicInfo
{
	char 	path[PATH_MAX_LEN];
	char 	name[NAME_MAX_LEN];
	void 	*buf;
	size_t 	size;
	PicTime tm;
} VD_PicInfo;

typedef struct __EP_PicInfo
{
	char 	path[FTP_PATH_DEPTH][FTP_PATH_LEN];
	char    alipath[10][FTP_PATH_LEN];
	char    aliname[NAME_MAX_LEN];
	char 	name[NAME_MAX_LEN];
	void 	*buf;
	int 	size;
	PicTime tm;
} EP_PicInfo;


typedef struct
{
	char buf[2][SIZE_JPEG_BUFFER];
	char name[2][128];
	int  size[2];
}str_aliyun_image;


typedef struct __EP_VidInfo
{
	char 	path[FTP_PATH_DEPTH][FTP_PATH_LEN];
	char 	name[NAME_MAX_LEN];
	short 	buf_num;
	void 	*buf[MAX_CAPTURE_NUM+1];//MAX_H264_SEG_NUM
	int 	size[MAX_CAPTURE_NUM+1];
} EP_VidInfo;


#ifdef  __cplusplus
extern "C"
{
#endif

int ftp_get_status(int ftp_channel);
int ftp_send_pic_buf(const EP_PicInfo *pic_info, int ftp_channel);
int ftp_send_pic_file(const char *src_file_name, EP_PicInfo *pic_info,
                      int ftp_channel);

int mq_get_status_traffic_flow_motor_vehicle(void);
int mq_get_status_traffic_flow_pedestrian(void);
int mq_send_traffic_flow_motor_vehicle(char *mq_text);
int mq_send_traffic_flow_pedestrian(char *mq_text);
int mq_send_traffic_flow_history(char *msg, int dest_mq, int num_record);

int ftp_get_status_event_alarm(void);
int mq_get_status_event_alarm(void);
int mq_send_event_alarm(char *msg);
int mq_send_event_alarm_history(char *msg, int dest_mq, int num_record);

//###过车处理函数###

int ftp_get_status_traffic_record(void);
int ftp_send_traffic_record_pic_buf(EP_PicInfo *file_info);
int ftp_send_traffic_record_pic_file(
    const char *pic_path, EP_PicInfo *pic_info);
int mq_get_status_traffic_record(void);
int mq_send_traffic_record(char *msg);
int mq_send_traffic_record_history(char *msg, int dest_mq, int num_record);


//###泊车处理函数###

int mq_get_status_park_record(void);
int mq_send_park_record(char *msg);        //add by lxd



//###违停处理函数###

int ftp_get_status_violation_records(void);
int ftp_get_status_violation_video(void);
int ftp_send_violation_video_buf(EP_VidInfo *file_info);
int mq_get_status_violation_records_motor_vehicle(void);
int mq_send_violation_records_motor_vehicle(char *msg);
int mq_send_violation_records_motor_vehicle_history(char *msg, int dest_mq, int num_record);



//###通用处理函数###


int ftp_put_pic_buf(const VD_PicInfo *pic_info, int ftp_channel);
int ftp_put_traffic_record_pic_buf(const VD_PicInfo *pic_info);
int ftp_put_park_record_pic_buf(const VD_PicInfo *pic_info);



#ifdef  __cplusplus
}
#endif


#endif 	/* _MQ_API_H_ */
