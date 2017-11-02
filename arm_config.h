/*
 * arm_param.h
 *
 *  Created on: 2013-6-6
 *      Author: shanhw
 */

#ifndef ARM_PARAM_H_
#define ARM_PARAM_H_

#include "commontypes.h"
#include "toggle/dsp/dsp_config.h"

#pragma pack(push,PACK1,4)	// �����ֽڶ��뷽ʽ
#define FTP_CHANEL_ILLEGAL		0x00
#define FTP_CHANEL_PASS_CAR 	0x01
#define FTP_CHANEL_H264			0x02

#define ARM_PARAM_FILE "arm_config.xml"
#define ARM_PARAM_FILE_PATH "/config/arm_config.xml"
#define BATCH_ARM_PARAM_FILE_PATH "/config/batch_arm_config.xml"

//���ݴ洢�����ṹ��
typedef struct DISK_WRI_DATA
{
	DWORD remain_disk; // ��ʣ����ٿռ�ʱ����ʼѭ������

	WORD illegal_picture; //�Ƿ�˫���ݣ�Υ����¼��ͼ��
	WORD vehicle; //�Ƿ�˫���ݣ�����ͨ�м�¼��ͼ��
	WORD event_picture; //�Ƿ�˫���ݣ��¼���¼��ͼ��
	WORD illegal_video; //�Ƿ�˫���ݣ�Υ��¼��
	WORD event_video; //�Ƿ�˫���ݣ��¼�¼��
	WORD flow_statistics; //�Ƿ�˫����: ����ͳ��
} DISK_WRI_DATA;

// �����ϴ������ṹ��
typedef struct _FTP_DATA_CONFIG_
{
	WORD illegal_picture; //�Ƿ��ϴ���Υ����¼��ͼ��
	WORD vehicle; //�Ƿ��ϴ�������ͨ�м�¼��ͼ��
	WORD event_picture; //�Ƿ��ϴ����¼���¼��ͼ��
	WORD illegal_video; //�Ƿ��ϴ���Υ��¼��
	WORD event_video; //�Ƿ��ϴ����¼�¼��
	WORD flow_statistics;//�Ƿ��ϴ�: ����ͳ��
} FTP_DATA_CONFIG;

typedef struct _RESUME_UPLOAD_PARAM_
{
	BOOL is_resume_passcar;
	BOOL is_resume_illegal;
	BOOL is_resume_event;
	BOOL is_resume_statistics;
} RESUME_UPLOAD_PARAM;

//�����ϴ�/�洢����( �ϲ�) �ṹ��
typedef struct DATA_SAVE
{
	DISK_WRI_DATA disk_wri_data;
	FTP_DATA_CONFIG ftp_data_config;
	RESUME_UPLOAD_PARAM resume_upload_data; //������������
} DATA_SAVE;

typedef struct _TYPE_FTP_CONFIG_PARAM_
{
	char user[32]; //�û���
	char passwd[32]; //����
	BYTE ip[4]; //IP��ַ IPV4: ǰ4�ֽڡ�
	int port; //�˿ں�
	BOOL allow_anonymous; //�Ƿ�����������¼
} TYPE_FTP_CONFIG_PARAM;

typedef struct _TYPE_MQ_CONFIG_PARAM_
{
	BYTE ip[4]; //IP��ַ IPV4: ǰ4�ֽڡ�
	int port; //�˿ں�
} TYPE_MQ_CONFIG_PARAM;

typedef struct
{
	BOOL useNTP; //�Ƿ���NTP��ʱ
	BYTE NTP_server_ip[4]; //NTP������IP
	WORD NTP_distance; //��ʱ���(��λ����)
} NTP_CONFIG_PARAM;

typedef struct
{
	char username[128];
	char passwd[128];
	char url[128];
	char bucket_name[128];
}OSS_ALIYUN_PARAM;

//  ���������ṹ
typedef struct _tagBASIC_PARAM
{
	WORD monitor_type; //��ص����ͣ������/·��

	char spot_id[16]; //��װ�ص��ţ�12λ������GA408.3Ҫ��
	char road_id[20]; //��·���
	char spot[100]; //��ص�����
	WORD direction; //��ʻ����

	NTP_CONFIG_PARAM ntp_config_param;

	//BOOL output_realdata_to_MQ; //�Ƿ����ʵʱ��¼����Ϣ������

	//BOOL record_video; //�Ƿ���Ƶ¼��

	// �����ϴ��ӿ�ʹ��
	WORD exp_type; // �ӿ����ͣ�������. 0-�豸�ڲ�Э�飬1-���Źܿ�ƽ̨Э��
	char exp_device_id[20]; // �ӿ�ʹ�õ��豸���
	int collect_actor_size; //�ɼ����ش���λ��
	int log_level; //0--��д, 1--error, 2--warn, 3--state, 4--debug
	DATA_SAVE data_save; // �����ϴ����洢����

	BOOL h264_record; // �Ƿ�����h264¼��洢
	TYPE_FTP_CONFIG_PARAM ftp_param_illegal; //  FTP����������(Υ��)
	TYPE_FTP_CONFIG_PARAM ftp_param_pass_car; //  FTP����������(����)
	TYPE_FTP_CONFIG_PARAM ftp_param_h264; //  FTP����������(h264)

	TYPE_MQ_CONFIG_PARAM mq_param; //  MQ����������

	BYTE ip_berth_front[4];               //��λ֮��ͨ��ʹ�õ�IP��ǰһ�������豸��IP
	BYTE ip_berth_back[4];               //��λ֮��ͨ��ʹ�õ�IP����һ����λ�豸��IP

	OSS_ALIYUN_PARAM oss_aliyun_param;     //aliyun oss�����ṹ�� by shp 2015/04/28
} BASIC_PARAM;

enum io_func
{
	IO_NONE = 0, //������
	IO_RED_DETECT_IN = 1, //��Ƽ����
	IO_YELLOW_DETECT_IN = 2, //�ƵƼ����
	IO_ALARM_DETECT_IN = 3,//���������
	IO_FUNC_COUNT
};

enum io_signal_direction
{
	IO_LEFT = 1, //��ת
	IO_THROUGH = 2, //ֱ��
	IO_RIGHT = 4, //��ת
	IO_TRUN = 8, //��ͷ
	IO_DIR_COUNT
};

typedef struct _IO_CFG
{
	unsigned char trigger_type; //0������, 1�½���, 2����
	unsigned char mode; //�μ�io_func �Ƿ����ú��/�ƵƼ����
	unsigned char io_drt; //�����ɺ�ƻ��߻ƵƼ����������ֶ���Ч���μ�io_signal_direction
} IO_cfg;

//���ڲ����ṹ��
typedef struct _tagSerialParam
{
	WORD dev_type;
	DWORD bps; //������  9600;
	WORD check;//У��λ  2-- even, 1---odd, 0---none
	WORD data; //����λ
	WORD stop; //ֹͣλ  1--1λ�� 2--2λ�� 3--1.5λ
} SerialParam;

//�ⲿ�ӿڲ����ṹ��
typedef struct _tagInterface_Param
{
	SerialParam serial[3]; //���ڲ���
	IO_cfg io_input_params[8]; //IO�������
	IO_cfg io_output_params[4]; //IO�������
} Interface_Param;

//��osd��������
typedef struct _Osd_item_
{
	int switch_on; //����
	int x; // start position
	int y;

	int is_time; // time osd
	char content[40];

} Osd_item;

//H264 osd����
typedef struct _Osd_info_
{
	Type_info_color color;
	Osd_item osd_item[8];
} Osd_info;

//��H264ͨ������
typedef struct _H264_chanel_
{
	BOOL h264_on;//����  false--off, true--on
	BOOL cast; //true --��������/�鲥�� false --�㲥
	BYTE ip[4]; //����/�鲥��IP��ַ
	int port;

	int fps; //֡��
	int rate; //����
	int width; //ͼ���
	int height; //ͼ���

	Osd_info osd_info;
} H264_chanel;

//H264����
typedef struct _H264_config_
{
	H264_chanel h264_channel[2];
} H264_config;

enum
{
	ILLEGAL_CODE_ILLEGALPARK = 0, //�Ƿ�ͣ��
	ILLEGAL_CODE_OVERSAFETYSTRIP, //ѹ��ȫ��
	ILLEGAL_CODE_ILLEGAL_ANELRUN, //��������ʻ
	ILLEGAL_CODE_CROSSSTOPLINE, //Υ��Խ��
	ILLEGAL_CODE_COVERLINE, //ѹʵ��
	ILLEGAL_CODE_CHANGELANE, //Υ�����
	ILLEGAL_CODE_CONVERSE_DRIVE, //����
	ILLEGAL_CODE_RUNRED, //�����
	ILLEGAL_CODE_EXCEED_SPEED, //����
	ILLEGAL_CODE_OCCUPYSPECIAL, //�Ƿ�ռ��ר�ó���
	ILLEGAL_CODE_OCCUPYNONMOTOR, //�Ƿ�ռ�÷ǻ�������
	ILLEGAL_CODE_FORCEDCROSS, //ӵ��ʱǿ��ʻ�뽻��·��
	ILLEGAL_CODE_LIMITTRAVEL, //Υ����ʱ����
	ILLEGAL_CODE_NOTWAYTOPEDES = 13,//����������
	ILLEGAL_CODE_COUNT = 20,

};

//Υ��������Ϣ
typedef struct
{
	int illeagal_type; //Υ������.  �� EVENT_TYPE
	int illeagal_num; //�û������Υ�����

} ILLEGAL_CODE_INFO;

typedef struct _ARM_config_
{
	BASIC_PARAM basic_param;
	//IO_Mode io_mode;
	Interface_Param interface_param;
	H264_config h264_config;
	ILLEGAL_CODE_INFO illegal_code_info[ILLEGAL_CODE_COUNT]; //Υ��������Ϣ

} ARM_config;

#pragma pack(pop, PACK1)
#endif /* ARM_PARAM_H_ */
