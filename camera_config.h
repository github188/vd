/*******************************************************************************
* �ļ����ƣ� ISP.h
* ��    ���� ISP��������ͷ�ļ�
* ��    �ߣ� ֣άѧ
* ��ʼ�汾�� 1.0
* �������ڣ� 2013�� 05�� 17��
* ������ڣ� 2013�� 05�� 17��
* Copyright (c) 2013 Hisense TransTech Co.,Ltd
*-------------------------------------------------------------------------------
* ��    ��       ��    ��		     �޸���        ԭ               ��
* -------      --------              -----        ------------------------------
*  1.0           2013-05-17			֣άѧ			��ʼ�汾
*******************************************************************************/
#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "commontypes.h"


#define CAMERA_PARAM_FILE "camera_config.xml"
#define CAMERA_PARAM_FILE_PATH "/config/camera_config.xml"

#pragma pack(push,PACK1,4)			// �����ֽڶ��뷽ʽ

//#ifndef WORD
//	typedef unsigned long       DWORD;
//	typedef int                 BOOL;
//	typedef unsigned char       BYTE;
//	typedef unsigned short      WORD;
//#endif


// 1.�ع����
typedef struct _Expmode
{
	int	flag; // 0.�ֶ��ع� �ֶ����� 1.�Զ��ع� �Զ����� 2.�Զ��ع� �ֶ�����  3.�ֶ��ع� �Զ�����
}s_ExpMode;

typedef struct _ExpManu
{
	int exp;	// �ֶ��ع��ع�ֵ
	int gain;	// �ֶ���������ֵ
}s_ExpManu;

typedef struct _ExpAuto
{
	int light_dt_mode;	// ���ģʽ
	int exp_max;	// ��������
	int gain_max;	// ��������
	int exp_mid;	// ��������
	int gain_mid;	// ��������
	int exp_min;	// ��������
	int gain_min;	// ��������
}s_ExpAuto;

typedef struct _ExpWnd
{
	char line1;	// 8*8��ⴰ�ڵ�ÿһ�У�ÿһλ����һ������
	char line2;
	char line3;
	char line4;
	char line5;
	char line6;
	char line7;
	char line8;
}s_ExpWnd;


// 2.��ƽ�����
typedef struct _AwbMode
{
	int flag;	// 0 �ֶ���ƽ�� 1 �Զ���ƽ��
}s_AwbMode;

typedef struct _AwbManu
{
	int gain_r; // �ֶ���ƽ�����������
	int gain_g;
	int gain_b;
}s_AwbManu;

// 3.ɫ�ʲ���
typedef struct _ColorParam
{
	int contrast;	// �Աȶ�
	int luma;	// ����
	int saturation;	// ���Ͷ�
}s_ColorParam;

typedef struct _tagSynchronous
{
	char is_syn_open;		//�Ƿ�������ͬ��:0���رգ�1������
	int  phase;				//��λ��0~20
}Syn_Info;

#define LAMP_COUNT  4
//���������
typedef struct _tagLampInfo
{
	int nMode[LAMP_COUNT];		//������ʽ 0:������  1:�½���
	int nLampType[LAMP_COUNT];  //��������� �ޣ�0 1:Ƶ���� 2: LED   3: ����
}Lamp_Info;


//����������ṹ��
typedef struct _tagCamera_Param
{
	// 1.�ع����
	s_ExpMode exp_mode;// 0.�ֶ��ع� �ֶ����� 1.�Զ��ع� �Զ����� 2.�Զ��ع� �ֶ�����  3.�ֶ��ع� �Զ�����

	// �ֶ��ع��ع�ֵ �ֶ���������ֵ
	s_ExpManu exp_manu;
	s_ExpAuto exp_auto;
	s_ExpWnd exp_wnd;

	// 2.��ƽ�����
	s_AwbMode awb_mode;
	s_AwbManu awb_manu;

	// 3.ɫ�ʲ���
	s_ColorParam color_param;

	Syn_Info syn_info;
	Lamp_Info lamp_info;
} Camera_config;
#pragma pack(pop,PACK1)
#endif
