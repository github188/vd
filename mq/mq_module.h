/*
 * mq_module.h
 *
 *  Created on: 2013-4-11
 *      Author: shanhongwei
 */

#include "mq.h"


#ifndef MQ_MODULE_H_
#define MQ_MODULE_H_

#define URI_PARK	 		"BITCOM.VD2014.PARK"					//违法记录会话
#define URI_ILLEGAL	 		"BITCOM.VD2014.ILLEGAL"					//违法记录会话
#define URI_PASS_CAR		"BITCOM.VD2014.PASS"					//过车记录会话
#define URI_TRAFFIC_FLOW 	"BITCOM.VD2014.FLOW"					//交通流
#define URI_EVENT_ALARM		"BITCOM.VD2014.EVENT"					//事件会话

#define URI_BROADCAST_UP	"BITCOM.VD2014.BROADCAST.UPDATA"		//广播上载会话;
#define URI_BROADCAST_DOWN	"BITCOM.VD2014.BROADCAST.DOWNDATA"		//广播下载会话;
#define URI_DOWN			"BITCOM.VD2014.DownData"				//下载会话
#define URI_UPDATA			"BITCOM.VD2014.UpData"					//上传会话


#define URI_DEV_STAT		"BITCOM.VD2014.DEVICE.DEVICESTATUS"		//设备状态会话
#define URI_PASS_NOCAR		"BITCOM.VD2014.NOCAR.PASS"				//非机动车通行记录会话。
#define URI_ILLEGAL_NOCAR	"BITCOM.VD2014.NOCAR.ILLEGAL"			//非机动车违法记录会话。
#define URI_FLOW_NOCAR		"BITCOM.VD2014.NOCAR.FLOW"				//行人流量会话。

#define URI_HISTORY_STATISTICS	"BITCOM.VD2014.HISTORY.FLOW"		//历史记录：交通流
#define URI_HISTORY_PASSCAR		"BITCOM.VD2014.HISTORY.PASS"		//历史记录：过车
#define URI_HISTORY_ILLEGAL		"BITCOM.VD2014.HISTORY.ILLEGAL"		//历史记录：违法
#define URI_HISTORY_EVENT		"BITCOM.VD2014.HISTORY.EVENT"		//历史记录：事件
#define URI_PARK_UPLOAD       "3afaec9e-cbc7-4c0e-a57a-b0855e6a8997"   //泊车系统上传参数MQ 队列名称
#define URI_PARK_DOWN           "BITCOM.VD2014.PLATFORM.CONTROL"

#define URI_VP_HISENSE   "HIATMP.HISENSE.ILLEGAL"            //专为海信平台添加的MQ会话
//#########################################//#########################################//
extern class_mq_consumer *g_mq_broadcast_listen; //上位机广播会话;
extern class_mq_consumer *g_mq_download; //参数下载会话;
extern int g_flg_mq_start; //MQ会话启动完成.		1:完成; 0: 未完成;
extern int g_flg_mq_start_songli; //MQ会话启动完成.		1:完成; 0: 未完成;

extern int g_flg_mq_sta_ok; //MQ各个会话状态相与的结果.
extern int mq_event_alarm_ok; //事件告警会话标志;
//extern int mq_ok; //MQ开启标志	置 1 为启动MQ服务
extern int upgrade; //设备升级失败

//#########################################//#########################################//

typedef enum
{
	MQ_INSTANCE_UPLOAD = 0, //参数传输回话
	MQ_INSTANCE_PASS_CAR,
	MQ_INSTANCE_ILLEGAL,
	MQ_INSTANCE_ILLEGAL_HISENSE,
	MQ_INSTANCE_PARK,
	MQ_INSTANCE_BROADCAST,
	MQ_INSTANCE_TRAFFIC_FLOW,
	MQ_INSTANCE_H264UP,
	MQ_INSTANCE_EVENT_ALARM,
	MQ_INSTANCE_NOCAR_PASS,
	MQ_INSTANCE_NOCAR_ILLEGAL,
	MQ_INSTANCE_NOCAR_FLOW,
	MQ_INSTANCE_DEV_STAT,
	
	MQ_INSTANCE_HISTORY_PASSCAR,
	MQ_INSTANCE_HISTORY_ILLEGAL,
	MQ_INSTANCE_HISTORY_STATISTICS,
	MQ_INSTANCE_HISTORY_EVENT,
	MQ_INSTANCE_PARK_UPLOAD,

	MQ_INSTANCE_COUNT,
}MQ_INSTANCE;



int get_mq_version(char *version);

int start_mq_thread();
void stop_mq_thread();

class_mq_producer *get_mq_producer_instance(int index);


#endif /* MQ_MODULE_H_ */
