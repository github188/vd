/*
 * main.cpp
 *
 *  Created on: 2013-5-7
 *      Author: shanhw
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <inttypes.h>

#include "commonfuncs.h"
#include "ftp.h"
#include "mq_module.h"
#include "global.h"
//#include "pcie.h"
#include "messagefile.h"
#include "ctrl.h"

#include "xmlCreater.h"
#include "xmlParser.h"

#include "dsp_config.h"
#include "arm_config.h"
#include "camera_config.h"
#include "defautConfig.h"
#include "interface.h"
#include "storage/data_process.h"
#include "storage/storage_api.h"
#include "debug.h"
#include "log_interface.h"
#include "commonfuncs.h"
#include "writePidFile.h"

#include <malloc.h>
#include <execinfo.h>

#include <signal.h>
#include "sys_msg_drv.h"

#include "file_msg_drv.h"
#include "others.h"
#include "storage/traffic_records_process.h"
#include "vd_msg_queue.h"
#include "upload.h"
#include "sys/time_util.h"
#include "syslog.h"
#include "toggle.h"
#include "vsparam_pthread.h"
#include "svn_version.h"
#include "logger/log.h"
#include "ve/config/cam_param.h"
#include "sys/signals.h"
#include "ve/dev/led.h"
#include "ve/dev/light.h"
#include "ve/dev/light_ports.h"
#include "sys/config.h"
#include "sys/xfile.h"
#include "ve/ve.h"
#include "oss_interface.h"
#include "park_lock_control.h"
#define VD_VERSION   "v1.0.1"

extern "C"
{
	void ALPU();
}                                    //add by lxd 2014.10.20

ARM_config g_arm_config; 		//arm参数结构体全局变量
Camera_config g_camera_config; 	//camera参数结构体全局变量

char g_SerialNum[50];	      //产品序列号全局变量
char g_SoftwareVersion[50];	   //产品软件版本信息全局变量
extern Online_Device online_device;

SemHandl_t park_sem = NULL;

static int NetParamInit(SysInfo *pSysInfo)
{
	unsigned int netAddr[4];
	char *pStr = NULL;

	static net_config net_cfg;

	net_cfg.ip = pSysInfo->lan_config.net.ip;
	net_cfg.netmask = pSysInfo->lan_config.net.netmask;
	net_cfg.gateway = pSysInfo->lan_config.net.gateway;
	/*alei: modify to get the MAC address */
	memset(net_cfg.mac, '\0', sizeof(net_cfg.mac));
	snprintf(net_cfg.mac, sizeof(net_cfg.mac), "%s",
			 pSysInfo->lan_config.net.MAC);

	INFO("g_set_net_param.m_NetParam.m_DeviceID:%s\n",
	      g_set_net_param.m_NetParam.m_DeviceID);

	pStr = inet_ntoa(net_cfg.ip);
	sscanf(pStr, "%u.%u.%u.%u",
	       &netAddr[0], &netAddr[1], &netAddr[2], &netAddr[3]);
	g_set_net_param.m_NetParam.m_IP[0] = (unsigned char) netAddr[0];
	g_set_net_param.m_NetParam.m_IP[1] = (unsigned char) netAddr[1];
	g_set_net_param.m_NetParam.m_IP[2] = (unsigned char) netAddr[2];
	g_set_net_param.m_NetParam.m_IP[3] = (unsigned char) netAddr[3];
	g_set_net_param.m_IP[0] = (unsigned char) netAddr[0];
	g_set_net_param.m_IP[1] = (unsigned char) netAddr[1];
	g_set_net_param.m_IP[2] = (unsigned char) netAddr[2];
	g_set_net_param.m_IP[3] = (unsigned char) netAddr[3];

	pStr = inet_ntoa(net_cfg.netmask);
	sscanf(pStr, "%u.%u.%u.%u",
	       &netAddr[0], &netAddr[1], &netAddr[2], &netAddr[3]);
	g_set_net_param.m_NetParam.m_MASK[0] = (unsigned char) netAddr[0];
	g_set_net_param.m_NetParam.m_MASK[1] = (unsigned char) netAddr[1];
	g_set_net_param.m_NetParam.m_MASK[2] = (unsigned char) netAddr[2];
	g_set_net_param.m_NetParam.m_MASK[3] = (unsigned char) netAddr[3];

	pStr = inet_ntoa(net_cfg.gateway);
	sscanf(pStr, "%u.%u.%u.%u",
	       &netAddr[0], &netAddr[1], &netAddr[2], &netAddr[3]);
	g_set_net_param.m_NetParam.m_GATEWAY[0] = (unsigned char) netAddr[0];
	g_set_net_param.m_NetParam.m_GATEWAY[1] = (unsigned char) netAddr[1];
	g_set_net_param.m_NetParam.m_GATEWAY[2] = (unsigned char) netAddr[2];
	g_set_net_param.m_NetParam.m_GATEWAY[3] = (unsigned char) netAddr[3];

	g_set_net_param.m_NetParam.m_btMac[0] = strtol(net_cfg.mac, NULL, 16);
	g_set_net_param.m_NetParam.m_btMac[1] = strtol(net_cfg.mac + 3, NULL, 16);
	g_set_net_param.m_NetParam.m_btMac[2] = strtol(net_cfg.mac + 6, NULL, 16);
	g_set_net_param.m_NetParam.m_btMac[3] = strtol(net_cfg.mac + 9, NULL, 16);
	g_set_net_param.m_NetParam.m_btMac[4] = strtol(net_cfg.mac + 12, NULL, 16);
	g_set_net_param.m_NetParam.m_btMac[5] = strtol(net_cfg.mac + 15, NULL, 16);

	INFO("g_set_net_param.m_NetParam.m_DeviceID:%s\n",
	      g_set_net_param.m_NetParam.m_DeviceID);

	INFO("m_NetParam:\n"
		 "ip: %d.%d.%d.%d\n"
		 "MSK: %d.%d.%d.%d\n"
		 "gateway: %d.%d.%d.%d\n",
		 g_set_net_param.m_NetParam.m_IP[0],
		 g_set_net_param.m_NetParam.m_IP[1],
		 g_set_net_param.m_NetParam.m_IP[2],
		 g_set_net_param.m_NetParam.m_IP[3],
		 g_set_net_param.m_NetParam.m_MASK[0],
		 g_set_net_param.m_NetParam.m_MASK[1],
		 g_set_net_param.m_NetParam.m_MASK[2],
		 g_set_net_param.m_NetParam.m_MASK[3],
		 g_set_net_param.m_NetParam.m_GATEWAY[0],
		 g_set_net_param.m_NetParam.m_GATEWAY[1],
		 g_set_net_param.m_NetParam.m_GATEWAY[2],
		 g_set_net_param.m_NetParam.m_GATEWAY[3]);

	return true;
}

//static void Modules_version_init()
//{
//	pcie_send_cmd_get_dsp_ver();
//	pcie_send_cmd_get_ver();
//}


void dev_init()
{

	spend_ms_start();
#if 1
	/*initialize serial com and start red signal detector thread of libinterface.so*/
	Serial_config_t comTemp[3];
	for (int i = 0; i < 3; i++)
	{
		comTemp[i].dev_type
		    = (int) g_arm_config.interface_param.serial[i].dev_type;
		comTemp[i].baudrate = (int) g_arm_config.interface_param.serial[i].bps;
		comTemp[i].parity = (char) g_arm_config.interface_param.serial[i].check;
		comTemp[i].databit = (char) g_arm_config.interface_param.serial[i].data;
		comTemp[i].stopbit = (char) g_arm_config.interface_param.serial[i].stop;
	}

//#ifndef WITHOUT_SYSSERVE
//	print_time("to set_serial_config :\n");
//	ret = set_serial_config(comTemp);
//	if (ret != SUCCESS)
//	{
//		debug("Cannot get serial device configure!\n");
//	}
//	print_time("after set_serial_config .\n");
//#endif

	/*set start time*/
	gettimeofday(&gEP_startTime, NULL);
#endif
	spend_ms_end(__func__);
}


//void storage_init(void)
//{
//	int ret = storage_module_init();
//	debug("storage_module_init() return %d\n ", ret);
//}

void sysserver_init()
{
#ifdef PLATEFORM_ARM
//#ifndef WITHOUT_SYSSERVE
	INFO("in sysserver_init\n\n");
	INFO("initialize appro.");
	Init_Appro();
	INFO("initialize com to sys.");
	Init_Com_To_Sys();
	INFO("initialize com to file.");
	Init_Com_To_File();
//#endif
#endif
}

#if (5 == DEV_TYPE)
static void ve_gpio_init(void)
{
	/* I2c */
	system("insmod /opt/ipnc/pinmux_module.ko a=0x48140924 v=0x20");
	system("rmmod /opt/ipnc/pinmux_module.ko");
	system("insmod /opt/ipnc/pinmux_module.ko a=0x48140928 v=0x20");
	system("rmmod /opt/ipnc/pinmux_module.ko");

	strncpy(gstr_light_gpio.red.name, "gpio73", strlen("gpio73"));
	strncpy(gstr_light_gpio.green.name, "gpio100", strlen("gpio100"));
	strncpy(gstr_light_gpio.enable.name, "gpio99", strlen("gpio99"));

	gstr_light_gpio.enable.turn_on = '1';
	gstr_light_gpio.enable.turn_off = '0';

	gstr_light_gpio.red.turn_on = '1';
	gstr_light_gpio.red.turn_off = '0';

	gstr_light_gpio.green.turn_on = '1';
	gstr_light_gpio.green.turn_off = '0';

	gstr_light_gpio.blue.turn_on = '1';
	gstr_light_gpio.blue.turn_off = '0';

	gi_platform_board = NEW_BOARD;
}
#endif

void gpio_init()
{
	if (((DEV_TYPE == 1) && (flag_parking_lock == 2))) {
		printf("to set lock gpio13,24\n");
		system("insmod /opt/ipnc/pinmux_module.ko a=0x481408a4 v=0x80"); //gpio 13  lock up
		system("rmmod /opt/ipnc/pinmux_module.ko");
		system("insmod /opt/ipnc/pinmux_module.ko a=0x481408D4 v=0x80"); //gpio 24  lock down
		system("rmmod /opt/ipnc/pinmux_module.ko");

		system("echo 13 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio13/direction");
		system("echo 1 > /sys/class/gpio/gpio13/value");
		system("echo 24 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio24/direction");
		system("echo 1 > /sys/class/gpio/gpio24/value");
	}
}

int init(void)
{
	flag_parking_lock = parking_lock_read();
	gpio_init();

#if (1 == DEV_TYPE)
    park_lock_init();
#endif
#if(5 == DEV_TYPE)
	light_ram_init();
	board_light_init();
#endif

    vd_msgueue_init();
    oss_init();
	DEBUG("Init msg queue done.");

	sysserver_init();
	DEBUG("Init sysserver done.");


	SysInfo *pSysInfo = GetSysInfo();
	DEBUG("Get sysinfo done.");

	memset(g_SerialNum,0,sizeof(g_SerialNum));
	memcpy(g_SerialNum,pSysInfo->camera_detail_info.SerialNum,
		   sizeof(pSysInfo->camera_detail_info.SerialNum));
	INFO("g_SerialNum:%s",g_SerialNum);

	memset(g_SoftwareVersion,0,sizeof(g_SoftwareVersion));
	memcpy(g_SoftwareVersion,pSysInfo->camera_detail_info.SoftwareVersion,
		   sizeof(pSysInfo->camera_detail_info.SoftwareVersion));
	INFO("g_SoftwareVersion:%s\n",g_SoftwareVersion);

	init_data_process();
	DEBUG("Init data process done.");


	memset(&g_set_net_param, 0, sizeof(g_set_net_param));
	memset(&g_arm_config, 0, sizeof(g_arm_config));
	memset(&online_device, 0, sizeof(online_device));

	//读取配置文件
	if (get_file_size(SERVER_CONFIG_FILE) <= 0)
	{
		save_file(SERVER_CONFIG_FILE, DEFAULT_ARM_PARAM_XML,
		          sizeof(DEFAULT_ARM_PARAM_XML));
	}
	DEBUG("Read config file done.");

	ReadConfigFile(SERVER_CONFIG_FILE);
	ReadFacFile(FACTORY_CONFIG_FILE);

    //设置网络参数
	if (NetParamInit(pSysInfo) == false)
	{
		ERROR("NetParamInit eror!!\n");
	} else {
		DEBUG("Init net param done.");
	}

	INFO("g_set_net_param.m_NetParam.m_DeviceID:%s\n",
		 g_set_net_param.m_NetParam.m_DeviceID);

    //检查arm_config.xml
	if (check_file_correct(ARM_PARAM_FILE_PATH) < 0)
	{
		int len = sizeof(DEFAULT_ARM_CONFIG_XML);
		char *tempBuf = (char *)malloc(256*1024);
		if(tempBuf != NULL)
		{
			convert_enc("GBK", "UTF-8",
			            (char*) DEFAULT_ARM_CONFIG_XML, len,
			            tempBuf, 256*1024);
			save_file(ARM_PARAM_FILE_PATH, tempBuf, len);
			free(tempBuf);
		}
		else
		{
			ERROR("cannot malloc enough space!\n");
		}
	}

	DEBUG("Check arm_config.xml done.");

    //检查camera_config.xml (现在已经不用camer_config.xml文件)
	if (check_file_correct(CAMERA_PARAM_FILE_PATH) < 0)
	{
		int len = sizeof(DEFAULT_CAMERA_CONFIG_XML);
		char *tempBuf = (char *)malloc(256*1024);
		if(tempBuf != NULL)
		{
			convert_enc("GBK", "UTF-8",
			            (char*) DEFAULT_CAMERA_CONFIG_XML, len,
			            tempBuf, 256*1024);
			save_file(CAMERA_PARAM_FILE_PATH, tempBuf, len);
			free(tempBuf);
		}
		else
		{
			ERROR("cannot malloc enough space!\n");
		}
	}
	DEBUG("Chk camera_config.xml done.");

	//解析arm_config.xml
	parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
	DEBUG("Parse arm_config.xml done.");

	INFO("mq_param:%d.%d.%d.%d:%d", g_arm_config.basic_param.mq_param.ip[0],
                                    g_arm_config.basic_param.mq_param.ip[1],
                                    g_arm_config.basic_param.mq_param.ip[2],
                                    g_arm_config.basic_param.mq_param.ip[3],
                                    g_arm_config.basic_param.mq_param.port);

	//解析camera_config.xml
	parse_xml_doc(CAMERA_PARAM_FILE_PATH, NULL, &g_camera_config, NULL);
	DEBUG("Parse camera_config.xml done");

	if (dsp_init() < 0)
	{
		ERROR("Init dsp failed!");
		return -1;
	}
	DEBUG("Init dsp done.");

	set_log_level(g_arm_config.basic_param.log_level);

	strcpy(online_device.m_strDeviceID, g_set_net_param.m_NetParam.m_DeviceID);
	strcpy(online_device.m_strSpotName, g_arm_config.basic_param.spot);
	strcpy(online_device.m_strVerion, EP_VER);

	/*print_time("to pcie_ipc_buff_init:\n");
	if (0 != pcie_ipc_buff_init())
	{
		log_error("FTP&MQ_MAIN(init)", "error when pcie_ipc_buff_init().");
		return FAILURE;
	}*/

#ifndef PARK_ZEHIN_THIN
	dev_init();
	DEBUG("Init device done.");
	ftp_module_init(); //init and start all ftp channels.
	DEBUG("Init ftp module done.");

	start_mq_thread(); //start mq thread
	DEBUG("Init mq done.");

	start_db_upload_handler_thr();//启动续传线程。
	DEBUG("Init upload done.");

	if(0 != toggle_init()){
		return -1;
	}
	DEBUG("Init toggle done.");
#endif

	return SUCCESS;
}


#ifdef MEM_TRACE
static void* (* old_malloc_hook) (size_t,const void *);
static void (* old_free_hook)(void *,const void *);
static void my_init_hook(void);
static void* my_malloc_hook(size_t,const void*);
static void my_free_hook(void*,const void *);

static void my_init_hook(void)
{
	old_malloc_hook = __malloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = my_malloc_hook;
	__free_hook = my_free_hook;
}

static void* my_malloc_hook(size_t size,const void *caller)
{
	void *result;
	//    print_trace();
	__malloc_hook = old_malloc_hook;
	result = malloc(size);
	old_malloc_hook = __malloc_hook;
	printf("@@@%p+%p 0x%x\n",caller,result,(unsigned long int)size);
	__malloc_hook = my_malloc_hook;

	return result;
}

static void my_free_hook(void *ptr,const void *caller)
{
	__free_hook = old_free_hook;
	free(ptr);
	old_free_hook = __free_hook;
	printf("@@@%p-%p\n",caller,ptr);
	__free_hook = my_free_hook;
}
#endif

static void create_run_dir(const char *dir)
{
	char cmd[PATH_MAX + 256];

	snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir);
	system(cmd);
}

#if (DEV_TYPE == 5)
static void ve_write_version(const char *path)
{
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		ERROR("Open pid file %s failed", path);
		return;
	}

	char version[32];
	ssize_t l = snprintf(version, sizeof(version),
						 "%d.%d.%d.%d\n",
						 VE_MAJOR_VERSION, VE_MINOR_VERSION,
						 VE_REVISION_VERSION, VE_BUILD_VERSION);
	fwrite(version, sizeof(char), l, f);
	fclose(f);
}
#endif

static void write_pid(const char *path)
{
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		ERROR("Open pid file %s failed", path);
		return;
	}

	char pid[32];
	ssize_t l = snprintf(pid, sizeof(pid), "%"PRId64"\n",
						 (int_fast64_t)getpid());
	fwrite(pid, sizeof(char), l, f);
	fclose(f);
}

int main(int argc, char *argv[])
{
	pthread_t ctrlThread;
	pthread_t othersThread;
	pthread_attr_t attr;

	set_thread("main");

	logger_init("vd");
	signals_init();

    /**************VD版本信息**************/
	MODULE_VERSION  version;
	version.Vd_Version = {DEV_TYPE, SVN_VERSION, 7, 100};     //version v1.0.2

	switch(DEV_TYPE)
	{
		case 1:
			strncpy(version.Vd_Version.dev_func_desc, "PARK", strlen("PARK"));
		break;

		case 2:
			strncpy(version.Vd_Version.dev_func_desc, "VM", strlen("VM"));
		break;

		case 3:
			strncpy(version.Vd_Version.dev_func_desc, "VM_P", strlen("VM_P"));
		break;

		case 4:
			strncpy(version.Vd_Version.dev_func_desc, "VP", strlen("VP"));
		break;

		case 5:
			strncpy(version.Vd_Version.dev_func_desc, "VE", strlen("VE"));
		break;
	}
	send_version_info((char *)&version, sizeof(version), MSG_TYPE_MSG17, VERSION_INFO_VD);
	log_state("VD", "VD version is %s v%d.%d.%d_%d\n",
			version.Vd_Version.dev_func_desc,
			version.Vd_Version.master,
			version.Vd_Version.secondary,
			version.Vd_Version.debug,
			version.Vd_Version.interface_num);
	ALERT("*********************************");
	ALERT("%s starting", version.Vd_Version.dev_func_desc);
	ALERT("Build @ %s %s", __DATE__, __TIME__);
	ALERT("Version: %d.%d.%d.%d",
		  version.Vd_Version.master,
		  version.Vd_Version.secondary,
		  version.Vd_Version.debug,
		  version.Vd_Version.interface_num);
	ALERT("*********************************");

	/**************************************/

	park_sem = MakeSem();

	writePidFile("vd");
	create_run_dir(RUN_DIR);
	write_pid(PID_FILE);
#if (DEV_TYPE == 5)
	ve_write_version(VERSION_FILE);
#endif

	if (SUCCESS != init())
	{
		ERROR("main exit.");
		return -1;
	}

	DEBUG("init passed.");

	/************** 创建其他线程任务和控制工作 **************/
 	if (pthread_attr_init(&attr))
	{
		ERROR("Failed to initialize thread attrs\n");
		return FAILURE;
	}
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&ctrlThread, &attr, ctrlThrFxn, NULL))
	{
		ERROR("Failed to create speech thread\n");
	}

	if (pthread_create(&othersThread, &attr, othersThrFxn, NULL))
	{
		ERROR("Failed to create speech thread\n");
	}

	if (pthread_create(&othersThread, &attr, vsparam_pthread, NULL))
	{
		ERROR("Create vsparam_pthread failed!!");
	}

	pthread_attr_destroy(&attr);
    /**********************************************************/
	while (1)
	{
		sleep(600);
	}

    oss_deinit();
	return 0;
}
