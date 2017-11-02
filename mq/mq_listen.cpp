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

extern BASIC_PARAM param_basic;//������mq.cpp
extern NORMAL_FILE_INFO ftp_filePath_down;

int heartBeat; //�ظ�����Ӧ��
int flag_heart_mq; //MQ������־�� 30���ڣ��յ���λ��������
int upLoadParam = 0; //���ز���

int upload_arm_param = 0; //�ϴ�arm����
int upload_dsp_param = 0; //�ϴ�dsp����
int upload_camera_param = 0; //�ϴ�camera����
int upload_hide_param_file = 0; //�ϴ����Բ���
int upload_normal_file = 0; //�ϴ�һ���ļ�
int upload_park_info = 0;  //�ϴ�������Ϣ

int download_arm_param = 0; //����arm����
int download_dsp_param = 0; //����dsp����
int download_camera_param = 0; //����camera����
int download_hide_param_file = 0; //�������Բ���
int download_normal_file = 0; //����һ���ļ�

int g_focus_setup = 0; //�۽�����
int g_iris_setup = 0; // ��Ȧ����

int capturePic = 0; //ץ��ͼƬ  1-->dsp  2-->ftp  3-->mq

int batch_config_arm_param = 0; //��������arm����
int batch_config_dsp_param = 0; //��������dsp����
int batch_config_camera_param = 0; //��������camera����
int batch_config_param = 0;

int g_history_request;
int g_exthistory_start;
int g_exthistory_stop;

SET_NET_PARAM g_set_net_param;
FTP_FILEPATH ftp_filePath_upgrade;
TYPE_HISTORY_RECORD g_history_record;

extern FTP_FILEPATH ftp_filePath; //�ϴ��ļ�
extern FTP_FILEPATH ftp_arm_filePath; //�ϴ�arm�ļ�
extern FTP_FILEPATH ftp_dsp_filePath; //�ϴ�dsp�ļ�
extern FTP_FILEPATH ftp_camera_filePath; //�ϴ�camera�ļ�
extern FTP_FILEPATH ftp_hide_filePath; //�ϴ�hide�ļ�


extern Device_Information device_info;

//#########################################

/*
 * ���ܣ���λ���㲥�Ự,��������;
 * ������
 * ���أ���
 */
void recv_msg_broadcast(const Message *message)
{
	int mq_type = 0;
	char type[3];
	int sta;
	char send_buf[256];

	DEBUG(">>>> in: %s .....\n", __func__);

	mq_type = atoi((message->getCMSType()).c_str());
	DEBUG("��λ���㲥�����Ự�յ���Ϣ >>>>   type=%d,\n", mq_type);

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
			ERROR((char *)"ftp ����ͨ��Ϊ NULL \n");
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
		DEBUG("����ˢ���豸��Ϣ��MQ������..��Ϣ����=%s\n", type)
		;
		sta
				= get_mq_producer_instance(MQ_INSTANCE_BROADCAST)->send_msg_with_property_text(
						send_buf, type, (char*) "IsRequest", 0,
						(char*) "dev_type", (char*) "ephi_2013_800W");
		if (sta == FLG_MQ_TRUE)
		{
			DEBUG("����ˢ���豸��Ϣ��MQ�������ɹ�.\n");
		}
		else
		{
			DEBUG("����ˢ���豸��Ϣ��MQ������ʧ��.\n");
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
			DEBUG("�յ����豸���dev_id=%s,�����豸dev_id=%s\n", str_dev_id,
					online_device.m_strDeviceID);

			if (strcmp(str_dev_id, online_device.m_strDeviceID) != 0) //���Ǳ�����Ϣ��
			{
				DEBUG("���Ǳ��豸����Ϣ,δִ�д��豸����!\n");
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

			DEBUG("ִ�д��豸����...\n");
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
				DEBUG("[�����ϴ��Ự] �Ѿ���!\n");
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
				DEBUG("[�������ػỰ] �Ѿ���!\n");
			}

			if (g_mq_download->get_mq_conn_state() == FLG_MQ_TRUE
					&& get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->get_mq_conn_state()
							== FLG_MQ_TRUE)
			{
				sprintf(type, "%d", MSG_OPEN_DEV_RETURN_TRUE); //���ش��豸�ɹ���Ϣ;
				DEBUG("[�㲥�����Ự]>> �������غ����ػỰ ��������!\n");
			}
			else
			{
				sprintf(type, "%d", MSG_OPEN_DEV_RETURN_FALSE); //���ش��豸ʧ����Ϣ;
				DEBUG("[�㲥�����Ự]>> �������غ����ػỰ ����ʧ��!\n");
			}

			sta
					= get_mq_producer_instance(MQ_INSTANCE_BROADCAST)->send_msg_with_property_text(
							NULL, type, "dev_id", online_device.m_strDeviceID);
			if (sta == FLG_MQ_TRUE)
			{
				DEBUG("���ʹ��豸��Ϣ��MQ�������ɹ�.\n");
			}
			else
			{
				DEBUG("���ʹ��豸��Ϣ��MQ������ʧ��.\n");
			}

		} catch (CMSException& e)
		{
			DEBUG("[�㲥�����Ự] ���մ��豸�����쳣 \n");
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
 * ���ܣ��Ƚ�ftp�����Ƿ���ͬ;
 * ������
 * ���أ���ͬ������1����ͬ����0
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
 * ���ܣ���������֪ͨ
 */
void deal_msg_heart(const Message *message)
{
	flag_heart_mq = 1;
	alarm(30); //30���ȡ����������
	heartBeat = 1;
}

/*
 * ���ܣ��ϴ�arm����
 */
void deal_msg_upload_arm_param(const Message *message)
{
	DEBUG("�Ự�յ���Ϣ:> type= MSG_UPLOAD_ARM_PARAM [�ϴ�arm����֪ͨ] \n");
	upload_arm_param = 1;
}

/*
 * ���ܣ��ϴ�dsp����
 */
void deal_msg_upload_dsp_param(const Message *message)
{
	DEBUG("�Ự�յ���Ϣ:> type= MSG_UPLOAD_DSP_PARAM [�ϴ�dsp����֪ͨ] \n");
	upload_dsp_param = 1;
}

/*
 * ���ܣ��ϴ�camera����
 */
void deal_msg_upload_camera_param(const Message *message)
{
	DEBUG("�Ự�յ���Ϣ:> type= MSG_UPLOAD_CAMERA_PARAM [�ϴ�camera����֪ͨ] \n");
	upload_camera_param = 1;
}

/*
 * ����arm����.
 */
void deal_msg_download_arm_param(const Message *message)
{
	DEBUG("�յ���Ϣ:> type= %d [����arm����] \n", MSG_DOWNLOAD_ARM_PARAM);

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
 * ��������arm����.
 */
void deal_msg_batch_config_arm_param(const Message *message)
{
	DEBUG("�յ���Ϣ:> type= %d [����arm����] \n", MSG_BATCH_CONFIG_ARM);

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
	DEBUG("�յ���Ϣ:> type= %d [�����������ò���] \n", MSG_BATCH_CONFIG);

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
 * ����dsp����
 */
void deal_msg_download_dsp_param(const Message *message)
{
	DEBUG("�������ػỰ�յ���Ϣ:> type= %d [����DSP����] \n", MSG_DOWNLOAD_DSP_PARAM);

	memset(&ftp_dsp_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);

	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_dsp_filePath, sizeof(FTP_FILEPATH)))
	{
		debug_ftp("ftp url:%s��\n",ftp_dsp_filePath.m_strFileURL);

		download_dsp_param = 1;
	}
	else
	{
		download_dsp_param = -1;
	}
}

/*
 * ���س���ʶ�����
 */
void deal_msg_download_plate_param(const Message* message)
{
	//	DEBUG("�������ػỰ�յ���Ϣ:> type= %d [���س���ʶ�����] \n", MSG_DOWNLOAD_PLATE_PARAM);

}

/*
 * ����camera����
 */
void deal_msg_download_camera_param(const Message *message)
{
	DEBUG("�������ػỰ�յ���Ϣ:> type= %d [����CAMERA����] \n", MSG_DOWNLOAD_CAMERA_PARAM);

	memset(&ftp_camera_filePath, 0, sizeof(FTP_FILEPATH));
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);

	if (sizeof(FTP_FILEPATH) == Message->readBytes(
			(unsigned char *) &ftp_camera_filePath, sizeof(FTP_FILEPATH)))
	{
		debug_ftp("ftp url:%s��\n",ftp_camera_filePath.m_strFileURL);

		download_camera_param = 1;
	}
	else
	{
		download_camera_param = -1;
	}
}

/*
 * ץ��ͼƬ,���ؽ��
 */
void deal_msg_capture_picture(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [ץ��ͼƬ] \n", MSG_CAPTURE_PICTURE);
	capturePic = 1;
}


/*
 * �鿴�豸������Ϣ
 */
void deal_msg_device_information(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�鿴�豸������Ϣ] \n", MSG_DEVICE_INFOMATION);

	upLoadDeviceInfo = 1;
	updateDeviceInfo = 1;
	printf("upload param_device_infomation success\n");
}

/*
 * ����
 */
void deal_msg_reboot(const Message *message)
{
	int ret;
	//char type[5];

	DEBUG("�������ػỰ:> type= %d [����] \n", MSG_REBOOT);

	printf("recv reboot\n");
	//TODO: ��������, ֪ͨ��������!!.

	//	// ֪ͨmq�ɹ�/ʧ��
	//	sprintf(type, "%d", MSG_REBOOT_r);
	//	get_mq_producer_instance(MQ_INSTANCE_UPLOAD)->send_msg_with_property_byte(
	//			NULL, 0, type, "Result", 1);

	ret = power_down();
	debug("power_down() return %d. \n",ret);
	CRIT("power down now...");
	//write_err_log_noflash("Զ������\n");
	sleep(1);
}

/*
 * ��ȡ�������õ�ͼƬ
 */
void deal_msg_capture_vehicle_spec(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [��ȡ�������õ�ͼƬ] \n", MSG_CAPTURE_VEHICLE_SPEC);

	printf("\n\nrecv plate config command\n");
	flag_plate_config_to_spi = 2;
	sleep(1);

}
/*
 * �鿴�豸״̬
 */
void deal_msg_device_status(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�鿴�豸״̬] \n", MSG_DEVICE_STATUS);
	upLoadDeviceStatus = 1;
	DEBUG("upload param_device_status success\n");

}
/*
 * �鿴���״̬
 */
void deal_msg_redlamp_status(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�鿴���״̬] \n", MSG_REDLAMP_STATUS);

	if (message->getIntProperty("Extend1") == 1)
		flag_redLamp = 1;
	else
		flag_redLamp = 0;

	printf("upload param_redlamp_status success %d\n", flag_redLamp);
}

/*
 * �豸�����ϴ�
 */
void deal_msg_device_alarm(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�豸�����ϴ�] \n", MSG_DEVICE_ALARM);

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
//		DEBUG("���ھ۽�:�ɹ�\n");
//		set_lens_ctrl(&temp);
//		g_focus_setup = 1;
//	}
//	else
//	{
//		DEBUG("���ھ۽�:ʧ��\n");
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
//		DEBUG("���ڹ�Ȧ:�ɹ�\n");
//		set_lens_ctrl(&temp);
//		g_iris_setup = 1;
//	}
//	else
//	{
//		DEBUG("���ڹ�Ȧ:ʧ��\n");
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
	DEBUG("�����ָ�Ĭ��ֵ\n");

	power_down();
}


/*
 * �豸������־�ϴ�
 */
void deal_msg_device_worklog(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [/�豸������־�ϴ�] \n", MSG_DEVICE_WORKLOG);

	printf("start to accept log\n");
	if (message->getIntProperty("Extend1") == 1)
		flag_log_mq = 1;
	else
		flag_log_mq = 0;
}
/*
 * �豸����
 */
void deal_msg_device_upgrade(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�豸����] \n", MSG_DEVICE_UPGRADE);

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
 * �豸��������
 */
void deal_msg_device_data_clear(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�豸��������] \n", MSG_DEVICE_DATA_CLEAR);

}

/*
 * �鿴��ʷ��¼
 */
void deal_msg_history_record(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�鿴��ʷ��¼] \n", MSG_HISTORY_RECORD);

}

/*
 * �ϴ����β����ļ�
 */
void deal_msg_upload_hide_param_file(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�ϴ������ļ�] \n", MSG_UPLOAD_HIDE_PARAM_FILE);

	upload_hide_param_file = 1;
}

/*
 * �������β����ļ�
 */
void deal_msg_download_hide_param_file(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�������β����ļ�] \n", MSG_DOWNLOAD_HIDE_PARAM_FILE);

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
 * ����һ���ļ�
 */
void deal_msg_download_normal_file(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [����һ���ļ�] \n", MSG_DOWNLOAD_NORMAL_FILE);

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
 * �ϴ�һ���ļ�
 */
void deal_msg_upload_normal_file(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [�ϴ�һ���ļ�] \n", MSG_UPLOAD_NORMAL_FILE);

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
 * NTP ����ʱ��
 */
void deal_msg_set_time(const Message *message)
{
	struct timeval this_time;
	gettimeofday(&this_time, NULL);

	DEBUG("(s:ms,  %d:%ld) �������ػỰ:> type= %ld [����ʱ��] .\n", MSG_SET_TIME, this_time.tv_sec, this_time.tv_usec/1000);

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
 * NTP��ʱ
 */
void deal_msg_get_time(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [��ȡʱ��] \n", MSG_GET_TIME);
	getTime = 1;
}

/*
 * Ӳ�̲���
 */
void deal_msg_hd_operation(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [Ӳ�̲���] \n", MSG_HD_OPERATION);

	//�жϵ�ǰӲ���Ƿ�����ʹ�ã���ʹ�ã���֪ͨ��λ����ѡ��
	const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
	Message->readBytes((unsigned char *) &diskClear, sizeof(Disk_Clear));

	//TODO:Ӳ�̲����ӿ�      //jacky


}

/*
 * ��ѯ��ʷ��¼
 */
void deal_msg_history_request(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [��ѯ��ʷ��¼] \n", MSG_HISTORY_RECORD_REQ);

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
 * ������ʷ��¼
 */
void deal_msg_exthistory_start(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [������ʷ��¼] \n", MSG_EXT_HISTORY_RECORD_START);

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
 * ֹͣ������ʷ��¼
 */
void deal_msg_exthistory_stop(const Message *message)
{
	DEBUG("�������ػỰ:> type= %d [ֹͣ������ʷ��¼] \n", MSG_EXT_HISTORY_RECORD_END);

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
 * PTZ����	//zmh add 20140526
 * ��ʱ����ʹ��mq��Ϣ���Ƿ����ͺ�
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
	DEBUG("�������ػỰ:> type= %d [PTZ CONTROL] \n", MSG_PTZ_CTRL);

	//��ȡPTZ������������ñ�־��
	if(flag_ptz_control==0)
	{
		const BytesMessage* Message = dynamic_cast<const BytesMessage*> (message);
		Message->readBytes((unsigned char *) &ptz_msg, sizeof(PTZ_MSG));

		//TODO:PTZ����
		flag_ptz_control=1;
		DEBUG("�������ػỰ:> [PTZ CONTROL] get ptz msg ok.  ptz_type=%d\n", (int)ptz_msg.ptz_type);
	}
	else
	{
		DEBUG("�������ػỰ:> [PTZ CONTROL] ptz is busy...\n", MSG_PTZ_CTRL);
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
	
	DEBUG("�������ػỰ111 :> [PTZ CONTROL1] get ptz msg ok!!!	ptz_type=%d qPTZId=0x%x\n", (int)msgbuf.Msg_buf.PTZ_Msg.ptz_type,qPTZId);
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
	
	//TODO:PTZ����
	DEBUG("�������ػỰ :> [PTZ CONTROL3] get ptz msg ok!!!	ptz_type=%d qPTZId=0x%x\n", (int)msgbuf.Msg_buf.PTZ_Msg.ptz_type,qPTZId);

}



/*
 * ���ܣ��������ػỰ,��������;
 * ������
 * 		message--�յ�����Ϣ
 * ���أ���
 */
void recv_msg_down(const Message *message)
{
	int mq_type;

	char str_type[128];

	char dev_mq_id[32];
	char dev_id[32];
	char dev_id_ipaddr[32] = {0};
	

	//DEBUG(">>> in (%s)  �������ػỰ�յ���Ϣ >>>> ....  \n", __func__);

	memset(dev_id, 0, sizeof(dev_id));
	memset(dev_mq_id, 0, sizeof(dev_mq_id));
	memset(str_type, 0, sizeof(str_type));

	sprintf(dev_id_ipaddr, "%d.%d.%d.%d", g_set_net_param.m_NetParam.m_IP[0], g_set_net_param.m_NetParam.m_IP[1], g_set_net_param.m_NetParam.m_IP[2], g_set_net_param.m_NetParam.m_IP[3]);

	
	sprintf(dev_mq_id, "%s", message->getCMSCorrelationID().c_str());
	sprintf(dev_id, "%s", g_set_net_param.m_NetParam.m_DeviceID);

	mq_type = atoi((message->getCMSType()).c_str());
	if ((strcmp(dev_id, dev_mq_id) != 0) && (strcmp(dev_id_ipaddr, dev_mq_id) != 0)) //���Ǳ�����Ϣ��
	{
		//DEBUG("�յ�MQ��Ϣ�Ǳ��豸!,���豸id = %s, MQ_id = %s,MQ_type=%d [%s]\n",
		//		dev_id, dev_mq_id, mq_type, str_type);
		return;
	}

	//DEBUG("�յ�MQ��Ϣ!,type=%d \n", mq_type);

	switch (mq_type)
	{
	case MSG_LOOPS:
		deal_msg_heart(message);
		break;
	case MSG_UPLOAD_ARM_PARAM:// �ϴ�arm����
		printf("##############MSG_UPLOAD_ARM_PARAM\n");
		deal_msg_upload_arm_param(message);
		break;
	case MSG_DOWNLOAD_ARM_PARAM:	//����arm����
		printf("##############MSG_DOWNLOAD_ARM_PARAM\n");
		deal_msg_download_arm_param(message);
		break;
	case MSG_BATCH_CONFIG_ARM: //��������ARM
		deal_msg_batch_config_arm_param(message);
		break;

	case MSG_UPLOAD_DSP_PARAM://�ϴ�DSP����
		printf("##############MSG_UPLOAD_DSP_PARAM\n");
		deal_msg_upload_dsp_param(message);
		break;
	case MSG_DOWNLOAD_DSP_PARAM://����DSP����
		printf("##############MSG_DOWNLOAD_DSP_PARAM\n");
		deal_msg_download_dsp_param(message);
		break;
	case MSG_BATCH_CONFIG_DSP://��������dsp
		//��������   �������Ѿ�ȡ��
		break;

	case MSG_UPLOAD_CAMERA_PARAM://�ϴ�camera����
		printf("##############MSG_UPLOAD_CAMERA_PARAM\n");
		deal_msg_upload_camera_param(message);
		break;
	case MSG_DOWNLOAD_CAMERA_PARAM://����camera����
		printf("##############MSG_DOWNLOAD_CAMERA_PARAM\n");
		deal_msg_download_camera_param(message);
		break;
	case MSG_BATCH_CONFIG_CAMERA://��������camera����
		// ��������. �������Ѿ�ȡ��
		break;
	case MSG_BATCH_CONFIG://��������
		deal_msg_batch_config_param(message);
		break;
	case MSG_UPLOAD_HIDE_PARAM_FILE://�ϴ����β����������������β���ֱ�ӷŵ��豸�в���Ҫ�ϴ���
		deal_msg_upload_hide_param_file(message);
		break;
	case MSG_DOWNLOAD_HIDE_PARAM_FILE://�������β���
		deal_msg_download_hide_param_file(message);
		break;

	case MSG_DOWNLOAD_NORMAL_FILE:
		deal_msg_download_normal_file(message);
		break;

	case MSG_UPLOAD_NORMAL_FILE:
		deal_msg_upload_normal_file(message);
		break;

	case MSG_CAPTURE_PICTURE:   //1ץ��ͼƬ
		deal_msg_capture_picture(message);
		break;

	case MSG_DEVICE_INFOMATION:
		deal_msg_device_information(message);
		break;
	case MSG_DEVICE_STATUS:  //1DEVIC_STATUS��Ӧ����������豸״̬���Ҳ���
		deal_msg_device_status(message);
		break;
	case MSG_REDLAMP_STATUS:
		deal_msg_redlamp_status(message);
		break;
	case MSG_DEVICE_ALARM:
		deal_msg_device_alarm(message);
		break;
	case MSG_DEVICE_ALARAM_HISTORY:
		sprintf(str_type, "%s", "�鿴�豸������ʷ");
		break;
	case MSG_DEVICE_WORKLOG:
		sprintf(str_type, "%s", "�豸������־�ϴ�");
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
		sprintf(str_type, "%s", "����У�����");
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

	//DEBUG("�������ػỰ�յ���Ϣ:> type= %d [%s] \n", mq_type, str_type);
}

