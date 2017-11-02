
#ifndef _PCIE_API_H_
#define _PCIE_API_H_


#include <stdint.h>

#include "commontypes.h"
//#include "alg_result.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"


#define ILLEGAL_PIC_MAX_NUM 4


typedef struct _Pcie_data_head
{
	uint32_t Head; //帧头，固定为0xABABABAB
	uint32_t Index; //实时视频流帧号，违法图片无意义
	uint32_t Size; //有效数据大小
	uint32_t Type; //数据类型，包括实时流（0）、抓拍（1）、信号灯信号（2）、机动车通行记录（3）、行人与非机动车通行记录（4）、机动车违法记录（5）、行人与非机动车违法记录（6）、事件（7）、交通流（8）等
	uint32_t EncType; //违法编码方式，三张合成一张编码（31）或者单独编（33）等
	uint32_t Wight; //JPEG宽
	uint32_t Height; //JPEG高
	uint32_t Dest; //JPEG发送目的，上位机（0）或FTP（1）
	uint32_t TimeStamp; //JPEG时间戳，可能某些同步需要使用，预留
	uint32_t NumPic; //JPEG图片张数
	uint32_t PosPic[ILLEGAL_PIC_MAX_NUM]; //JPEG图片的偏移位置，最多4张：第一张偏移为0，
	//第二张的位置为第一张的尾，然后进行4字节对齐后的位置；后面的类似。
} Pcie_data_head;


/*************************  算法输出，8147转MQ消息的信息  ***********************/

//2，机动车通行记录

//图片信息结构体－－由控制核赋值
typedef struct PicInfo
{
	int picIndex; 	//图片索引，
	int picSize; 	//图片大小，
	WORD year; 		//年,   图片产生时间，（YUV接收时间）
	WORD month; 	//月
	WORD day; 		//日
	WORD hour; 		//时
	WORD minute; 	//分
	WORD second; 	//秒
	WORD msecond; 	//毫秒
} PicInfo;

//单个过车记录：
typedef struct passRecordVehicle

{
	PicInfo picInfo;//图片信息
	TrfcVehiclePlatePoint passVehicle;//机动车通行信息
} passRecordVehicle;



//3，行人与非机动车通行记录

//单个行人与非机动车通行记录：
typedef struct passRecordNoVehicle
{
	PicInfo picInfo; 				//图片信息
	NoVehiclePoint passNoVehicle; 	//非机动车通行信息
} passRecordNoVehicle;


//单个机动车违法记录：
typedef struct illegalRecordVehicle
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM]; 	//图片信息,最多4个
	IllegalInfoPer illegalVehicle; 			//机动车违法信息
} illegalRecordVehicle;


//5，行人与非机动车违法记录：

//单个行人与非机动车违法记录：
typedef struct illegalRecordNoVehicle
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM];//图片信息,最多4个
	NoVehiclePoint illegalNoVehicle;//非机动车违法信息
} illegalRecordNoVehicle;


//6，事件：

//单个事件报警记录
typedef struct SingleEventAlarm
{
	PicInfo picInfo[ILLEGAL_PIC_MAX_NUM];//图片信息	,最多4个
	EventAlarmInfo eventAlarmInfo; //事件报警信息
} SingleEventAlarm;


#endif 	/* _PCIE_API_H_ */
