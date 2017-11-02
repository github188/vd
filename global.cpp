/*
 * global.cpp
 *
 *  Created on: 2013-5-3
 *      Author: shanhw
 */

#include "ep_type.h"
#include "global.h"
#include <sys/time.h>

/**********************系统运行标志*********************************/
struct timeval gEP_startTime;
struct timeval gEP_endTime;
int gTimming_flag;

int flag_alarm_mq = 0; //4报警上传mq标志
int flag_log_mq = 0; //4日志上传mq标志
int flag_redLamp = 0; //4上传mq信号灯状态标志，通过配置软件设置
int upLoadDeviceInfo = 0; //发送设备信息
int upLoadDeviceStatus = 0; //发送设备状态信息
int updateDeviceInfo = 0;//更新设备信息
int getTime = 0; //获取系统时间
int setTime = 0; //设置设备时间
int onlineDevice = 0; //在线设备应答
int flag_ptz_control=0;//ptz控制标志
int flag_parking_lock=0;//地锁使用标志

NORMAL_FILE_INFO ftp_filePath_up;
Disk_Clear diskClear;
TrafficFlowStru traffic_info; //save the traffic info
PTZ_MSG ptz_msg;


/**********************卡口第三方平台全局变量*********************************/
str_dahua_data gstr_dahua_data;
str_bitcom_data gstr_bitcom_data;





