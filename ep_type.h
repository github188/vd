/*
 * ep_type.h
 *
 *  Created on: 2013-4-15
 *      Author: shanhongwei
 */

#ifndef EP_TYPE_H_
#define EP_TYPE_H_

#include "commontypes.h"
#include "dsp_config.h"
#include "arm_config.h"
#include "camera_config.h"
#include "ver.h"

#pragma pack(push,PACK1,4)	// �����ֽڶ��뷽ʽ

#define DSP_VERSION_STR_LENGTH_MAX (64u)
//by lxd
/*
typedef struct _dsp_version
{
	char acVersion[DSP_VERSION_STR_LENGTH_MAX];
} DSP_version;
*/           


////////////////////////////////////  ��Ϣ���Ͷ��� ////////////////////////////////////////////////////////

enum ENUM_MESSAGE_QUEUE_TYPE
{
	MSG_PARAM_ERR = -1,

	MSG_LOOPS = 0, //������Ϣ

	MSG_UPLOAD_ARM_PARAM = 1, // �ϴ�arm����
	MSG_DOWNLOAD_ARM_PARAM, // ����arm����
	MSG_BATCH_CONFIG_ARM, //��������ARM


	MSG_UPLOAD_DSP_PARAM, // �ϴ�DSP����
	MSG_DOWNLOAD_DSP_PARAM, //����DSP����
	MSG_BATCH_CONFIG_DSP, //��������DSP

	MSG_UPLOAD_CAMERA_PARAM, // �ϴ�camera����
	MSG_DOWNLOAD_CAMERA_PARAM, //����camera����
	MSG_BATCH_CONFIG_CAMERA, //��������camera

	MSG_UPLOAD_HIDE_PARAM_FILE = 10, //�ϴ����β����ļ�
	MSG_DOWNLOAD_HIDE_PARAM_FILE, //���������ļ�

	MSG_UPLOAD_NORMAL_FILE, //�ϴ�һ���ļ�
	MSG_DOWNLOAD_NORMAL_FILE, //����һ���ļ�

	MSG_CAPTURE_PICTURE, // ץ��ͼƬ 14

	MSG_DEVICE_UPGRADE, //�豸����  15
	MSG_GET_TIME, //��ѯ�豸ʱ��
	MSG_SET_TIME, //�����豸ʱ��

	MSG_REDLAMP_STATUS, //�źŵƵ�״̬ Signallamp_status ��Ҫ���º��ʱ��.
	MSG_REDLAMP_POSITION, //�źŵ�λ�� Signal_detect_video
	MSG_PLATE_SIZE = 20, //���Ƴߴ緶Χ   Plate_size ��Ҫ���²���.

	MSG_BATCH_CONFIG, //������������
	MSG_REBOOT, //����
	MSG_FOCUS_ADJUST = 23, //�۽�����
	MSG_IRIS_SET = 24, //��Ȧ����
	MSG_DEVICE_INFOMATION = 25, //�鿴�豸������Ϣ
	MSG_DEVICE_STATUS =26,		//�鿴�豸״̬

	///////// ���´��� /////////////////

	/*************************
	 MQ�豸������Ϣ
	 *************************/
	MSG_FACTORY_CONTROL, //��ȡд�볧�̿���ѡ��

	MSG_DEVICE_ALARAM_HISTORY, //�鿴�豸������ʷ
	MSG_DEVICE_WORKLOG, //�豸������־�ϴ�
	MSG_DEVICE_ALARM, //�豸�����ϴ�

	MSG_DEVICE_DATA_CLEAR, //�豸��������
	MSG_HISTORY_RECORD, //�鿴��ʷ��¼

	MSG_CAPTURE_VEHICLE_SPEC, //ץ�ĳ���ָ��λ��ͼƬ

	MSG_HD_OPERATION, //Ӳ�̸����֪ͨ��extend=0 ����Ӳ�̣�extend=1 ��ʽ��Ӳ��


	MSG_HD_OPERATION_ERROR,//�յ�Ӳ�����Ӳ������ʹ�ã�������λ��
	MSG_HD_OPERATION_OK, //Ӳ��ȷ�����в���

	////////////// ��Ӧ ///////////////////////
	//arm����
	MSG_UPLOAD_ARM_PARAM_r = 101, // �ϴ�arm����
	MSG_DOWNLOAD_ARM_PARAM_r, // ����arm����
	MSG_BATCH_CONFIG_ARM_r, //��������ARM

	//DSP����
	MSG_UPLOAD_DSP_PARAM_r, // �ϴ�DSP����
	MSG_DOWNLOAD_DSP_PARAM_r, //����DSP����
	MSG_BATCH_CONFIG_DSP_r, //��������DSP

	//camera����
	MSG_UPLOAD_CAMERA_PARAM_r, // �ϴ�camera����
	MSG_DOWNLOAD_CAMERA_PARAM_r, //����camera����
	MSG_BATCH_CONFIG_CAMERA_r, //��������camera

	//���β���
	MSG_UPLOAD_HIDE_PARAM_FILE_r, //�ϴ����β����ļ�
	MSG_DOWNLOAD_HIDE_PARAM_FILE_r, //���������ļ�  11

	//һ���ļ�
	MSG_UPLOAD_NORMAL_FILE_r, //�ϴ�һ���ļ�
	MSG_DOWNLOAD_NORMAL_FILE_r = 113, //����һ���ļ�

	//����
	MSG_CAPTURE_PICTURE_r,// ץ��ͼƬ,���ؽ��
	MSG_DEVICE_UPGRADE_r, //�豸����, ��Ӧ
	MSG_GET_TIME_r, //��ѯ�豸ʱ��, ��Ӧ��Ϣ
	MSG_SET_TIME_r,//����ʱ��

	MSG_BATCH_CONFIG_r, //������������
	MSG_REBOOT_r, //����

	MSG_IRIS_SET_r,
	MSG_DEVICE_INFOMATION_r = 121, //�鿴�豸������Ϣ
	MSG_DEVICE_STATUS_r = 122, //�鿴�豸״̬

	MSG_PARK_UPLOAD_r = 123,   // ������Ϣ�ϴ�
	MSG_HISTORY_RECORD_REQ=1000, 		//��ѯ��ʷ��¼;
	MSG_HISTORY_RECORD_REQ_ACK = 1001,  //Ӧ��鿴��ʷ��¼
	MSG_EXT_HISTORY_RECORD_START = 1002, //������ʷ��¼
	MSG_EXT_HISTORY_RECORD_START_ACK = 1003, //Ӧ�𵼳���ʷ��¼
	MSG_EXT_HISTORY_RECORD_END = 1004, //ֹͣ������¼
	MSG_EXT_HISTORY_RECORD_END_ACK = 1005, //Ӧ��ֹͣ������¼

	/*******************************************************************************
	��̨�����Ϣ
	******************************************************************************/
	MSG_PTZ_CTRL = 1200,	// ��̨���������Ϣ����Ŵ�1200��ʼ   add by zhengweixue 20140520

};

enum ENUM_MSG_BROADCAST_TYPE
{
	MSG_ONLINE_DEVICE = 1, //mqˢ��, socket�ֳ������豸
	MSG_DEVICE_NETWORK_PARAM = 2, //��ȡ�豸�������
	MSG_DEVICE_CURRENT_PARAM = 3, //�����豸��ǰ�������
	MSG_SET_NETWORK_PARAM = 4, //�����������
	MSG_SET_METWORK_RESULT = 5, //������������������
	MSG_RECOVERY_DEFAULTCONFIG = 6, //�ָ���������
	MSG_RECOVERY_DEFAULTCONFIG_RESULT = 7, //�ָ��������ý������
	MSG_SET_NETWORK_PARAM_CENTER = 8,//�������������������
	MSG_OPEN_DEV = 9, //���ù���������豸��
	MSG_CLOSE_DEV = 10, //���ù�������ر��豸��
	MSG_OPEN_DEV_RETURN_TRUE = 11,//�յ����ù���������豸������ɹ�,���ظ����ù��������
	MSG_OPEN_DEV_RETURN_FALSE = 12,//�յ����ù���������豸������ʧ��,���ظ����ù��������
	MSG_CLOSE_DEV_RETURN_TRUE = 13,//�յ����ù�������ر��豸������ɹ�,���ظ����ù��������
	MSG_CLOSE_DEV_RETURN_FALSE = 14,//�յ����ù�������ر��豸������ʧ��,���ظ����ù��������
	MSG_ONLINE_RESULT = 15,
//�豸���߷������ù��������Ϣ;
};

typedef struct
{
	int event_type; //�¼�����:0--����, 1--Υ��,2-����ͳ��,3-�¼�,
	int dest_mq; //����MQ�Ự��ַ: 0:����������mq�Ự; 1:�������ܿ�ƽ̨mq�Ự;
	char time_start[64];//ʱ����� ʾ����2013-06-01 12:01:02
	char time_end[64];  //ʱ���յ�
} TYPE_HISTORY_RECORD;

//��ѯ��ʷ��¼��Ϣ���;
typedef struct
{
	int count_record;		//���������ļ�¼����;

} TYPE_HISTORY_RECORD_RESULT;


//Υ�����ͺ�;
typedef struct
{
	int ill_stop; //�Ƿ�ͣ��
	int over_safe_tystrip;//ѹ��ȫ��
	int ill_lane_run;//��������ʻ
	int over_line;//Υ��Խ��
	int over_yellow_line;//ѹʵ��
	int over_lane;// Υ�����
	int wrong_driect;// ����
	int run_red;//�����
	int over_speed;// ����
	int ill_occup_special;//�Ƿ�ռ��ר�ó���
	int ill_occup_no_motor;//�Ƿ�ռ�÷ǻ�������
	int over_flow_alarm;//ǿ��ʻ��;
	int vehicle_travel_restriction; //��ʱ����
	int nocar_over_car_lan; //���˴���������;
} type_ill_num;

//////////////////////////////////// ����ֵ�Ķ��� //////////////////////////////////////////////////////////////

//MQ�����ϴ���Ϣ����
enum ENUM_MSG_TYPE
{
	MSG_ILLEGALDATA = 1, //Υ������
	MSG_VEHICULAR_TRAFFIC = 2, //����ͨ��
	MSG_EVENT_ALARM = 3, //�¼�����
	MSG_DEVICE_INFO = 4, //�豸״̬
	MSG_TRAFFICFLOW_INFO = 5,
	MSG_NOCAR_REC = 6, //�ǻ�����ͨ�м�¼;
	MSG_NOCAR_ILLEGAL = 7, //�ǻ�����Υ����¼;
	MSG_NOCAR_FLOW_INFO = 8,
	MSG_PARK=9, //������¼  add by lxd                  
//��������;
//��ͨ����Ϣ
};

//�¼���������
enum ENUM_EVENT_ALARM
{
	EVENT_VEHICLE_RETROGRADE = 1, //��������
	EVENT_ILLEGAL_LANE = 2, //����������ʻ
	EVENT_ILLEGAL_STOP = 3, //Υ��ͣ��
	EVENT_ON_SOLIDLINE = 4, //ѹʵ��
	EVENT_ILLEGAL_CHANGELINE = 5, //Υ�����
	EVENT_ILLEGAL_CROSSLINE = 6, //Υ��Խ��
	EVENT_TREAD_SAFEISLAND = 8,
//��������ѹ��ȫ��
};

//�����״̬
enum ENUM_CAMEAR_STATUS
{
	CAMERA_NORMAL = 0, //����
	CAMERA_NOCONNECT = 1, //���Ӳ���
	CAMERA_PICTURE_ABNORMAL = 2,
//ͼ���쳣
};

//�豸����״̬
enum ENUM_DEVICE_ERROR
{
	DEVICE_NORMAL = 0, //����
	DEVICE_FAULT = 1,
//����
};

//��ص�����
enum ENUM_TRAFFIC_MODE
{
	CROSSROAD = 1, //����·��
	SECTION_ROAD = 2,
//·��
};

// ���������и�λ�Ķ���
#define FUNC_VIA		0x0001
#define FUNC_GATEWAY	0x0002
#define FUNC_EVENT		0x0004
#define FUNC_SPEED		0x0008
#define FUNC_SIGNAL		0x0010

//�豸������������
enum ENUM_DEVICE_ALARM
{
	ALARM_OTHER = 0, //��������
	ALARM_HARDDISK_ERROR = 1, //Ӳ�̹��ϣ�Ӳ����/Ӳ�̼���ʧ��/Ӳ��д��ʧ�ܵ�
	ALARM_CAMERA_ERROR, //��������ϣ�ͨѶʧ�ܵ�
	ALARM_FTP_ERROR, //FTP���ϣ��޷�����/д��ʧ�ܵ�
	ALARM_BOARD_COMMUNICATION, //���Ӱ忨ͨѶ���ϣ��򿪴Ӱ�ʧ��/д�볬ʱ��
	ALARM_MQ, //MQ����
	ALARM_FLASH, //Flash���ϣ�д��ʧ��/��
	ALARM_I2C, //I2C���ϣ�i2c��ʧ�� / �¶ȴ���������ʧ�� / RTC����ʧ��
	ALARM_UART, //UART����ʧ��
	ALARM_SPI,//SPI����
	ALARM_DOOR_OPEN,
//�����ű���
};

//�豸������������
enum ENUM_DEVICE_DATACLEAR
{
	ALL_CLAER = 0, //ȫ��
	VEHICLERECORD_CLEAR = 1, //������¼
	ILLEGALRECORD_CLEAR = 2, //Υ����¼
	EVENTALARM_CLEAR = 3, //�¼�����
	WORKLOG_CLEAR = 4,
//��־

};

typedef struct
{
	u8 str_head[8]; //����˵��
	u16 master; //���汾��
	u16 slaver; //�Ӱ汾��
	u16 revise; //�����汾��
	u8 str_build[64]; //����汾��
	u16 protocol_master; //��λ���ӿ� ���汾��;
	u16 protocol_slaver; //��λ���ӿ� �ΰ汾��;
	u16 debug; //���԰汾��
	u32 arm_dsp; //�㷨�ӿں�
} type_version_arm;

//typedef struct
//{
//	u8 str_head[8]; //����˵��
//	u16 decode; //�����㷨�汾��
//	u16 encode; //�����㷨�汾��
//	u16 slave; //�Ӱ�汾��
//	u16 debug; //���԰汾��
//	u32 arm_dsp; //arm�ӿں�
//} type_version_dsp;

typedef struct
{
	SHORT m_nLeftIo; //��ת����״̬���μ�ö��ENUM_LAMP_STATUS
	SHORT m_nThroughIo; //ֱת����״̬
	SHORT m_nRightIo; //��ת����״̬
	SHORT m_nTurnIo; //��ͷ����״̬
} RED_LAMP_IO;

typedef struct
{
	SHORT m_nLeftStatus; //��ת����״̬���μ�ö��ENUM_LAMP_STATUS
	SHORT m_nThroughStatus; //ֱת����״̬
	SHORT m_nRightStatus; //��ת����״̬
	SHORT m_nTurnStatus; //��ͷ����״̬
	//SHORT		m_nLeftChange;
	//SHORT		m_nThroughChange;
	//SHORT		m_nRightChange;
	//SHORT		m_nTurnChange;
} RED_LAMP_STATUS;

typedef struct
{
	SHORT m_nlane1Flag; //����1����Ʊ�־
	SHORT m_nlane2Flag; //����2����Ʊ�־
	SHORT m_nlane3Flag; //����3����Ʊ�־

} RED_LAMP_VIOLATE;

typedef struct
{
	SHORT m_nlane1Flag; //����1������־
	SHORT m_nlane1Speed; //����1��������
	SHORT m_nlane2Flag; //����1������־
	SHORT m_nlane2Speed;
	SHORT m_nlane3Flag; //����3������־
	SHORT m_nlane3Speed;
} CAR_PASS;

//ӵ�¼�ⱳ������;
typedef struct OverFlowAreaInfo
{
	short startPosX;
	short startPosY;
	short endPosX;
	short endPosY;
} OverFlowAreaInfo;

typedef struct LaneTrafficFlow
{
	int m_wBigCarFlow;
	int m_wSmallCarFlow;
	int m_wAverageSpeed; // average speed
} LaneTrafficFlow;

typedef struct TrafficFlowStru
{
	int m_wLaneNum; //total num of lanes
	LaneTrafficFlow m_cLane[5];
} TrafficFlowStru;
//########## ���Ӱ崫�����Բ��� ##############//
typedef struct
{
	u8 run_red_mode; //�����ץ��ģʽ
	OverFlowAreaInfo overFlowInfo[2]; //������궨
	u16 VehicleSpeedThr; //�쳣�ٶ�����ֵ
	float fOverLaneRatio; //Υ����������ȣ�������
	float fOverLineRatio; //Υ��Խ�������ȣ�������
	int smallYellowCarEvent;//С�����¼����ƣ���λ������-1���ر�С���Ƽ�⣬0��С���ƹ�����⣬��û���¼���⣬1��С���Ƽ��Υ��ͣ����2��С���Ƽ�ѹ��ȫ��
	char lowConsumeMode; //�ͺ�ģʽ���أ�0���رյͺ�ģʽ��1�������ͺ�ģʽ��������
	char cNoVehiCaptureLevel;//�ǻ��������������� 1-3�� 1������֤��Ч�ʣ� 3������֤������
	char cPeopleCaptureLevel;//���˲��������� 1-3�� 1������֤��Ч�ʣ� 3������֤������
} type_config;

////////////////////////////////////////////ʵʱ���ݽṹ�嶨�� (�豸ʹ��) ///////////////////////////////////
// ������ҵ��׼�͹ܿ�ƽ̨��׼�ƶ����豸�ϴ��ı���ʽ���˽ṹ��Ϊ�ڲ�ʹ�á��������ı��ͽṹ����໥ת����
//Υ����¼
typedef struct _tagIllegal_Records
{
	char m_PlateNum[16]; //���ƺ���
	char m_PlateType[3]; //�������࣬����GA24.7Ҫ��
	char m_Time[25]; //Υ��ʱ�䣬��ȷ����
	char m_Violation[8]; //Υ�����ͣ�����GA408.1Ҫ��
	char m_Speed[4]; //�����ٶȣ���λ������/Сʱ
	char m_FilePath1[100]; //ͼ��1�����·��
	char m_FilePath2[100]; //ͼ��2�����·��
	char m_FilePath3[100]; //ͼ��3�����·��
	char m_FilePath4[100]; //ͼ��4�����·��
	char m_AviPath[100]; //Υ���������·��
	char m_Description[151]; //Υ��˵��
	char m_PlateColor[2]; //������ɫ
	char m_Lane[4]; //������
	char redLampStartTime[20]; //�����ʼʱ��
	int redLampKeepTime; //��Ƴ���ʱ��
	char m_Reserve[51]; //������Ϣ������Ϊ��x/y/cx/cy/lane
} Illegal_Records;

//����ͨ�м�¼
typedef struct _tagVehicle_Records
{
	char m_PlateNum[16]; //���ƺ���
	char m_PlateType[3]; //�������࣬����GA24.7Ҫ��
	char m_Time[25]; //����ʱ�䣬��ȷ����
	char m_Speed[4]; //�����ٶȣ���λ������/Сʱ
	char m_VehicleLength[6]; //�����ȳ�����λ������
	char m_PlateColor[2]; //������ɫ
	char m_FilePath1[100]; //ͼ��1�����·��
	char m_Lane[4]; //������
	char m_Reserve[51]; //������Ϣ������Ϊ��x/y/cx/cy/lane
} Vehicle_Records;

//�¼�����
typedef struct _tagEvent_Alarm
{
	char m_DeviceID[12]; //�豸���
	char m_Time[15]; //����ʱ��
	char m_AlarmType[3]; //��������
	char m_FilePath1[61]; //ͼ��1�����·��
	char m_FilePath2[61]; //ͼ��2�����·��
	char m_Description[151]; //����
} Event_Alarm;

//�豸״̬
typedef struct _tagDevice_Status
{
	char m_DeviceID[12]; //�豸���
	char m_CameraStatus[2]; //�����״̬
	char m_ErrorStatus[2]; //�豸����״̬
} Device_Status;

//===========================================�ⲿ�ӿڲ���============================//


/***************************************MQ�豸������Ϣ ***********************************/
//FTP�ļ�·���ṹ��
typedef struct _tagFTP_FILEPATH
{
	int type; //�������á� 1--����arm�� 2--����dsp�� 3--����fpga�� 4--������Ƭ��1, 5--������Ƭ��2
	char m_strFileURL[255]; //FTP���ļ����·��
	unsigned long file_size; //ftp�ϵ��ļ���С. ��byteΪ��λ.
} FTP_FILEPATH;

typedef enum
{
	DISK_BLOCK_STATUS_NOT_DETECTED = 0, // ̽�ⲻ��
	DISK_BLOCK_STATUS_MOUNT_FAILED = 1, // mount ʧ��
	DISK_BLOCK_STATUS_NORMAL = 2, //����
	DISK_BLOCK_STATUS_IS_CLEANING = 3, // Ӳ����������
	DISK_BLOCK_STATUS_IS_FORMATTING = 4
// Ӳ�����ڸ�ʽ��

} DISK_BLOCK_STATUS;

typedef struct DISK_STATUS
{
	DWORD m_diskBlockSize;
	WORD m_diskBlockUsed; //ʹ���ʣ���ʾ��Ҫ��%
	DISK_BLOCK_STATUS m_diskBlockStatus; //��0��̽�ⲻ��; 1mount ʧ�� ; 2����;3,��������4�����ڸ�ʽ��

} DISK_STATUS;

//Ӳ����Ϣ
typedef struct DISK_INFO
{
	WORD m_curNum; //��ǰʹ�õķ�����
	WORD m_IsPartition; // �Ƿ����ڷ���
	DISK_STATUS diskStatus[4];

} DISK_INFO;

//====================================�鿴�豸������Ϣ==========================//
//�����豸������Ϣ�ṹ��
typedef struct _tagDevice_Information
{
	char ver_boa[64]; //Ƕ��ʽ���boa�汾
	char ver_dsp[64]; //DSP����汾
	char ver_mcfw[64]; //Ƕ��ʽ���mcfwģ��汾
	char ver_vpss[64]; //Ƕ��ʽ���vpssģ��汾
	char ver_sysserver[64]; //FPGA����汾
	char ver_vd[64]; //Ƕ��ʽVd �汾
	char mcu_ver[64]; //��Ƭ���汾��

	DISK_INFO diskInfo; //Ӳ��״̬
	WORD m_wLimitSpeed; //·�����٣���λ������/Сʱ

	char hw_uuid[15]; //�豸Ψһ���,12λ
	char hw_ver[12]; //�豸Ӳ���汾��

	char ftp_sta_pass_car; //ftp 0����  1������ 2 �����쳣 3 ftp��
	char ftp_sta_illegal; //ftp 0����  1������ 2 �����쳣 3 ftp��
	char ftp_sta_h264; //ftp 0����  1������ 2 �����쳣 3 ftp��

	char uart_status; // uart 0����1 ��ʧ��2.�޴Ӱ���Ϣ

	char tem_status; //�¶ȴ����� 0���� 1��ʧ��
	char gpio_status; //gpio  0���� 1 ��ʧ��
	char mcu_status; //��Ƭ��״̬0:���� 1���� 2:������
	char fan_status; //0--close, 1--open

	int preset_index;	//	Ԥ��λ��Ŷ�Ӧ��ȡ	====������ 2014.76.26

} Device_Information;

//================================���鿴�豸״̬=================================//
//�����豸״̬�ṹ��
typedef struct _tagDevice_Status_Return
{
	WORD m_CPU; //CPUռ�ðٷֱ�
	WORD m_Memory; //�ڴ�ռ�ðٷֱ�
	WORD m_DSP[8]; //DSPռ�ðٷֱ�
	int m_Temperature; //�忨�¶ȣ���λ��
	LONG m_DiskFree;//��λΪKB
	u64 m_EpTimes;//��λΪs
	char m_strDSPStatus[255]; //DSP״̬�ı�
} Device_Status_Return;

//================================���鿴���״̬=================================//
//���غ��״̬�ṹ��
typedef struct _tagRedLamp_Status
{
	SHORT m_nLeftStatus; //��ת����״̬���μ�ö��ENUM_LAMP_STATUS
	SHORT m_nThroughStatus; //ֱת����״̬
	SHORT m_nRightStatus; //��ת����״̬
	SHORT m_nTurnStatus; //��ͷ����״̬
} RedLamp_Status;

//======================================�豸�����ϴ�==============================//

//�豸�����ṹ��
typedef struct _tagDevice_Alarm
{
	TIME m_tmTime; //��������ʱ��
	WORD m_wErrorType; //��������
	char m_strErrorDesc[100]; //��������
} Device_Alarm;

typedef struct tagDeviceAlarm
{
	char gpioFlag;
	char i2cFlag;
	char tmpSetFlag;
	char tmpReadFlag;
	char mallocFlag;
	char spiFlag;
	//char		spiReadFlag;
} deviceAlarm;

//==================================�鿴�豸��ʷ������¼============================//
//�鿴�豸��ʷ������¼����ṹ��
typedef struct _tagAlarm_TimeSet
{
	TIME m_tmBegin; //��ʼʱ��
	TIME m_tmEnd; //����ʱ��
} Alarm_TimeSet;

//�����豸��ʷ������¼�ṹ��
typedef struct _tagAlarm_Record
{
	TIME m_tmTime; //��������ʱ��
	WORD m_wErrorType; //��������
	char strErrorDesc[100]; //����
} Alarm_Record;

//===================================�豸������־�ϴ�=======================//
//�����豸������־�ṹ��
typedef struct _tagDevice_Worklog
{
	char m_strPrint[255]; //���һ�л����
} Device_Worklog;

//====================================�豸�ڲ��ļ�����=============================//
typedef struct _tagNORMAL_FILE_INFO
{
	FTP_FILEPATH m_cFTPFilePath; // FTP���ļ�·��
	char m_strDeviceFilePath[255]; // �豸�ڵ��ļ�·��

} NORMAL_FILE_INFO;

//===================================�豸��������======================//
//�豸������������ṹ��
typedef struct _tagDevice_DataClear
{
	WORD m_wDataType; //������������
	TIME m_wTimeClearBefore; //�����ֹʱ��
	BOOL m_bOnlyDeletePictures; //�Ƿ�ֻ����ͼƬ
} Device_DataClear;

//=================================�鿴��ʷ��¼=======================//
//�鿴��ʷ��¼����ṹ��
typedef struct _tagHistory_TimeSet
{
	TIME m_tmBegin; //��ʷ��¼��ʼʱ��
	TIME m_tmEnd; //��ʷ��¼����ʱ��
} History_TimeSet;

//================================�ϴ���¼ͼƬ===============================//
//�ϴ���¼ͼƬ��������ṹ��
typedef struct _tagSendPicture_Request
{
	char m_strRecID[16]; //ͳһ��¼��ţ�15λ
} SendPicture_Request;

//================================ץ�ĳ���ָ��λ��ͼƬ������===================//
typedef struct _tagSCapture_Vehicle_Spec
{
	char m_strFileStart[255]; // ��ʼʶ��λ�õĳ���ͼƬ
	char m_strFileStop[255]; // ����ʶ��λ�õĳ���ͼƬ
} SCapture_Vehicle_Spec;

//================================NTP��ʱ��Ϣ������====================//
typedef struct _tagNTP_Device_Time
{
	TIME m_tDeviceTime; // �豸ʱ��
	char m_Reserve[50]; // ��չ�ֶ�
} NTP_Device_Time;
/***************************************MQ���߹㲥 **************************************/

//���������豸������Ϣ
typedef struct _tagOnline_Device
{
	BYTE ip[4]; //
	char m_strDeviceID[12]; //�豸���  ����10λ
	char m_strSpotName[100]; //��װ�ص�����
	char m_strDirection[8]; //��ʻ����
	char m_strFTP_URL[256]; //FTP��������URL
	char m_strVerion[64]; //Ƕ��ʽ����汾
	//Protocol_Version m_cVerion; //Э��汾
} Online_Device;

/***************************************�㲥��Ϣ��UDP��Ϣ ********************************/
enum
{
	SPORT_ID = 0, //�ص���
	DEV_ID = 1, //�豸���
	YEAR_MONTH = 2, //��/��
	DAY = 4, //��
	EVENT_NAME = 5, //�¼�����
	HOUR = 6, //ʱ
	FACTORY_NAME = 7, //�������ơ�

	NONE = 9, //��ʹ��
	LEVEL_NUM = 10
//������
};

//���ݰ�ͷ�ṹ��
typedef struct _tagMSG_HEADER
{
	WORD m_StartID; //����ʶ���̶�Ϊ0x00 68
	WORD m_MsgType; //��Ϣ���ͣ������ֵΪ6�����ǻظ�������������
	BYTE m_IsRequest; //������/Ӧ����Ϣ
	BYTE m_NeedReply; //����������Ϣ���Ƿ���ҪӦ��
	BYTE m_Result; //����Ӧ����Ϣ��ִ�гɹ�/ʧ��
	BYTE m_extent1[9]; //���� ����
	WORD m_ContenLength; //��Ϣ���ݳ���
	BYTE m_Content[256]; //��Ϣ���ݣ����ݲ�ͬ��Ϣ���ͣ����岻ͬ
	U32 sum_check; //У���,�ӵ�һ���ֽڿ�ʼ�����һ���ֽ�;
} MSG_HEADER;

typedef struct _tagFTP_URL_Level
{
	int levelNum; //����
	int urlLevel[LEVEL_NUM]; //���������������ڲ�
} FTP_URL_Level;

//��������ṹ��
typedef struct _tagNET_PARAM
{
	BYTE m_IP[4]; //IP��ַ����ǰ�������
	BYTE m_MASK[4]; //���룬��ǰ�������
	BYTE m_GATEWAY[4]; //���أ���ǰ�������
	char m_DeviceID[12]; //�豸��ţ�10λ����ҵ���
	BYTE m_MQ_IP[4]; //MQ��������IP����ǰ�������
	WORD m_MQ_PORT; //MQ������ ��TCP�˿�
	BYTE m_btMac[6]; //MAC��ַ
	TYPE_FTP_CONFIG_PARAM ftp_param_conf; //  FTP����������(�����ļ��ϴ�����)
	FTP_URL_Level ftp_url_level;
} NET_PARAM;

//���ݰ�ͷ�ṹ��
typedef struct _tagSET_NET_PARAM
{
	BYTE m_IP[4]; //ԭIP��ַ
	NET_PARAM m_NetParam; //��Ϣ���ݣ����ݲ�ͬ��Ϣ���ͣ����岻ͬ
} SET_NET_PARAM;

/***************************************�洢�豸���� **************************************/

typedef enum
{
	DISK_CLEAR = 1, //����Ӳ��
	DISK_FORMAT = 2, //��ʽ��Ӳ��
	DISK_FDISK = 3, //���·�������ʽ��Ӳ��
	DISK_MAX = 4
//< File manager command number.
} DISK_Cmd;

typedef struct _tagDisk_Clear
{
	WORD m_cmd;
	WORD m_partition; //0-3 λ����Ҫ��ʽ��������ķ���
} Disk_Clear;

/***************************************Υ����Ϣ����********************************/
//typedef struct _tagVIOLATE_DESCRIPTION
//{
//	char m_cUseFlag; //���ñ�־
//	char m_strOverSpeed[50]; //��������
//	char m_strRunRedLight[50]; //���������
//	char m_strGoAgaistTraffic[50]; //��������
//	char m_strNotByRoad[50]; //����������
//	char m_strOverLine[50]; //ѹʵ������
//	char m_strIllegalChangeLane[50]; //Υ�����
//	char m_strIllegalCrossLane[50]; //Υ��Խ��
//	char m_strIllegalStop[50]; //Υ��ͣ������
//	char m_strOverSafeIsland[50]; //ѹ��ȫ������
//	char m_strIllOccupySpeciallane[50]; // �Ƿ�ռ��ר�ó���
//	char m_strIllOccupyNon_motorized[50]; //�Ƿ�ռ�÷ǻ�������
//	//	char isUploadOverflowImg;       //�Ƿ��ϴ����ͼƬ
//	char m_strForcedIntoCrossroad[50]; //ӵ��ʱǿ��ʻ��·��Υ������
//	char m_strIllegalTravelRestriction[50]; //��ʱ����Υ������
//	char m_strReserve[49]; //����
//} VIOLATE_DESCRIPTION;

/***************************************ͼƬ������Ϣ����********************************/
//typedef struct
//{
//	unsigned char color_r; //��ɫRֵ
//	unsigned char color_g; //��ɫGֵ
//	unsigned char color_b; //��ɫBֵ
//
//} type_info_color;

//typedef struct _tagOVERLY_INFO
//{
//	//	WORD	m_wStartLine;				//��ʼ��
//	//	WORD	m_wColor;					//��ɫ	1����ɫ2����ɫ 3����ɫ
//
//	unsigned short start_x; //Υ��������ʼ����X;
//	unsigned short start_y; //Υ��������ʼ����Y;
//	type_info_color color; //Υ��������ɫ��Ϣ.
//
//	VIOLATE_DESCRIPTION m_cVioateDesc; //Υ������			//11.Υ������
//	WORD m_wTimeFlag; //�Ƿ���д����ʱ�� //1.ʱ��
//	WORD m_wRedLightStartTimeFlag; //�Ƿ���д�����ʼʱ��2.
//	WORD m_wRedLightKeepTimeFlag; //�Ƿ���д��Ʊ���ʱ��3.
//	WORD m_wSpotNameFlag; //�Ƿ���д�ص����� //4.�ص�����
//	WORD m_wDeviceNumFlag; //�Ƿ���д�豸���//5.�豸���
//	WORD m_wDirectionFlag; //�Ƿ���д��ʻ����//6.����
//	WORD m_wLaneNumFlag; //�Ƿ���д������	//7.����
//	WORD m_wLaneDirectionFlag; //�Ƿ���д��������	//8.��������
//	WORD m_wPlateNumFlag; //�Ƿ���д���ƺ���//9.���ƺ���
//	WORD m_wPlateColorFlag; //�Ƿ���д������ɫ//10.���ƺ���
//	WORD m_wPicNum; //���Ӽ���ͼƬ�������1,��ڶ�����ֻ����ʱ�䣬3-3��ͼƬȫ������
//	WORD m_wSpeedFlag; //�Ƿ���д�����ٶ�//11.�����ٶ�
//	WORD m_wPlateTypeFlag; // �Ƿ���Ӻ�������
//
//} OVERLY_INFO;

/***************************************���ڵ�����Ϣ����********************************/

//typedef struct _tagTRFCOVERLY_INFO
//{
//	char isOverlayFlag; //�Ƿ���ӹ�����Ϣ
//	unsigned short start_x; //����������ʼ����X;
//	unsigned short start_y; //����������ʼ����Y;
//	type_info_color color; //����������ɫ��Ϣ.
//
//	short m_wTimeFlag; //�Ƿ���д����ʱ�� //1.ʱ��
//	short m_wSpotNameFlag; //�Ƿ���д�ص����� //4.�ص�����
//	short m_wDeviceNumFlag; //�Ƿ���д�豸���//5.�豸���
//	short m_wDirectionFlag; //�Ƿ���д��ʻ����//6.����
//	short m_wLaneNumFlag; //�Ƿ���д������	//7.����
//	short m_wLaneDirectionFlag; //�Ƿ���д��������	//8.��������
//	short m_wPlateNumFlag; //�Ƿ���д���ƺ���//9.���ƺ���
//	short m_wPlateColorFlag; //�Ƿ���д������ɫ//10.���ƺ���
//	short m_wPlateTypeFlag; //�Ƿ���ӳ�������
//	short m_wSpeedFlag; //�Ƿ���д�����ٶ�//12.�����ٶ�
//
//} TRFCOVERLY_INFO;

/***************************************ʵʱ��Ƶ********************************/
// UDP�ְ�������Ƶ֡�Ķ��塣��ͷ��8�ֽڡ�
// Ŀǰ��ƵΪMJPEG��ʽ��֡ΪJPEGͼ��

typedef struct _tagVIDEOPACKAGE
{
	DWORD m_dwTimeStamp; // ʱ�����ʹ��9k HZʱ�ӡ���1��ֵ����9000��
	WORD m_wSeq; // ˳��š�ֵ0~65535��
	BYTE m_btTag; // ��־λ��
	// ��8λ����ʼ��־����ʾ������֡��
	// ��7λ��������־����ʾ��ǰ֡���ͽ�����
	// ��6��5λ���汾�š�(��������Ϊ0)
	// ��4,3,2,1λ��(��������Ϊ0)
	BYTE m_btPaloadType; // �������� (��������Ϊ0)
} VIDEOPACKAGE;

////====================================H264��ѯ�ϴ�FTP�ӿ�=============================//
//typedef struct _tagH264_TO_FTP
//{
//	char m_time_start[20]; // ʱ����� ʾ����201206110900
//	char m_time_end[20]; // ʱ���յ�
//	int m_nu; //����Ҫ���H264�ļ���
//	char m_H264_files[H264_MAX][100]; //�ϴ���FTP�е�H264�ļ���
//	char m_time_part[H264_MAX][30];   //��ѯ����ʱ���
//
//} H264_TO_FTP;

//====================================H264��ѯ�ӿ�=============================//
typedef struct _tagH264DOWN
{
	char m_time_start[20]; // ʱ����� ʾ����201206110900
	char m_time_end[20]; // ʱ���յ�
} H264_DOWN;

//====================================H264�ϴ�FTP�ӿ�=============================//
typedef struct _tagH264_TO_FTP
{
	int index; //���
	int total; //H264�ܸ���
	char timeStart[15]; //��ʼʱ��
	char timeEnd[15]; //����ʱ��
	int timeLength; //����ʱ�䳤��
	char path[100]; //H264��FTP�е�·��
	char device_id[20]; //�豸ID��
	int partition; //�ϴ��ɹ���־���ɹ���1�����ɹ���-1
} H264_UP;

//#########################################//#########################################// add by xsh

#pragma pack(pop, PACK1)

#endif /* EP_TYPE_H_ */
