#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdio.h>
#include <time.h>
#include <bits/pthreadtypes.h>
#include "ep_type.h"
#include "ptz_protocol.h"

#define PLATEFORM_ARM

#define DIR_LEN 80
#define DIR_NUM 5
#define FILE_NAME_LEN 100
#define OUTLEN 1100

/**********************卡口第三方平台结构体定义*********************************/
typedef struct 
{
	char buf[1024*1024];
	int   buf_lens;
	int   flag;
}str_dahua_data;

typedef struct 
{
	char buf[1024*1024];
	int   buf_lens;
	int   flag;
	uint8_t pic[1024 * 1024];
	size_t sz_pic;

}str_bitcom_data;



extern str_bitcom_data gstr_bitcom_data;
extern str_dahua_data gstr_dahua_data;


/**********************系统运行标志*********************************/
extern struct timeval gEP_startTime;
extern struct timeval gEP_endTime;
extern int gTimming_flag;

extern int flag_alarm_mq;
extern int flag_log_mq;
extern int flag_redLamp;
extern int flag_ptz_control;
extern int flag_parking_lock;//地锁使用标志


/**********************线程优先级*************************************/
#define UART_THREAD_PRIORITY 		sched_get_priority_max(SCHED_FIFO) - 0

#define SPI_THREAD_PRIORITY 		sched_get_priority_max(SCHED_FIFO) - 1

#define ENCODE_THREAD_PRIORITY  	sched_get_priority_max(SCHED_FIFO) - 1
#define CY_NET_THREAD_PRIORITY		sched_get_priority_max(SCHED_FIFO) - 2
#define DECODE_THREAD_PRIORITY  	sched_get_priority_max(SCHED_FIFO) - 2
#define DISPLAY_THREAD_PRIORITY 	sched_get_priority_max(SCHED_FIFO) - 2

#define CTL_THREAD_PRIORITY  		sched_get_priority_max(SCHED_FIFO) - 3

#define MQ_QUEUE_THREAD_PRIORITY	sched_get_priority_max(SCHED_FIFO) - 4
#define MQ_TOPIC_THREAD_PRIORITY	sched_get_priority_max(SCHED_FIFO) - 4
#define FTP_THREAD_PRIORITY 		sched_get_priority_max(SCHED_FIFO) - 5
#define ACCIDENT_THREAD_PRIORITY   	sched_get_priority_max(SCHED_FIFO) - 5

#define NET_THREAD_PRIORITY			sched_get_priority_max(SCHED_FIFO) - 6
#define VIDEO_THREAD_PRIORITY       sched_get_priority_max(SCHED_FIFO) - 7///new
#define LOG_THREAD_PRIORITY  		sched_get_priority_max(SCHED_FIFO) - 7
#define NTP_THREAD_PRIORITY 		sched_get_priority_max(SCHED_FIFO) - 7
#define RECORD_SEND_PRIORITY       sched_get_priority_max(SCHED_FIFO) - 7///new
#define TIME_CLOCK_PRIORITY       sched_get_priority_max(SCHED_FIFO) - 8///new
#define MASTER_SLAVE_HEART_PRIORITY  sched_get_priority_max(SCHED_FIFO) - 9///new
#define H264_RECV_THREAD_PRIORITY  sched_get_priority_max(SCHED_FIFO) - 3
/**********************************************************************/

/*********************系统变量********************************/
extern int upLoadDeviceInfo; //发送设备信息
extern int updateDeviceInfo; //更新设备信息
extern int upLoadDeviceStatus; //发送设备状态信息

extern int getTime; //核对系统时间
extern int setTime; //设置系统时间
extern int onlineDevice; //在线设备应答
extern int capturePic; //抓拍图片成功
extern NORMAL_FILE_INFO ftp_filePath_up;
extern SET_NET_PARAM g_set_net_param;
extern Disk_Clear diskClear;
extern PTZ_MSG ptz_msg;

extern int ParseCommands(char * command);
extern void * ftpTransformFxn(void * arg);






#endif
