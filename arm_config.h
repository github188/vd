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

#pragma pack(push,PACK1,4)	// 定义字节对齐方式
#define FTP_CHANEL_ILLEGAL		0x00
#define FTP_CHANEL_PASS_CAR 	0x01
#define FTP_CHANEL_H264			0x02

#define ARM_PARAM_FILE "arm_config.xml"
#define ARM_PARAM_FILE_PATH "/config/arm_config.xml"
#define BATCH_ARM_PARAM_FILE_PATH "/config/batch_arm_config.xml"

//数据存储操作结构体
typedef struct DISK_WRI_DATA
{
	DWORD remain_disk; // 当剩余多少空间时，开始循环覆盖

	WORD illegal_picture; //是否双备份：违法记录和图像
	WORD vehicle; //是否双备份：车辆通行记录和图像
	WORD event_picture; //是否双备份：事件记录和图像
	WORD illegal_video; //是否双备份：违法录像
	WORD event_video; //是否双备份：事件录像
	WORD flow_statistics; //是否双备份: 流量统计
} DISK_WRI_DATA;

// 数据上传操作结构体
typedef struct _FTP_DATA_CONFIG_
{
	WORD illegal_picture; //是否上传：违法记录和图像
	WORD vehicle; //是否上传：车辆通行记录和图像
	WORD event_picture; //是否上传：事件记录和图像
	WORD illegal_video; //是否上传：违法录像
	WORD event_video; //是否上传：事件录像
	WORD flow_statistics;//是否上传: 流量统计
} FTP_DATA_CONFIG;

typedef struct _RESUME_UPLOAD_PARAM_
{
	BOOL is_resume_passcar;
	BOOL is_resume_illegal;
	BOOL is_resume_event;
	BOOL is_resume_statistics;
} RESUME_UPLOAD_PARAM;

//数据上传/存储设置( 合并) 结构体
typedef struct DATA_SAVE
{
	DISK_WRI_DATA disk_wri_data;
	FTP_DATA_CONFIG ftp_data_config;
	RESUME_UPLOAD_PARAM resume_upload_data; //数据续传配置
} DATA_SAVE;

typedef struct _TYPE_FTP_CONFIG_PARAM_
{
	char user[32]; //用户名
	char passwd[32]; //密码
	BYTE ip[4]; //IP地址 IPV4: 前4字节。
	int port; //端口号
	BOOL allow_anonymous; //是否允许匿名登录
} TYPE_FTP_CONFIG_PARAM;

typedef struct _TYPE_MQ_CONFIG_PARAM_
{
	BYTE ip[4]; //IP地址 IPV4: 前4字节。
	int port; //端口号
} TYPE_MQ_CONFIG_PARAM;

typedef struct
{
	BOOL useNTP; //是否开启NTP对时
	BYTE NTP_server_ip[4]; //NTP服务器IP
	WORD NTP_distance; //对时间隔(单位分钟)
} NTP_CONFIG_PARAM;

typedef struct
{
	char username[128];
	char passwd[128];
	char url[128];
	char bucket_name[128];
}OSS_ALIYUN_PARAM;

//  基本参数结构
typedef struct _tagBASIC_PARAM
{
	WORD monitor_type; //监控点类型：交叉口/路段

	char spot_id[16]; //安装地点编号，12位，符合GA408.3要求
	char road_id[20]; //道路编号
	char spot[100]; //监控点名称
	WORD direction; //行驶方向

	NTP_CONFIG_PARAM ntp_config_param;

	//BOOL output_realdata_to_MQ; //是否输出实时记录到消息服务器

	//BOOL record_video; //是否视频录像

	// 数据上传接口使用
	WORD exp_type; // 接口类型（保留）. 0-设备内部协议，1-海信管控平台协议
	char exp_device_id[20]; // 接口使用的设备编号
	int collect_actor_size; //采集机关代码位数
	int log_level; //0--不写, 1--error, 2--warn, 3--state, 4--debug
	DATA_SAVE data_save; // 数据上传、存储参数

	BOOL h264_record; // 是否启用h264录像存储
	TYPE_FTP_CONFIG_PARAM ftp_param_illegal; //  FTP服务器参数(违法)
	TYPE_FTP_CONFIG_PARAM ftp_param_pass_car; //  FTP服务器参数(过车)
	TYPE_FTP_CONFIG_PARAM ftp_param_h264; //  FTP服务器参数(h264)

	TYPE_MQ_CONFIG_PARAM mq_param; //  MQ服务器参数

	BYTE ip_berth_front[4];               //泊位之间通信使用的IP，前一个泊车设备的IP
	BYTE ip_berth_back[4];               //泊位之间通信使用的IP，后一个泊位设备的IP

	OSS_ALIYUN_PARAM oss_aliyun_param;     //aliyun oss参数结构体 by shp 2015/04/28
} BASIC_PARAM;

enum io_func
{
	IO_NONE = 0, //不配置
	IO_RED_DETECT_IN = 1, //红灯检测器
	IO_YELLOW_DETECT_IN = 2, //黄灯检测器
	IO_ALARM_DETECT_IN = 3,//报警检测器
	IO_FUNC_COUNT
};

enum io_signal_direction
{
	IO_LEFT = 1, //左转
	IO_THROUGH = 2, //直行
	IO_RIGHT = 4, //右转
	IO_TRUN = 8, //调头
	IO_DIR_COUNT
};

typedef struct _IO_CFG
{
	unsigned char trigger_type; //0上升沿, 1下降沿, 2脉冲
	unsigned char mode; //参见io_func 是否配置红灯/黄灯检测器
	unsigned char io_drt; //如果配成红灯或者黄灯检测器，则此字段有效，参见io_signal_direction
} IO_cfg;

//串口参数结构体
typedef struct _tagSerialParam
{
	WORD dev_type;
	DWORD bps; //波特率  9600;
	WORD check;//校验位  2-- even, 1---odd, 0---none
	WORD data; //数据位
	WORD stop; //停止位  1--1位， 2--2位， 3--1.5位
} SerialParam;

//外部接口参数结构体
typedef struct _tagInterface_Param
{
	SerialParam serial[3]; //串口参数
	IO_cfg io_input_params[8]; //IO输入参数
	IO_cfg io_output_params[4]; //IO输出参数
} Interface_Param;

//单osd内容配置
typedef struct _Osd_item_
{
	int switch_on; //开关
	int x; // start position
	int y;

	int is_time; // time osd
	char content[40];

} Osd_item;

//H264 osd配置
typedef struct _Osd_info_
{
	Type_info_color color;
	Osd_item osd_item[8];
} Osd_info;

//单H264通道配置
typedef struct _H264_chanel_
{
	BOOL h264_on;//开关  false--off, true--on
	BOOL cast; //true --启动单播/组播， false --点播
	BYTE ip[4]; //单播/组播的IP地址
	int port;

	int fps; //帧率
	int rate; //码率
	int width; //图像宽
	int height; //图像高

	Osd_info osd_info;
} H264_chanel;

//H264配置
typedef struct _H264_config_
{
	H264_chanel h264_channel[2];
} H264_config;

enum
{
	ILLEGAL_CODE_ILLEGALPARK = 0, //非法停车
	ILLEGAL_CODE_OVERSAFETYSTRIP, //压安全岛
	ILLEGAL_CODE_ILLEGAL_ANELRUN, //不按道行驶
	ILLEGAL_CODE_CROSSSTOPLINE, //违法越线
	ILLEGAL_CODE_COVERLINE, //压实线
	ILLEGAL_CODE_CHANGELANE, //违法变道
	ILLEGAL_CODE_CONVERSE_DRIVE, //逆行
	ILLEGAL_CODE_RUNRED, //闯红灯
	ILLEGAL_CODE_EXCEED_SPEED, //超速
	ILLEGAL_CODE_OCCUPYSPECIAL, //非法占用专用车道
	ILLEGAL_CODE_OCCUPYNONMOTOR, //非法占用非机动车道
	ILLEGAL_CODE_FORCEDCROSS, //拥堵时强行驶入交叉路口
	ILLEGAL_CODE_LIMITTRAVEL, //违反限时限行
	ILLEGAL_CODE_NOTWAYTOPEDES = 13,//不礼让行人
	ILLEGAL_CODE_COUNT = 20,

};

//违法编码信息
typedef struct
{
	int illeagal_type; //违法类型.  见 EVENT_TYPE
	int illeagal_num; //用户定义的违法编号

} ILLEGAL_CODE_INFO;

typedef struct _ARM_config_
{
	BASIC_PARAM basic_param;
	//IO_Mode io_mode;
	Interface_Param interface_param;
	H264_config h264_config;
	ILLEGAL_CODE_INFO illegal_code_info[ILLEGAL_CODE_COUNT]; //违法编码信息

} ARM_config;

#pragma pack(pop, PACK1)
#endif /* ARM_PARAM_H_ */
