
#ifndef _PCIE_API_H_
#define _PCIE_API_H_


#include <stdint.h>

#include "commontypes.h"
//#include "alg_result.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"


#define ILLEGAL_PIC_MAX_NUM 4


typedef struct _Pcie_data_head
{
	uint32_t Head; //֡ͷ���̶�Ϊ0xABABABAB
	uint32_t Index; //ʵʱ��Ƶ��֡�ţ�Υ��ͼƬ������
	uint32_t Size; //��Ч���ݴ�С
	uint32_t Type; //�������ͣ�����ʵʱ����0����ץ�ģ�1�����źŵ��źţ�2����������ͨ�м�¼��3����������ǻ�����ͨ�м�¼��4����������Υ����¼��5����������ǻ�����Υ����¼��6�����¼���7������ͨ����8����
	uint32_t EncType; //Υ�����뷽ʽ�����źϳ�һ�ű��루31�����ߵ����ࣨ33����
	uint32_t Wight; //JPEG��
	uint32_t Height; //JPEG��
	uint32_t Dest; //JPEG����Ŀ�ģ���λ����0����FTP��1��
	uint32_t TimeStamp; //JPEGʱ���������ĳЩͬ����Ҫʹ�ã�Ԥ��
	uint32_t NumPic; //JPEGͼƬ����
	uint32_t PosPic[ILLEGAL_PIC_MAX_NUM]; //JPEGͼƬ��ƫ��λ�ã����4�ţ���һ��ƫ��Ϊ0��
	//�ڶ��ŵ�λ��Ϊ��һ�ŵ�β��Ȼ�����4�ֽڶ�����λ�ã���������ơ�
} Pcie_data_head;


/*************************  �㷨�����8147תMQ��Ϣ����Ϣ  ***********************/

//2��������ͨ�м�¼

//ͼƬ��Ϣ�ṹ�壭���ɿ��ƺ˸�ֵ
typedef struct PicInfo
{
	int picIndex; 	//ͼƬ������
	int picSize; 	//ͼƬ��С��
	WORD year; 		//��,   ͼƬ����ʱ�䣬��YUV����ʱ�䣩
	WORD month; 	//��
	WORD day; 		//��
	WORD hour; 		//ʱ
	WORD minute; 	//��
	WORD second; 	//��
	WORD msecond; 	//����
} PicInfo;

//����������¼��
typedef struct passRecordVehicle

{
	PicInfo picInfo;//ͼƬ��Ϣ
	TrfcVehiclePlatePoint passVehicle;//������ͨ����Ϣ
} passRecordVehicle;



//3��������ǻ�����ͨ�м�¼

//����������ǻ�����ͨ�м�¼��
typedef struct passRecordNoVehicle
{
	PicInfo picInfo; 				//ͼƬ��Ϣ
	NoVehiclePoint passNoVehicle; 	//�ǻ�����ͨ����Ϣ
} passRecordNoVehicle;


//����������Υ����¼��
typedef struct illegalRecordVehicle
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM]; 	//ͼƬ��Ϣ,���4��
	IllegalInfoPer illegalVehicle; 			//������Υ����Ϣ
} illegalRecordVehicle;


//5��������ǻ�����Υ����¼��

//����������ǻ�����Υ����¼��
typedef struct illegalRecordNoVehicle
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM];//ͼƬ��Ϣ,���4��
	NoVehiclePoint illegalNoVehicle;//�ǻ�����Υ����Ϣ
} illegalRecordNoVehicle;


//6���¼���

//�����¼�������¼
typedef struct SingleEventAlarm
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM];//ͼƬ��Ϣ	,���4��
	EventAlarmInfo eventAlarmInfo; //�¼�������Ϣ
} SingleEventAlarm;


#endif 	/* _PCIE_API_H_ */
