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
//	EP_POWER_OFF_3S = 0, //	 000－主板掉电3s     －－解决系统异常死机；解决部分codec打不开的情况。
//	EP_POWER_OFF_30S = 1, //	 001－主板与硬盘同时掉电30s  －－解决系统异常死机；解决部分硬盘探测不到问题。
//	HDD_POWER_OFF_2H = 2, //	 010－硬盘掉电2小时    －－解决部分硬盘探测不到问题。给硬盘降温。
//	HDD_POWER_OFF = 3, //	 011－硬盘一直掉电    －－确认硬盘坏了时，停止硬盘。
//	EP_POWER_OFF_2H = 4, //	 100－主板与硬盘掉电2小时  －－温度过高
//	EP_POWER_HEART_BEAT = 5, //	 101－心跳消息     －－每隔20分钟，发送一次。
//	EP_HDD_POWER_ON = 6, //	 110－硬盘动态加载。
//	EP_POWER_DEFAULT = 7
////	 111－默认值。     －－默认值。作为其他信息的结束标志。
//
//} POWER_CTRL_Cmd_t;
//

/*指示灯控制*/
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

/*补光灯控制*/
typedef struct
{
	unsigned char addr; //灯的地址
	unsigned short sn;  //序号
	unsigned char cmd;  //命令;
	unsigned short len;
    unsigned char data[1];
}Light_cmd;

typedef struct
{
	short header;		//头固定为‘H’
	short len;			//长度sizeof（Light_cmd）
	Light_cmd data;		//具体指令信息
	int	chk;		//校验和
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
Description:    指示灯控制函数
                指示灯有红，绿，蓝以及白光灯，红:gpio99 绿:gpio73 蓝:gpio72 白:使用pwm控制
Author:  	    lxd
Date:	        2014.1.5
*************************************/
int set_light_by_gpio(const char *val,char a);

/*************************************
Function:       read_gpio_status
Description:   读取GPIO状态信息
Author:  	    
Date:	        
*************************************/
extern char read_gpio_status(const char *val);
#endif /* CTRL_H_ */
