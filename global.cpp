/*
 * global.cpp
 *
 *  Created on: 2013-5-3
 *      Author: shanhw
 */

#include "ep_type.h"
#include "global.h"
#include <sys/time.h>

/**********************ϵͳ���б�־*********************************/
struct timeval gEP_startTime;
struct timeval gEP_endTime;
int gTimming_flag;

int flag_alarm_mq = 0; //4�����ϴ�mq��־
int flag_log_mq = 0; //4��־�ϴ�mq��־
int flag_redLamp = 0; //4�ϴ�mq�źŵ�״̬��־��ͨ�������������
int upLoadDeviceInfo = 0; //�����豸��Ϣ
int upLoadDeviceStatus = 0; //�����豸״̬��Ϣ
int updateDeviceInfo = 0;//�����豸��Ϣ
int getTime = 0; //��ȡϵͳʱ��
int setTime = 0; //�����豸ʱ��
int onlineDevice = 0; //�����豸Ӧ��
int flag_ptz_control=0;//ptz���Ʊ�־
int flag_parking_lock=0;//����ʹ�ñ�־

NORMAL_FILE_INFO ftp_filePath_up;
Disk_Clear diskClear;
TrafficFlowStru traffic_info; //save the traffic info
PTZ_MSG ptz_msg;


/**********************���ڵ�����ƽ̨ȫ�ֱ���*********************************/
str_dahua_data gstr_dahua_data;
str_bitcom_data gstr_bitcom_data;





