
#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


//#include "videoAnalysisLink_parm.h"
#include <../../../ipnc_mcfw/mcfw/interfaces/link_api/videoAnalysisLink_parm.h>

#include "arm_config.h"
#include "camera_config.h"
//#include "config_epcs.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/config_vdcs.h"

#define FAIL -1
#define SUCCESS 0
// net config
typedef struct _NET_CONFIG
{
    struct in_addr ip;
    struct in_addr gateway;
    struct in_addr netmask;
    char mac[60];
} net_config;

enum IRIS_TYPE
{
	SHIMA20_F1_8 = 1,			//适马20mm-F1.8, F1.8 ~ F22
	SHIMA24_F1_8 = 2,			//适马24mm-F1.8, F1.8 ~ F22
	SHIMA18TO35_F1_8 = 3,		//适马18~35mm-F1.8, F1.8 ~ F16
};

enum IRIS_SIZE
{
	FULL_OPEN,
	F1_4,
	F1_6,
	F1_8,
	F2_0,
	F2_2,
	F2_5,
	F2_8,
	F3_2,
	F3_5,
	F4_0,
	F4_5,
	F5_0,
	F5_6,
	F6_3,
	F7_1,
	F8,
	F9,
	F10,
	F11,
	F13,
	F14,
	F16,
	F18,
	F20,
	F22
};

typedef struct Camera_info
{
	int iris_type; // 镜头型号 1:适马24mm-F1.8；2：适马20mm-F1.8；3：适马18~35mm-F1.8
	int iris_size; //光圈大小 对应枚举IRIS_SIZE内的定义；（范围多定义一些，从F1.4开始，F1.4、F1.6、F1.8也需要列进去）
	int focus_type; //0:停止；1：近；2：远
}Camera_info;

typedef struct _FAN_CTRL
{
	int UpperTemp; // upper than it, fan on
	int LowerTemp;	// lower than it, fan off
}Fan_Ctrl;

typedef struct _VER_DETAIL
{
	char HdVer[20];		// Hardware Version
	char FPGAVer[20];		///< FPGA version
	char McuVersion[20];	///< mcu version
	char SysServerVer[50]; 	///< sys_server version
	char serialnum[50];   ///< serial number
	char devicetype[10];  ///< device type
}Version_Detail;

/* add by dsl, 2013-8-16 */
enum {
	DEV_NONE = 0,
	DEV_FLASH,
	DEV_REDLIGHT_DETECT,
	DEV_MD44,
	DEV_MAX
};

typedef struct {
	int dev_type; //连接的设备类型，0：不连接 1：频闪 2：红灯检测
	int baudrate; // 波特率
	char parity; // 校验位， even, odd, none
	char databit; // 数据位
	char stopbit; // 停止位，1:1位, 2:2位, 3:1.5位
} Serial_config_t;

typedef struct {
	unsigned char mode; // 输入输出配置接口， 0:不配置；1：红灯检测器；2：黄灯检测器；3：报警检测器
	unsigned char trigger_type; // 0：上升沿触发；1：下降沿触发
	unsigned char direction; // bit0:左转；bit1：直行；bit2：右转；bit3：掉头(mode为红灯检测器时该位有效)
} IO_config_t;
/* end add, dsl, 2013-8-16 */

/* following add by dsl, 2013-8-21 */
typedef struct {
	unsigned char addr;		//目的设备地址
	unsigned short sn;		//消息序列号
	unsigned char cmd;		//控制命令
	unsigned short len;		//data数据段字节数
	unsigned char data;		//数据
} Flash_control_t;
/* end added, dsl, 2013-8-21 */

/*	following add by zdy, 2013-08-28	*/
typedef struct {
	///< Temperature
	float FpgaTemp;		///< FPGA Temperature, i2c3 0x48
	float Temp8147;		///< 8147 Temperature, i2c2 
	float DriverTemp;	///< Driver Temperature, i2c3 0x4a
	int McuTemp;		///< MCU temp
	float CCDTemp;		///< CCD temp, i2c-3 0x48
	float Temp6678;		///< 6678 temp

	///< MCU current and voltage
	float McuCurrent;
	float McuVoltage;
	///< CCD current and voltage, i2c-2 0x6f
	float CCDCurrent;
	float CCDVoltage;
	///< FPGA current and voltage, i2c-2 0x67
	float FPGACurrent;
	float FPGAVoltage;
	///< 8147 current and voltage, i2c-1 0x67
	float Current8147;
	float Voltage8147;
	///< 6678 current and voltage, mcu
	float Current6678;
	float Voltage6678;
} Temp_Detail_t;
/*	end added, zdy, 2013-08-28*/

/* add by dsl, 2013-9-18 */

typedef struct {
	unsigned char dev_id;/*红灯检测器的设备id*/
	unsigned char light_status[2]; /*有效位：light_status[0]:bit0:3 light_status[1]:bit0:7*/
} signal_status;

typedef void (*CallBack)(signal_status *);

/* end added, dsl, 2013-9-18 */


/* add by dsl, 2013-12-16 */
typedef struct
{
	unsigned char enable; //启用频闪灯 0:未使用 1:使用
	unsigned char polarity; //极性： 0:负 1:正
	unsigned int delay_time; //频闪灯延迟：0~80000us
} strobe_t;
/* end added, dsl, 2013-12-16 */

typedef struct
{
	unsigned char addr;			//设备地址，0x00~0xff
	unsigned char code;			//读：0x13; 写：0x10
	unsigned char regAddr[2];	//起始寄存器地址，高字节在前
	unsigned char regNum[2];	//寄存器个数，高字节在前
	unsigned char length;		//数据长度，为寄存器个数*2
	unsigned char data[8];		//寄存器个数*2字节，每个数据高字节在前
	unsigned char crc;			//校验和
} MD44_control_t;


#ifdef  __cplusplus
extern "C"
{
#endif

int Init_Com_To_File();
void Deinit_Com_To_File();

net_config *get_net_config();
int set_net_config(net_config *net);

H264_config *get_h264_config();
int set_h264_config(H264_config *h264cfg);

//Camera_config *get_camera_config();
int set_camera_config(Camera_config *camcfg);
//
//NTP_CONFIG_PARAM *get_ntp_config();
//int set_ntp_config(NTP_CONFIG_PARAM *ntp_config);
//
//Camera_info *get_lens_ctrl();
//int set_lens_ctrl(Camera_info *camera_info);

int power_down();

int get_prepos_station(void);

Version_Detail *get_version_detail();

//int get_fan_status(); // if (status==0), fan off; ele if(status==1), fan on
//Fan_Ctrl *get_fan_ctrl();
//int set_fan_ctrl(Fan_Ctrl *fan_ctrl);

/* add by dsl, 2013-8-19 */
//int get_serial_config(Serial_config_t conf[]);
//int set_serial_config(Serial_config_t conf[]);
//
//int get_in_config(IO_config_t iconf[]);
//int set_in_config(IO_config_t iconf[]);
//int get_out_config(IO_config_t oconf[]);
//int set_out_config(IO_config_t oconf[]);

/* end add, dsl, 2013-8-19 */

/* following add by dsl, 2013-8-21 */
int flash_control(Flash_control_t *cmd);
/*end added, dsl, 2013-8-21 */

/*	Following add by zdy, 2013-08-23	*/
//int mcu_upgrade(const char *path); // 单片机板升级
//int mcu_fpga_upgrade(const char *path);	// FPGA上的单片机升级
/*	end added, zdy, 2013-08-23	*/

/*	following add by zdy, 2013-08-28	*/
Temp_Detail_t *get_temp_detail();
/*	end added, zdy, 2013-08-28	*/

/*	following add by zdy, 2013-09-03	*/
//int harddisk_power_onoff(int id, int action); // id: 0/1 , action: 0:power off; 1:power on
/*	end added, zdy, 2013-09-03	*/


int Init_Com_To_Sys();
void Deinit_Com_To_Sys();


///< SIP, 2013-08-22
int functions();
void Init_Appro(void);
void Clean_Appro(void);
// int get_cmd_info(char *cmd, char *result);
//int get_sys_status();
//int zoom_fun(int zoom_code);
//int set_alarm(int alarm_code);

/* add by dsl, 2013-9-6 */
int set_current_time(time_t time);/*0:success, -1:fail*/ /*2013-11-8 modify arg type from long int to time_t*/
/* end added, dsl, 2013-9-6 */

/*	following add by zdy, 2013-09-22	*/
int restore_factory_set(void);  ///< factory reset
/*	end added, zdy, 2013-09-22	*/

/* add by dsl, 2013-9-18 */
//int set_signal_detect_callback(CallBack func);/*-1:开启红灯检测失败 0:开启红灯检测成功*/
/* end added, dsl, 2013-9-18 */

/* add by dsl, 2013-10-14 */
// int set_syn_info(Syn_Info * info);	/*-1: fail 0: success*/
// Syn_Info * get_syn_info();			/*-1: fail 0:success*/
/* end added, dsl, 2013-10-14 */

/* add by dsl, 2013-11-5 */
//int set_camera_brightness(Brightness_control * arg);
/* end added, dsl, 2013-11-5 */
/* add by dsl, 2013-12-16 */
int set_strobe_control(strobe_t * arg); /*返回-1:程序执行失败 返回0:程序执行成功*/
strobe_t* get_strobe_ctrl(); /*返回NULL失败，反之执行成功*/
/* end added, dsl, 2013-12-16 */

/*add by dsl, 2013-12-24 */
//int md44_control(char *buf, int len);		//成功返回0，不成功返回-1
/* end added, dsl, 2013-12-24 */

/* add by dsl, 2014-1-6 */
//int Heart();
//int stopMcuHeart();
//int openMcuHeart();
/* end added, dsl, 2014-1-6 */

#ifdef  __cplusplus
}
#endif
