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

#pragma pack(push,PACK1,4)	// 定义字节对齐方式

#define DSP_VERSION_STR_LENGTH_MAX (64u)
//by lxd
/*
typedef struct _dsp_version
{
	char acVersion[DSP_VERSION_STR_LENGTH_MAX];
} DSP_version;
*/           


////////////////////////////////////  消息类型定义 ////////////////////////////////////////////////////////

enum ENUM_MESSAGE_QUEUE_TYPE
{
	MSG_PARAM_ERR = -1,

	MSG_LOOPS = 0, //心跳消息

	MSG_UPLOAD_ARM_PARAM = 1, // 上传arm参数
	MSG_DOWNLOAD_ARM_PARAM, // 下载arm参数
	MSG_BATCH_CONFIG_ARM, //批量配置ARM


	MSG_UPLOAD_DSP_PARAM, // 上传DSP参数
	MSG_DOWNLOAD_DSP_PARAM, //下载DSP参数
	MSG_BATCH_CONFIG_DSP, //批量配置DSP

	MSG_UPLOAD_CAMERA_PARAM, // 上传camera参数
	MSG_DOWNLOAD_CAMERA_PARAM, //下载camera参数
	MSG_BATCH_CONFIG_CAMERA, //批量配置camera

	MSG_UPLOAD_HIDE_PARAM_FILE = 10, //上传隐形参数文件
	MSG_DOWNLOAD_HIDE_PARAM_FILE, //下载隐形文件

	MSG_UPLOAD_NORMAL_FILE, //上传一般文件
	MSG_DOWNLOAD_NORMAL_FILE, //下载一般文件

	MSG_CAPTURE_PICTURE, // 抓拍图片 14

	MSG_DEVICE_UPGRADE, //设备升级  15
	MSG_GET_TIME, //查询设备时间
	MSG_SET_TIME, //设置设备时间

	MSG_REDLAMP_STATUS, //信号灯灯状态 Signallamp_status 需要更新红灯时间.
	MSG_REDLAMP_POSITION, //信号灯位置 Signal_detect_video
	MSG_PLATE_SIZE = 20, //车牌尺寸范围   Plate_size 需要更新参数.

	MSG_BATCH_CONFIG, //批量配置命令
	MSG_REBOOT, //重启
	MSG_FOCUS_ADJUST = 23, //聚焦调整
	MSG_IRIS_SET = 24, //光圈设置
	MSG_DEVICE_INFOMATION = 25, //查看设备基本信息
	MSG_DEVICE_STATUS =26,		//查看设备状态

	///////// 以下待定 /////////////////

	/*************************
	 MQ设备管理消息
	 *************************/
	MSG_FACTORY_CONTROL, //读取写入厂商控制选项

	MSG_DEVICE_ALARAM_HISTORY, //查看设备报警历史
	MSG_DEVICE_WORKLOG, //设备工作日志上传
	MSG_DEVICE_ALARM, //设备报警上传

	MSG_DEVICE_DATA_CLEAR, //设备数据清理
	MSG_HISTORY_RECORD, //查看历史记录

	MSG_CAPTURE_VEHICLE_SPEC, //抓拍车辆指定位置图片

	MSG_HD_OPERATION, //硬盘格操作通知，extend=0 清理硬盘；extend=1 格式化硬盘


	MSG_HD_OPERATION_ERROR,//收到硬盘命令，硬盘正在使用，报警上位机
	MSG_HD_OPERATION_OK, //硬盘确定进行操作

	////////////// 回应 ///////////////////////
	//arm参数
	MSG_UPLOAD_ARM_PARAM_r = 101, // 上传arm参数
	MSG_DOWNLOAD_ARM_PARAM_r, // 下载arm参数
	MSG_BATCH_CONFIG_ARM_r, //批量配置ARM

	//DSP参数
	MSG_UPLOAD_DSP_PARAM_r, // 上传DSP参数
	MSG_DOWNLOAD_DSP_PARAM_r, //下载DSP参数
	MSG_BATCH_CONFIG_DSP_r, //批量配置DSP

	//camera参数
	MSG_UPLOAD_CAMERA_PARAM_r, // 上传camera参数
	MSG_DOWNLOAD_CAMERA_PARAM_r, //下载camera参数
	MSG_BATCH_CONFIG_CAMERA_r, //批量配置camera

	//隐形参数
	MSG_UPLOAD_HIDE_PARAM_FILE_r, //上传隐形参数文件
	MSG_DOWNLOAD_HIDE_PARAM_FILE_r, //下载隐形文件  11

	//一般文件
	MSG_UPLOAD_NORMAL_FILE_r, //上传一般文件
	MSG_DOWNLOAD_NORMAL_FILE_r = 113, //下载一般文件

	//其他
	MSG_CAPTURE_PICTURE_r,// 抓拍图片,返回结果
	MSG_DEVICE_UPGRADE_r, //设备升级, 回应
	MSG_GET_TIME_r, //查询设备时间, 回应消息
	MSG_SET_TIME_r,//设置时间

	MSG_BATCH_CONFIG_r, //批量配置命令
	MSG_REBOOT_r, //重启

	MSG_IRIS_SET_r,
	MSG_DEVICE_INFOMATION_r = 121, //查看设备基本信息
	MSG_DEVICE_STATUS_r = 122, //查看设备状态

	MSG_PARK_UPLOAD_r = 123,   // 泊车信息上传
	MSG_HISTORY_RECORD_REQ=1000, 		//查询历史记录;
	MSG_HISTORY_RECORD_REQ_ACK = 1001,  //应答查看历史记录
	MSG_EXT_HISTORY_RECORD_START = 1002, //导出历史记录
	MSG_EXT_HISTORY_RECORD_START_ACK = 1003, //应答导出历史记录
	MSG_EXT_HISTORY_RECORD_END = 1004, //停止导出记录
	MSG_EXT_HISTORY_RECORD_END_ACK = 1005, //应答停止导出记录

	/*******************************************************************************
	云台相关消息
	******************************************************************************/
	MSG_PTZ_CTRL = 1200,	// 云台控制相关消息，编号从1200开始   add by zhengweixue 20140520

};

enum ENUM_MSG_BROADCAST_TYPE
{
	MSG_ONLINE_DEVICE = 1, //mq刷新, socket现场搜索设备
	MSG_DEVICE_NETWORK_PARAM = 2, //获取设备网络参数
	MSG_DEVICE_CURRENT_PARAM = 3, //返回设备当前网络参数
	MSG_SET_NETWORK_PARAM = 4, //设置网络参数
	MSG_SET_METWORK_RESULT = 5, //设置网络参数结果反馈
	MSG_RECOVERY_DEFAULTCONFIG = 6, //恢复出厂设置
	MSG_RECOVERY_DEFAULTCONFIG_RESULT = 7, //恢复出厂设置结果反馈
	MSG_SET_NETWORK_PARAM_CENTER = 8,//设置中心配置网络参数
	MSG_OPEN_DEV = 9, //配置管理软件打开设备。
	MSG_CLOSE_DEV = 10, //配置管理软件关闭设备。
	MSG_OPEN_DEV_RETURN_TRUE = 11,//收到配置管理软件打开设备的命令成功,返回给配置管理软件。
	MSG_OPEN_DEV_RETURN_FALSE = 12,//收到配置管理软件打开设备的命令失败,返回给配置管理软件。
	MSG_CLOSE_DEV_RETURN_TRUE = 13,//收到配置管理软件关闭设备的命令成功,返回给配置管理软件。
	MSG_CLOSE_DEV_RETURN_FALSE = 14,//收到配置管理软件关闭设备的命令失败,返回给配置管理软件。
	MSG_ONLINE_RESULT = 15,
//设备在线返回配置管理软件消息;
};

typedef struct
{
	int event_type; //事件类型:0--过车, 1--违法,2-流量统计,3-事件,
	int dest_mq; //导出MQ会话地址: 0:导出到备用mq会话; 1:导出到管控平台mq会话;
	char time_start[64];//时间起点 示例：2013-06-01 12:01:02
	char time_end[64];  //时间终点
} TYPE_HISTORY_RECORD;

//查询历史记录消息结果;
typedef struct
{
	int count_record;		//符合条件的记录条数;

} TYPE_HISTORY_RECORD_RESULT;


//违法类型号;
typedef struct
{
	int ill_stop; //非法停车
	int over_safe_tystrip;//压安全岛
	int ill_lane_run;//不按道行驶
	int over_line;//违法越线
	int over_yellow_line;//压实线
	int over_lane;// 违法变道
	int wrong_driect;// 逆行
	int run_red;//闯红灯
	int over_speed;// 超速
	int ill_occup_special;//非法占用专用车道
	int ill_occup_no_motor;//非法占用非机动车道
	int over_flow_alarm;//强行驶入;
	int vehicle_travel_restriction; //限时限行
	int nocar_over_car_lan; //行人闯机动车道;
} type_ill_num;

//////////////////////////////////// 数据值的定义 //////////////////////////////////////////////////////////////

//MQ数据上传消息类型
enum ENUM_MSG_TYPE
{
	MSG_ILLEGALDATA = 1, //违法数据
	MSG_VEHICULAR_TRAFFIC = 2, //车辆通行
	MSG_EVENT_ALARM = 3, //事件报警
	MSG_DEVICE_INFO = 4, //设备状态
	MSG_TRAFFICFLOW_INFO = 5,
	MSG_NOCAR_REC = 6, //非机动车通行记录;
	MSG_NOCAR_ILLEGAL = 7, //非机动车违法记录;
	MSG_NOCAR_FLOW_INFO = 8,
	MSG_PARK=9, //泊车记录  add by lxd                  
//行人流量;
//交通流信息
};

//事件报警类型
enum ENUM_EVENT_ALARM
{
	EVENT_VEHICLE_RETROGRADE = 1, //车辆逆行
	EVENT_ILLEGAL_LANE = 2, //不按车道行驶
	EVENT_ILLEGAL_STOP = 3, //违法停车
	EVENT_ON_SOLIDLINE = 4, //压实线
	EVENT_ILLEGAL_CHANGELINE = 5, //违法变道
	EVENT_ILLEGAL_CROSSLINE = 6, //违法越线
	EVENT_TREAD_SAFEISLAND = 8,
//机动车辆压安全岛
};

//摄像机状态
enum ENUM_CAMEAR_STATUS
{
	CAMERA_NORMAL = 0, //正常
	CAMERA_NOCONNECT = 1, //连接不上
	CAMERA_PICTURE_ABNORMAL = 2,
//图像异常
};

//设备故障状态
enum ENUM_DEVICE_ERROR
{
	DEVICE_NORMAL = 0, //正常
	DEVICE_FAULT = 1,
//故障
};

//监控点类型
enum ENUM_TRAFFIC_MODE
{
	CROSSROAD = 1, //交叉路口
	SECTION_ROAD = 2,
//路段
};

// 开启功能中各位的定义
#define FUNC_VIA		0x0001
#define FUNC_GATEWAY	0x0002
#define FUNC_EVENT		0x0004
#define FUNC_SPEED		0x0008
#define FUNC_SIGNAL		0x0010

//设备报警故障类型
enum ENUM_DEVICE_ALARM
{
	ALARM_OTHER = 0, //其它故障
	ALARM_HARDDISK_ERROR = 1, //硬盘故障：硬盘满/硬盘加载失败/硬盘写入失败等
	ALARM_CAMERA_ERROR, //摄像机故障：通讯失败等
	ALARM_FTP_ERROR, //FTP故障：无法连接/写入失败等
	ALARM_BOARD_COMMUNICATION, //主从板卡通讯故障：打开从板失败/写入超时等
	ALARM_MQ, //MQ故障
	ALARM_FLASH, //Flash故障：写入失败/满
	ALARM_I2C, //I2C故障：i2c打开失败 / 温度传感器访问失败 / RTC访问失败
	ALARM_UART, //UART访问失败
	ALARM_SPI,//SPI故障
	ALARM_DOOR_OPEN,
//机柜开门报警
};

//设备清理数据类型
enum ENUM_DEVICE_DATACLEAR
{
	ALL_CLAER = 0, //全部
	VEHICLERECORD_CLEAR = 1, //车辆记录
	ILLEGALRECORD_CLEAR = 2, //违法记录
	EVENTALARM_CLEAR = 3, //事件报警
	WORKLOG_CLEAR = 4,
//日志

};

typedef struct
{
	u8 str_head[8]; //类型说明
	u16 master; //主版本号
	u16 slaver; //子版本号
	u16 revise; //修正版本号
	u8 str_build[64]; //编译版本号
	u16 protocol_master; //上位机接口 主版本号;
	u16 protocol_slaver; //上位机接口 次版本号;
	u16 debug; //调试版本号
	u32 arm_dsp; //算法接口号
} type_version_arm;

//typedef struct
//{
//	u8 str_head[8]; //类型说明
//	u16 decode; //解码算法版本号
//	u16 encode; //编码算法版本号
//	u16 slave; //从板版本号
//	u16 debug; //调试版本号
//	u32 arm_dsp; //arm接口号
//} type_version_dsp;

typedef struct
{
	SHORT m_nLeftIo; //左转方向状态。参见枚举ENUM_LAMP_STATUS
	SHORT m_nThroughIo; //直转方向状态
	SHORT m_nRightIo; //右转方向状态
	SHORT m_nTurnIo; //调头方向状态
} RED_LAMP_IO;

typedef struct
{
	SHORT m_nLeftStatus; //左转方向状态。参见枚举ENUM_LAMP_STATUS
	SHORT m_nThroughStatus; //直转方向状态
	SHORT m_nRightStatus; //右转方向状态
	SHORT m_nTurnStatus; //调头方向状态
	//SHORT		m_nLeftChange;
	//SHORT		m_nThroughChange;
	//SHORT		m_nRightChange;
	//SHORT		m_nTurnChange;
} RED_LAMP_STATUS;

typedef struct
{
	SHORT m_nlane1Flag; //车道1闯红灯标志
	SHORT m_nlane2Flag; //车道2闯红灯标志
	SHORT m_nlane3Flag; //车道3闯红灯标志

} RED_LAMP_VIOLATE;

typedef struct
{
	SHORT m_nlane1Flag; //车道1过车标志
	SHORT m_nlane1Speed; //车道1过车车速
	SHORT m_nlane2Flag; //车道1过车标志
	SHORT m_nlane2Speed;
	SHORT m_nlane3Flag; //车道3过车标志
	SHORT m_nlane3Speed;
} CAR_PASS;

//拥堵检测背景区域;
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
//########## 主从板传递隐性参数 ##############//
typedef struct
{
	u8 run_red_mode; //闯红灯抓拍模式
	OverFlowAreaInfo overFlowInfo[2]; //溢出检测标定
	u16 VehicleSpeedThr; //异常速度上限值
	float fOverLaneRatio; //违法变道灵敏度（新增）
	float fOverLineRatio; //违法越线灵敏度（新增）
	int smallYellowCarEvent;//小黄牌事件控制，安位开启，-1：关闭小黄牌检测，0：小黄牌过车检测，但没有事件检测，1：小黄牌检测违法停车，2：小黄牌检压安全岛
	char lowConsumeMode; //低耗模式开关，0：关闭低耗模式，1：开启低耗模式（新增）
	char cNoVehiCaptureLevel;//非机动车捕获灵敏度 1-3级 1级：保证有效率； 3级：保证捕获率
	char cPeopleCaptureLevel;//行人捕获灵敏度 1-3级 1级：保证有效率； 3级：保证捕获率
} type_config;

////////////////////////////////////////////实时数据结构体定义 (设备使用) ///////////////////////////////////
// 参照行业标准和管控平台标准制订。设备上传文本格式，此结构体为内部使用。可以在文本和结构体间相互转换。
//违法记录
typedef struct _tagIllegal_Records
{
	char m_PlateNum[16]; //号牌号码
	char m_PlateType[3]; //号牌种类，符合GA24.7要求
	char m_Time[25]; //违法时间，精确到秒
	char m_Violation[8]; //违法类型，符合GA408.1要求
	char m_Speed[4]; //车辆速度，单位：公里/小时
	char m_FilePath1[100]; //图像1的相对路径
	char m_FilePath2[100]; //图像2的相对路径
	char m_FilePath3[100]; //图像3的相对路径
	char m_FilePath4[100]; //图像4的相对路径
	char m_AviPath[100]; //违法序列相对路径
	char m_Description[151]; //违法说明
	char m_PlateColor[2]; //号牌颜色
	char m_Lane[4]; //车道号
	char redLampStartTime[20]; //红灯起始时间
	int redLampKeepTime; //红灯持续时间
	char m_Reserve[51]; //保留信息，定义为：x/y/cx/cy/lane
} Illegal_Records;

//车辆通行记录
typedef struct _tagVehicle_Records
{
	char m_PlateNum[16]; //号牌号码
	char m_PlateType[3]; //号牌种类，符合GA24.7要求
	char m_Time[25]; //经过时间，精确到秒
	char m_Speed[4]; //车辆速度，单位：公里/小时
	char m_VehicleLength[6]; //车外廊长，单位：厘米
	char m_PlateColor[2]; //号牌颜色
	char m_FilePath1[100]; //图像1的相对路径
	char m_Lane[4]; //车道号
	char m_Reserve[51]; //保留信息，定义为：x/y/cx/cy/lane
} Vehicle_Records;

//事件报警
typedef struct _tagEvent_Alarm
{
	char m_DeviceID[12]; //设备编号
	char m_Time[15]; //报警时间
	char m_AlarmType[3]; //报警类型
	char m_FilePath1[61]; //图像1的相对路径
	char m_FilePath2[61]; //图像2的相对路径
	char m_Description[151]; //描述
} Event_Alarm;

//设备状态
typedef struct _tagDevice_Status
{
	char m_DeviceID[12]; //设备编号
	char m_CameraStatus[2]; //摄像机状态
	char m_ErrorStatus[2]; //设备故障状态
} Device_Status;

//===========================================外部接口参数============================//


/***************************************MQ设备管理消息 ***********************************/
//FTP文件路径结构体
typedef struct _tagFTP_FILEPATH
{
	int type; //仅升级用。 1--升级arm， 2--升级dsp， 3--升级fpga， 4--升级单片机1, 5--升级单片机2
	char m_strFileURL[255]; //FTP中文件相对路径
	unsigned long file_size; //ftp上的文件大小. 以byte为单位.
} FTP_FILEPATH;

typedef enum
{
	DISK_BLOCK_STATUS_NOT_DETECTED = 0, // 探测不到
	DISK_BLOCK_STATUS_MOUNT_FAILED = 1, // mount 失败
	DISK_BLOCK_STATUS_NORMAL = 2, //正常
	DISK_BLOCK_STATUS_IS_CLEANING = 3, // 硬盘正在清理
	DISK_BLOCK_STATUS_IS_FORMATTING = 4
// 硬盘正在格式化

} DISK_BLOCK_STATUS;

typedef struct DISK_STATUS
{
	DWORD m_diskBlockSize;
	WORD m_diskBlockUsed; //使用率，显示需要加%
	DISK_BLOCK_STATUS m_diskBlockStatus; //（0，探测不到; 1mount 失败 ; 2正常;3,正在清理，4，正在格式化

} DISK_STATUS;

//硬盘信息
typedef struct DISK_INFO
{
	WORD m_curNum; //当前使用的分区盘
	WORD m_IsPartition; // 是否正在分区
	DISK_STATUS diskStatus[4];

} DISK_INFO;

//====================================查看设备基本信息==========================//
//返回设备基本信息结构体
typedef struct _tagDevice_Information
{
	char ver_boa[64]; //嵌入式软件boa版本
	char ver_dsp[64]; //DSP软件版本
	char ver_mcfw[64]; //嵌入式软件mcfw模块版本
	char ver_vpss[64]; //嵌入式软件vpss模块版本
	char ver_sysserver[64]; //FPGA软件版本
	char ver_vd[64]; //嵌入式Vd 版本
	char mcu_ver[64]; //单片机版本号

	DISK_INFO diskInfo; //硬盘状态
	WORD m_wLimitSpeed; //路段限速，单位：公里/小时

	char hw_uuid[15]; //设备唯一编号,12位
	char hw_ver[12]; //设备硬件版本号

	char ftp_sta_pass_car; //ftp 0正常  1无连接 2 连接异常 3 ftp满
	char ftp_sta_illegal; //ftp 0正常  1无连接 2 连接异常 3 ftp满
	char ftp_sta_h264; //ftp 0正常  1无连接 2 连接异常 3 ftp满

	char uart_status; // uart 0正常1 打开失败2.无从板消息

	char tem_status; //温度传感器 0正常 1打开失败
	char gpio_status; //gpio  0正常 1 打开失败
	char mcu_status; //单片机状态0:正常 1重启 2:无连接
	char fan_status; //0--close, 1--open

	int preset_index;	//	预置位标号对应存取	====沈自晓 2014.76.26

} Device_Information;

//================================＝查看设备状态=================================//
//返回设备状态结构体
typedef struct _tagDevice_Status_Return
{
	WORD m_CPU; //CPU占用百分比
	WORD m_Memory; //内存占用百分比
	WORD m_DSP[8]; //DSP占用百分比
	int m_Temperature; //板卡温度，单位℃
	LONG m_DiskFree;//单位为KB
	u64 m_EpTimes;//单位为s
	char m_strDSPStatus[255]; //DSP状态文本
} Device_Status_Return;

//================================＝查看红灯状态=================================//
//返回红灯状态结构体
typedef struct _tagRedLamp_Status
{
	SHORT m_nLeftStatus; //左转方向状态。参见枚举ENUM_LAMP_STATUS
	SHORT m_nThroughStatus; //直转方向状态
	SHORT m_nRightStatus; //右转方向状态
	SHORT m_nTurnStatus; //调头方向状态
} RedLamp_Status;

//======================================设备报警上传==============================//

//设备报警结构体
typedef struct _tagDevice_Alarm
{
	TIME m_tmTime; //报警发生时间
	WORD m_wErrorType; //故障类型
	char m_strErrorDesc[100]; //故障描述
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

//==================================查看设备历史报警记录============================//
//查看设备历史报警记录命令结构体
typedef struct _tagAlarm_TimeSet
{
	TIME m_tmBegin; //开始时间
	TIME m_tmEnd; //结束时间
} Alarm_TimeSet;

//返回设备历史报警记录结构体
typedef struct _tagAlarm_Record
{
	TIME m_tmTime; //报警发生时间
	WORD m_wErrorType; //故障类型
	char strErrorDesc[100]; //描述
} Alarm_Record;

//===================================设备工作日志上传=======================//
//返回设备工作日志结构体
typedef struct _tagDevice_Worklog
{
	char m_strPrint[255]; //输出一行或多行
} Device_Worklog;

//====================================设备内部文件传输=============================//
typedef struct _tagNORMAL_FILE_INFO
{
	FTP_FILEPATH m_cFTPFilePath; // FTP的文件路径
	char m_strDeviceFilePath[255]; // 设备内的文件路径

} NORMAL_FILE_INFO;

//===================================设备数据清理======================//
//设备数据清理命令结构体
typedef struct _tagDevice_DataClear
{
	WORD m_wDataType; //清理数据类型
	TIME m_wTimeClearBefore; //清理截止时间
	BOOL m_bOnlyDeletePictures; //是否只清理图片
} Device_DataClear;

//=================================查看历史记录=======================//
//查看历史记录命令结构体
typedef struct _tagHistory_TimeSet
{
	TIME m_tmBegin; //历史记录开始时间
	TIME m_tmEnd; //历史记录结束时间
} History_TimeSet;

//================================上传记录图片===============================//
//上传记录图片请求命令结构体
typedef struct _tagSendPicture_Request
{
	char m_strRecID[16]; //统一记录编号，15位
} SendPicture_Request;

//================================抓拍车辆指定位置图片，返回===================//
typedef struct _tagSCapture_Vehicle_Spec
{
	char m_strFileStart[255]; // 开始识别位置的车辆图片
	char m_strFileStop[255]; // 结束识别位置的车辆图片
} SCapture_Vehicle_Spec;

//================================NTP对时消息，返回====================//
typedef struct _tagNTP_Device_Time
{
	TIME m_tDeviceTime; // 设备时间
	char m_Reserve[50]; // 扩展字段
} NTP_Device_Time;
/***************************************MQ在线广播 **************************************/

//查找在线设备返回消息
typedef struct _tagOnline_Device
{
	BYTE ip[4]; //
	char m_strDeviceID[12]; //设备编号  数据10位
	char m_strSpotName[100]; //安装地点名称
	char m_strDirection[8]; //行驶方向
	char m_strFTP_URL[256]; //FTP服务器的URL
	char m_strVerion[64]; //嵌入式软件版本
	//Protocol_Version m_cVerion; //协议版本
} Online_Device;

/***************************************广播消息和UDP消息 ********************************/
enum
{
	SPORT_ID = 0, //地点编号
	DEV_ID = 1, //设备编号
	YEAR_MONTH = 2, //年/月
	DAY = 4, //日
	EVENT_NAME = 5, //事件类型
	HOUR = 6, //时
	FACTORY_NAME = 7, //厂家名称。

	NONE = 9, //不使用
	LEVEL_NUM = 10
//最多层数
};

//数据包头结构体
typedef struct _tagMSG_HEADER
{
	WORD m_StartID; //包标识，固定为0x00 68
	WORD m_MsgType; //消息类型，如果该值为6，则是回复出厂设置命令
	BYTE m_IsRequest; //是请求/应答消息
	BYTE m_NeedReply; //对于请求消息，是否需要应答
	BYTE m_Result; //对于应答消息，执行成功/失败
	BYTE m_extent1[9]; //备用 对齐
	WORD m_ContenLength; //消息内容长度
	BYTE m_Content[256]; //消息内容，根据不同消息类型，定义不同
	U32 sum_check; //校验和,从第一个字节开始到最后一个字节;
} MSG_HEADER;

typedef struct _tagFTP_URL_Level
{
	int levelNum; //层数
	int urlLevel[LEVEL_NUM]; //数组索引代表所在层
} FTP_URL_Level;

//网络参数结构体
typedef struct _tagNET_PARAM
{
	BYTE m_IP[4]; //IP地址，从前向后排列
	BYTE m_MASK[4]; //掩码，从前向后排列
	BYTE m_GATEWAY[4]; //网关，从前向后排列
	char m_DeviceID[12]; //设备编号，10位，行业编号
	BYTE m_MQ_IP[4]; //MQ服务器的IP，从前向后排列
	WORD m_MQ_PORT; //MQ服务器 的TCP端口
	BYTE m_btMac[6]; //MAC地址
	TYPE_FTP_CONFIG_PARAM ftp_param_conf; //  FTP服务器参数(其他文件上传下载)
	FTP_URL_Level ftp_url_level;
} NET_PARAM;

//数据包头结构体
typedef struct _tagSET_NET_PARAM
{
	BYTE m_IP[4]; //原IP地址
	NET_PARAM m_NetParam; //消息内容，根据不同消息类型，定义不同
} SET_NET_PARAM;

/***************************************存储设备清理 **************************************/

typedef enum
{
	DISK_CLEAR = 1, //清理硬盘
	DISK_FORMAT = 2, //格式化硬盘
	DISK_FDISK = 3, //重新分区并格式化硬盘
	DISK_MAX = 4
//< File manager command number.
} DISK_Cmd;

typedef struct _tagDisk_Clear
{
	WORD m_cmd;
	WORD m_partition; //0-3 位代表要格式化或清理的分区
} Disk_Clear;

/***************************************违法信息描述********************************/
//typedef struct _tagVIOLATE_DESCRIPTION
//{
//	char m_cUseFlag; //启用标志
//	char m_strOverSpeed[50]; //超速描述
//	char m_strRunRedLight[50]; //闯红灯描述
//	char m_strGoAgaistTraffic[50]; //逆行描述
//	char m_strNotByRoad[50]; //不按道描述
//	char m_strOverLine[50]; //压实线描述
//	char m_strIllegalChangeLane[50]; //违法变道
//	char m_strIllegalCrossLane[50]; //违法越线
//	char m_strIllegalStop[50]; //违法停车描述
//	char m_strOverSafeIsland[50]; //压安全岛描述
//	char m_strIllOccupySpeciallane[50]; // 非法占用专用车道
//	char m_strIllOccupyNon_motorized[50]; //非法占用非机动车道
//	//	char isUploadOverflowImg;       //是否上传溢出图片
//	char m_strForcedIntoCrossroad[50]; //拥堵时强行驶入路口违法描述
//	char m_strIllegalTravelRestriction[50]; //限时限行违法描述
//	char m_strReserve[49]; //备用
//} VIOLATE_DESCRIPTION;

/***************************************图片叠加信息设置********************************/
//typedef struct
//{
//	unsigned char color_r; //颜色R值
//	unsigned char color_g; //颜色G值
//	unsigned char color_b; //颜色B值
//
//} type_info_color;

//typedef struct _tagOVERLY_INFO
//{
//	//	WORD	m_wStartLine;				//起始行
//	//	WORD	m_wColor;					//颜色	1、黄色2、蓝色 3、红色
//
//	unsigned short start_x; //违法叠加起始坐标X;
//	unsigned short start_y; //违法叠加起始坐标Y;
//	type_info_color color; //违法叠加颜色信息.
//
//	VIOLATE_DESCRIPTION m_cVioateDesc; //违法表述			//11.违法描述
//	WORD m_wTimeFlag; //是否描写车辆时间 //1.时间
//	WORD m_wRedLightStartTimeFlag; //是否描写红灯起始时间2.
//	WORD m_wRedLightKeepTimeFlag; //是否描写红灯保持时间3.
//	WORD m_wSpotNameFlag; //是否描写地点名称 //4.地点名称
//	WORD m_wDeviceNumFlag; //是否描写设备编号//5.设备编号
//	WORD m_wDirectionFlag; //是否描写行驶方向//6.方向
//	WORD m_wLaneNumFlag; //是否描写车道号	//7.车道
//	WORD m_wLaneDirectionFlag; //是否描写车道方向	//8.车道方向
//	WORD m_wPlateNumFlag; //是否描写车牌号码//9.车牌号码
//	WORD m_wPlateColorFlag; //是否描写车牌颜色//10.车牌号码
//	WORD m_wPicNum; //叠加几幅图片，如果是1,则第二三张只叠加时间，3-3副图片全部叠加
//	WORD m_wSpeedFlag; //是否描写车辆速度//11.车辆速度
//	WORD m_wPlateTypeFlag; // 是否叠加号牌类型
//
//} OVERLY_INFO;

/***************************************卡口叠加信息设置********************************/

//typedef struct _tagTRFCOVERLY_INFO
//{
//	char isOverlayFlag; //是否叠加过车信息
//	unsigned short start_x; //过车叠加起始坐标X;
//	unsigned short start_y; //过车叠加起始坐标Y;
//	type_info_color color; //过车叠加颜色信息.
//
//	short m_wTimeFlag; //是否描写车辆时间 //1.时间
//	short m_wSpotNameFlag; //是否描写地点名称 //4.地点名称
//	short m_wDeviceNumFlag; //是否描写设备编号//5.设备编号
//	short m_wDirectionFlag; //是否描写行驶方向//6.方向
//	short m_wLaneNumFlag; //是否描写车道号	//7.车道
//	short m_wLaneDirectionFlag; //是否描写车道方向	//8.车道方向
//	short m_wPlateNumFlag; //是否描写车牌号码//9.车牌号码
//	short m_wPlateColorFlag; //是否描写车牌颜色//10.车牌号码
//	short m_wPlateTypeFlag; //是否叠加车辆类型
//	short m_wSpeedFlag; //是否描写车辆速度//12.车辆速度
//
//} TRFCOVERLY_INFO;

/***************************************实时视频********************************/
// UDP分包发送视频帧的定义。包头共8字节。
// 目前视频为MJPEG格式，帧为JPEG图像。

typedef struct _tagVIDEOPACKAGE
{
	DWORD m_dwTimeStamp; // 时间戳，使用9k HZ时钟。即1秒值增加9000。
	WORD m_wSeq; // 顺序号。值0~65535。
	BYTE m_btTag; // 标志位。
	// 第8位：开始标志。表示发送新帧。
	// 第7位：结束标志。表示当前帧发送结束。
	// 第6，5位：版本号。(保留，设为0)
	// 第4,3,2,1位：(保留，设为0)
	BYTE m_btPaloadType; // 负荷类型 (保留，设为0)
} VIDEOPACKAGE;

////====================================H264查询上传FTP接口=============================//
//typedef struct _tagH264_TO_FTP
//{
//	char m_time_start[20]; // 时间起点 示例：201206110900
//	char m_time_end[20]; // 时间终点
//	int m_nu; //满足要求的H264文件数
//	char m_H264_files[H264_MAX][100]; //上传到FTP中的H264文件名
//	char m_time_part[H264_MAX][30];   //查询到的时间段
//
//} H264_TO_FTP;

//====================================H264查询接口=============================//
typedef struct _tagH264DOWN
{
	char m_time_start[20]; // 时间起点 示例：201206110900
	char m_time_end[20]; // 时间终点
} H264_DOWN;

//====================================H264上传FTP接口=============================//
typedef struct _tagH264_TO_FTP
{
	int index; //序号
	int total; //H264总个数
	char timeStart[15]; //开始时间
	char timeEnd[15]; //结束时间
	int timeLength; //持续时间长度
	char path[100]; //H264在FTP中的路径
	char device_id[20]; //设备ID号
	int partition; //上传成功标志，成功：1，不成功：-1
} H264_UP;

//#########################################//#########################################// add by xsh

#pragma pack(pop, PACK1)

#endif /* EP_TYPE_H_ */
