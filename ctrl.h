/*
 * ctrl.h
 *
 *  Created on: 2013-5-1
 *      Author: shanhw
 */

#ifndef CTRL_H_
#define CTRL_H_

#define OLD_BOARD         (0)
#define NEW_BOARD        (1)

#define RED_LIGHT    (1)
#define GREEN_LIGHT    (2)
#define BLUE_LIGHT    (3)
#define WHITE_LIGHT    (4)
#define LIGHT_OFF    (0)
#define LIGHT_ON     (1)


//typedef enum
//{
//	EP_POWER_OFF_3S = 0, //	 000���������3s     �������ϵͳ�쳣�������������codec�򲻿��������
//	EP_POWER_OFF_30S = 1, //	 001��������Ӳ��ͬʱ����30s  �������ϵͳ�쳣�������������Ӳ��̽�ⲻ�����⡣
//	HDD_POWER_OFF_2H = 2, //	 010��Ӳ�̵���2Сʱ    �����������Ӳ��̽�ⲻ�����⡣��Ӳ�̽��¡�
//	HDD_POWER_OFF = 3, //	 011��Ӳ��һֱ����    ����ȷ��Ӳ�̻���ʱ��ֹͣӲ�̡�
//	EP_POWER_OFF_2H = 4, //	 100��������Ӳ�̵���2Сʱ  �����¶ȹ���
//	EP_POWER_HEART_BEAT = 5, //	 101��������Ϣ     ����ÿ��20���ӣ�����һ�Ρ�
//	EP_HDD_POWER_ON = 6, //	 110��Ӳ�̶�̬���ء�
//	EP_POWER_DEFAULT = 7
////	 111��Ĭ��ֵ��     ����Ĭ��ֵ����Ϊ������Ϣ�Ľ�����־��
//
//} POWER_CTRL_Cmd_t;
//

/*ָʾ�ƿ���*/
typedef struct
{
	char name[20];
	char turn_on;
	char turn_off;
}str_light_info;




typedef struct
{
	str_light_info enable;
	str_light_info red;
	str_light_info green;
	str_light_info blue;
}str_light_gpio;

/*����ƿ���*/
typedef struct
{
	unsigned char addr; //�Ƶĵ�ַ
	unsigned short sn;  //���
	unsigned char cmd;  //����;
	unsigned short len;
    unsigned char data[1];
}Light_cmd;

typedef struct
{
	short header;		//ͷ�̶�Ϊ��H��
	short len;			//����sizeof��Light_cmd��
	Light_cmd data;		//����ָ����Ϣ
	int	chk;		//У���
}Light_msg;


typedef struct
{
	unsigned int year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
}str_time;

typedef struct
{	
	str_time start_time;  
	str_time end_time;
}str_ctrl;


extern str_light_gpio gstr_light_gpio;
extern int gi_platform_board;
extern str_light_info gstr_filllight_info;

//extern int flag_use_gpio10;//test temp

//#########################################

extern void *ctrlThrFxn(void *arg);

/*************************************
Function:       set_light_by_gpio
Description:    ָʾ�ƿ��ƺ���
                ָʾ���к죬�̣����Լ��׹�ƣ���:gpio99 ��:gpio73 ��:gpio72 ��:ʹ��pwm����
Author:  	    lxd
Date:	        2014.1.5
*************************************/
int set_light_by_gpio(const char *val,char a);

/*************************************
Function:       read_gpio_status
Description:   ��ȡGPIO״̬��Ϣ
Author:  	    
Date:	        
*************************************/
extern char read_gpio_status(const char *val);
#endif /* CTRL_H_ */
