/***********************************************************************
 * mq_send.cpp
 *
 *  Created on: 2013-4-9
 *
 ***********************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <string>


#include <signal.h>
#include <sys/wait.h>
#include <sys/vfs.h>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/core/ActiveMQConnection.h>
#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Integer.h>
#include <activemq/util/Config.h>
#include <decaf/util/Date.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <iconv.h>
#include  <locale.h>
#include <json/json.h>   //add by lxd
#include "global.h"
#include "messagefile.h"
#include "ep_type.h"
#include "ctrl.h"
//#include "pcie.h"
#include "mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h"
#include "sysserver/interface.h"
#include "mq_send.h"
#include "mq_listen.h"
#include "commonfuncs.h"
#include "mq_module.h"
#include "ftp.h"
#include "arm_config.h"
#include "dsp_config.h"
#include "camera_config.h"
#include "storage/data_process.h"
#include "h264_buffer.h"
#include "logger/log.h"
#include <file_msg_drv.h>
#include "Msg_Def.h"

int register_park = 0;
#define UPDATE_TRY_TIME 3

extern SemHandl_t park_sem;
//#########################################//#########################################//
//extern char store_name[100];
//extern int outBufSize_enc;

extern pthread_mutex_t mutex_mq;
extern pthread_mutex_t mutex_db;
extern pthread_cond_t cond_broadcast;
extern pthread_mutex_t mutex_broadcast;
//extern int flag_broadcast;
//extern SCapture_Vehicle_Spec file_path;

extern pthread_mutex_t mutex_spi_to_upgrade;
//extern int flag_spi_to_upgrade;
extern pthread_cond_t cond_spi_to_upgrade;

//int uart_open_err;
//int alg_para_config_flag = 0; //参数配置失败

extern int heartBeat; //回复心跳应答
extern int flag_heart_mq; //MQ心跳标志
extern int upLoadParam; //上载参数

extern int download_arm_thread_running ; //下载arm参数线程运行状态.

extern FTP_FILEPATH ftp_filePath;
extern FTP_FILEPATH ftp_arm_filePath; //上传arm文件
extern FTP_FILEPATH ftp_dsp_filePath; //上传dsp文件
extern FTP_FILEPATH ftp_camera_filePath; //上传camera文件
extern FTP_FILEPATH ftp_hide_filePath; //上传hide文件

extern int g_flg_mq_start; //MQ会话启动完成.		1:完成; 0: 未完成;
extern Device_Information device_info;
extern Device_Status_Return device_status_return;
extern TYPE_HISTORY_RECORD g_history_record;
extern SET_NET_PARAM g_set_net_param;
extern FTP_FILEPATH ftp_filePath_upgrade;
extern TYPE_HISTORY_RECORD g_history_record;

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

extern int batch_config_arm_param; //批量配置arm参数
extern int batch_config_param;

extern int g_focus_setup; //聚焦控制
extern int g_iris_setup; // 光圈设置

extern int g_history_request;
extern int g_exthistory_start;
extern int g_exthistory_stop;


extern class_mq_producer *g_mq_park_upload;
extern int upload_park_info;


/************************************************************************************
 Function:	MQ  消息发送线程
 Description:
 Parameters:  Null
*************************************************************************************/
void *thread_mq_send(void *arg)
{
	char type[5];
	NTP_Device_Time ntpTime;

	set_thread("mq_send");
	DEBUG("start thread_mq_send.....\n");

	upload_park_info = 1;
	while (1)
	{
		if(g_flg_mq_start) {
			//#########################################/上传参数信息/#########################################//

			/*
			   上位机双击连接板卡之后，arm开始通过ftp上传arm,dsp,cameral的xml文件，上传成功后，将upload_arm_param,
			   upload_dsp_param,upload_camera_param置为2，然后进入以下的if语句内，通过MQ服务器，将ftp服务器的路径发送给
			   上位机软件
			//上传xml配置文件反馈。  2:成功， -1:失败
			*/

			//##################### // #####################//

			if ((2 == upload_arm_param) || (-1 == upload_arm_param))
			{

				DEBUG("upload arm param ...upload_arm_param=%d  ..MQ上传参数信息!\n",upload_arm_param);
				DEBUG("MQ消息内容: 设备编号=%s,安装地点编号=%s \n",
						g_set_net_param.m_NetParam.m_DeviceID, g_arm_config.basic_param.spot
					 );

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_UPLOAD_ARM_PARAM_r);

				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_arm_filePath, sizeof(ftp_arm_filePath), type,
						"Result", upload_arm_param == -1 ? 0 : 1, NULL, 0);

				if (upload_arm_param == 2)
				{
					DEBUG("upload_arm_param ..成功!\n");
				}
				else
				{
					DEBUG("upload_arm_param..失败!\n");
				}

				upload_arm_param = 0;

				usleep(100);
			}

			if ((2 == download_arm_param) || (-1 == download_arm_param)) // -1:失败， 2:成功
			{
				DEBUG("download arm param .....MQ下载参数信息!\n");
				DEBUG("MQ消息内容: 设备编号=%s,安装地点编号=%s \n",
						g_set_net_param.m_NetParam.m_DeviceID, g_arm_config.basic_param.spot
					 );

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DOWNLOAD_ARM_PARAM_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_arm_filePath, sizeof(ftp_arm_filePath), type,
						"Result", download_arm_param == -1 ? 0 : 1, NULL, 0);

				if (download_arm_param == 2)
				{
					DEBUG("download arm param..MQ下载参数成功!\n");
				}
				else
				{
					DEBUG("download arm param..MQ下载参数失败!\n");
				}

				download_arm_param = 0;
				download_arm_thread_running = 0; //下载arm参数线程运行状态.

				usleep(100);
			}


			//##################### // #####################//


			if ((2 == upload_dsp_param) || (-1 == upload_dsp_param))
			{
				DEBUG("upload dsp param .....\nMQ上传参数信息!");
				DEBUG("MQ消息内容: 设备编号=%s,安装地点编号=%s \n",
						g_set_net_param.m_NetParam.m_DeviceID, g_arm_config.basic_param.spot
					 );

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_UPLOAD_DSP_PARAM_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_dsp_filePath, sizeof(ftp_dsp_filePath), type,
						"Result", upload_dsp_param == -1 ? 0 : 1, NULL, 0);

				if (upload_dsp_param == 2)
				{
					DEBUG("download dsp param..MQ上传参数成功!\n");
				}
				else
				{
					DEBUG("download dsp param..MQ上传参数失败!\n");
				}
				upload_dsp_param = 0;
				printf("after send upload_dsp_param:%d\n",upload_dsp_param);
			}


			if ((2 == download_dsp_param) || (-1 == download_dsp_param)) // -1:失败， 2:成功
			{
				DEBUG("download dsp param .....\nMQ下载参数信息!");
				DEBUG("MQ消息内容: 设备编号=%s,安装地点编号=%s \n",
						g_set_net_param.m_NetParam.m_DeviceID, g_arm_config.basic_param.spot
					 );

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DOWNLOAD_DSP_PARAM_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_dsp_filePath, sizeof(ftp_dsp_filePath), type,
						"Result", download_dsp_param == -1 ? 0 : 1, NULL, 0);
				if (download_dsp_param == 2)
				{
					DEBUG("download_dsp_param..成功!\n");
				}
				else
				{
					DEBUG("download_dsp_param..失败!\n");
				}
				download_dsp_param = 0;
			}


			//##################### // #####################//


			if ((2 == upload_camera_param) || (-1 == upload_camera_param))
			{
				printf("[in] upload_camera_param%d\n", upload_camera_param);
				DEBUG("upload camera param .....\nMQ上传参数信息!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_UPLOAD_CAMERA_PARAM_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_camera_filePath, sizeof(ftp_camera_filePath), type,
						"Result", upload_camera_param == -1 ? 0 : 1, NULL, 0);

				if (upload_camera_param == 2)
				{
					DEBUG("upload camera param..MQ上传参数成功!\n");
				}
				else
				{
					DEBUG("upload camera param..MQ上传参数失败!\n");
				}
				upload_camera_param = 0;
			}


			if ((2 == download_camera_param) || (-1 == download_camera_param)) // -1:失败， 2:成功
			{
				DEBUG("download camera param .....\nMQ下载参数信息!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DOWNLOAD_CAMERA_PARAM_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_camera_filePath, sizeof(ftp_camera_filePath), type,
						"Result", download_camera_param == -1 ? 0 : 1, NULL, 0);

				if (download_camera_param == 2)
				{
					DEBUG("download_camera_param..成功!\n");
				}
				else
				{
					DEBUG("download_camera_param..失败!\n");
				}
				download_camera_param = 0;
			}


			//##################### // #####################//


			if ((2 == upload_hide_param_file) || (-1 == upload_hide_param_file)) // -1:失败， 2:成功
			{
				DEBUG("upload_hide_param_file .....\nMQ上传隐形参数!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_UPLOAD_HIDE_PARAM_FILE_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_hide_filePath, sizeof(ftp_hide_filePath), type,
						"Result", upload_hide_param_file == -1 ? 0 : 1, NULL, 0);

				if (upload_hide_param_file == 2)
				{
					DEBUG("upload_hide_param_file..成功!\n");
				}
				else
				{
					DEBUG("upload_hide_param_file..失败!\n");
				}
				upload_hide_param_file = 0;
			}

			if ((2 == download_hide_param_file) || (-1 == download_hide_param_file)) // -1:失败， 2:成功
			{

				printf("[in] download_hide_param_file%d\n",  download_hide_param_file);
				DEBUG("download_hide_param_file .....\nMQ下载隐性参数信息!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DOWNLOAD_HIDE_PARAM_FILE_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_hide_filePath, sizeof(ftp_hide_filePath), type,
						"Result", download_hide_param_file == -1 ? 0 : 1, NULL, 0);

				if (download_hide_param_file == 2)
				{
					DEBUG("download_hide_param_file..成功!\n");
				}
				else
				{
					DEBUG("download_hide_param_file..失败!\n");
				}
				download_hide_param_file = 0;
			}


			//##################### // #####################//


			if ((2 == upload_normal_file) || (-1 == upload_normal_file)) // -1:失败， 2:成功
			{

				DEBUG("upload normal file .....\nMQ上传一般文件!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_UPLOAD_NORMAL_FILE_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_filePath, sizeof(ftp_filePath), type, "Result",
						upload_normal_file == -1 ? 0 : 1, NULL, 0);

				if (upload_normal_file == 2)
				{
					DEBUG("upload_normal_file..成功!\n");
				}
				else
				{
					DEBUG("upload_normal_file..失败!\n");
				}
				upload_normal_file = 0;
			}

			if ((2 == download_normal_file) || (-1 == download_normal_file))
			{

				DEBUG("download normal file .....\nMQ下载一般文件!");
				DEBUG("MQ消息内容: 设备编号=%s,安装地点编号=%s \n",
						g_set_net_param.m_NetParam.m_DeviceID, g_arm_config.basic_param.spot
					 );

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DOWNLOAD_NORMAL_FILE_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_filePath, sizeof(ftp_filePath), type, "Result",
						download_normal_file == -1 ? 0 : 1, NULL, 0);

				if (download_normal_file == 2)
				{
					DEBUG("download_normal_file..成功!\n");
				}
				else
				{
					DEBUG("download_normal_file..失败!\n");
				}
				download_normal_file = 0;
			}


			//##################### /capturePic/ #####################//


			if ((2 == capturePic) || (-1 == capturePic))
			{
				DEBUG("抓图回应!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_CAPTURE_PICTURE_r);
				int ret = get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_filePath, sizeof(ftp_filePath), type, "Result",
						capturePic == -1 ? 0 : 1, NULL, 0);
				if(ret != FLG_MQ_TRUE)
				{
					ERROR("false when send_msg_with_property_byte().\n");
				}
				if (capturePic == 2)
				{
					DEBUG("抓图.成功!\n");
				}
				else
				{
					DEBUG("抓图.失败!\n");
				}
				capturePic = 0;
			}


			//##################### /upgrade/#####################//

			if ((2 == upgrade) || (-1 == upgrade))
			{
				DEBUG("升级回应!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_DEVICE_UPGRADE_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_filePath, sizeof(ftp_filePath), type, "Result",
						upgrade == -1 ? 0 : 1, NULL, 0);

				if (upgrade == 2)
				{
					DEBUG("升级.成功!\n");
				}
				else
				{
					DEBUG("升级.失败!\n");
				}
				upgrade = 0;
			}


			/*批量配置*/
			//##################### / batch_config_param/#####################//

			if ((2 == batch_config_param) || (-1 == batch_config_param))
			{
				DEBUG("升级回应!");

				// 通知mq成功/失败
				sprintf(type, "%d", MSG_BATCH_CONFIG_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						&ftp_filePath, sizeof(ftp_filePath), type, "Result",
						batch_config_param == -1 ? 0 : 1, NULL, 0);

				if (batch_config_param == 2)
				{
					DEBUG("批量配置.成功!\n");
				}
				else
				{
					DEBUG("批量配置.失败!\n");
				}
				batch_config_param = 0;
			}


			//#####################/通过MQ 服务器 上传设备基本信息(Device_Information结构体)/#####################//

			if ((upLoadDeviceInfo == 2) || (updateDeviceInfo == 1))
			{
				printf("upLoadDeviceInfo = %d,updateDeviceInfo=%d\n",
						upLoadDeviceInfo, updateDeviceInfo);
				sprintf(type, "%d", MSG_DEVICE_INFOMATION_r);

				//spend_ms_start();
				//		pcie_send_cmd_get_dsp_ver();                          //获取DSP软件版本，注释掉by lxd  2014.12.02
				//		pcie_send_cmd_get_ver();                              //获取PCIE 版本，注释掉by lxd 2014.12.02
				//spend_ms_end((char*)"pci_send_cmd_get_xx_ver()");


				//############/设置违法FPT通道状态/############//

				if (get_ftp_chanel(FTP_CHANNEL_ILLEGAL)->get_status() == STA_FTP_OK)
				{
					device_info.ftp_sta_illegal = 0;

				}
				else if (get_ftp_chanel(FTP_CHANNEL_ILLEGAL)->getServer_is_full())
				{
					device_info.ftp_sta_illegal = 3;
				}
				else if (get_ftp_chanel(FTP_CHANNEL_ILLEGAL)->get_status()
						== STA_FTP_EXP)
				{
					device_info.ftp_sta_illegal = 2;
				}
				else
				{
					device_info.ftp_sta_illegal = 1;
				}

				//############/设置过车FPT通道状态/############//

				if (get_ftp_chanel(FTP_CHANNEL_PASSCAR)->get_status() == STA_FTP_OK)
				{
					device_info.ftp_sta_pass_car = 0;

				}
				else if (get_ftp_chanel(FTP_CHANNEL_PASSCAR)->getServer_is_full())
				{
					device_info.ftp_sta_pass_car = 3;
				}
				else if (get_ftp_chanel(FTP_CHANNEL_PASSCAR)->get_status()
						== STA_FTP_EXP)
				{
					device_info.ftp_sta_pass_car = 2;
				}
				else
				{
					device_info.ftp_sta_pass_car = 1;
				}

				//############/设置H264 FTP通过状态/############//

				if (get_ftp_chanel(FTP_CHANNEL_H264)->get_status() == STA_FTP_OK)
				{
					device_info.ftp_sta_h264 = 0;

				}
				else if (get_ftp_chanel(FTP_CHANNEL_H264)->getServer_is_full())
				{
					device_info.ftp_sta_h264 = 3;
				}
				else if (get_ftp_chanel(FTP_CHANNEL_H264)->get_status()
						== STA_FTP_EXP)
				{
					device_info.ftp_sta_h264 = 2;
				}
				else
				{
					device_info.ftp_sta_h264 = 1;
				}

				DEBUG("上传设备基本信息...\n");

				/*modules version*/
				//spend_ms_start();
				if(4 == DEV_TYPE)
				{
					device_info.preset_index = get_prepos_station();
				}

				// 注释掉 by lxd 2014.12.02
#if 0
				Version_Detail *pVer = get_version_detail();
				//	spend_ms_end((char*)"get_version_detail()");
				if(!pVer)
				{
					printf("get_version_detail failed \n");
				}
				else
				{
					strncpy(device_info.hw_ver,pVer->HdVer,strlen(pVer->HdVer));
					strncpy(device_info.hw_uuid,pVer->serialnum,strlen(device_info.hw_uuid));
					strncpy(device_info.fpga_ver,pVer->FPGAVer,strlen(pVer->FPGAVer));
					strncpy(device_info.mcu_ver,pVer->McuVersion,strlen(pVer->McuVersion));
					strncpy(device_info.arm_sysserver_ver,pVer->SysServerVer,strlen(pVer->SysServerVer));
				}
				strncpy(device_info.ep_ver,EP_VER,strlen(EP_VER));
				device_info.fan_status = get_fan_status();

				//spend_ms_start();
				int ret = get_version_h264_pack(device_info.arm_log_ver,
						sizeof(device_info.arm_log_ver));
				//spend_ms_end((char*)"get_version_h264_pack()");
				if (ret < 0)
				{
					printf("get_version_h264_pack failed \n");
				}

				printf("fpga_ver = %s\n\
						mcu_ver = %s\narm_sysserver_ver = %s\n",\
						device_info.fpga_ver,device_info.mcu_ver,device_info.arm_sysserver_ver);
				printf("arm_pcie_ver = %s\nep_ver = %s\n\
						arm_log_ver = %s\ndsp_ver = %s\n",device_info.arm_pcie_ver,\
						device_info.ep_ver,device_info.arm_log_ver,device_info.dsp_ver.acVersion);
#endif

				//重新定义各个模块版本的信息，通过新的方式获得模块版本,add by lxd 201.12.02
				MODULE_VERSION_MSG*  ver;
				ver  = GetVersionInfo();



				char ver_boa[32];
				sprintf(ver_boa,"%d.%d.%d_%d",ver->Module_version_Msg.Boa_Version.master,ver->Module_version_Msg.Boa_Version.secondary,
						ver->Module_version_Msg.Boa_Version.debug,ver->Module_version_Msg.Boa_Version.interface_num);

				printf("##############################Boa:%s\n",ver_boa);

				char ver_dsp[32];
				sprintf(ver_dsp,"%d.%d.%d_%d",ver->Module_version_Msg.Dsp_Version.master,ver->Module_version_Msg.Dsp_Version.secondary,
						ver->Module_version_Msg.Dsp_Version.debug,ver->Module_version_Msg.Dsp_Version.interface_num);

				printf("##############################dsp:%s\n",ver_dsp);

				char ver_mcfw[32];
				sprintf(ver_mcfw,"%d.%d.%d_%d",ver->Module_version_Msg.Mcfw_Out_Version.master,ver->Module_version_Msg.Mcfw_Out_Version.secondary,
						ver->Module_version_Msg.Mcfw_Out_Version.debug,ver->Module_version_Msg.Mcfw_Out_Version.interface_num);

				printf("##############################mcfw:%s\n",ver_mcfw);

				char ver_vpss[32];
				sprintf(ver_vpss,"%d.%d.%d_%d",ver->Module_version_Msg.Vpss_Version.master,ver->Module_version_Msg.Vpss_Version.secondary,
						ver->Module_version_Msg.Vpss_Version.debug,ver->Module_version_Msg.Vpss_Version.interface_num);

				printf("##############################vpss:%s\n",ver_vpss);

				char ver_sysserver[32];
				sprintf(ver_sysserver,"%d.%d.%d_%d",ver->Module_version_Msg.Sys_Server_Version.master,ver->Module_version_Msg.Sys_Server_Version.secondary,
						ver->Module_version_Msg.Sys_Server_Version.debug,ver->Module_version_Msg.Sys_Server_Version.interface_num);

				printf("##############################server:%s\n",ver_sysserver);

				char ver_vd[32];
				sprintf(ver_vd,"%d.%d.%d_%d",ver->Module_version_Msg.Vd_Version.master,ver->Module_version_Msg.Vd_Version.secondary,
						ver->Module_version_Msg.Vd_Version.debug,ver->Module_version_Msg.Vd_Version.interface_num);

				printf("##############################vd:%s\n",ver_vd);

				strncpy(device_info.ver_boa,ver_boa,strlen(ver_boa));
				strncpy(device_info.ver_dsp,ver_dsp,strlen(ver_dsp));
				strncpy(device_info.ver_mcfw,ver_mcfw,strlen(ver_mcfw));
				strncpy(device_info.ver_vpss,ver_vpss,strlen(ver_vpss));
				strncpy(device_info.ver_sysserver,ver_sysserver,strlen(ver_sysserver));
				strncpy(device_info.ver_vd,ver_vd,strlen(ver_vd));


				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &device_info, sizeof(device_info), type,
						"IsRequest", 0, "Result", 1);
				if (upLoadDeviceInfo == 2)
				{
					upLoadDeviceInfo = 0;
				}
				if (updateDeviceInfo == 1)
				{
					updateDeviceInfo = 0;
				}
				usleep(10000);
			}

			//#####################/上传设备状态信息/#####################//

			if (upLoadDeviceStatus)
			{

				sprintf(type, "%d", MSG_DEVICE_STATUS_r);

				DEBUG("上传设备状态信息\n");
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &device_status_return,
						sizeof(device_status_return), type, "IsRequest", 0,
						"Result", 1);

				upLoadDeviceStatus = 0;
			}

			//#####################/上传设备时间/#####################//

			if (getTime == 1)
			{
				time(&(ntpTime.m_tDeviceTime));
				sprintf(type, "%d", MSG_GET_TIME_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &ntpTime, sizeof(ntpTime), type, "Result", 1,
						NULL, 0);

				getTime = 0;
				usleep(10000);
			}

			//##########################################//

			if (setTime == 1 || setTime == -1)
			{
				struct timeval this_time;
				gettimeofday(&this_time, NULL);
				debug("setTime = %d, (s:ms)==(%ld:%ld)\n",setTime, this_time.tv_sec,this_time.tv_usec/1000);

				time(&(ntpTime.m_tDeviceTime));
				sprintf(type, "%d", MSG_SET_TIME_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &ntpTime, sizeof(ntpTime), type, "Result", setTime==-1?0:1,
						NULL, 0);

				gettimeofday(&this_time, NULL);
				debug("setTime send to mq, (s:ms)==(%ld:%ld)\n", this_time.tv_sec,this_time.tv_usec/1000);

				setTime = 0;
				usleep(1000);
			}

			if (g_iris_setup != 0)
			{
				sprintf(type, "%d", MSG_IRIS_SET_r);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &ntpTime, sizeof(ntpTime), type, "Result", g_iris_setup==-1?0:1,
						NULL, 0);

				g_iris_setup = 0;
			}

#ifdef _HISTORY_RECORD_
			if (g_history_request != 0)
			{
				TYPE_HISTORY_RECORD_RESULT result;
				sprintf(type, "%d", MSG_HISTORY_RECORD_REQ_ACK);
				result.count_record = get_record_count(&g_history_record);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &result, sizeof(TYPE_HISTORY_RECORD_RESULT), type, "Result", g_history_request ==-1?0:1,
						NULL, 0);

				g_history_request = 0;
			}

			if (g_exthistory_start != 0)
			{
				sprintf(type, "%d", MSG_EXT_HISTORY_RECORD_START_ACK);
				ret = start_upload_history_record(&g_history_record);
				if(ret < 0)
				{
					g_exthistory_start = -1;
				}
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &g_history_record, sizeof(TYPE_HISTORY_RECORD), type, "Result", g_exthistory_start==-1?0:1,
						NULL, 0);
				g_exthistory_start = 0;
			}

			if (g_exthistory_stop != 0)
			{
				sprintf(type, "%d", MSG_EXT_HISTORY_RECORD_END_ACK);
				ret = stop_upload_history_record();
				if(ret < 0)
				{
					g_exthistory_stop = -1;
				}
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						(char *) &g_history_record, sizeof(TYPE_HISTORY_RECORD), type, "Result", g_exthistory_stop==-1?0:1,
						NULL, 0);
				g_exthistory_stop = 0;
			}
#endif

			if (heartBeat == 1)
			{
				sprintf(type, "%d", MSG_LOOPS);
				get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
						NULL, 0, type, NULL, 0, NULL, 0);
				heartBeat = 0;
			}

		}

		//##################/泊车系统信息上传/#################//

		//if (1 == upload_park_info && 1 == DEV_TYPE && g_flg_mq_start_songli == 1)
		//{

		//	log_send(LOG_LEVEL_STATUS,0,"VD: ","upload park register");
		//	NET_PARAM netparam;
		//	memcpy((char*) &netparam,
		//			(char*) &(g_set_net_param.m_NetParam),
		//			sizeof(NET_PARAM));
		//	char ipAddr[16];
		//	//		char direction[8];
		//	char collector[32];
		//	printf("My id is %d.%d.%d.%d\n",netparam.m_IP[0],netparam.m_IP[1],netparam.m_IP[2],netparam.m_IP[3]);
		//	sprintf(ipAddr,"%d.%d.%d.%d",netparam.m_IP[0],netparam.m_IP[1],netparam.m_IP[2],netparam.m_IP[3]);
		//	//		sprintf(direction,"%d",EP_DIRECTION);


		//	snprintf(collector,
		//			EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);


		//	Json::Value child;
		//	Json::Value root;
		//	Json::FastWriter fast_writer;

		//	child["category"] = 4;                                                            // 1.电警  2.卡口  3.违停  4.泊车
		//	child["collector"] =  collector;                                        // 采集机关代码
		//	child["direction"] =  (int)EP_DIRECTION;                                          //行驶方向
		//	child["ipAddr"] = ipAddr;                                                           //设备IP
		//	child["version"] = g_SoftwareVersion;

		//	root["cmdCode"] = 1;
		//	root["devId"] =  g_SerialNum; //产品序列号
		//	root["psId"] = g_arm_config.basic_param.spot_id;                                           	              //泊位编号
		//	root["data"] = child;
		//	root["maker"] = "bitcom";



		//	std::string  result_json = fast_writer.write(root);
		//	std::cout<<"park_register:"<<result_json<<std::endl;

		//	TRACE_LOG_SYSTEM("1111111111111Park WUYOU register !!!!!!");

		//	if (upload_park_info == 1)
		//	{

		//		SemWait(park_sem);
		//		while(FLG_MQ_FALSE == get_mq_producer_instance(MQ_INSTANCE_PARK_UPLOAD)->get_mq_conn_state())
		//		{
		//			while (g_mq_park_upload->run() != 1)
		//			{
		//				g_mq_park_upload->close();
		//				sleep(5);
		//			}
		//		}


		//		get_mq_producer_instance(MQ_INSTANCE_PARK_UPLOAD)->send_msg_text_string(
		//				result_json);

		//		g_mq_park_upload->close();
		//		SemRelease(park_sem);


		//		upload_park_info = 0;

		//		register_park = 1;

		//		TRACE_LOG_SYSTEM("Park WUYOU register !!!!!!");

		//		log_send(LOG_LEVEL_STATUS,0,"VD:","park register finished!");

		//	}

		//}

		//#############################################################

		usleep(1000);

	} //end of while

	pthread_exit(NULL);
}

