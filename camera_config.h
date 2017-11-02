/*******************************************************************************
* 文件名称： ISP.h
* 描    述： ISP参数定义头文件
* 作    者： 郑维学
* 初始版本： 1.0
* 开发日期： 2013年 05月 17日
* 完成日期： 2013年 05月 17日
* Copyright (c) 2013 Hisense TransTech Co.,Ltd
*-------------------------------------------------------------------------------
* 版    本       日    期		     修改人        原               因
* -------      --------              -----        ------------------------------
*  1.0           2013-05-17			郑维学			初始版本
*******************************************************************************/
#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "commontypes.h"


#define CAMERA_PARAM_FILE "camera_config.xml"
#define CAMERA_PARAM_FILE_PATH "/config/camera_config.xml"

#pragma pack(push,PACK1,4)			// 定义字节对齐方式

//#ifndef WORD
//	typedef unsigned long       DWORD;
//	typedef int                 BOOL;
//	typedef unsigned char       BYTE;
//	typedef unsigned short      WORD;
//#endif


// 1.曝光参数
typedef struct _Expmode
{
	int	flag; // 0.手动曝光 手动增益 1.自动曝光 自动增益 2.自动曝光 手动增益  3.手动曝光 自动增益
}s_ExpMode;

typedef struct _ExpManu
{
	int exp;	// 手动曝光曝光值
	int gain;	// 手动增益增益值
}s_ExpManu;

typedef struct _ExpAuto
{
	int light_dt_mode;	// 测光模式
	int exp_max;	// 快门上限
	int gain_max;	// 增益上限
	int exp_mid;	// 快门中限
	int gain_mid;	// 增益中限
	int exp_min;	// 快门下限
	int gain_min;	// 增益下限
}s_ExpAuto;

typedef struct _ExpWnd
{
	char line1;	// 8*8测光窗口的每一行，每一位代表一个窗口
	char line2;
	char line3;
	char line4;
	char line5;
	char line6;
	char line7;
	char line8;
}s_ExpWnd;


// 2.白平衡参数
typedef struct _AwbMode
{
	int flag;	// 0 手动白平衡 1 自动白平衡
}s_AwbMode;

typedef struct _AwbManu
{
	int gain_r; // 手动白平衡红绿蓝增益
	int gain_g;
	int gain_b;
}s_AwbManu;

// 3.色彩参数
typedef struct _ColorParam
{
	int contrast;	// 对比度
	int luma;	// 亮度
	int saturation;	// 饱和度
}s_ColorParam;

typedef struct _tagSynchronous
{
	char is_syn_open;		//是否开启交流同步:0：关闭；1：开启
	int  phase;				//相位：0~20
}Syn_Info;

#define LAMP_COUNT  4
//补光灯配置
typedef struct _tagLampInfo
{
	int nMode[LAMP_COUNT];		//触发方式 0:上升沿  1:下降沿
	int nLampType[LAMP_COUNT];  //补光灯类型 无：0 1:频闪灯 2: LED   3: 气体
}Lamp_Info;


//摄像机参数结构体
typedef struct _tagCamera_Param
{
	// 1.曝光参数
	s_ExpMode exp_mode;// 0.手动曝光 手动增益 1.自动曝光 自动增益 2.自动曝光 手动增益  3.手动曝光 自动增益

	// 手动曝光曝光值 手动增益增益值
	s_ExpManu exp_manu;
	s_ExpAuto exp_auto;
	s_ExpWnd exp_wnd;

	// 2.白平衡参数
	s_AwbMode awb_mode;
	s_AwbManu awb_manu;

	// 3.色彩参数
	s_ColorParam color_param;

	Syn_Info syn_info;
	Lamp_Info lamp_info;
} Camera_config;
#pragma pack(pop,PACK1)
#endif
