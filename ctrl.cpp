/*
 * ctrl.cpp
 *
 * Created on: 2013-5-1
 * Author: shanhw
 */
#include <sstream>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <netdb.h>
#include <time.h>

#include <string.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/un.h>
#include <sys/msg.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>
#include <math.h>

#include <fcntl.h>
#include <pthread.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <errno.h>
#include <net/if.h>
#include <resolv.h>
#include <ctype.h>
#include <inttypes.h>

#include "global.h"
#include "commonfuncs.h"
#include "mq_listen.h"
#include "ctrl.h"
#include "messagefile.h"
#include "ftp.h"
#include "mq_module.h"
#include "xmlCreater.h"
#include "xmlParser.h"
//#include "pcie.h"
#include "sysserver/interface.h"

#include "storage/violation_records_process.h"
#include "storage/park_records_process.h"
#include "storage/traffic_records_process.h"
#include "storage/event_alarm_process.h"
#include "storage/traffic_flow_process.h"
#include "storage/data_process_park.h"
#include "storage/data_process_traffic.h"
#include "storage/data_process_violation.h"
#include "storage/data_process_event.h"
#include "storage/data_process_flow.h"
#include "storage/data_process.h"

#include "h264/h264_buffer.h"
#include "h264/debug.h"

#include "json/json.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>

#include <park_records_process.h>
#include "profile.h"
#include "sys/time_util.h"
#include "ve/platform/bitcom/thread.h"
#include "eplate.h"
#include "file_msg_drv.h"
#include "logger/log.h"

#include "light_ctl.h"
#include "park_status.h"

#define MAX_BUF_SIZE	1472

Device_Information device_info;
Device_Status_Return device_status_return;

int gi_platform_board = 0;
int ParkLightFlag = 0;
int gi_filllight_smart = 0;
//int flag_use_gpio10=0;//test：9.179,使用gpio10做线圈输入;9.178,使用gpio11做线圈输入;
str_light_info gstr_filllight_info;

extern FTP_FILEPATH ftp_filePath; //上传文件路径
extern FTP_FILEPATH ftp_arm_filePath; //上传arm文件
extern FTP_FILEPATH ftp_dsp_filePath; //上传dsp文件
extern FTP_FILEPATH ftp_camera_filePath; //上传camera文件
extern FTP_FILEPATH ftp_hide_filePath; //上传hide文件

extern ARM_config g_arm_config; //arm参数结构体全局变量
extern Camera_config g_camera_config;

extern int upload_arm_param; //上传arm参数
extern int upload_dsp_param; //上传dsp参数
extern int upload_camera_param; //上传camera参数
extern int upload_hide_param_file; //上传隐性参数
extern int upload_normal_file; //上传一般文件

extern int download_arm_param; //下载arm参数
extern int download_dsp_param; //下载dsp参数
extern int download_camera_param; //下载camera参数
extern int download_hide_param_file; //下载隐性参数
extern int download_normal_file; //下载一般文件


extern class_mq_producer *g_mq_park_upload;

extern  int  register_park;

extern SemHandl_t park_sem;
void *timer2ThrFxn(void *arg);
void *timer1ThrFxn(void *arg);
void * BroadcastThrFxn(void *arg);
void *LightMonitorThrFxn(void *arg);
void *ParkUploadHeartbeat(void * arg);   //add by lxd

typedef struct
{
	short header;		//头固定为‘C’
	short len;			//长度sizeof（strobe_t）
	strobe_t data;		//具体指令信息
	int cmd;		//0x01:请求  0x02:设置
	int	chk;		//校验和
}Light_switch;

typedef struct
{
	char mode[10];
	char lon[12];
	char lat[12];
	char sunrisetime[12];
	char sunsettime[12];
	char smart_flag[12];
}str_fillinlight_init;


//POWER_CTRL_Cmd_t power_ctrl_flag = EP_POWER_DEFAULT;//电源模块控制信号:



#define RADEG     ( 180.0 / M_PI )
#define sind(x)  sin((x)*DEGRAD)
#define cosd(x)  cos((x)*DEGRAD)
#define tand(x)  tan((x)*DEGRAD)

#define DEGRAD    ( M_PI / 180.0 )
#define atand( x )    (RADEG*atan(x))
#define asind(x)    (RADEG*asin(x))
#define acosd(x)    (RADEG*acos(x))
#define atan2d(y,x) (RADEG*atan2(y,x))

#define days_since_2000_Jan_0(y,m,d) \
	    (367*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530)

void sunriset( int year, int month, int day, double lon,
		               double lat, double *rise, double *set );
//year, month, date = calendar date, 1801-2099 only.
//Eastern longitude positive, Western longitude negative
//Northern latitude positive, Southern latitude negative
//The longitude value is critical in this function!
//*rise = where to store the rise time
//*set  = where to store the set  time

void sunpos( double d, double *lon, double *r );
//Computes the Sun's ecliptic longitude and distance at an instant given in d
//number of days since 2000 Jan 1
//The Sun's ecliptic latitude is not computed, it's always very near 0.

void sun_RA_dec( double d, double *RA, double *dec, double *r );
//Compute Sun's ecliptic longitude add slant

double revolution( double x );
//Reduce angle to within 0..360 degrees

double rev180( double x );
//Reduce angle to within -180..+180 degrees

double GMST0( double d );
//This function computes the Greenwich Mean Sidereal Time at 0h UT
//GMST is then the sidereal time at Greenwich at any time of the day
//GMST = (GMST0) + UT * (366.2422/365.2422)

void sunriset( int year, int month, int day, double lon, double lat, double *rise, double *set )
{
    double sr;           //sun's distance
    double sRA;          //sun's ecliptic longitude
    double sdec;         //slant of the sun
    double t;            //offset

    double d = days_since_2000_Jan_0( year, month, day ) + 0.5 - lon / 360.0;
    //Compute d of 12h local mean solar time, from 2000 Jan 1 to now
    double sidtime = revolution( GMST0(d) + 180.0 + lon );
    //Compute local sidereal time of this moment
    double altit = -35.0/60.0;
    //Compute Sun's RA + Decl at this moment
    sun_RA_dec( d, &sRA, &sdec, &sr );
    //Compute time when Sun is at south - in hours UT
    double tsouth = 12.0 - rev180(sidtime - sRA)/15.0;
    //Compute the Sun's apparent radius, degrees
    double sradius = 0.2666 / sr;
    //Do correction to upper limb, if necessary
    altit -= sradius;
    //Compute the diurnal arc that the Sun traverses to reach

    //the specified altitude altit:
    double cost = ( sind(altit) - sind(lat) * sind(sdec) ) /
	           ( cosd(lat) * cosd(sdec) );
    if ( cost >= 1.0 )
        t = 0.0;       //Sun always below altit
    else if ( cost <= -1.0 )
        t = 12.0;      //Sun always above altit
    else
        t = acosd(cost)/15.0;   //The diurnal arc, hours

    //Store rise and set times - in hours UT
    *rise = tsouth - t;
    *set  = tsouth + t;
}

void sunpos( double d, double *lon, double *r )
{
    //Compute mean elements
    double M = revolution( 356.0470 + 0.9856002585 * d ); //anomaly
    double w = 282.9404 + 4.70935E-5 * d;                 //sun's ecliptic longitude
    double e = 0.016709 - 1.151E-9 * d;                   //earth's offset

    //Compute true longitude and radius vector
    double E = M + e * RADEG * sind(M) * ( 1.0 + e * cosd(M) ); //eccentric anomaly
    double x = cosd(E) - e;
    double y = sqrt( 1.0 - e*e ) * sind(E); //orbit coordinate
    *r = sqrt( x*x + y*y );                 //Solar distance
    double v = atan2d( y, x );              //True anomaly
    *lon = v + w;                           //True solar longitude
    if ( *lon >= 360.0 )
        *lon -= 360.0;                      //Make it 0..360 degrees
}

void sun_RA_dec( double d, double *RA, double *dec, double *r )
{
    double lon;
    //Compute Sun's ecliptical coordinates
    sunpos( d, &lon, r );
    //Compute ecliptic rectangular coordinates (z=0)
    double x = *r * cosd(lon);
    double y = *r * sind(lon);
    /* Compute obliquity of ecliptic (inclination of Earth's axis) */
    double obl_ecl = 23.4393 - 3.563E-7 * d;
    /* Convert to equatorial rectangular coordinates - x is unchanged */
    double z = y * sind(obl_ecl);
    y = y * cosd(obl_ecl);
    /* Convert to spherical coordinates */
    *RA = atan2d( y, x );
    *dec = atan2d( z, sqrt(x*x + y*y) );
}

double revolution( double x )
{
    return( x - 360.0 * floor( x / 360 ) );
}

double rev180( double x )
{
    return( x - 360.0 * floor( x / 360 + 0.5 ) );
}

double GMST0( double d )
{
    //Sidtime at 0h UT = L (Sun's mean longitude) + 180.0 deg
    //L = M + w, as defined in sunpos().
    //Any decent C compiler will add the constants at compile time
    double sidtim0 = revolution( ( 180.0 + 356.0470 + 282.9404 ) +
	                          ( 0.9856002585 + 4.70935E-5 ) * d );
    return sidtim0;
}

/**
Function:    getArmCpuLoad()
Description: 函数获得CPU负载情况
Input:
Output:	int *procLoad		- 进程的CPU负载
int *cpuLoad		- CPU总负载
Return:    int  函数调用是否成功       0 成功，－1  失败
 **/
int getArmCpuLoad(int *procLoad, short *cpuLoad)
{
	static unsigned long prevIdle = 0;
	static unsigned long prevTotal = 0;
	static unsigned long prevProc = 0;
	int cpuLoadFound = 0;
	unsigned long user, nice, sys, idle, total, proc;
	unsigned long uTime, sTime, cuTime, csTime;
	unsigned long deltaTotal, deltaIdle, deltaProc;
	char textBuf[5];
	FILE *fptr = NULL;

	// Read the overall system information
	if (!(fptr = fopen("/proc/stat", "r")))
	{
		printf("err open /proc/stat\n");
		return FAILURE;
	}

	// Scan the file line by line
	while (fscanf(fptr, "%3s %lu %lu %lu %lu %*[^\n]", textBuf, &user, &nice,
				&sys, &idle) != EOF)
	{
		if (strcmp(textBuf, "cpu") == 0)
		{
			cpuLoadFound = 1;
			break;
		}
	}

	if (fclose(fptr) != 0)
	{
		return FAILURE;
	}

	if (!cpuLoadFound)
	{
		return FAILURE;
	}

	// Read the current process information
	if (!(fptr = fopen("/proc/self/stat", "r")))
	{
		printf("err open /proc/self/stat\n");
		return FAILURE;

	}

	if (fscanf(fptr, "%*d %*s %*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %lu "
				"%lu %lu %lu", &uTime, &sTime, &cuTime, &csTime) != 4)
	{
		DEBUG("Failed to get process load information.\n");
		fclose(fptr);
		return FAILURE;
	}

	if (fclose(fptr) != 0)
	{
		return FAILURE;
	}

	total = user + nice + sys + idle;
	proc = uTime + sTime + cuTime + csTime;

	// Check if this is the first time, if so init the prev values
	if (prevIdle == 0 && prevTotal == 0 && prevProc == 0)
	{
		prevIdle = idle;
		prevTotal = total;
		prevProc = proc;
		return SUCCESS;
	}

	deltaIdle = idle - prevIdle;
	deltaTotal = total - prevTotal;
	deltaProc = proc - prevProc;

	prevIdle = idle;
	prevTotal = total;
	prevProc = proc;

	*cpuLoad = 100 - deltaIdle * 100 / deltaTotal;
	*procLoad = deltaProc * 100 / deltaTotal;

	return SUCCESS;
}

/***********************************************
Function:           	getFilesystemFree()
Description:         获取当前目录的可用空间
Input:                 目录路径
Output:			无

Return:               空闲空间的字节数

 ***********************************************/
unsigned long int getFilesystemFree(const char *pathname)
{
	int ret = 0;
	struct statfs disk_statfs;

	ret = statfs(pathname, &disk_statfs);
	if (ret < 0)
	{
		return ret;
	}
	return (4 * disk_statfs.f_bavail);
}

/***********************************************
Function:           	ParseCommands()
Description:           该函数用于解析命令
Input:                 命令字
Output:			    无
Return:                无
 ***********************************************/
int ParseCommands(char * command)
{
	time_t timep; //实时时钟时间
	struct tm *p;
	//int hour,min,sec;
	time(&timep);
	p = gmtime(&timep);
	int procLoad;

	static char last_cmd[100];
	if (strlen(command) == 1 && command[0] == 38)
	{
		strcpy(command, last_cmd);
	}
	else
	{
		strcpy(last_cmd, command);
	}

	if (!strcasecmp(command, "quit"))
	{
		printf("in ParseCommands : quit \n");
		printf("for quit command\n");

		sleep(3);

		//write_err_log_noflash("exit for quit command\n");

		exit(0); //quit命令

	}
	if (!strcasecmp(command, "off_ftp"))
	{
	}
	else if (!strcasecmp(command, "print"))
	{
		printf("++++++++++++++++++++++\n");
		//IP,MAC,gw
		printf("\nlocal IP is %d.%d.%d.%d netmask %d.%d.%d.%d \n",
				g_set_net_param.m_NetParam.m_IP[0],
				g_set_net_param.m_NetParam.m_IP[1],
				g_set_net_param.m_NetParam.m_IP[2],
				g_set_net_param.m_NetParam.m_IP[3],
				g_set_net_param.m_NetParam.m_MASK[0],
				g_set_net_param.m_NetParam.m_MASK[1],
				g_set_net_param.m_NetParam.m_MASK[2],
				g_set_net_param.m_NetParam.m_MASK[3]);
		//		printf("eth0 hw ether %s \n", MAC_address);
		printf("route add default gw %d.%d.%d.%d \n",
				g_set_net_param.m_NetParam.m_GATEWAY[0],
				g_set_net_param.m_NetParam.m_GATEWAY[1],
				g_set_net_param.m_NetParam.m_GATEWAY[2],
				g_set_net_param.m_NetParam.m_GATEWAY[3]);
		//mq,ftp
		printf("MQ IP is %d.%d.%d.%d \n",
				g_set_net_param.m_NetParam.m_MQ_IP[0],
				g_set_net_param.m_NetParam.m_MQ_IP[1],
				g_set_net_param.m_NetParam.m_MQ_IP[2],
				g_set_net_param.m_NetParam.m_MQ_IP[3]);
		//		printf("FTP IP is %d.%d.%d.%d \n\n", param_basic.m_cFTP_Param.m_IP[0], param_basic.m_cFTP_Param.m_IP[1], param_basic.m_cFTP_Param.m_IP[2], param_basic.m_cFTP_Param.m_IP[3]);


		printf("+++++++++++++++++++++++++\n");
		printf("[ftp配置] ip:%d.%d.%d.%d:%d, user:%s, passwd:%s\n",
				g_set_net_param.m_NetParam.ftp_param_conf.ip[0],
				g_set_net_param.m_NetParam.ftp_param_conf.ip[1],
				g_set_net_param.m_NetParam.ftp_param_conf.ip[2],
				g_set_net_param.m_NetParam.ftp_param_conf.ip[3],
				g_set_net_param.m_NetParam.ftp_param_conf.port,
				g_set_net_param.m_NetParam.ftp_param_conf.user,
				g_set_net_param.m_NetParam.ftp_param_conf.passwd);
		printf("[过车ftp ] ip:%d.%d.%d.%d:%d, user:%s, passwd:%s\n",
				g_arm_config.basic_param.ftp_param_pass_car.ip[0],
				g_arm_config.basic_param.ftp_param_pass_car.ip[1],
				g_arm_config.basic_param.ftp_param_pass_car.ip[2],
				g_arm_config.basic_param.ftp_param_pass_car.ip[3],
				g_arm_config.basic_param.ftp_param_pass_car.port,
				g_arm_config.basic_param.ftp_param_pass_car.user,
				g_arm_config.basic_param.ftp_param_pass_car.passwd);
		printf("[违法ftp ] ip:%d.%d.%d.%d:%d,user:%s, passwd:%s\n",
				g_arm_config.basic_param.ftp_param_illegal.ip[0],
				g_arm_config.basic_param.ftp_param_illegal.ip[1],
				g_arm_config.basic_param.ftp_param_illegal.ip[2],
				g_arm_config.basic_param.ftp_param_illegal.ip[3],
				g_arm_config.basic_param.ftp_param_illegal.port,
				g_arm_config.basic_param.ftp_param_illegal.user,
				g_arm_config.basic_param.ftp_param_illegal.passwd);
		printf("[h264 ftp ] ip:%d.%d.%d.%d:%d, user:%s, passwd:%s\n",
				g_arm_config.basic_param.ftp_param_h264.ip[0],
				g_arm_config.basic_param.ftp_param_h264.ip[1],
				g_arm_config.basic_param.ftp_param_h264.ip[2],
				g_arm_config.basic_param.ftp_param_h264.ip[3],
				g_arm_config.basic_param.ftp_param_h264.port,
				g_arm_config.basic_param.ftp_param_h264.user,
				g_arm_config.basic_param.ftp_param_h264.passwd);
		printf("[ntp] enable:%d,  ip:%d.%d.%d.%d, distance:%d \n",
				g_arm_config.basic_param.ntp_config_param.useNTP,
				g_arm_config.basic_param.ntp_config_param.NTP_server_ip[0],
				g_arm_config.basic_param.ntp_config_param.NTP_server_ip[1],
				g_arm_config.basic_param.ntp_config_param.NTP_server_ip[2],
				g_arm_config.basic_param.ntp_config_param.NTP_server_ip[3],
				g_arm_config.basic_param.ntp_config_param.NTP_distance);

		for (int i = 0; i < 2; i++)
		{
			printf("-----------H264 channel %d: -------------\n", i);
			printf("\t switch: %d,  cast:%d \n",
					g_arm_config.h264_config.h264_channel[i].h264_on,
					g_arm_config.h264_config.h264_channel[i].cast);
			printf("\t ip: %d.%d.%d.%d,  port:%d \n",
					g_arm_config.h264_config.h264_channel[i].ip[0],
					g_arm_config.h264_config.h264_channel[i].ip[1],
					g_arm_config.h264_config.h264_channel[i].ip[2],
					g_arm_config.h264_config.h264_channel[i].ip[3],
					g_arm_config.h264_config.h264_channel[i].port);
			printf("\t fps: %d, rate:%d, resolution:%d X %d \n",
					g_arm_config.h264_config.h264_channel[i].fps,
					g_arm_config.h264_config.h264_channel[i].rate,
					g_arm_config.h264_config.h264_channel[i].width,
					g_arm_config.h264_config.h264_channel[i].height);

			printf("\t color: %d,%d,%d \n",
					g_arm_config.h264_config.h264_channel[i].osd_info.color.r,
					g_arm_config.h264_config.h264_channel[i].osd_info.color.g,
					g_arm_config.h264_config.h264_channel[i].osd_info.color.b);
			for (int j = 0; j < 8; j++)
			{
				printf("======  osd_item %d ========\n", j);
				printf(
						"\tswitch_on:%d,  position:(%d,%d), istime:%d, content:%s\n",
						g_arm_config.h264_config.h264_channel[i].osd_info.osd_item[j].switch_on,
						g_arm_config.h264_config.h264_channel[i].osd_info.osd_item[j].x,
						g_arm_config.h264_config.h264_channel[i].osd_info.osd_item[j].y,
						g_arm_config.h264_config.h264_channel[i].osd_info.osd_item[j].is_time,
						g_arm_config.h264_config.h264_channel[i].osd_info.osd_item[j].content);
			}
			printf("-------------------------------------\n");
		}
	}
	else if (!strcasecmp(command, "TMP_r"))
	{
		Temp_Detail_t *tmp;
		tmp = get_temp_detail();
		DEBUG("8147 温度:%f\n",
				tmp->Temp8147);

	}
	else if (!strcasecmp(command, "printcpu"))
	{
		if (getArmCpuLoad(&procLoad, (short *) &device_status_return.m_CPU)
				!= FAILURE)
		{
			printf("ARM: %d%%\n", device_status_return.m_CPU); //procLoad);
		}
		else
		{
			DEBUG("Failed to get ARM CPU load\n");
		}

		printf("DSP:%d%%\n", device_status_return.m_DSP[0]);

	}
	else if (!strcasecmp(command, "sysserver"))
	{
		printf("sysserver test command: \n"
				"\treboot\n"
				"\tget_net_config\n"
				"\tset_net_config\n"
				"\tget_h264_config\n"
				"\tset_h264_config\n"
				"");
	}
	else if (!strcasecmp(command, "reboot"))
	{

		power_down();
		//write_err_log_noflash("cmd: input reboot\n");
		printf("input reboot\n");

	}
#ifdef PLATEFORM_ARM
	else if (!strcasecmp(command, "set_net_config"))
	{

		net_config net;

		inet_aton("172.16.5.199", &net.ip);
		inet_aton("172.16.5.129", &net.gateway);
		inet_aton("255.255.255.0", &net.netmask);

		int ret = set_net_config(&net);
		DEBUG("cmd %s excuted, return:%d", command, ret);
	}
#if 0        //del by lxd  2014.1.13
	else if (!strcasecmp(command, "get_net_config"))
	{
		net_config *net_c;

		net_c = get_net_config();

		printf("ip:%s\n ", inet_ntoa(net_c->ip));
		printf("mask:%s\n ", inet_ntoa(net_c->netmask));
		printf("gateway:%s\n ", inet_ntoa(net_c->gateway));
		printf("mac:%s\n ", net_c->mac);

	}
#endif
	else if (!strcasecmp(command, "get_h264_config"))
	{
		H264_config *h264_config;

		h264_config = get_h264_config();
		if (h264_config != NULL)
		{
			printf("channel 0: \n \tcolor: %d,%d,%d\n ",
					h264_config->h264_channel[0].osd_info.color.r,
					h264_config->h264_channel[0].osd_info.color.g,
					h264_config->h264_channel[0].osd_info.color.b);
			printf("time enable:%d\n ",
					h264_config->h264_channel[0].osd_info.osd_item[0].is_time);
			printf("content:%s\n ",
					h264_config->h264_channel[0].osd_info.osd_item[1].content);
		}
		else
		{
			printf("h264_config is NULL\n");
		}
	}

	else if (!strcasecmp(command, "set_h264_config"))
	{
		int ret;

		printf("cast enable: %d, ip: %d.%d.%d.%d \n"
				"osd color: %d,%d,%d \n",
				g_arm_config.h264_config.h264_channel[0].cast,
				g_arm_config.h264_config.h264_channel[0].ip[0],
				g_arm_config.h264_config.h264_channel[0].ip[1],
				g_arm_config.h264_config.h264_channel[0].ip[2],
				g_arm_config.h264_config.h264_channel[0].ip[3],
				g_arm_config.h264_config.h264_channel[0].osd_info.color.r,
				g_arm_config.h264_config.h264_channel[0].osd_info.color.g,
				g_arm_config.h264_config.h264_channel[0].osd_info.color.b

			  );
		ret = set_h264_config(&g_arm_config.h264_config);
		printf("set_h264_config return %d. \n", ret);

	}
#endif
	else if (!strcasecmp(command, "get_h264_video"))
	{
		int ret;
		time_t begin, end;
		H264_Seg_Info h264_record_info;
		H264Buffer *h264Buffer;
		h264Buffer = new H264Buffer();

		end = time(NULL);
		begin = end - 10;

		ret = h264Buffer->get_h264_for_record(&h264_record_info, begin, end,
				MIN_VIDEO_INTERVAL);

		//h264的nal流信息
		printf("seg:%d\t"
				"seg1:0x%p,  len:%d\n"
				"seg2:0x%p,  len:%d\n"
				"time: %ds\n", h264_record_info.h264_seg,//h264流的分段数，通常为1段，或者2段，0表示无效
				h264_record_info.h264_buf_seg[0],//指向某一段h264流的起始地址
				h264_record_info.h264_buf_seg_size[0],//某一段h264流的长度
				h264_record_info.h264_buf_seg[1],//指向某一段h264流的起始地址
				h264_record_info.h264_buf_seg_size[1],//某一段h264流的长度
				h264_record_info.seconds_record_ret//最终的违法记录对应的h264视频时间长度
			  );

		FILE *f;
		f = fopen("h264.video", "wb");
		if (f == NULL)
		{
			printf("fopen error!\n");
		}
		else
		{
			if (h264_record_info.h264_seg > 0)
				fwrite(h264_record_info.h264_buf_seg[0],
						h264_record_info.h264_buf_seg_size[0], 1, f);
			if (h264_record_info.h264_seg > 1)
			{
				fwrite(h264_record_info.h264_buf_seg[1],
						h264_record_info.h264_buf_seg_size[1], 1, f);

			}
			fclose(f);
		}

		delete h264Buffer;
	}
	else if (!strcasecmp(command, "ftp"))
	{
		printf("ftp test command: \n"
				"\tftp_open\n"
				"\tftp_close\n"
				"");
	}
	else if (!strcasecmp(command, "ftp_close"))
	{
		get_ftp_chanel(FTP_CHANNEL_CONFIG)->close_server();
		DEBUG("cmd %s excuted", command);
	}
	else if (!strcasecmp(command, "ftp_open"))
	{
		int ret = get_ftp_chanel(FTP_CHANNEL_CONFIG)->open_server();
		DEBUG("cmd %s excuted, return:%d", command, ret);
	}
	else if (!strcasecmp(command, "parse_dsp"))
	{
		VDConfigData *p_dsp_cfg = (VDConfigData *)get_dsp_cfg_pointer();

		if(4 == DEV_TYPE)
		{
			parse_xml_doc(DSP_PD_PARAM_FILE_PATH, p_dsp_cfg, NULL, NULL);

			printf("pdcs device:%s, spot:%s,direction:%d, sartLane:%d, encodeType=%d\n",
					p_dsp_cfg->vdConfig.pdCS_config.device_config.deviceID,
					p_dsp_cfg->vdConfig.pdCS_config.device_config.strSpotName,
					p_dsp_cfg->vdConfig.pdCS_config.device_config.direction,
					p_dsp_cfg->vdConfig.pdCS_config.device_config.startLane,
					p_dsp_cfg->vdConfig.pdCS_config.device_config.encodeType);
		}
		else if(1 == DEV_TYPE || 2 == DEV_TYPE || 3 == DEV_TYPE || 5 == DEV_TYPE)
		{
			parse_xml_doc(DSP_PARAM_FILE_PATH, p_dsp_cfg, NULL, NULL);

			printf("vdcs device:%s, spot:%s,direction:%d, sartLane:%d, encodeType=%d\n",
					p_dsp_cfg->vdConfig.vdCS_config.device_config.deviceID,
					p_dsp_cfg->vdConfig.vdCS_config.device_config.strSpotName,
					p_dsp_cfg->vdConfig.vdCS_config.device_config.direction,
					p_dsp_cfg->vdConfig.vdCS_config.device_config.startLane,
					p_dsp_cfg->vdConfig.vdCS_config.device_config.encodeType);
		}
	}
	else if (!strcasecmp(command, "parse_camera"))
	{
		parse_xml_doc(CAMERA_PARAM_FILE_PATH, NULL, &g_camera_config,
				NULL);
	}
	else if (!strcasecmp(command, "parse_arm"))
	{
		parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
	}
	else if (!strcasecmp(command, "upload_arm"))
	{
		upload_arm_param = 1;
	}
	else if (!strcasecmp(command, "download_arm"))
	{
		memset(&ftp_arm_filePath, 0, sizeof(FTP_FILEPATH));
		strcpy(ftp_arm_filePath.m_strFileURL,
				"/370203456789/3702020065/filetrans/upload/arm_config.xml");
		download_arm_param = 1;
	}
	else if (!strcasecmp(command, "ver"))
	{
		Version_Detail *ver_detail;
		ver_detail = get_version_detail();
		if (ver_detail != NULL)
		{
			printf("SysServerVer: %s\n", ver_detail->SysServerVer);
			printf("FPGAVer: %s\n", ver_detail->FPGAVer);
			printf("McuVersion: %s\n", ver_detail->McuVersion);
		}

		char version_h264_pack[64];
		int ret = get_version_h264_pack(version_h264_pack,
				sizeof(version_h264_pack));
		if (ret < 0)
		{
			printf("get_version_h264_pack failed \n");
		}
		else
		{
			printf("ver is :%s, len is %d\n", version_h264_pack, ret);
		}
	}
	else if (!strcasecmp(command, "set_net_b")) //广播设置网络参数

	{
		MSG_HEADER msg_header;
		SET_NET_PARAM net_param;
		u32 sum;

		memcpy(&net_param, &g_set_net_param, sizeof(net_param));

		net_param.m_IP[0] = 192;
		net_param.m_IP[1] = 168;
		net_param.m_IP[2] = 1;
		net_param.m_IP[3] = 201;

		strcpy(net_param.m_NetParam.m_DeviceID, "3702123456");
		net_param.m_NetParam.m_IP[3] = 159;
		net_param.m_NetParam.ftp_param_conf.ip[3] = 58;

		DEBUG("IP1: %d.%d.%d.%d\n"
				"IP2: %d.%d.%d.%d\n"
				"MSK: %d.%d.%d.%d\n"
				"gateway: %d.%d.%d.%d\n"
				"mq: %d.%d.%d.%d\n"
				"ftp: %d.%d.%d.%d\n",
				net_param.m_IP[0],net_param.m_IP[1],net_param.m_IP[2],net_param.m_IP[3],
				net_param.m_NetParam.m_IP[0],net_param.m_NetParam.m_IP[1],net_param.m_NetParam.m_IP[2],net_param.m_NetParam.m_IP[3],
				net_param.m_NetParam.m_MASK[0],net_param.m_NetParam.m_MASK[1],net_param.m_NetParam.m_MASK[2],net_param.m_NetParam.m_MASK[3],
				net_param.m_NetParam.m_GATEWAY[0],net_param.m_NetParam.m_GATEWAY[1],net_param.m_NetParam.m_GATEWAY[2],net_param.m_NetParam.m_GATEWAY[3],
				net_param.m_NetParam.m_MQ_IP[0],net_param.m_NetParam.m_MQ_IP[1],net_param.m_NetParam.m_MQ_IP[2],net_param.m_NetParam.m_MQ_IP[3],
				net_param.m_NetParam.ftp_param_conf.ip[0],net_param.m_NetParam.ftp_param_conf.ip[1],net_param.m_NetParam.ftp_param_conf.ip[2],net_param.m_NetParam.ftp_param_conf.ip[3]
			 );

		memset(&msg_header, 0, sizeof(msg_header));
		msg_header.m_StartID = 0x68;
		msg_header.m_MsgType = MSG_SET_NETWORK_PARAM;
		msg_header.m_IsRequest = 1;
		msg_header.m_Result = 0;
		msg_header.m_ContenLength = sizeof(net_param);
		memcpy(msg_header.m_Content, &net_param, sizeof(net_param));

		sum = check_sum((u8 *) (&msg_header), sizeof(MSG_HEADER) - 4);
		msg_header.sum_check = sum;

		int sock = socket(PF_INET, SOCK_DGRAM, 0);//设置SOCKET
		const int opt = 1;
		//设置该套接字为广播类型，
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &opt, sizeof(opt));

		struct sockaddr_in addrto;
		bzero(&addrto, sizeof(struct sockaddr_in));
		addrto.sin_family = AF_INET;
		addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		addrto.sin_port = htons(6080);

		sendto(sock, &msg_header, sizeof(msg_header), 0,
				(struct sockaddr*) &addrto, sizeof(addrto));

	}
	else if (!strcasecmp(command, "netcheck"))
	{
		//netCheck = 1;
		printf("input feed_dog\n");

	}
	else if (!strcasecmp(command, "log_debug"))
	{
		for (int i = 0; i < 101; i++)
		{
			char txt[100];
			sprintf(txt, "test log_debug-  %d\n", i);
			log_debug_ftp(txt);
		}
		DEBUG("test log_debug\n");
	}
	else if (!strcasecmp(command, "log_state"))
	{
		for (int i = 0; i < 101; i++)
		{
			char txt[100];
			sprintf(txt, "test log_state-  %d\n", i);
			log_state_ftp(txt);
		}
		DEBUG("test log_state\n");
	}
	else if (!strcasecmp(command, "log_warn"))
	{
		for (int i = 0; i < 101; i++)
		{
			char txt[100];
			sprintf(txt, "test log_warn-  %d\n", i);
			log_warn_ftp(txt);
		}
		DEBUG("test log_warn\n");
	}
	else if (!strcasecmp(command, "log_error"))
	{
		for (int i = 0; i < 101; i++)
		{
			char txt[100];
			sprintf(txt, "test log_error-  %d\n", i);
			log_error_ftp(txt);
		}
		DEBUG("test log_error\n");
	}
	else if (!strcasecmp(command, "log_nobuffer"))
	{

		char txt[100];
		sprintf(txt, "test log_error  no_buffer-  \n");
		log_error_ftp(txt);

		sprintf(txt, "test log_warn  buffered- \n");
		log_warn_ftp(txt);

	}

	else if (!strcasecmp(command, "create_xml"))
	{
		VDConfigData *p_dsp_cfg = (VDConfigData *)get_dsp_cfg_pointer();
		p_dsp_cfg->flag_func = DEV_TYPE;

		create_xml_file("arm_config.xml", NULL, NULL, &g_arm_config);
		create_xml_file("dsp_config.xml", p_dsp_cfg, NULL, NULL);

	}
	else if (!strcasecmp(command, "mq1"))
	{
		//		g_debug_sleep = 123;
		//		g_flg_topic_err = 1;
		//		g_flg_queue_err = 1;
	}
	else if (!strcasecmp(command, "mq0"))
	{
		stop_mq_thread();
		DEBUG("mq close.........\n");
	}

	else if (!strcasecmp(command, "stime0"))
	{
		set_systime(2012, 7, 5, 15, 1, 0);
	}
	else if (!strcasecmp(command, "stime1"))
	{
		set_systime(2012, 7, 5, 16, 1, 0);
	}
	else if (!strcasecmp(command, "help"))
	{
		printf(
				"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		printf("         Commands help.                           \n");
		printf("         loadcfg      --update system config.          \n");
		printf("         savecfg      --save system config.          \n");
		printf("         cpuon        --turn on cpu  percent display.          \n");
		printf("         cpuoff       --turn off cpu  percent display.          \n");
		printf("         debugon      --turn on dsp status display.          \n");
		printf("         debugoff     --turn off dsp status display.          \n");
		printf("         broad        --send ip for finding server.          \n");

		printf("         alive .      --show threads alive.    \n");
		printf("         clear .      --clear OSD.    \n");
		printf("         reboot .     --reboot system.      \n");
		printf("         ver .        --look version.  \n");
		printf("         printcpu .   -- show cpu\n");
		printf("         print .      -- print system config.   \n");
		printf("         http .       -- http alleyway_senddatas_to_bitcom.   \n");
		printf("         cap .        -- send capture message to dsp.   \n");
		printf("         help    	  --print this infomation.          \n");
		printf(
				"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}
	else if (!strcasecmp(command, "config"))
	{
		//		flag_config_to_ftp = 1;
		//		flag_config_to_disk = 1;
		//		alg_para_config_flag = 2;
		printf("input config\n");

	}
	else if (!strcasecmp(command, "http"))
	{
		printf("test: http send data\n");

		DB_TrafficRecord db_traffic_record;
		EP_PicInfo pic_info;
		memset(&db_traffic_record,0,sizeof(DB_TrafficRecord));
		memset(&pic_info,0,sizeof(EP_PicInfo));
		char buf[200]="aabbccdd_aabb";
		pic_info.buf=buf;
		pic_info.size=strlen(buf);

		alleyway_senddatas_to_bitcom(&db_traffic_record,&pic_info);

	}
	else if (!strcasecmp(command, "cap"))
	{
		printf_with_ms("test: send capture msg to dsp\n");

		//send ParkRecordInput to DSP
		SnapRequestInput snap_request_input;
		snap_request_input.flag_snap=1;
		snap_request_input.dev_type=DEV_TYPE;

		TRACE_LOG_SYSTEM("==== dsp : snap_request_input.flag_snap=%d,dev_type=%d.\n",
				snap_request_input.flag_snap,
				snap_request_input.dev_type
		);
		send_dsp_msg(&snap_request_input,sizeof(SnapRequestInput),8);//1￡???·¨????2?êy￡?2￡?òtD?????2?êy￡?3￡?3μ???a￡?4￡?μà?￠????ê?è?2?êy￡?5￡?μ?×ó3μ??′￥·￠2?êy￡?6￡??????°??????′?￡?7￡?arm2?êy￡?8￡?×￥?????ó
		TRACE_LOG_SYSTEM("%s: after send_dsp_msg SnapRequestInput init \n",__func__);

	}
	else if (!strcasecmp(command, "up"))
	{
    	system("echo 1 > /sys/class/gpio/gpio24/value");
    	system("echo 0 > /sys/class/gpio/gpio13/value");
    	usleep(1000000);
    	system("echo 1 > /sys/class/gpio/gpio13/value");
    	//system("echo 1 > /sys/class/gpio/gpio24/value");

	}
	else if (!strcasecmp(command, "down"))
	{
       	system("echo 1 > /sys/class/gpio/gpio13/value");
		system("echo 0 > /sys/class/gpio/gpio24/value");
		usleep(1000000);
		//system("echo 1 > /sys/class/gpio/gpio13/value");
		system("echo 1 > /sys/class/gpio/gpio24/value");

	}

	else
	{
		printf("invalid command : %s\n", command);
		sleep(1);
	}
	return 0;

}

/************************************************************/
#define RTACTION_ADD   1
#define RTACTION_DEL   2
#define RTACTION_HELP  3

#define E_OPTERR        3
#define E_USAGE         4
#define E_SOCK          5
#define E_LOOKUP        6

/* Keep this in sync with /usr/src/linux/include/linux/route.h */
#define RTF_UP          0x0001          /* route usable                 */
#define RTF_GATEWAY     0x0002          /* destination is a gateway     */
#define RTF_HOST        0x0004          /* host entry (net otherwise)   */

#define mask_in_addr(x) (((struct sockaddr_in *)&((x).rt_genmask))->sin_addr.s_addr)
#define full_mask(x) (x)

//Like strncpy but make sure the resulting string is always 0 terminated.
char * safe_strncpy(char *dst, const char *src, size_t size)
{
	dst[size - 1] = '\0';
	return strncpy(dst, src, size - 1);
}

/*****************************************************
 *函数:ip_judeg(SET_NET_PARAM *ip_net)
 *功能:判断ip，网关，掩码等是否符合规则定义
 *输入参数:ip数据相关结构体
 *输出参数:正确为1，错误为0
 *编码时间:2011-12-16 作者:xnd
 ******************************************************/
int ip_judge(SET_NET_PARAM *ip_net)
{
	//判断MQ的IP 与端口号是否有效
	if ((ip_net->m_NetParam.m_MQ_IP[0] == 0)
			|| (ip_net->m_NetParam.m_MQ_IP[0] == 255)
			|| (ip_net->m_NetParam.m_MQ_IP[3] == 0)
			|| (ip_net->m_NetParam.m_MQ_IP[3] == 255)
			|| (ip_net->m_NetParam.m_MQ_PORT == 0))
	{
		return -1;
	}

	//判断设备的IP 是否有效
	if ((ip_net->m_NetParam.m_IP[0] >= 1)
			&& (ip_net->m_NetParam.m_IP[0] <= 126))//A类地址
	{
		DEBUG("A类地址\n");
	}
	else if ((ip_net->m_NetParam.m_IP[0] >= 128) && (ip_net->m_NetParam.m_IP[0]<= 191))//B类地址
	{
		DEBUG("B类地址\n");
	}
	else if ((ip_net->m_NetParam.m_IP[0] >= 192) && (ip_net->m_NetParam.m_IP[0]<= 223))//C类地址
	{
		DEBUG("C类地址\n");
	}
	else
	{
		//D,E或者无效的地址
		DEBUG(" ip_net->m_NetParam.m_IP[0]=%d \n",ip_net->m_NetParam.m_IP[0]);
		return 0;
	}

	//判断ip与网关的匹配
	if ((ip_net->m_NetParam.m_IP[0] & ip_net->m_NetParam.m_MASK[0])
			== (ip_net->m_NetParam.m_GATEWAY[0]	& ip_net->m_NetParam.m_MASK[0]))//子网判断
		{
			if ((ip_net->m_NetParam.m_IP[1] & ip_net->m_NetParam.m_MASK[1])
					== (ip_net->m_NetParam.m_GATEWAY[1] & ip_net->m_NetParam.m_MASK[1]))//子网判断
			{
				if ((ip_net->m_NetParam.m_IP[2] & ip_net->m_NetParam.m_MASK[2])
						== (ip_net->m_NetParam.m_GATEWAY[2] & ip_net->m_NetParam.m_MASK[2]))//子网判断
				{
					if ((ip_net->m_NetParam.m_IP[3] & ip_net->m_NetParam.m_MASK[3])
							== (ip_net->m_NetParam.m_GATEWAY[3] & ip_net->m_NetParam.m_MASK[3]))//子网判断
					{
						return 1;
					}
					else
					{
						return 0;
					}
				}
				else
				{
					return 0;
				}
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	return 0;
}

void printNetSet()
{
	DEBUG("IP1: %d.%d.%d.%d\n"
			"m_NetParam:\n"
			"ip: %d.%d.%d.%d\n"
			"MSK: %d.%d.%d.%d\n"
			"gateway: %d.%d.%d.%d\n"
			"mq: %d.%d.%d.%d\n"
			"ftp: %d.%d.%d.%d\n",
			g_set_net_param.m_IP[0],g_set_net_param.m_IP[1],g_set_net_param.m_IP[2],g_set_net_param.m_IP[3],
			g_set_net_param.m_NetParam.m_IP[0],g_set_net_param.m_NetParam.m_IP[1],g_set_net_param.m_NetParam.m_IP[2],g_set_net_param.m_NetParam.m_IP[3],
			g_set_net_param.m_NetParam.m_MASK[0],g_set_net_param.m_NetParam.m_MASK[1],g_set_net_param.m_NetParam.m_MASK[2],g_set_net_param.m_NetParam.m_MASK[3],
			g_set_net_param.m_NetParam.m_GATEWAY[0],g_set_net_param.m_NetParam.m_GATEWAY[1],g_set_net_param.m_NetParam.m_GATEWAY[2],g_set_net_param.m_NetParam.m_GATEWAY[3],
			g_set_net_param.m_NetParam.m_MQ_IP[0],g_set_net_param.m_NetParam.m_MQ_IP[1],g_set_net_param.m_NetParam.m_MQ_IP[2],g_set_net_param.m_NetParam.m_MQ_IP[3],
			g_set_net_param.m_NetParam.ftp_param_conf.ip[0],g_set_net_param.m_NetParam.ftp_param_conf.ip[1],g_set_net_param.m_NetParam.ftp_param_conf.ip[2],g_set_net_param.m_NetParam.ftp_param_conf.ip[3]
		 )

}

void* receiveThrFrn(void* arg)
{
	Flash_control_t lightCtrl;
	Light_msg lightPak;
	Light_switch *light_switch;
	strobe_t *strobe;
	char data[32] = {0};
	int ret;
	int newsfd = *(int*) arg;
	while (1)
	{
		if(newsfd < 0)
			break;
		ret = read(newsfd, data, sizeof(data));
		if (ret < 0)
		{
			printf("failed to read data, len=%d\n", ret);
			break;
		}
		else
		{

			if (data[0] == 'H')
			{
				memcpy(&lightCtrl, data, sizeof(lightCtrl));
				lightCtrl.addr = lightPak.data.addr;
				lightCtrl.sn = lightPak.data.sn;
				lightCtrl.cmd = lightPak.data.cmd;
				lightCtrl.len = lightPak.data.len;
				lightCtrl.data = lightPak.data.data[0];
				DEBUG("@@@@@@@@@@@@@@addr=%d sn=%d cmd=%d len=%d data=%d\n",
						lightPak.data.addr,lightPak.data.sn,lightPak.data.cmd,lightPak.data.len,lightPak.data.data[0]);
				ret = flash_control(&lightCtrl);
				if (ret != 0)
				{
					DEBUG("ERROR:addr=%d sn=%d cmd=%d len=%d data=%d\n",
							lightPak.data.addr,lightPak.data.sn,lightPak.data.cmd,lightPak.data.len,lightPak.data.data[0]);
				}
				lightPak.data.addr = lightCtrl.addr;
				lightPak.data.sn = lightCtrl.sn;
				lightPak.data.cmd = lightCtrl.cmd;
				lightPak.data.len = lightCtrl.len;
				lightPak.data.data[0] = lightCtrl.data;
				DEBUG("addr=%d sn=%d cmd=%d len=%d data=%d\n",
						lightPak.data.addr,lightPak.data.sn,lightPak.data.cmd,lightPak.data.len,lightPak.data.data[0]);
				int chk = 0;
				for (unsigned int i = 0; i < (sizeof(Light_msg) - 4); i++)
				{
					chk += *((char*) &lightPak + i);
				}
				lightPak.chk = chk;

				ret = write(newsfd, &lightPak, sizeof(lightPak));
				if (ret == -1)
				{
					printf("[%s][%d]write error\n", __FUNCTION__, __LINE__);
					break;
				}
				else
				{
					continue;
				}
			}
			else if (data[0] == 'C')
			{
				light_switch = (Light_switch *)data;
				strobe = (strobe_t *)(&light_switch->data);
				printf("\n\n\n");
				printf("[%s]%d : %d : %d\n", __FUNCTION__,(int)strobe->enable, \
						(int)strobe->polarity, (int)strobe->delay_time);
				if(light_switch->cmd == 0x02)
				{
					ret = set_strobe_control(strobe);
					if(ret != SUCCESS)
					{
						printf("[%s]set_strobe_control error \n", __FUNCTION__);
					}
				}
				else if(light_switch->cmd == 0x01)
				{
					strobe = get_strobe_ctrl();
					ret = write(newsfd, data, sizeof(data));
					if (ret == -1)
					{
						printf("[%s][%d]write error\n", __FUNCTION__, __LINE__);
						break;
					}
					else
					{
						printf("[%s]%d : %d : %d\n", __FUNCTION__,(int)strobe->enable, \
								(int)strobe->polarity, (int)strobe->delay_time);
						continue;
					}
				}
			}
			else
			{
				printf("[%s]receive error package\n", __FUNCTION__);
				break;
			}

		}
	}
	close(newsfd);
	return NULL;
}


/*************************************
Function:       read_gpio_status
Description:   读取GPIO状态信息
Author:
Date:
 *************************************/
char read_gpio_status(const char *val)
{
	int fd;
	char gpio_value = 1;
	char buf[64];

	sprintf(buf,"/sys/class/gpio/%s/value", val);

	fd = open(buf, O_RDONLY);

	if(fd < 0){
		perror("Can't open /sys/class/gpio/gpiox");
		return -1;
	}

	read(fd, &gpio_value, 1);

	close(fd);

	return gpio_value;
}


/*************************************
Function:       set_light_by_gpio
Description:    泊车指示灯控制函数
泊车指示灯有红，绿，蓝以及白光灯，红:gpio99 绿:gpio73 蓝:gpio72 白:使用pwm控制
Author:  	    lxd
Date:	        2014.1.5
 *************************************/
int set_light_by_gpio(const char *val,char a)
{
	int fd;
	char buf[64];

    sprintf(buf,"/sys/class/gpio/%s/value",val);
    fd = open(buf, O_WRONLY);

    if(fd < 0){
        perror("Can't open /sys/class/gpio/gpiox");
        return -1;
    }

    write(fd, &a, sizeof(char));

    close(fd);

    if(gi_platform_board == NEW_BOARD)
    {
        usleep(5000);//requisite. 3ms ok,1ms fail. //maybe related to hardware
    }

	return 0;

}

#if (5 == DEV_TYPE)
static int32_t get_white_light_max_rate(void)
{
	profile_t profile;
	char val[128];
	int32_t rate = 10;

	if(0 == profile_open(&profile, "/config/hide_param", "r")){
		if (profile_rd_val(&profile, "WhiteLightMaxRate", val) > 0) {
			rate = atoi(val);
			if (rate < 0) {
				rate = 10;
			}
		}

		profile_close(&profile);
	}

	INFO("WhiteLightMaxRate: %d", rate);

	return rate;
}
#endif

int sun_rise_set_time(str_ctrl *lstr_ctrltm, double ad_lon, double ad_lat)
{
	int li_switch = 0;
	time_t now;
	struct tm *lstr_timenow;
	double sunrise = 0;
	double sunset = 0;
	time(&now);
	lstr_timenow = localtime(&now);
	li_switch = 0;

	sunriset(lstr_timenow->tm_year+1900, lstr_timenow->tm_mon+1, lstr_timenow->tm_mday, ad_lon, ad_lat, &sunrise, &sunset);

	int day = lstr_timenow->tm_mday;
	int hour = lstr_timenow->tm_hour;
	int minute;
	struct tm *gmt = gmtime( &now );
	int zone = hour - gmt->tm_hour + (day - gmt->tm_mday)*24;

	//calcute sunrise and sunset time
	hour = sunrise+zone;
	minute = (sunrise - hour+zone)*60;
	if( hour > 24 ) hour -= 24;
	else if( hour < 0 ) hour += 24;

	lstr_ctrltm->end_time.hour = hour;
	lstr_ctrltm->end_time.min = minute;

	hour = sunset+zone;
	minute = (sunset - hour+zone)*60;
	if( hour > 24 ) hour -= 24;
	else if( hour < 0 ) hour += 24;

	lstr_ctrltm->start_time.hour = hour;
	lstr_ctrltm->start_time.min = minute;

	lstr_ctrltm->start_time.year = lstr_timenow->tm_year;
	lstr_ctrltm->start_time.mon = lstr_timenow->tm_mon;
	lstr_ctrltm->start_time.day = lstr_timenow->tm_mday;

	return 0;
}

void user_define_control(str_ctrl lstr_ctrltm)
{
	int li_switch = 0;
	time_t now;
	struct tm *lstr_timenow;
	time(&now);
	lstr_timenow = localtime(&now);
	li_switch = 0;

	//sunrisetime ,is fill-in light end_time  e.g 5:32
	//sunsettime ,is fill-in light start_time  e.g 18:46
	if((lstr_timenow->tm_hour > lstr_ctrltm.start_time.hour) || (lstr_timenow->tm_hour < lstr_ctrltm.end_time.hour))
	{
		li_switch = 1;//before sunrise or after sunset, turn on fill-in light
	}
	else if(lstr_timenow->tm_hour==lstr_ctrltm.start_time.hour)
	{
		if(lstr_timenow->tm_min>=lstr_ctrltm.start_time.min)//after sunset
		{
			li_switch = 1;
		}
	}
	else if(lstr_timenow->tm_hour==lstr_ctrltm.end_time.hour)
	{
		if(lstr_timenow->tm_min<=lstr_ctrltm.end_time.min)//before sunraise
		{
			li_switch = 1;
		}
	}


	if(li_switch == 0)//close the fill-in light
	{
//		system("echo 1 > /sys/class/gpio/gpio73/value");
		set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_off);
	}
	else if(li_switch == 1)//open the fill-in light
	{
//		system("echo 0 > /sys/class/gpio/gpio73/value");
		set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_on);
	}
}

void fillin_file_write(const char *ac_path, char ac_buf[][64], int ai_lens)
{
	FILE *fp = NULL;
	char lcr_buf[128] = { 0 };
	char lc_count = 0;
	char lc_buf_one[32] = { 0 };
	char lc_buf_two[32] = { 0 };
	char lc_buf_thr[32] = { 0 };
	char lc_buf_four[32] = { 0 };
	char lc_buf_5[32] = { 0 };

	fp = fopen(ac_path, "a+");
	if (!fp) {
		ERROR("Open %s failed!", ac_path);
		return;
	}

	while (fgets(lcr_buf, 100, fp) != NULL)
	{
		switch(lc_count)
		{
			case 0:
				strcpy(lc_buf_one, lcr_buf);
			break;

			case 1:
				strcpy(lc_buf_two, lcr_buf);
			break;

			case 2:
				strcpy(lc_buf_thr, lcr_buf);
			break;

			case 3:
				strcpy(lc_buf_four, lcr_buf);
			break;

			case 4:
				strcpy(lc_buf_5, lcr_buf);
			break;

		}

		lc_count++;

	}
	fclose(fp);

	fp = fopen(ac_path, "w");
	if (!fp) {
		ERROR("Open %s failed!", ac_path);
		return;
	}

	if (strncmp(lc_buf_one, ac_buf[0], strlen("[mode:")) == 0)
	{
		fprintf(fp, "%s\n", ac_buf[0]);
	}
	else
	{
		if(strncmp(ac_buf[0], "[mode:", strlen("[mode:")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[0]);
		}
	}

	if(strncmp(lc_buf_two,  "[longitude]", strlen("[longitude]")) == 0 )
	{
		if(strncmp(lc_buf_two,  ac_buf[1], strlen("[longitude]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[1]);
		}
		else
		{
			fprintf(fp, "%s\n", lc_buf_two);
		}
	}
	else
	{
		if(strncmp(ac_buf[1], "[longitude]", strlen("[longitude]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[1]);
		}
	}

	if(strncmp(lc_buf_thr,  "[latitude]", strlen("[latitude]")) == 0 )
	{
		if(strncmp(lc_buf_thr,  ac_buf[2], strlen("[latitude]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[2]);
		}
		else
		{
			fprintf(fp, "%s\n", lc_buf_thr);
		}
	}
	else
	{
		if(strncmp(ac_buf[2], "[latitude]", strlen("[latitude]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[2]);
		}
	}

	if(strncmp(lc_buf_four, "[risetime]", strlen("[risetime]")) == 0 )
	{
		if(strncmp(lc_buf_four,  ac_buf[3], strlen("[risetime]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[3]);
		}
		else
		{
			fprintf(fp, "%s\n", lc_buf_four);
		}
	}
	else
	{
		if(strncmp(ac_buf[3], "[risetime]", strlen("[risetime]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[3]);
		}
	}

	if(strncmp(lc_buf_5, "[settime]", strlen("[settime]")) == 0 )
	{
		if(strncmp(lc_buf_5,  ac_buf[4], strlen("[settime]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[4]);
		}
		else
		{
			fprintf(fp, "%s\n", lc_buf_5);
		}
	}
	else
	{
		if(strncmp(ac_buf[4], "[settime]", strlen("[settime]")) == 0 )
		{
			fprintf(fp, "%s\n", ac_buf[4]);
		}
	}

	fclose(fp);

}

void fillin_init_cfg(str_fillinlight_init *as_fillinlight)
{
	FILE *fp = NULL;
	char lcr_buf[128] = {0};

	fp = fopen("/mnt/nand/fillinlight.cfg", "a+");
	if (!fp) {
		DEBUG("Open /mnt/nand/fillinlight.cfg failed!");
		return;
	}

	while(fgets(lcr_buf, 100, fp) != NULL)
	{
		if(strncmp(lcr_buf, "[mode:0]", strlen("[mode:0]")) == 0 )
		{
			strcpy(as_fillinlight->mode, "0");
		}
		else if(strncmp(lcr_buf, "[mode:1]", strlen("[mode:1]")) == 0 )
		{
			strcpy(as_fillinlight->mode, "1");
		}
		else if(strncmp(lcr_buf, "[mode:2]", strlen("[mode:2]")) == 0 )
		{
			strcpy(as_fillinlight->mode, "2");
		}
		else if(strncmp(lcr_buf, "[mode:3]", strlen("[mode:3]")) == 0 )
		{
			strcpy(as_fillinlight->mode, lcr_buf+strlen("[mode:3]="));
		}
		else if(strncmp(lcr_buf, "[longitude]", strlen("[longitude]")) == 0 )
		{
			strcpy(as_fillinlight->lon, lcr_buf+strlen("[longitude]="));
		}
		else if(strncmp(lcr_buf, "[latitude]", strlen("[latitude]")) == 0 )
		{
			strcpy(as_fillinlight->lat, lcr_buf+strlen("[latitude]="));
		}
		else if(strncmp(lcr_buf, "[risetime]", strlen("[risetime]")) == 0 )
		{
			strcpy(as_fillinlight->sunrisetime, lcr_buf+strlen("[risetime]="));
		}
		else if(strncmp(lcr_buf, "[settime]", strlen("[settime]")) == 0 )
		{
			strcpy(as_fillinlight->sunsettime, lcr_buf+strlen("[settime]="));
		}
	}

	fclose(fp);

	fp = fopen("/tmp/fillinlight_smart_control.info", "a+");
	if (!fp) {
		DEBUG("Open /tmp/fillinlight_smart_control.info failed!");
		return;
	}
	fgets(as_fillinlight->smart_flag, 3, fp);
	fclose(fp);

}

/*******************************************************************************
 *函数:FillinLightThrFxn()
 *功能:用于监控补光灯的状态
 *******************************************************************************/
void *FillinLightThrFxn(void *arg)
{
	int li_recv_lens = 0;
	str_ctrl lstr_ctrltm;
	int li_mode = 2;
	char lc_mode[32] = {0};
	double lon, lat;
	char lc_sunrise[32] = {0};
	char lc_sunset[32] = {0};
	char file_buf[12][64];
	FILLIN_LIGHT_SET_MSG lstr_web_control;
	str_fillinlight_init lstr_fillinlight;
	Json::Reader reader;
	Json::FastWriter fast_writer;
	Json::Value root;
	Json::Value childroot;
	Json::Value send_root;
	Json::Value send_childroot;
	std::string result_json;
	int qid = Msg_Init(SYS_MSG_KEY);

	memset(&lstr_ctrltm, 0, sizeof(lstr_ctrltm));
	memset(&lstr_fillinlight, 0, sizeof(lstr_fillinlight));
	memset(&lstr_web_control, 0, sizeof(lstr_web_control));

//	system("insmod pinmux_module.ko a=0x48140a3c v=0x80");
//	system("rmmod pinmux_module.ko");
//	system("echo 73 > /sys/class/gpio/export");
//	system("echo out > /sys/class/gpio/gpio73/direction");
//	system("echo 1 > /sys/class/gpio/gpio73/value");

	fillin_init_cfg(&lstr_fillinlight);
	li_mode = atoi(lstr_fillinlight.mode);

	int hour, min;
	sscanf(lstr_fillinlight.sunrisetime, "%d:%d", &hour, &min);
	lstr_ctrltm.end_time.hour = (unsigned char)hour;
	lstr_ctrltm.end_time.min = (unsigned char)min;
	sscanf(lstr_fillinlight.sunsettime, "%d:%d", &hour, &min);
	lstr_ctrltm.start_time.hour = (unsigned char)hour;
	lstr_ctrltm.start_time.min = (unsigned char)min;

	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.mode = %s", lstr_fillinlight.mode);
	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.lon = %s", lstr_fillinlight.lon);
	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.lat = %s", lstr_fillinlight.lat);
	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.sunrisetime = %s", lstr_fillinlight.sunrisetime);
	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.sunsettime = %s", lstr_fillinlight.sunsettime);
	TRACE_LOG_PLATFROM_INTERFACE("FillinLight init lstr_fillinlight.smart_flag = %s", lstr_fillinlight.smart_flag);

	while(1)
	{
		//recive boa control message!
		li_recv_lens = msgrcv(qid, &lstr_web_control, sizeof(lstr_web_control) - sizeof(long), MSG_TYPE_VD, IPC_NOWAIT);
		if(li_recv_lens > 0)
		{
			TRACE_LOG_PLATFROM_INTERFACE("Vd recieve from server : type=%d, des=%s, data=%s", lstr_web_control.msg_type, lstr_web_control.des, lstr_web_control.buf);

			if(strncmp(lstr_web_control.des, "vd", strlen("vd")) == 0)
			{
				if(reader.parse(lstr_web_control.buf, root))
				{
					if(root["method"].asString() == "setFlashConfig")
					{
						childroot = root["param"];

						//关
						if(childroot["mode"].asString()=="0")
						{
							//send message to boa
							lstr_web_control.msg_type = MSG_TYPE_SERVER;
							strncpy(lstr_web_control.des, "boa", strlen("boa"));
							send_root["result"] = "true";
							result_json = fast_writer.write(send_root);
							strcpy(lstr_web_control.buf, result_json.data());
							msgsnd(qid, &lstr_web_control, sizeof(lstr_web_control) - sizeof(long),0);

							li_mode = 0;

							strcpy(lc_mode, "0");
							strcpy(lstr_fillinlight.mode, lc_mode);
							memset(file_buf[0], 0, 64);
							strcpy(file_buf[0], "[mode:0]=0");
							fillin_file_write("/mnt/nand/fillinlight.cfg", file_buf, 0);
						}
						//开
						else if(childroot["mode"].asString()=="1")
						{
							//send message to boa
							lstr_web_control.msg_type = MSG_TYPE_SERVER;
							strncpy(lstr_web_control.des, "boa", strlen("boa"));
							send_root["result"] = "true";
							result_json = fast_writer.write(send_root);
							strcpy(lstr_web_control.buf, result_json.data());
							msgsnd(qid, &lstr_web_control, sizeof(lstr_web_control)  - sizeof(long),0);

							li_mode = 1;
							strcpy(lc_mode, "1");
							strcpy(lstr_fillinlight.mode, lc_mode);
							memset(file_buf[0], 0, 64);
							strcpy(file_buf[0], "[mode:1]=1");
							fillin_file_write("/mnt/nand/fillinlight.cfg", file_buf, 0);
						}
						//智能
						else if(childroot["mode"].asString()=="2")
						{
							//send message to boa
							lstr_web_control.msg_type = MSG_TYPE_SERVER;
							strncpy(lstr_web_control.des, "boa", strlen("boa"));
							send_root["result"] = "true";
							result_json = fast_writer.write(send_root);
							strcpy(lstr_web_control.buf, result_json.data());
							msgsnd(qid, &lstr_web_control, sizeof(lstr_web_control) - sizeof(long),0);

							li_mode = 2;
							strcpy(lc_mode, "2");
							strcpy(lstr_fillinlight.mode, lc_mode);
							memset(file_buf[0], 0, 64);
							strcpy(file_buf[0], "[mode:2]=2");
							fillin_file_write("/mnt/nand/fillinlight.cfg", file_buf, 0);
						}
						//自定义(经纬度)
						else if(childroot["mode"].asString()=="3")
						{

							stringstream sstrlon(childroot["longitude"].asString());
							sstrlon >> lon;

							stringstream sstrlat(childroot["latitude"].asString());
							sstrlat >> lat;
							sun_rise_set_time(&lstr_ctrltm, lon, lat);
							sprintf(lc_sunrise, "%d:%d", lstr_ctrltm.end_time.hour, lstr_ctrltm.end_time.min);
							sprintf(lc_sunset, "%d:%d", lstr_ctrltm.start_time.hour, lstr_ctrltm.start_time.min);

							//send message to boa
							lstr_web_control.msg_type = MSG_TYPE_SERVER;
							strncpy(lstr_web_control.des, "boa", strlen("boa"));
							send_childroot["sunrise"] = lc_sunrise;
							send_childroot["sunset"] = lc_sunset;
							send_root["result"] = send_childroot;
							result_json = fast_writer.write(send_root);
							strcpy(lstr_web_control.buf, result_json.data());
							msgsnd(qid, &lstr_web_control, sizeof(lstr_web_control) - sizeof(long),0);

							TRACE_LOG_PLATFROM_INTERFACE("send_to_boa's json:%s", lstr_web_control.buf);
							TRACE_LOG_PLATFROM_INTERFACE("lon=%f, lat=%f", lon, lat);
							TRACE_LOG_PLATFROM_INTERFACE("sunrise=%d:%d, ", lstr_ctrltm.end_time.hour, lstr_ctrltm.end_time.min);

							TRACE_LOG_PLATFROM_INTERFACE("sunset=%d:%d, ", lstr_ctrltm.start_time.hour, lstr_ctrltm.start_time.min);
							li_mode = 3;
							strcpy(lc_mode, "3");
							strcpy(lstr_fillinlight.mode, lc_mode);
							sprintf(lstr_fillinlight.lon, "%f", lon);
							sprintf(lstr_fillinlight.lat, "%f", lat);
							strcpy(lstr_fillinlight.sunrisetime, lc_sunrise);
							strcpy(lstr_fillinlight.sunsettime, lc_sunset);
							memset(file_buf[0], 0, 64);
							memset(file_buf[1], 0, 64);
							memset(file_buf[2], 0, 64);
							memset(file_buf[3], 0, 64);
							memset(file_buf[4], 0, 64);
							strcpy(file_buf[0], "[mode:3]=3");
							sprintf(file_buf[1], "[longitude]=%s", lstr_fillinlight.lon);
							sprintf(file_buf[2], "[latitude]=%s", lstr_fillinlight.lat);
							sprintf(file_buf[3], "[risetime]=%s", lstr_fillinlight.sunrisetime);
							sprintf(file_buf[4], "[settime]=%s", lstr_fillinlight.sunsettime);
							fillin_file_write("/mnt/nand/fillinlight.cfg", file_buf, 0);
						}

					}
					else if(root["method"].asString() == "getFlashConfig")
					{
						//send message to boa
						lstr_web_control.msg_type = MSG_TYPE_SERVER;
						strncpy(lstr_web_control.des, "boa", strlen("boa"));
						send_childroot["mode"] = lstr_fillinlight.mode;
						send_childroot["longitude"] = lstr_fillinlight.lon;
						send_childroot["latitude"] = lstr_fillinlight.lat;
						send_childroot["sunrise"] = lstr_fillinlight.sunrisetime;
						send_childroot["sunset"] = lstr_fillinlight.sunsettime;
						send_root["result"] = send_childroot;
						result_json = fast_writer.write(send_root);
						strcpy(lstr_web_control.buf, result_json.data());
						msgsnd(qid, &lstr_web_control, sizeof(lstr_web_control) - sizeof(long),0);
					}
				}
			}
		}

		switch(li_mode)
		{
			case 0:
//				system("echo 1 > /sys/class/gpio/gpio73/value");
				set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_off);
				break;

			case 1:
//				system("echo 0 > /sys/class/gpio/gpio73/value");
				set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_on);
				break;

			case 2:
				if(gi_filllight_smart == 1)
				{
//					system("echo 1 > /sys/class/gpio/gpio73/value");
					set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_on);
				}else
				{
//					system("echo 0 > /sys/class/gpio/gpio73/value");
					set_light_by_gpio(gstr_filllight_info.name, gstr_filllight_info.turn_off);
				}
				break;

			case 3:
				user_define_control(lstr_ctrltm);
				break;
		}
		sleep(1);
	}

	return NULL;
}

/*******************************************************************************
 *函数:ElectronicPlateThrFxn()
 *功能:电子车牌状态监控线程
 *******************************************************************************/
void *ElectronicPlateThrFxn(void *arg)
{
	ElectronicPlateInput eplate_info;

	for (;;) {
		usleep(100000);

		if (0 == eplate_get_status()) {
			/* 发送有车信号给算法 */
			eplate_info.plateStatus = 1;
			eplate_info.ElectronicPlateMode = 1;
			send_dsp_msg(&eplate_info, sizeof(eplate_info), 5);

			/* 10ms后发送无车信号给算法 */
			usleep(10000);
			eplate_info.plateStatus = 2;
			eplate_info.ElectronicPlateMode = 1;
			send_dsp_msg(&eplate_info, sizeof(eplate_info), 5);

			TRACE_LOG_SYSTEM("send epc info to dsp finished.");
		}
	}

	return NULL;
}

/*******************************************************************************
 *函数:LightMonitorThrFxn()
 *功能:用于监控补光灯的状态
 *******************************************************************************/
void *LightMonitorThrFxn(void *arg)
{
	int ret = 0;
	socklen_t len = 0;
	int sockfd, newsfd;
	struct sockaddr_in destAddr, srcAddr;

	while (1)
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			printf("failed to create light monitor socket!\n");
			continue;
		}
		srcAddr.sin_family = AF_INET;
		srcAddr.sin_port = htons(28800);
		srcAddr.sin_addr.s_addr = INADDR_ANY;

		ret = ::bind(sockfd, (struct sockaddr *) &srcAddr,
				sizeof(struct sockaddr));
		if (ret == -1)
		{
			//printf("failed to bind light monitor socket!\n");
			close(sockfd);
			continue;
		}

		if (listen(sockfd, 5) == -1)
		{
			printf("failed to listen light monitor socket!\n");
			close(sockfd);
			continue;
		}
		while (1)
		{
			len = sizeof(struct sockaddr);
			newsfd = accept(sockfd, (struct sockaddr *) &destAddr, &len);
			if (newsfd == -1)
			{
				printf("failed to accept light monitor socket!\n");
				continue;
			}
			pthread_t tid;
			pthread_create(&tid, NULL, receiveThrFrn, &newsfd);
		}
		close(sockfd);
	}
	pthread_exit(NULL);
}

/*******************************************************************************
Function:     BroadcastThrFxn()
Description :用于设备搜索，并接受相关配置信息
当上位机软件进入现场配置设备点击搜索设备的时候，就会发送广播，
recvfrom接收到消息之后解除阻塞，并进入 	case MSG_ONLINE_DEVICE: //现场搜索设备
执行之后的代码
 *******************************************************************************/
void * BroadcastThrFxn(void *arg)
{
	NET_PARAM netparam;
	SET_NET_PARAM *psetnet;
	int sock_broad;
	struct sockaddr_in sin;
	MSG_HEADER header;
	header.m_StartID = 0x0068;
	int _errno;
	int nLen = sizeof(MSG_HEADER);

	struct sockaddr_in recvsock, sendsock;
	int nlength = sizeof(sin);
	BOOL fBroadcast = 1;
	int ret = -1;
	int nRet;
	char sBuf[MAX_BUF_SIZE];
	u32 sum;
	char ip_old[16];
	int ip_changed = 0, mq_changed = 0;

	printf(">> enter %s()\n", __func__);

	while (1)
	{
		sock_broad = socket(PF_INET, SOCK_DGRAM, 0);//设置SOCKET

		sin.sin_family = AF_INET;
		sin.sin_port = htons(6080);//收监听端口
		sin.sin_addr.s_addr = htonl(INADDR_ANY);

		//绑定本机
		if (::bind(sock_broad, (struct sockaddr*) &sin, sizeof(sin)) != 0)
		{
			printf("Can't bind socket to local port!Program stop.\n");//初始化失败返回-1
			return NULL;
		}
		printf("bind ok!\n");

		/////   收消息    /////

		while (1)
		{
			nRet = recvfrom(sock_broad, sBuf, MAX_BUF_SIZE, 0,
					(struct sockaddr*) &recvsock, (socklen_t *) &nlength);

			if (nRet < 0)
			{
				fprintf(stderr, "recvfrom Error:%s\n", strerror(errno));
				printf("recv failed!\n");
				break;
			}
			printf("after recvfrom BroadcastThrFxn : %s\n", inet_ntoa(recvsock.sin_addr));

			if ((unsigned int) nRet < sizeof(MSG_HEADER))
			{
				printf("broadcast: recv data is less .\n");
				//sleep(1);
				continue;
			}

			MSG_HEADER *ptemp = (MSG_HEADER *) sBuf;
			if (ptemp->m_StartID != 0x0068)//加一个判断是否信息正确，正确发送，错误等待监听
			{
				continue;
			}

			sum = check_sum((u8 *) (&ptemp->m_StartID), sizeof(MSG_HEADER) - 4);
			//判断校验和;
			if (sum != ptemp->sum_check)
			{
				DEBUG( "check_sum = %u ,MSG_HEADER.sum_check= %d\n",
						sum, (int)ptemp->sum_check);
				continue;
			}

			switch (ptemp->m_MsgType)
			{
				case MSG_RECOVERY_DEFAULTCONFIG://恢复出厂设置

					psetnet = (SET_NET_PARAM *) (ptemp->m_Content);
					if (psetnet->m_IP[0] == g_set_net_param.m_NetParam.m_IP[0]
							&& psetnet->m_IP[1]
							== g_set_net_param.m_NetParam.m_IP[1]
							&& psetnet->m_IP[2]
							== g_set_net_param.m_NetParam.m_IP[2]
							&& psetnet->m_IP[3]
							== g_set_net_param.m_NetParam.m_IP[3])
					{
						DEBUG("recovery default config of device(%d:%d:%d:%d)!\n",
								psetnet->m_IP[0],psetnet->m_IP[1],psetnet->m_IP[2],psetnet->m_IP[3]);

						strncpy(g_set_net_param.m_NetParam.m_DeviceID,
								NET_DEVICE_ID,
								sizeof(g_set_net_param.m_NetParam.m_DeviceID));
						g_set_net_param.m_NetParam.m_MQ_IP[0] = NET_MQIP1;
						g_set_net_param.m_NetParam.m_MQ_IP[1] = NET_MQIP2;
						g_set_net_param.m_NetParam.m_MQ_IP[2] = NET_MQIP3;
						g_set_net_param.m_NetParam.m_MQ_IP[3] = NET_MQIP4;
						g_set_net_param.m_NetParam.m_MQ_PORT = NET_MQPORT;
						g_set_net_param.m_NetParam.ftp_param_conf.ip[0]
							= NET_FTPIP1;
						g_set_net_param.m_NetParam.ftp_param_conf.ip[1]
							= NET_FTPIP2;
						g_set_net_param.m_NetParam.ftp_param_conf.ip[2]
							= NET_FTPIP3;
						g_set_net_param.m_NetParam.ftp_param_conf.ip[3]
							= NET_FTPIP4;
						g_set_net_param.m_NetParam.ftp_param_conf.port
							= NET_FTPPORT;
						g_set_net_param.m_NetParam.ftp_param_conf.allow_anonymous
							= NET_FTPANONYMOUS;
						strncpy(
								g_set_net_param.m_NetParam.ftp_param_conf.user,
								NET_FTPUSER,
								sizeof(g_set_net_param.m_NetParam.ftp_param_conf.user));
						strncpy(
								g_set_net_param.m_NetParam.ftp_param_conf.passwd,
								NET_FTPPASSWD,
								sizeof(g_set_net_param.m_NetParam.ftp_param_conf.passwd));

						ret = restore_factory_set();
						if (ret != SUCCESS)
						{
							printf("[%s]recovery_factory_set failed!\n ",
									__FUNCTION__);
							ptemp->m_Result = 0;
						}
						else
						{
							write_config_file((char*) SERVER_CONFIG_FILE);
							/*FIXME: file "factory_set.txt" maybe not useful!*/
							write_factory_file((char*) FACTORY_CONFIG_FILE);

							ptemp->m_Result = 1;
						}

						ptemp->m_MsgType = MSG_RECOVERY_DEFAULTCONFIG_RESULT;
						ptemp->m_IsRequest = 0;
						memcpy(ptemp->m_Content, (char *) psetnet,
								sizeof(SET_NET_PARAM));

						sendsock.sin_addr.s_addr = recvsock.sin_addr.s_addr;
						sendsock.sin_family = AF_INET;
						sendsock.sin_port = htons(6081);
						int nRet = sendto(sock_broad, ptemp, nLen, 0,
								(struct sockaddr*) &sendsock, sizeof(sendsock));
						if (nRet < 0)
						{
							fprintf(stderr, "Bind Error:%s\n", strerror(errno));
							printf("send failed!\n");
							break;
						}

						remove(ARM_PARAM_FILE_PATH);
						remove(CAMERA_PARAM_FILE_PATH);
						remove(DSP_PARAM_FILE_PATH);
						remove(DSP_PD_PARAM_FILE_PATH);
						//remove(SERVER_CONFIG_FILE);
						printf("reboot\n");
						/*recovery defaut config sucessfully and reboot*/
						if (ret == SUCCESS)
						{
							power_down();
						}
						else
						{
							printf("recovery default config failed!\n");
						}
					}

					break;
				case MSG_ONLINE_DEVICE: //现场搜索设备

					fBroadcast = 1;

					INFO("Commander ip is %s\n", inet_ntoa(recvsock.sin_addr));
					memcpy((char*) &netparam,
							(char*) &(g_set_net_param.m_NetParam),
							sizeof(NET_PARAM));

					sendsock.sin_port = htons(6081);
					sendsock.sin_family = AF_INET;
					sendsock.sin_addr.s_addr = inet_addr("255.255.255.255");

					INFO("Local ip is %d.%d.%d.%d\n",
						 netparam.m_IP[0], netparam.m_IP[1],
						 netparam.m_IP[2], netparam.m_IP[3]);
					memset(&header, 0, nLen);
					header.m_MsgType = MSG_DEVICE_CURRENT_PARAM;
					header.m_IsRequest = 0;
					header.m_Result = 1;
					header.m_ContenLength = sizeof(NET_PARAM);
					memcpy(header.m_Content, (char*) &netparam,
						   sizeof(NET_PARAM));
					INFO("Mac: %02X%02X%02X%02X%02X%02X",
						 netparam.m_btMac[0], netparam.m_btMac[1],
						 netparam.m_btMac[2], netparam.m_btMac[3],
						 netparam.m_btMac[4], netparam.m_btMac[5]);

					header.sum_check = check_sum((u8 *)&header,
												 sizeof(header) - 4);
					setsockopt(sock_broad, SOL_SOCKET, SO_BROADCAST,
							(char*) &fBroadcast, sizeof(BOOL));

					nRet = sendto(sock_broad, &header, nLen, 0,
							(struct sockaddr*) &sendsock, sizeof(sendsock));
					if (nRet < 0)
					{
						_errno = errno;
						ERROR("Broadcast sendto failed:%s", strerror(_errno));
						break;
					}
					INFO("Send to search online success!") ;

					break;
				case MSG_DEVICE_NETWORK_PARAM:

					memcpy((char*) &netparam,
							(char*) &(g_set_net_param.m_NetParam),
							sizeof(NET_PARAM));

					sendsock.sin_family = AF_INET;
					sendsock.sin_addr.s_addr = recvsock.sin_addr.s_addr;
					sendsock.sin_port = htons(6081);

					memset(&header, 0, nLen);//结构体
					header.m_MsgType = MSG_DEVICE_CURRENT_PARAM;
					header.m_IsRequest = 0;
					header.m_Result = 1;
					header.m_ContenLength = sizeof(NET_PARAM);
					//memset(header.m_Content, 0, 256);//字符数组
					memcpy(header.m_Content, (char*) &netparam, sizeof(NET_PARAM));
					header.sum_check
						= check_sum((u8 *) &header, sizeof(header) - 4);
					fBroadcast = 0;
					setsockopt(sock_broad, SOL_SOCKET, SO_BROADCAST,
							(char*) &fBroadcast, sizeof(BOOL)); //设置为非广播模式

					nRet = sendto(sock_broad, &header, nLen, 0,
							(struct sockaddr*) &sendsock, sizeof(sendsock));
					if (nRet < 0)
					{
						fprintf(stderr, "Bind Error:%s\n", strerror(errno));
						printf("send failed!\n");
						break;
					}
					printf("发送udp参数成功\n");

					break;
				case MSG_SET_NETWORK_PARAM:

					DEBUG("现场配制网络广播参数: the recvsock.sin_addr is %s\n", inet_ntoa(
								recvsock.sin_addr))
						;
					sprintf(ip_old, "%d.%d.%d.%d",
							g_set_net_param.m_NetParam.m_IP[0],
							g_set_net_param.m_NetParam.m_IP[1],
							g_set_net_param.m_NetParam.m_IP[2],
							g_set_net_param.m_NetParam.m_IP[3]);////后期用来替换的//////////
					psetnet = (SET_NET_PARAM *) (ptemp->m_Content);
					if (psetnet->m_IP[0] == g_set_net_param.m_NetParam.m_IP[0]
						&& psetnet->m_IP[1]	== g_set_net_param.m_NetParam.m_IP[1]
						&& psetnet->m_IP[2]	== g_set_net_param.m_NetParam.m_IP[2]
						&& psetnet->m_IP[3]	== g_set_net_param.m_NetParam.m_IP[3])/////用来比较是否是发给自己的
					{
						ret = ip_judge(psetnet);

						DEBUG("ip_judge return %d. \n",ret);

						ptemp->m_Result = 0;
						if (ret == -1)
						{
							char str[100];
							memset(str, 0, sizeof(str));
							sprintf(str,
									"broadcast set err : MQ IP is %d.%d.%d.%d:%d",
									psetnet->m_NetParam.m_MQ_IP[0],
									psetnet->m_NetParam.m_MQ_IP[1],
									psetnet->m_NetParam.m_MQ_IP[2],
									psetnet->m_NetParam.m_MQ_IP[3],
									psetnet->m_NetParam.m_MQ_PORT);
							//write_err_log_noflash(str);
						}
						else if (ret == 0)
						{
							char str[100];
							memset(str, 0, sizeof(str));
							sprintf(
									str,
									"broadcast set : the dev ip is err: %d.%d.%d.%d",
									psetnet->m_NetParam.m_IP[0],
									psetnet->m_NetParam.m_IP[1],
									psetnet->m_NetParam.m_IP[2],
									psetnet->m_NetParam.m_IP[3]);
							//write_err_log_noflash(str);
						}
						else if (ret == 1)
						{
							ptemp->m_Result = 1;//正确时，给上位机返回成功信息
						}

						if (ptemp->m_Result == 1)
						{
							mq_changed = ip_changed = 0;
							if ( ((psetnet->m_NetParam.m_IP)
										!=  (g_set_net_param.m_NetParam.m_IP))
									|| ( (psetnet->m_NetParam.m_GATEWAY)
										!= (g_set_net_param.m_NetParam.m_GATEWAY))
									||  ((psetnet->m_NetParam.m_MASK)
										!=  (g_set_net_param.m_NetParam.m_MASK)))
							{
								ip_changed = 1;
							}

							if ((psetnet->m_NetParam.m_MQ_IP
										!=  (g_set_net_param.m_NetParam.m_MQ_IP))
									|| (psetnet->m_NetParam.m_MQ_PORT
										!= g_set_net_param.m_NetParam.m_MQ_PORT))
							{
								mq_changed = 1;
							}

							DEBUG("ip_changed = %d, mq_changed =%d \n" , ip_changed, mq_changed);
							memcpy((char*) &g_set_net_param, (char *) psetnet,
									sizeof(SET_NET_PARAM));

							memcpy(online_device.m_strDeviceID,
									g_set_net_param.m_NetParam.m_DeviceID,
									sizeof(online_device.m_strDeviceID));
							get_ftp_chanel(FTP_CHANNEL_CONFIG)->set_config(
									&g_set_net_param.m_NetParam.ftp_param_conf);

							printNetSet();

							write_config_file((char*) SERVER_CONFIG_FILE);
							write_factory_file((char*) FACTORY_CONFIG_FILE);
							DEBUG("write config file finished.\n");

							// 1. 调用设置接口。 2.发送网络配置消息，jacky
							if (ip_changed == 1)
							{
								net_config nc;

								struct in_addr *ip = (struct in_addr *)g_set_net_param.m_NetParam.m_IP;
								struct in_addr *gw = (struct in_addr *)g_set_net_param.m_NetParam.m_GATEWAY;
								struct in_addr *mask = (struct in_addr *)g_set_net_param.m_NetParam.m_MASK;
								nc.ip = *ip;
								nc.gateway = *gw;
								nc.netmask = *mask;
								sprintf(nc.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
										g_set_net_param.m_NetParam.m_btMac[0],
										g_set_net_param.m_NetParam.m_btMac[1],
										g_set_net_param.m_NetParam.m_btMac[2],
										g_set_net_param.m_NetParam.m_btMac[3],
										g_set_net_param.m_NetParam.m_btMac[4],
										g_set_net_param.m_NetParam.m_btMac[5]);

								ret = set_net_config(&nc);
								DEBUG("set_net_config() return %d.\n",ret);
							}
						}
						else
						{
							printf("ip_set is failed!\n");
						}

						char *ip_new = inet_ntoa(recvsock.sin_addr);
						if (0 == strncmp(ip_old, ip_new, 10))//判断是否是在同一个域内，是则统认为单播
						{
							sendsock.sin_addr.s_addr = recvsock.sin_addr.s_addr;
							printf("in the same network!\n");
						}
						else
						{
							sendsock.sin_addr.s_addr = inet_addr("255.255.255.255");
							printf("this is broadcast!\n");
						}

						sendsock.sin_family = AF_INET;
						sendsock.sin_port = htons(6081);
						////配置完后发送
						ptemp->m_MsgType = MSG_SET_METWORK_RESULT;
						ptemp->m_IsRequest = 0;
						//ptemp->m_Result = 1;
						memcpy(ptemp->m_Content, (char *) psetnet,
								sizeof(SET_NET_PARAM));
						ptemp->sum_check = check_sum((u8 *) ptemp,
								sizeof(MSG_HEADER) - 4);
						fBroadcast = 1;
						setsockopt(sock_broad, SOL_SOCKET, SO_BROADCAST,
								(char*) &fBroadcast, sizeof(BOOL)); //设置为广播模式

						int nRet = sendto(sock_broad, ptemp, nLen, 0,
								(struct sockaddr*) &sendsock, sizeof(sendsock));
						if (nRet < 0)
						{
							fprintf(stderr, "sendto Error:%s\n", strerror(errno));
							printf("send failed!\n");
							break;
						}
						if (ptemp->m_Result == 1)
						{
							if ((mq_changed == 1) || (ip_changed == 1))
							{
								ret = power_down();
								printf(
										"现场配制网络参数完成，等待重新启动. power_down() return %d.\n",
										ret);
							}

						}
						else
						{
							printf("现场配制网络参数设置失败\n");
							continue;
						}
					}
					else
					{
						printf("this is not sending to mine!\n");
						continue;
					}

					break;
				case MSG_SET_NETWORK_PARAM_CENTER:

					DEBUG("中心设置网络参数: the recvsock.sin_addr is %s\n", inet_ntoa(
								recvsock.sin_addr))
						;

					sprintf(ip_old, "%d.%d.%d.%d",
							g_set_net_param.m_NetParam.m_IP[0],
							g_set_net_param.m_NetParam.m_IP[1],
							g_set_net_param.m_NetParam.m_IP[2],
							g_set_net_param.m_NetParam.m_IP[3]);////后期用来替换的//////////
					psetnet = (SET_NET_PARAM *) (ptemp->m_Content);
					if (psetnet->m_IP[0] == g_set_net_param.m_NetParam.m_IP[0]
							&& psetnet->m_IP[1]
							== g_set_net_param.m_NetParam.m_IP[1]
							&& psetnet->m_IP[2]
							== g_set_net_param.m_NetParam.m_IP[2]
							&& psetnet->m_IP[3]
							== g_set_net_param.m_NetParam.m_IP[3])//用来比较是否是发给自己的
					{
						ret = ip_judge(psetnet);
						ptemp->m_Result = 0;
						if (ret == -1)
						{
							char str[100];
							memset(str, 0, sizeof(str));
							sprintf(str, "center set err: MQ IP is %d.%d.%d.%d:%d",
									psetnet->m_NetParam.m_MQ_IP[0],
									psetnet->m_NetParam.m_MQ_IP[1],
									psetnet->m_NetParam.m_MQ_IP[2],
									psetnet->m_NetParam.m_MQ_IP[3],
									psetnet->m_NetParam.m_MQ_PORT);
							//write_err_log_noflash(str);
							DEBUG(str);
						}
						else if (ret == 0)
						{
							char str[100];
							memset(str, 0, sizeof(str));
							sprintf(str,
									"center set : the dev ip is err: %d.%d.%d.%d",
									psetnet->m_NetParam.m_IP[0],
									psetnet->m_NetParam.m_IP[1],
									psetnet->m_NetParam.m_IP[2],
									psetnet->m_NetParam.m_IP[3]);
							//write_err_log_noflash(str);
							DEBUG(str);
						}
						else if (ret == 1)
						{
							ptemp->m_Result = 1;//正确时，给上位机返回成功信息
						}
						if (ptemp->m_Result == 1)
						{
							mq_changed = ip_changed = 0;

							const struct in_addr *new_ip = (const struct in_addr *)psetnet->m_NetParam.m_IP;
							const struct in_addr *new_mask = (const struct in_addr *)psetnet->m_NetParam.m_MASK;
							const struct in_addr *new_gw = (const struct in_addr *)psetnet->m_NetParam.m_GATEWAY;

							const struct in_addr *old_ip = (const struct in_addr *)g_set_net_param.m_NetParam.m_IP;
							const struct in_addr *old_mask = (const struct in_addr *)g_set_net_param.m_NetParam.m_MASK;
							const struct in_addr *old_gw = (const struct in_addr *)g_set_net_param.m_NetParam.m_GATEWAY;

							if (((new_ip->s_addr ^ old_ip->s_addr)
								| (new_mask->s_addr ^ old_mask->s_addr)
								| (new_gw->s_addr ^ old_gw->s_addr)) != 0) {
								ip_changed = 1;
							}

							const struct in_addr *mq_old_ip = (const struct in_addr *)g_set_net_param.m_NetParam.m_MQ_IP;
							const uint16_t mq_old_port = g_set_net_param.m_NetParam.m_MQ_PORT;

							const struct in_addr *mq_new_ip = (const struct in_addr *)psetnet->m_NetParam.m_MQ_IP;
							const uint16_t mq_new_port = psetnet->m_NetParam.m_MQ_PORT;

							if (((mq_new_ip->s_addr ^ mq_old_ip->s_addr)
								 | (mq_new_port ^ mq_old_port)) != 0) {
								mq_changed = 1;
							}

							/*写文件...*/
							memcpy((char*) &g_set_net_param, (char *) psetnet,
									sizeof(SET_NET_PARAM));

							strncpy(online_device.m_strDeviceID,
									psetnet->m_NetParam.m_DeviceID,
									sizeof(online_device.m_strDeviceID));

							get_ftp_chanel(FTP_CHANNEL_CONFIG)->set_config(
									&g_set_net_param.m_NetParam.ftp_param_conf);

							printNetSet();

							// 更新配置文件。 jacky
							write_config_file((char*) SERVER_CONFIG_FILE);
							write_factory_file((char*) FACTORY_CONFIG_FILE);

							if (ip_changed == 1)
							{ //

								const struct in_addr *ip = (const struct in_addr *)g_set_net_param.m_NetParam.m_IP;
								const struct in_addr *mask = (const struct in_addr *)g_set_net_param.m_NetParam.m_MASK;
								const struct in_addr *gw = (const struct in_addr *)g_set_net_param.m_NetParam.m_GATEWAY;

								net_config nc;
								nc.ip = *ip;
								nc.gateway = *gw;
								nc.netmask = *mask;;
								sprintf(nc.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
										g_set_net_param.m_NetParam.m_btMac[0],
										g_set_net_param.m_NetParam.m_btMac[1],
										g_set_net_param.m_NetParam.m_btMac[2],
										g_set_net_param.m_NetParam.m_btMac[3],
										g_set_net_param.m_NetParam.m_btMac[4],
										g_set_net_param.m_NetParam.m_btMac[5]);

								ret = set_net_config(&nc);
								DEBUG("set_net_config() return %d.\n",ret);
							}
						}
						else
						{
							printf("ip_set is failed!\n");
						}
						char *ip_new = inet_ntoa(recvsock.sin_addr);
						if (0 == strncmp(ip_old, ip_new, 10))//判断是否是在同一个域内，是则统认为单播
						{
							sendsock.sin_addr.s_addr = recvsock.sin_addr.s_addr;
							printf("in the same network!\n");
						}
						else
						{
							sendsock.sin_addr.s_addr = inet_addr("255.255.255.255");
							printf("this is broadcast!\n");
						}
						sendsock.sin_port = htons(6081);
						sendsock.sin_family = AF_INET;
						////配置完后发送
						ptemp->m_MsgType = MSG_SET_METWORK_RESULT;
						ptemp->m_IsRequest = 0;
						//ptemp->m_Result = 1;
						memcpy(ptemp->m_Content, (char *) psetnet,
								sizeof(SET_NET_PARAM));
						ptemp->sum_check = check_sum((u8 *) ptemp,
								sizeof(MSG_HEADER) - 4);

						fBroadcast = 0;
						setsockopt(sock_broad, SOL_SOCKET, SO_BROADCAST,
								(char*) &fBroadcast, sizeof(BOOL)); //设置为非广播模式

						int nRet = sendto(sock_broad, ptemp, nLen, 0,
								(struct sockaddr*) &sendsock, sizeof(sendsock));
						if (nRet < 0)
						{
							fprintf(stderr, "Bind Error:%s\n", strerror(errno));
							printf("send failed!\n");
							break;
						}

						if (ptemp->m_Result == 1)
						{
							printf(
									"#############---this is the 4 type send---############\n");
							if ((mq_changed == 1) || (ip_changed == 1))
							{
								ret = power_down();
								log_debug("[ctrl]","中心配制网络参数完成，等待重新启动.  power_down() return %d.\n", ret);
							}
						}
						else
						{
							printf("中心配制网络参数设置失败\n");
							continue;
						}
					}
					else
					{
						printf("this is not sending to mine!\n");
						continue;
					}

					break;
				default:
					break;
			}
		}
		printf("Broadcast reset\n");
		close(sock_broad);
	}
}
/************************************************************/
/*定时器线程，
  1、负责在一定时间内诊断硬盘
  2、发送板卡报警消息
  3、负责管理mq线程销毁及创建
 ************************************************************/
void *timer2ThrFxn(void *arg)
{
	char meminfo_file_name[20] = "/proc/meminfo";
	FILE * meminfo_file = NULL;
	char mem_str[30];
	char mem_use_str[20];
	int total_mem = 0;
	int free_mem = 0;
	int procLoad;
	int alarm_cnt = 0;

	Temp_Detail_t *pTemperature = NULL;

	unsigned long Ep_spendtimes = 0;

	set_thread("timer2ThrFxn");
	for (;;)
	{
		sleep(1);

		print_time("in timer2ThrFxn:  \n");

		if (alarm_cnt % 5 == 0)
		{


			//			DEBUG("循环查看内存情况:\n");
			if (!(meminfo_file = fopen(meminfo_file_name, "rt")))
			{
				printf("err open %s  in ctrl\n", meminfo_file_name);
			}
			else
			{
				memset(mem_str, 0, sizeof(mem_str));
				memset(mem_use_str, 0, sizeof(mem_use_str));

				fread(mem_str, 17, 1, meminfo_file);
				memset(mem_str, 0, sizeof(mem_str));
				fread(mem_str, 9, 1, meminfo_file);
				total_mem = atoi(mem_str);
				fread(mem_str, 17, 1, meminfo_file);
				memset(mem_str, 0, sizeof(mem_str));
				fread(mem_str, 9, 1, meminfo_file);
				free_mem = atoi(mem_str);
				fclose(meminfo_file);

				device_status_return.m_Memory = ((total_mem - free_mem) * 100)
					/ total_mem;
				printf("device_status_return.m_Memory is %d\n",
						device_status_return.m_Memory);
			}

			//5个循环查看cpu占用??
			getArmCpuLoad(&procLoad, (short *) &device_status_return.m_CPU);
			printf("device_status_return.m_CPU= %d\n",
					device_status_return.m_CPU);




			//			pcie_send_cmd_get_DSPload();

#ifndef WITHOUT_SYSSERVE
			//5个循环读温度传感器
			print_time("to get_temp_detail :\n");
			pTemperature = get_temp_detail();
			if (!pTemperature)
			{
				printf("cannot get temperature!\n");
			}
			else
			{
				device_status_return.m_Temperature
					= (int) pTemperature->Temp8147;
				//DEBUG("temperature:%d \n",device_status_return.m_Temperature);

				//every 10 minutes ，wirte temperature to status.log
				static int num_wt=0;
				if(num_wt%120==0)
				{
					printf("num_wt=%d, temperature: %.1f \n",num_wt, pTemperature->Temp8147);
//					log_state("vd_status","temperature: %d \n",device_status_return.m_Temperature);
					log_state("vd_status","temperature: %.1f \n",pTemperature->Temp8147);//
				}
				printf("num_wt=%d, temperature: %.1f \n",num_wt, pTemperature->Temp8147);
				num_wt++;

			}
#endif
			//查看文件系统空间
			device_status_return.m_DiskFree = getFilesystemFree("/opt/ipnc");
			//printf("\n\n\n");

			//计算本程序运行时间
			if (gTimming_flag == 1)
			{
				Ep_spendtimes = device_status_return.m_EpTimes;
				gTimming_flag = 0;
			}
			gettimeofday(&gEP_endTime, NULL);
			device_status_return.m_EpTimes = gEP_endTime.tv_sec
				- gEP_startTime.tv_sec + Ep_spendtimes;
			printf("spend times=%ld m_EpTimes=%ld\n", (long)Ep_spendtimes,
					(long)device_status_return.m_EpTimes);

			//printf("$$$$$$$:%d\n",get_process_stat());
			//errAlarm = 1;
			alarm_cnt = 0;
		}

		alarm_cnt++;

	}
	return NULL;
}

extern "C" {
extern void* LightCtlFun(void* args);
}

void *ctrlThrFxn(void *arg)
{
    set_thread("ctrlThrFxn");
    pthread_t light_ctl_thread;

	if (pthread_create(&light_ctl_thread, NULL, LightCtlFun, NULL)) {
		ERROR("Failed to create light thread\n");
	}

	pthread_t StatusThread;
	pthread_t ftpThread;
	pthread_t broadcastThread;
//	pthread_t ParkUploadThread;


#ifdef VM_e_CHANGZHOU
	pthread_t FillinLightThread;
#endif

#ifdef VM_ElectronicPlate
	pthead_t ElectronicPlateThread;
#endif

	pthread_attr_t attr;

	struct sched_param schedParam;

	if (pthread_attr_init(&attr))
	{
		DEBUG("Failed to initialize thread attrs\n");
	}

	log_state("VD:","Enter ctrl thread\n");

#ifndef PARK_ZEHIN_THIN
	/*******************Create the ftpTransformFxn thread****************/

	schedParam.sched_priority = FTP_THREAD_PRIORITY;

	TRACE_LOG_SYSTEM("to create ftpTransformFxn");
	if (pthread_create(&ftpThread, &attr, ftpTransformFxn, NULL))
	{
		ERROR("Failed to create speech thread\n");
	}
#endif
	TRACE_LOG_SYSTEM("to create timer2ThrFxn");
	if (pthread_create(&StatusThread, &attr, timer2ThrFxn, NULL))
	{
		ERROR("Failed to create timer2ThrFxn thread\n");
		//goto cleanup;
	}
#ifndef PARK_ZEHIN_THIN
	TRACE_LOG_SYSTEM("to create BroadcastThrFxn");
	if (pthread_create(&broadcastThread, &attr, BroadcastThrFxn, NULL))
	{
		log_error((char*)"[ctrl]", (char*)"Failed to create BroadcastThrFxn thread\n");
	}
	TRACE_LOG_SYSTEM("to create LightMonitorThrFxn");

#if (DEV_TYPE != 5)
	//control_light_pthread
	pthread_t MonitorThread;
	if (pthread_create(&MonitorThread, &attr, LightMonitorThrFxn, NULL))

	{
		log_error((char*)"[ctrl]", (char*)"Failed to create BroadcastThrFxn thread\n");
	}
#endif
#endif

	if(2 == DEV_TYPE)
	{

#ifdef VM_e_CHANGZHOU
		TRACE_LOG_SYSTEM("to create FillinLightThrFxn");

		//常州卡口补光灯控制
		if (pthread_create(&FillinLightThread, &attr, FillinLightThrFxn, NULL))
		{
			log_error((char*)"[ctrl]", (char*)"Failed to create FillinLightThrFxn thread\n");
		}
#endif

//#define VM_ElectronicPlate
#ifdef VM_ElectronicPlate
		TRACE_LOG_SYSTEM("to create ElectronicPlateThrFxn");
		//电子车牌信号监控
		if (pthread_create(&ElectronicPlateThread, &attr, ElectronicPlateThrFxn, NULL))
		{
			ERROR("Failed to create ElectronicPlateThrFxn thread\n");
			log_error((char*)"[ctrl]", (char*)"Failed to create ElectronicPlateThrFxn thread\n");
		}
#endif

	}

	pthread_attr_destroy(&attr);

	int rev;
	char command[40];
	memset(command, 0, sizeof(command));

	while (strcasecmp(command, "quit"))
	{
		memset(command, 0, sizeof(command));
		printf("\n>");
		//alive |= CTRL_ALIVE; //如果循环执行，设置存活标志。
		//ctrlAlive = 1; //如果长时间没有命令输入，此标志设置ctrl 线程存活标志
		scanf("%39s", command);
		//ctrlAlive = 0;
		if (strlen(command) > 0)
		{
			rev = ParseCommands(command);

			if (rev == 1)
				break;
		}
		else
		{
			sleep(1);
		}
	}

	pthread_exit(NULL);
}
