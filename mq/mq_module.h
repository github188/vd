/*
 * mq_module.h
 *
 *  Created on: 2013-4-11
 *      Author: shanhongwei
 */

#include "mq.h"


#ifndef MQ_MODULE_H_
#define MQ_MODULE_H_

#define URI_PARK	 		"BITCOM.VD2014.PARK"					//Υ����¼�Ự
#define URI_ILLEGAL	 		"BITCOM.VD2014.ILLEGAL"					//Υ����¼�Ự
#define URI_PASS_CAR		"BITCOM.VD2014.PASS"					//������¼�Ự
#define URI_TRAFFIC_FLOW 	"BITCOM.VD2014.FLOW"					//��ͨ��
#define URI_EVENT_ALARM		"BITCOM.VD2014.EVENT"					//�¼��Ự

#define URI_BROADCAST_UP	"BITCOM.VD2014.BROADCAST.UPDATA"		//�㲥���ػỰ;
#define URI_BROADCAST_DOWN	"BITCOM.VD2014.BROADCAST.DOWNDATA"		//�㲥���ػỰ;
#define URI_DOWN			"BITCOM.VD2014.DownData"				//���ػỰ
#define URI_UPDATA			"BITCOM.VD2014.UpData"					//�ϴ��Ự


#define URI_DEV_STAT		"BITCOM.VD2014.DEVICE.DEVICESTATUS"		//�豸״̬�Ự
#define URI_PASS_NOCAR		"BITCOM.VD2014.NOCAR.PASS"				//�ǻ�����ͨ�м�¼�Ự��
#define URI_ILLEGAL_NOCAR	"BITCOM.VD2014.NOCAR.ILLEGAL"			//�ǻ�����Υ����¼�Ự��
#define URI_FLOW_NOCAR		"BITCOM.VD2014.NOCAR.FLOW"				//���������Ự��

#define URI_HISTORY_STATISTICS	"BITCOM.VD2014.HISTORY.FLOW"		//��ʷ��¼����ͨ��
#define URI_HISTORY_PASSCAR		"BITCOM.VD2014.HISTORY.PASS"		//��ʷ��¼������
#define URI_HISTORY_ILLEGAL		"BITCOM.VD2014.HISTORY.ILLEGAL"		//��ʷ��¼��Υ��
#define URI_HISTORY_EVENT		"BITCOM.VD2014.HISTORY.EVENT"		//��ʷ��¼���¼�
#define URI_PARK_UPLOAD       "3afaec9e-cbc7-4c0e-a57a-b0855e6a8997"   //����ϵͳ�ϴ�����MQ ��������
#define URI_PARK_DOWN           "BITCOM.VD2014.PLATFORM.CONTROL"

#define URI_VP_HISENSE   "HIATMP.HISENSE.ILLEGAL"            //רΪ����ƽ̨��ӵ�MQ�Ự
//#########################################//#########################################//
extern class_mq_consumer *g_mq_broadcast_listen; //��λ���㲥�Ự;
extern class_mq_consumer *g_mq_download; //�������ػỰ;
extern int g_flg_mq_start; //MQ�Ự�������.		1:���; 0: δ���;
extern int g_flg_mq_start_songli; //MQ�Ự�������.		1:���; 0: δ���;

extern int g_flg_mq_sta_ok; //MQ�����Ự״̬����Ľ��.
extern int mq_event_alarm_ok; //�¼��澯�Ự��־;
//extern int mq_ok; //MQ������־	�� 1 Ϊ����MQ����
extern int upgrade; //�豸����ʧ��

//#########################################//#########################################//

typedef enum
{
	MQ_INSTANCE_UPLOAD = 0, //��������ػ�
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
