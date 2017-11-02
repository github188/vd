/***********************************************************************
 * mq_listen.cpp
 *
 *  Created on: 2013-4-9
 *
 ***********************************************************************/

#include <memory.h>

#include "mq_listen.h"
#include "mq.h"
#include "commonfuncs.h"
#include "ftp.h"
#include "global.h"
#include "mq_module.h"

#include "camera_config.h"
#include "arm_config.h"
#include "dsp_config.h"
#include "xmlParser.h"
#include "messagefile.h"

#include "sysserver/interface.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>

#include "Msg_Def.h"
#include "logger/log.h"

//#########################################

int flag_plate_config_to_spi;

Online_Device online_device;
char online_device_buf[160];

extern BASIC_PARAM param_basic;//定义在mq.cpp
extern NORMAL_FILE_INFO ftp_filePath_down;

int heartBeat; //回复心跳应答
int flag_heart_mq; //MQ心跳标志， 30秒内，收到上位机心跳。
int upLoadParam = 0; //上载参数

int upload_arm_param = 0; //上传arm参数
int upload_dsp_param = 0; //上传dsp参数
int upload_camera_param = 0; //上传camera参数
int upload_hide_param_file = 0; //上传隐性参数
int upload_normal_file = 0; //上传一般文件
int upload_park_info = 0;  //上传泊车信息

int download_arm_param = 0; //下载arm参数
int download_dsp_param = 0; //下载dsp参数
int download_camera_param = 0; //下载camera参数
int download_hide_param_file = 0; //下载隐性参数
int download_normal_file = 0; //下载一般文件

int g_focus_setup = 0; //聚焦控制
int g_iris_setup = 0; // 光圈设置

int capturePic = 0; //抓拍图片  1-->dsp  2-->ftp  3-->mq

int batch_config_arm_param = 0; //批量配置arm参数
int batch_config_dsp_param = 0; //批量配置dsp参数
int batch_config_camera_param = 0; //批量配置camera参数
int batch_config_param = 0;

int g_history_request;
int g_exthistory_start;
int g_exthistory_stop;

SET_NET_PARAM g_set_net_param;
FTP_FILEPATH ftp_filePath_upgrade;
TYPE_HISTORY_RECORD g_history_record;

extern FTP_FILEPATH ftp_filePath; //上传文件
extern FTP_FILEPATH ftp_arm_filePath; //上传arm文件
extern FTP_FILEPATH ftp_dsp_filePath; //上传dsp文件
extern FTP_FILEPATH ftp_camera_filePath; //上传camera文件
extern FTP_FILEPATH ftp_hide_filePath; //上传hide文件


extern Device_Information device_info;

//#########################################

/*
 * 功能：上位机广播会话,监听函数;
 * 参数：
 * 返回：无
 */
void recv_msg_broadcast(const Message *message)
{
	int mq_type = 0;
	char type[3];
	int sta;
	char send_buf[256];

	DEBUG(">>>> in: %s .....\n", __func__);

	mq_type = atoi((message->getCMSType()).c_str());
	DEBUG("上位机广播监听会话收到消息 >>>>   type=%d,\n", mq_type);

	switch (mq_type)
	{
	case MSG_ONLINE_DEVICE:

		memset(send_buf, 0, sizeof(send_buf));
		memset(online_device.m_strFTP_URL, 0,
				sizeof(online_device.m_strFTP_URL));

		TYPE_FTP_CONFIG_PARAM *ftp_conf;
		ftp_conf = get_ftp_chanel(FTP_CHANNEL_CONFIG)->get_config();
		if (NULL == ftp_conf)
		{
			ERROR((char *)"ftp 配置通道为 NULL \n");
			return;
		}

		sprintf(online_device.m_strFTP_URL, "ftp://%s:%s@%d.%d.%d.%d:%d",
				ftp_conf->user, ftp_conf->passwd, ftp_conf->ip[0],
				ftp_conf->ip[1], ftp_conf->ip[2], ftp_conf->ip[3],
				ftp_conf->port);

		sprintf(
				online_device_buf,
				"%s,%s,%d,%s,%s,%d,%d,%d,%d.%d.%d.%d,%s,%d",
				g_set_net_param.m_NetParam.m_DeviceID,
				g_arm_config.basic_param.spot,
				g_arm_config.basic_param.direction,
				online_device.m_strFTP_URL,
				online_device.m_strVerion,
				5,//online_device.m_cVerion.m_wMainVersion,
				4, //online_device.m_cVerion.m_wSecondVersion,
				751,//online_device.m_cVerion.m_wAreaVersion,
				g_set_net_param.m_IP[0], g_set_net_param.m_IP[1],
				g_set_net_param.m_IP[2], g_set_net_param.m_IP[3],
				g_arm_config.basic_param.spot_id,DEV_TYPE);

		printf("online_device_buf:%s \n", online_device_buf);
		convert_enc((char*) "GBK", (char*) "UTF-8", online_device_buf, strlen(
				online_device_buf), send_buf, sizeof(send_buf));
		DEBUG("device IP=%d.%d.%d.%d:%s\n", g_set_net_param.m_IP[0],
				g_set_net_param.m_IP[1],
				g_set_net_param.m_IP[2],
				g_set_net_param.m_IP[3],g_arm_config.basic_param.spot_id)
		;
		sprintf(type, "%d", MSG_ONLINE_RESULT);
		DEBUG("发送刷新设备信息到MQ服务器..消息类型=%s\n", type)
		;
		sta
				= get_mq_producer_instance(MQ_INSTANCE_BROADCAST)->send_msg_with_property_text(
						send_buf, type, (char*) "IsRequest", 0,
						(char*) "dev_type", (char*) "ephi_2013_800W");
		if (sta == FLG_MQ_TRUE)
		{
			DEBUG("发送刷新设备信息到MQ服务器成功.\n");
		}
		else
		{
			DEBUG("发送刷新设备信息到MQ服务器失败.\n");
		}

		break;

	case MSG_OPEN_DEV:
		try
		{
			printf("########################MQ recive MSG_OPEN_DEV message!\n############################");
			char str_dev_id[12];
			std::string dev_id = message->getStringProperty("dev_id");

			memset(str_dev_id, 0, sizeof(str_dev_id));

			sprintf(str_dev_id, "%s", dev_id.c_str());
			DEBUG("收到打开设备命令，dev_id=%s,本机设备dev_id=%s\n", str_dev_id,
					online_device.m_strDeviceID);

			if (strcmp(str_dev_id, online_device.m_strDeviceID) != 0) //不是本机消息。
			{
				DEBUG("不是本设备的消息,未执行打开设备命令!\n");
				for (unsigned int i = 0; i
						< sizeof(online_device.m_strDeviceID); i++)
				{
					DEBUG(
							"online_device.m_strDeviceID[%d] =0x%x, dev_id.c_str(%d)=0x%x,\n",
							i, online_device.m_strDeviceID[i], i, *(str_dev_id
									+ i));
				}
				break;
			}

			DEBUG("执行打开设备命令...\n");
			if (get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->get_mq_conn_state()
					!= FLG_MQ_TRUE)
			{
				for (int i = 0; i < 5; i++)
				{
					if (get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->run()
							!= 1)
					{
						get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->close();
						sleep(1);
						continue;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				DEBUG("[参数上传会话] 已经打开!\n");
			}

			if (g_mq_download->get_mq_conn_state() != FLG_MQ_TRUE)
			{
				for (int i = 0; i < 5; i++)
				{
					if (g_mq_download->run() != FLG_MQ_TRUE)
					{
						g_mq_download->close();
						sleep(1);
						continue;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				DEBUG("[参数下载会话] 已经打开!\n");
			}

			if (g_mq_download->get_mq_conn_state() == FLG_MQ_TRUE
					&& get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->get_mq_conn_state()
							== FLG_MQ_TRUE)
			{
				sprintf(type, "%d", MSG_OPEN_DEV_RETURN_TRUE); //返回打开设备成功消息;
				DEBUG("[广播监听会话]>> 参数上载和下载会话 连接正常!\n");
			}
			else
			{
				sprintf(type, "%d", MSG_OPEN_DEV_RETURN_FALSE); //返回打开设备失败消息;
				DEBUG("[广播监听会话]>> 参数上载和下载会话 连接失败!\n");
			}

			sta
					= get_mq_producer_instance(MQ_INSTANCE_BROADCAST)->send_msg_with_property_text(
							NULL, type, "dev_id", online_device.m_strDeviceID);
			if (sta == FLG_MQ_TRUE)
			{
				DEBUG("发送打开设备信息到MQ服务器成功.\n");
			}
			else
			{
				DEBUG("发送打开设备消息到MQ服务器失败.\n");
			}

		} catch (CMSException& e)
		{
			DEBUG("[广播监听会话] 接收打开设备命令异常 \n");
			e.printStackTrace();
			return;
		}

		break;
	case MSG_CLOSE_DEV:

		break;
	}

	DEBUG("<<< out \n");
}

/*
 * 功能：比较ftp参数是否相同;
 * 参数：
 * 返回：不同，返回1；相同返回0
 */
int ftp_set_is_changed(TYPE_FTP_CONFIG_PARAM ftp_param_1,
		TYPE_FTP_CONFIG_PARAM ftp_param_2)
{
	if (memcmp(ftp_param_1.ip, ftp_param_2.ip, 4) != 0)
	{
		return 1;
	}

	if (ftp_param_1.port != ftp_param_2.port)
	{
		return 1;
	}
	if (ftp_param_1.allow_anonymous != ftp_param_2.allow_anonymous)
	{
		return 1;
	}
	if (ftp_param_1.allow_anonymous == 0)
	{
		if (strcasecmp(ftp_param_1.user, ftp_param_2.user))
		{
			return 1;
		}
		if (strcasecmp(ftp_param_1.passwd, ftp_param_2.passwd))
		{
			return 1;
		}
	}

	return 0;
}

/*
 * 功能：上载心跳通知
 */
void deal_msg_heart(const Message *message)
{
	flag_heart_mq = 1;
	alarm(30); //30秒后取消发送心跳
	heartBeat = 1;
}

/*
 * 功能：上传arm参数
 */
void deal_msg_upload_arm_param(const Message *message)
{
	DEBUG("会话收到消息:> type= MSG_UPLOAD_ARM_PARAM [上传arm参数通知] \n");
	upload_arm_param = 1;
}

/*
 * 功能：上传dsp参数
 */
void deal_msg_upload_dsp_param(const Message *message)
{
	DEBUG("会话收到消息:> type= MSG_UPLOAD_DSP_PARAM [上传dsp参数通知] \n");
	upload_dsp_param = 1;
}

/*
 * 功能：上传camera参数
 */
void deal_msg_upload_camera_param(const Message *message)
{
	DEBUG("会话收到消息:> type= MSG_UPLOAD_CAMERA_PARAM [上传camera参数通知] \n");
	upload_camera_param = 1;
}

/*
 * 下载arm参数.
 */
void deal_msg_download_arm_param(const Message *message)
{
	DEBUG("收到消息:> type= %d [下载arm参数] \n", MSG_DOWNLOAD_ARM_PARAM);

	memset(&ftp_arm_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_arm_filePath, sizeof(FTP_FILEPATH)))
	{
		DEBUG("arm_config.xml path: %s \n", ftp_arm_filePath.m_strFileURL);
		download_arm_param = 1;
	}
	else
	{
		download_arm_param = -1;
	}
}

/*
 * 批量配置arm参数.
 */
void deal_msg_batch_config_arm_param(const Message *message)
{
	DEBUG("收到消息:> type= %d [下载arm参数] \n", MSG_BATCH_CONFIG_ARM);

	memset(&ftp_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_filePath, sizeof(FTP_FILEPATH)))
	{
		DEBUG("batch config arm.  xml path: %s \n", ftp_filePath.m_strFileURL);
		batch_config_arm_param = 1;
	}
	else
	{
		batch_config_arm_param = -1;
	}
}
void deal_msg_batch_config_param(const Message *message)
{
	DEBUG("收到消息:> type= %d [下载批量配置参数] \n", MSG_BATCH_CONFIG);

	memset(&ftp_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_filePath, sizeof(FTP_FILEPATH)))
	{
		DEBUG("batch config xml path: %s \n", ftp_filePath.m_strFileURL);
		batch_config_param = 1;
	}
	else
	{
		batch_config_param = -1;
	}
}

/*
 * 下载dsp参数
 */
void deal_msg_download_dsp_param(const Message *message)
{
	DEBUG("参数下载会话收到消息:> type= %d [下载DSP参数] \n", MSG_DOWNLOAD_DSP_PARAM);

	memset(&ftp_dsp_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);

	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_dsp_filePath, sizeof(FTP_FILEPATH)))
	{
		debug_ftp("ftp url:%s！\n",ftp_dsp_filePath.m_strFileURL);

		download_dsp_param = 1;
	}
	else
	{
		download_dsp_param = -1;
	}
}

/*
 * 下载车牌识别参数
 */
void deal_msg_download_plate_param(const Message* message)
{
	//	DEBUG("参数下载会话收到消息:> type= %d [下载车牌识别参数] \n", MSG_DOWNLOAD_PLATE_PARAM);

}

/*
 * 下载camera参数
 */
void deal_msg_download_camera_param(const Message *message)
{
	DEBUG("参数下载会话收到消息:> type= %d [下载CAMERA参数] \n", MSG_DOWNLOAD_CAMERA_PARAM);

	memset(&ftp_camera_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);

	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_camera_filePath, sizeof(FTP_FILEPATH)))
	{
		debug_ftp("ftp url:%s！\n",ftp_camera_filePath.m_strFileURL);

		download_camera_param = 1;
	}
	else
	{
		download_camera_param = -1;
	}
}

/*
 * 抓拍图片,返回结果
 */
void deal_msg_capture_picture(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [抓拍图片] \n", MSG_CAPTURE_PICTURE);
	capturePic = 1;
}


/*
 * 查看设备基本信息
 */
void deal_msg_device_information(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [查看设备基本信息] \n", MSG_DEVICE_INFOMATION);

	upLoadDeviceInfo = 1;
	updateDeviceInfo = 1;
	printf("upload param_device_infomation success\n");
}

/*
 * 重启
 */
void deal_msg_reboot(const Message *message)
{
	int ret;
	//char type[5];

	DEBUG("参数下载会话:> type= %d [重启] \n", MSG_REBOOT);

	printf("recv reboot\n");
	//TODO: 重启流程, 通知其他进程!!.

	//	// 通知mq成功/失败
	//	sprintf(type, "%d", MSG_REBOOT_r);
	//	get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
	//			NULL, 0, type, "Result", 1);

	ret = power_down();
	debug("power_down() return %d. \n",ret);
	CRIT("power down now...");
	//write_err_log_noflash("远程重启\n");
	sleep(1);
}

/*
 * 获取车牌配置的图片
 */
void deal_msg_capture_vehicle_spec(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [获取车牌配置的图片] \n", MSG_CAPTURE_VEHICLE_SPEC);

	printf("\n\nrecv plate config command\n");
	flag_plate_config_to_spi = 2;
	sleep(1);

}
/*
 * 查看设备状态
 */
void deal_msg_device_status(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [查看设备状态] \n", MSG_DEVICE_STATUS);
	upLoadDeviceStatus = 1;
	DEBUG("upload param_device_status success\n");

}
/*
 * 查看红灯状态
 */
void deal_msg_redlamp_status(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [查看红灯状态] \n", MSG_REDLAMP_STATUS);

	if (message->getIntProperty("Extend1") == 1)
		flag_redLamp = 1;
	else
		flag_redLamp = 0;

	printf("upload param_redlamp_status success %d\n", flag_redLamp);
}

/*
 * 设备报警上传
 */
void deal_msg_device_alarm(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [设备报警上传] \n", MSG_DEVICE_ALARM);

	printf("start to accept alarm\n");
	if (message->getIntProperty("Extend1") == 1)
		flag_alarm_mq = 1;
	else
		flag_alarm_mq = 0;

}

//void deal_msg_focus_adjust(const Message *message)
//{
//	Camera_info temp;
//	int ret;
//
//	memset(&temp, 0, sizeof(Camera_info));
//	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
//	ret = Message->readBytes((unsigned char *) &temp, sizeof(temp));
//	if (ret == sizeof(Camera_info))
//	{
//		DEBUG("调节聚焦:成功\n");
//		set_lens_ctrl(&temp);
//		g_focus_setup = 1;
//	}
//	else
//	{
//		DEBUG("调节聚焦:失败\n");
//		g_focus_setup = -1;
//	}
//
//}
//
//void deal_msg_iris_set(const Message *message)
//{
//
//	Camera_info temp;
//	int ret;
//
//	memset(&temp, 0, sizeof(Camera_info));
//	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
//	ret = Message->readBytes((unsigned char *) &temp, sizeof(temp));
//	if (ret == sizeof(Camera_info))
//	{
//		DEBUG("调节光圈:成功\n");
//		set_lens_ctrl(&temp);
//		g_iris_setup = 1;
//	}
//	else
//	{
//		DEBUG("调节光圈:失败\n");
//		g_iris_setup = -1;
//	}
//}

void deal_msg_factory_control(const Message *message)
{
	DEBUG("remove file <%s> <%s> <%s>\n",ARM_PARAM_FILE_PATH,DSP_PARAM_FILE_PATH,CAMERA_PARAM_FILE_PATH);
	remove(ARM_PARAM_FILE_PATH);
	remove(CAMERA_PARAM_FILE_PATH);
	remove(DSP_PARAM_FILE_PATH);
	remove(SERVER_CONFIG_FILE);
	DEBUG("重启恢复默认值\n");

	power_down();
}


/*
 * 设备工作日志上传
 */
void deal_msg_device_worklog(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [/设备工作日志上传] \n", MSG_DEVICE_WORKLOG);

	printf("start to accept log\n");
	if (message->getIntProperty("Extend1") == 1)
		flag_log_mq = 1;
	else
		flag_log_mq = 0;
}
/*
 * 设备升级
 */
void deal_msg_device_upgrade(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [设备升级] \n", MSG_DEVICE_UPGRADE);

	if (upgrade == 0)
	{
		memset(&ftp_filePath_upgrade, 0, sizeof(FTP_FILEPATH));
		const BytesMessage* Message =
				dynamic_cast<const BytesMessage*> (message);

		Message->readBytes((unsigned char *) &ftp_filePath_upgrade,
				sizeof(ftp_filePath_upgrade));
		upgrade = 1;
	}
}

/*
 * 设备数据清理
 */
void deal_msg_device_data_clear(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [设备数据清理] \n", MSG_DEVICE_DATA_CLEAR);

}

/*
 * 查看历史记录
 */
void deal_msg_history_record(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [查看历史记录] \n", MSG_HISTORY_RECORD);

}

/*
 * 上传隐形参数文件
 */
void deal_msg_upload_hide_param_file(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [上传隐形文件] \n", MSG_UPLOAD_HIDE_PARAM_FILE);

	upload_hide_param_file = 1;
}

/*
 * 下载隐形参数文件
 */
void deal_msg_download_hide_param_file(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [下载隐形参数文件] \n", MSG_DOWNLOAD_HIDE_PARAM_FILE);

	memset(&ftp_hide_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_hide_filePath, sizeof(FTP_FILEPATH)))
	{
		download_hide_param_file = 1;
	}
	else
	{
		char text[100];
		sprintf(text, "(%s|%d)get config ftp channel failed.\n", __FUNCTION__,
				__LINE__);
		log_debug((char*)"MQ",text);
		download_hide_param_file = -1;
	}
}
/*
 * 下载一般文件
 */
void deal_msg_download_normal_file(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [下载一般文件] \n", MSG_DOWNLOAD_NORMAL_FILE);

	memset(&ftp_filePath_down, 0, sizeof(NORMAL_FILE_INFO));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(NORMAL_FILE_INFO) == Message->readBytes(
			(unsigned char *) &ftp_filePath_down, sizeof(NORMAL_FILE_INFO)))
	{
		download_normal_file = 1;
	}
	else
	{
		char text[100];
		sprintf(text, "(%s|%d)get config ftp channel failed.\n", __FUNCTION__,
				__LINE__);
		log_debug((char*)"MQ",text);
		download_normal_file = -1;
	}
}
/*
 * 上传一般文件
 */
void deal_msg_upload_normal_file(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [上传一般文件] \n", MSG_UPLOAD_NORMAL_FILE);

	memset(&ftp_filePath_up, 0, sizeof(NORMAL_FILE_INFO));

	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	if (sizeof(NORMAL_FILE_INFO) == Message->readBytes(
			(unsigned char *) &ftp_filePath_up, sizeof(NORMAL_FILE_INFO)))
	{
		debug("the m_strDeviceFilePath is %s\n",
				ftp_filePath_up.m_strDeviceFilePath);

		upload_normal_file = 1;
	}
	else
	{
		char text[100];
		sprintf(text, "(%s|%d)get config ftp channel failed.\n", __FUNCTION__,
				__LINE__);
		log_debug((char*)"MQ",text);
		upload_normal_file = -1;
	}
}

/*
 * NTP 设置时间
 */
void deal_msg_set_time(const Message *message)
{
	struct timeval this_time;
	gettimeofday(&this_time, NULL);

	DEBUG("(s:ms,  %d:%ld) 参数下载会话:> type= %ld [设置时间] .\n", MSG_SET_TIME, this_time.tv_sec, this_time.tv_usec/1000);

	NTP_Device_Time t;

	memset(&t, 0, sizeof(NTP_Device_Time));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	printf("the time is %ld\n", time(NULL));

	Message->readBytes((unsigned char *) &t, sizeof(t));
	printf("the recv ntpTime.m_tDeviceTime =%ld, ntpTime.m_Reserve =%s\n",
			t.m_tDeviceTime, t.m_Reserve);

	//gettimeofday(&gEP_endTime, NULL);
	//stime(&(t.m_tDeviceTime));
	//system("hwclock");
	spend_ms_start();
	int ret = set_current_time(t.m_tDeviceTime);
	spend_ms_end((char*) "sysserver set_current_time()");

	if (SUCCESS == ret)

	{
		gettimeofday(&gEP_endTime, NULL);
		gettimeofday(&gEP_startTime, NULL);
		setTime = 1;
		gTimming_flag = 1;
	}
	else
	{
		setTime = -1;
	}

}

/*
 * NTP对时
 */
void deal_msg_get_time(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [获取时间] \n", MSG_GET_TIME);
	getTime = 1;
}

/*
 * 硬盘操作
 */
void deal_msg_hd_operation(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [硬盘操作] \n", MSG_HD_OPERATION);

	//判断当前硬盘是否正在使用，若使用，则通知上位机做选择。
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	Message->readBytes((unsigned char *) &diskClear, sizeof(Disk_Clear));

	//TODO:硬盘操作接口      //jacky


}

/*
 * 查询历史记录
 */
void deal_msg_history_request(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [查询历史记录] \n", MSG_HISTORY_RECORD_REQ);

	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	int ret = Message->readBytes((unsigned char *) &g_history_record,
			sizeof(TYPE_HISTORY_RECORD));
	if (ret == sizeof(TYPE_HISTORY_RECORD))
	{
		g_history_request = 1;
	}
	else
	{
		g_history_request = -1;
	}
}

/*
 * 导出历史记录
 */
void deal_msg_exthistory_start(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [导出历史记录] \n", MSG_EXT_HISTORY_RECORD_START);

	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	int ret = Message->readBytes((unsigned char *) &g_history_record,
			sizeof(TYPE_HISTORY_RECORD));
	if (ret == sizeof(TYPE_HISTORY_RECORD))
	{
		g_exthistory_start = 1;
	}
	else
	{
		g_exthistory_start = -1;
	}
}

/*
 * 停止导出历史记录
 */
void deal_msg_exthistory_stop(const Message *message)
{
	DEBUG("参数下载会话:> type= %d [停止导出历史记录] \n", MSG_EXT_HISTORY_RECORD_END);

	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	int ret = Message->readBytes((unsigned char *) &g_history_record,
			sizeof(TYPE_HISTORY_RECORD));
	if (ret == sizeof(TYPE_HISTORY_RECORD))
	{
		g_exthistory_stop = 1;
	}
	else
	{
		g_exthistory_stop = -1;
	}
}




/*
 * PTZ操作	//zmh add 20140526
 * 即时控制使用mq消息，是否有滞后？
 */
 #if 0
int Send_vd_cmd(char *buffer)
{
	DOWNLOAD_MSG msgbuf;

	static int flag_first=1;
	static int qPTZId=0;
	int num_buffer;
	PTZ_MSG *test_ptr;
	
	if(flag_first==1)
	{
		flag_first=0;
		qPTZId = Msg_Init(PAR_MSG_KEY);
		
		ptz_msg_ptr.start = (char *)ptz_control_msg;
		ptz_msg_ptr.end   = &ptz_control_msg[PTZ_PC_CMD_NUM-1] + sizeof(PTZ_MSG);
		ptz_msg_ptr.read  = ptz_control_msg;
		ptz_msg_ptr.write = ptz_control_msg;
		ptz_msg_ptr.flag  = 0x55aa55aa;
	}
	printf("start: 0x%x,end: 0x%x write: 0x%x read: 0x%x sizeof(PTZ_MSG):%d\n",ptz_msg_ptr.start,ptz_msg_ptr.end,ptz_msg_ptr.write,ptz_msg_ptr.read,sizeof(PTZ_MSG));

	if(ptz_msg_ptr.read < ptz_msg_ptr.write)
	{
		
		num_buffer = ((char *)(ptz_msg_ptr.write) - (char *)(ptz_msg_ptr.read))/sizeof(PTZ_MSG);
		printf("num_buffer1 is %d\n",num_buffer);
		num_buffer = PTZ_PC_CMD_NUM - num_buffer;
		printf("num_buffer2 is %d\n",num_buffer);
	}
	else if(ptz_msg_ptr.read > ptz_msg_ptr.write)
	{		
		
		num_buffer = ((char *)(ptz_msg_ptr.read) - (char *)(ptz_msg_ptr.write))/sizeof(PTZ_MSG);
		printf("num_buffer3 is %d\n",num_buffer);
	}
	else
	{
		num_buffer = PTZ_PC_CMD_NUM;
		printf("num_buffer4 is %d\n",num_buffer);
	}

	printf("num_buffer is %d\n",num_buffer);
	if(num_buffer == 1)
	{
		printf("ptz message buffer full\n");
		return -1;
	}
	else
	{
		printf("before write++:0x%x\n",ptz_msg_ptr.write);
		ptz_msg_ptr.write++;
		printf("after write++:0x%x\n",ptz_msg_ptr.write);
		if(ptz_msg_ptr.write > ptz_msg_ptr.end)
		{
			ptz_msg_ptr.write = ptz_msg_ptr.start;
		}
		memcpy(ptz_msg_ptr.write,buffer,sizeof(PTZ_MSG));
		test_ptr = (PTZ_MSG *)buffer;
		printf("ptz action1 is %d %d\n",(ptz_msg_ptr.write)->ptz_type,test_ptr->ptz_type);
	}
	
	memset(&msgbuf,0,sizeof(MSG_BUF));
	msgbuf.Des = MSG_TYPE_MSG18;
	msgbuf.Src = MSG_TYPE_MSG20; 
//	msgbuf.cmd = cmd;
    msgbuf.ret = 0;
	

	printf("call send_vd_cmd\n");
    return Msg_Send( qPTZId, &msgbuf, sizeof(MSG_BUF));//sizeof(ArithmOutput)
}
#endif

void deal_msg_ptz_operation(const Message *message)
{
#if 0
	DEBUG("参数下载会话:> type= %d [PTZ CONTROL] \n", MSG_PTZ_CTRL);

	//获取PTZ控制命令，并设置标志。
	if(flag_ptz_control==0)
	{
		const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
		Message->readBytes((unsigned char *) &ptz_msg, sizeof(PTZ_MSG));

		//TODO:PTZ操作
		flag_ptz_control=1;
		DEBUG("参数下载会话:> [PTZ CONTROL] get ptz msg ok.  ptz_type=%d\n", (int)ptz_msg.ptz_type);
	}
	else
	{
		DEBUG("参数下载会话:> [PTZ CONTROL] ptz is busy...\n", MSG_PTZ_CTRL);
	}
#endif
	DOWNLOAD_MSG msgbuf;

	static int flag_first=1;
	static int qPTZId=0;

	if(flag_first==1)
	{
		flag_first=0;
		qPTZId = Msg_Init(PAR_MSG_KEY);	
	}
	
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	
	DEBUG("参数下载会话111 :> [PTZ CONTROL1] get ptz msg ok!!!	ptz_type=%d qPTZId=0x%x\n", (int)msgbuf.Msg_buf.PTZ_Msg.ptz_type,qPTZId);
	//ssage->readBytes((unsigned char *) &(lstr_ptz_msg), sizeof(lstr_ptz_msg));
	Message->readBytes((unsigned char *) &(msgbuf.Msg_buf.PTZ_Msg), sizeof(PTZ_MSG));

	msgbuf.Des = MSG_TYPE_MSG18;
	msgbuf.Src = MSG_TYPE_MSG20;
	msgbuf.Cmd = DOWNLOAD_PTZ;

#if 0
	switch(msgbuf.Msg_buf.PTZ_Msg.ptz_type)
	{	
		case 1:
			msgbuf.Cmd = SYS_SENSOR_ISP_CONTROL;
			msgbuf.Sensor_isp_buf.Ispctr_param.isp_type = e_isp_lens;
			msgbuf.Sensor_isp_buf.Ispctr_param.lensParam.lens_type = e_lens;
			msgbuf.Sensor_isp_buf.Ispctr_param.lensParam.param_flag = focusNear_Flag;
		break;

		case 3:
			
		break;

		case 4:
		break;
	}
#endif

    Msg_Send(qPTZId, &msgbuf, sizeof(DOWNLOAD_MSG));
	
	//TODO:PTZ操作
	DEBUG("参数下载会话 :> [PTZ CONTROL3] get ptz msg ok!!!	ptz_type=%d qPTZId=0x%x\n", (int)msgbuf.Msg_buf.PTZ_Msg.ptz_type,qPTZId);

}



/*
 * 功能：参数下载会话,监听函数;
 * 参数：
 * 		message--收到的消息
 * 返回：无
 */
void recv_msg_down(const Message *message)
{
	int mq_type;

	char str_type[128];

	char dev_mq_id[32];
	char dev_id[32];
	char dev_id_ipaddr[32] = {0};
	

	//DEBUG(">>> in (%s)  参数下载会话收到消息 >>>> ....  \n", __func__);

	memset(dev_id, 0, sizeof(dev_id));
	memset(dev_mq_id, 0, sizeof(dev_mq_id));
	memset(str_type, 0, sizeof(str_type));

	sprintf(dev_id_ipaddr, "%d.%d.%d.%d", g_set_net_param.m_NetParam.m_IP[0], g_set_net_param.m_NetParam.m_IP[1], g_set_net_param.m_NetParam.m_IP[2], g_set_net_param.m_NetParam.m_IP[3]);

	
	sprintf(dev_mq_id, "%s", message->getCMSCorrelationID().c_str());
	sprintf(dev_id, "%s", g_set_net_param.m_NetParam.m_DeviceID);

	mq_type = atoi((message->getCMSType()).c_str());
	if ((strcmp(dev_id, dev_mq_id) != 0) && (strcmp(dev_id_ipaddr, dev_mq_id) != 0)) //不是本机消息。
	{
		//DEBUG("收到MQ消息非本设备!,本设备id = %s, MQ_id = %s,MQ_type=%d [%s]\n",
		//		dev_id, dev_mq_id, mq_type, str_type);
		return;
	}

	//DEBUG("收到MQ消息!,type=%d \n", mq_type);

	switch (mq_type)
	{
	case MSG_LOOPS:
		deal_msg_heart(message);
		break;
	case MSG_UPLOAD_ARM_PARAM:// 上传arm参数
		printf("##############MSG_UPLOAD_ARM_PARAM\n");
		deal_msg_upload_arm_param(message);
		break;
	case MSG_DOWNLOAD_ARM_PARAM:	//下载arm参数
		printf("##############MSG_DOWNLOAD_ARM_PARAM\n");
		deal_msg_download_arm_param(message);
		break;
	case MSG_BATCH_CONFIG_ARM: //批量配置ARM
		deal_msg_batch_config_arm_param(message);
		break;

	case MSG_UPLOAD_DSP_PARAM://上传DSP参数
		printf("##############MSG_UPLOAD_DSP_PARAM\n");
		deal_msg_upload_dsp_param(message);
		break;
	case MSG_DOWNLOAD_DSP_PARAM://下载DSP参数
		printf("##############MSG_DOWNLOAD_DSP_PARAM\n");
		deal_msg_download_dsp_param(message);
		break;
	case MSG_BATCH_CONFIG_DSP://批量配置dsp
		//批量配置   该命令已经取消
		break;

	case MSG_UPLOAD_CAMERA_PARAM://上传camera参数
		printf("##############MSG_UPLOAD_CAMERA_PARAM\n");
		deal_msg_upload_camera_param(message);
		break;
	case MSG_DOWNLOAD_CAMERA_PARAM://下载camera参数
		printf("##############MSG_DOWNLOAD_CAMERA_PARAM\n");
		deal_msg_download_camera_param(message);
		break;
	case MSG_BATCH_CONFIG_CAMERA://批量配置camera参数
		// 批量配置. 该命令已经取消
		break;
	case MSG_BATCH_CONFIG://批量配置
		deal_msg_batch_config_param(message);
		break;
	case MSG_UPLOAD_HIDE_PARAM_FILE://上传隐形参数，但是现在隐形参数直接放到设备中不需要上传了
		deal_msg_upload_hide_param_file(message);
		break;
	case MSG_DOWNLOAD_HIDE_PARAM_FILE://下载隐形参数
		deal_msg_download_hide_param_file(message);
		break;

	case MSG_DOWNLOAD_NORMAL_FILE:
		deal_msg_download_normal_file(message);
		break;

	case MSG_UPLOAD_NORMAL_FILE:
		deal_msg_upload_normal_file(message);
		break;

	case MSG_CAPTURE_PICTURE:   //1抓拍图片
		deal_msg_capture_picture(message);
		break;

	case MSG_DEVICE_INFOMATION:
		deal_msg_device_information(message);
		break;
	case MSG_DEVICE_STATUS:  //1DEVIC_STATUS对应于配置软件设备状态的右侧栏
		deal_msg_device_status(message);
		break;
	case MSG_REDLAMP_STATUS:
		deal_msg_redlamp_status(message);
		break;
	case MSG_DEVICE_ALARM:
		deal_msg_device_alarm(message);
		break;
	case MSG_DEVICE_ALARAM_HISTORY:
		sprintf(str_type, "%s", "查看设备报警历史");
		break;
	case MSG_DEVICE_WORKLOG:
		sprintf(str_type, "%s", "设备工作日志上传");
		break;
	case MSG_DEVICE_UPGRADE:
		deal_msg_device_upgrade(message);
		break;
	case MSG_DEVICE_DATA_CLEAR:
		deal_msg_device_data_clear(message);
		break;
	case MSG_HISTORY_RECORD:
		//deal_msg_history_record(message);
		break;
//	case MSG_FOCUS_ADJUST:
//		deal_msg_focus_adjust(message);
//		break;
//	case MSG_IRIS_SET:
//		deal_msg_iris_set(message);
//		break;

	case MSG_FACTORY_CONTROL:
		//deal_msg_factory_control(message);
		break;

	case MSG_GET_TIME:
		deal_msg_get_time(message);
		break;
	case MSG_SET_TIME:
		deal_msg_set_time(message);
		break;
	case MSG_HD_OPERATION:
		deal_msg_hd_operation(message);
		break;

	case MSG_PTZ_CTRL:
		deal_msg_ptz_operation(message);
		break;

	case MSG_PARAM_ERR:
		sprintf(str_type, "%s", "参数校验错误");
		break;

	case MSG_REBOOT:
		deal_msg_reboot(message);
		break;
#ifdef _HISTORY_RECORD_
	case MSG_HISTORY_RECORD_REQ:
		deal_msg_history_request(message);
		break;
	case MSG_EXT_HISTORY_RECORD_START:
		deal_msg_exthistory_start(message);
		break;
	case MSG_EXT_HISTORY_RECORD_END:
		deal_msg_exthistory_stop(message);
		break;
#endif
	default:
		break;
	}

	//DEBUG("参数下载会话收到消息:> type= %d [%s] \n", mq_type, str_type);
}

