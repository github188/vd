
#include <stdlib.h>
#include <string.h>

#include "storage_common.h"
#include "ftp.h"
#include "upload.h"
#include "ep_type.h"
#include "commonfuncs.h"
#include "mq_module.h"
#include "data_process.h"
#include "logger/log.h"


/*******************************************************************************
 * 函数名: ftp_get_status
 * 功  能: 获取FTP状态
 * 参  数: ftp_channel，FTP通道
 * 返回值: 连接正常，返回0；其他，返回-1
*******************************************************************************/
int ftp_get_status(int ftp_channel)
{
	CLASS_FTP *ftp = NULL;
	ftp = get_ftp_chanel(ftp_channel);
	if (NULL == ftp)
	{

		return -1;
	}

	int ftp_status = ftp->get_status();

	if (ftp_status != STA_FTP_OK)
	{
		TRACE_LOG_SYSTEM("FTP connect is failed! ftp_channel:%d, errornum:%d", ftp_channel, ftp_status);

		return -1;
	}

	//TRACE_LOG_SYSTEM("FTP connect is successful!");

	return 0;
}


/*******************************************************************************
 * 函数名: ftp_send_pic_buf
 * 功  能: 通过FTP上传图片缓存
 * 参  数: pic_info，文件信息
 * 返回值:
*******************************************************************************/
int ftp_send_pic_buf(const EP_PicInfo *pic_info, int ftp_channel)
{
	int li_i = 0;
	int li_count = 0;
	int li_stauts = 0;
	CLASS_FTP * ftp;
	ftp = get_ftp_chanel(ftp_channel);
	if (NULL == ftp)
	{
		return -1;
	}

	if(SUCCESS != check_jpeg((char *)(pic_info->buf), pic_info->size))
	{
		debug("[EP]" "----- image  check failure !!! ----------\n");
	}



	for(li_i=0; li_i<3; li_i++)
	{
		li_stauts = ftp->ftp_put_data((char (*)[80])pic_info->path, (char *)pic_info->name, (char *)pic_info->buf, pic_info->size);
		if (SUCCESS != li_stauts)
		{
			li_count ++;

			TRACE_LOG_SYSTEM("FTP send datas is %d failed__stauts=%d, pic_name=%s, pic_size=%d", li_count, li_stauts, pic_info->name, pic_info->size);

			if(li_count == 3)
			{
				return -1;
			}
		}
		else
		{
			break;
		}
	}

	TRACE_LOG_SYSTEM("FTP send datas is successful", li_count);

	return 0;
}


/*******************************************************************************
 * 函数名: ftp_send_pic_file
 * 功  能: 通过FTP上传图片文件
 * 参  数:
 * 返回值:
*******************************************************************************/
int ftp_send_pic_file(const char *src_file_name, EP_PicInfo *pic_info,
                      int ftp_channel)
{
	resume_print("In ftp_send_pic_file\n");
	CLASS_FTP * ftp;
	ftp = get_ftp_chanel(ftp_channel);
	if (NULL == ftp)
	{
		return -1;
	}

	resume_print("upload file: /%s/%s/%s/%s/%s/%s/%s\n", pic_info->path[0],
	      pic_info->path[1], pic_info->path[2], pic_info->path[3],
	      pic_info->path[4], pic_info->path[5], pic_info->name);

	resume_print("src_file_name:%s\n",src_file_name);

	TRACE_LOG_SYSTEM("**************src_file_name = %s, ftpname = %s, ftppath = %s**************",src_file_name,  pic_info->name, pic_info->path);

	if (SUCCESS != ftp->ftp_put_file(src_file_name, pic_info->path,
	                                 pic_info->name))
	{
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: mq_get_status_traffic_flow_motor_vehicle
 * 功  能: 获取机动车交通流MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_traffic_flow_motor_vehicle(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_TRAFFIC_FLOW);

	if (NULL == producer)
	{
		log_warn_storage(
		    "Get mq producer instance of motor vehicle flow failed !\n");
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}

	return -1;
}


/*******************************************************************************
 * 函数名: mq_get_status_traffic_flow_pedestrian
 * 功  能: 获取行人交通流MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_traffic_flow_pedestrian(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_NOCAR_FLOW);

	if (NULL == producer)
	{
		log_warn_storage(
		    "Get mq producer instance of pedestrian flow failed !\n");
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}

	return -1;
}


/*******************************************************************************
 * 函数名: mq_send_traffic_flow_motor_vehicle
 * 功  能: 通过MQ发送机动车交通流信息
 * 参  数: mq_text，发送的信息
 * 返回值: 成功，返回0；断开，返回-1
*******************************************************************************/
int mq_send_traffic_flow_motor_vehicle(char *mq_text)
{
	int rc = -1;

	class_mq_producer *mq_producer
	    = get_mq_producer_instance(MQ_INSTANCE_TRAFFIC_FLOW);
	if (mq_producer == NULL)
	{
		log_debug_storage("Get MQ_INSTANCE_TRAFFIC_FLOW return NULL !\n");
		return rc;
	}

	size_t mq_text_len 	= strlen(mq_text);
	size_t msg_len 		= mq_text_len * 2;
	char *msg 			= (char *)malloc(msg_len);
	if (msg == NULL)
	{
		log_warn_storage("Allocate memory for motor vehicle "
		                 "traffic flow message failed !\n");
		return rc;
	}

	do
	{
		if (convert_enc("GBK", "UTF-8", mq_text, mq_text_len, msg, msg_len) < 0)
			break;

		char type[4];
		sprintf(type, "%d", MSG_NOCAR_FLOW_INFO);

		if (FLG_MQ_TRUE == mq_producer->send_msg_with_property_text(
		            msg, type, "DeviceID", EP_DEV_ID))
		{
			rc = 0;
		}
		else
		{
			log_debug_storage(
			    "Send motor vehicle traffic flow message failed !\n");
		}
	}
	while (0);

	safe_free(msg);

	return rc;
}


/*******************************************************************************
 * 函数名: mq_send_traffic_flow_pedestrian
 * 功  能: 通过MQ发送行人交通流信息
 * 参  数: mq_text，要发送的文本信息
 * 返回值: 成功，返回0；断开，返回-1
*******************************************************************************/
int mq_send_traffic_flow_pedestrian(char *mq_text)
{
	int rc = -1;

	class_mq_producer *mq_producer
	    = get_mq_producer_instance(MQ_INSTANCE_NOCAR_FLOW);
	if (mq_producer == NULL)
	{
		log_debug_storage("Get MQ_INSTANCE_NOCAR_FLOW return NULL !\n");
		return rc;
	}

	size_t mq_text_len 	= strlen(mq_text);
	size_t msg_len 		= mq_text_len * 2;
	char *msg 			= (char *)malloc(msg_len);
	if (msg == NULL)
	{
		log_warn_storage(
		    "Allocate memory for pedestrian traffic flow message failed !\n");
		return rc;
	}

	do
	{
		if (convert_enc("GBK", "UTF-8", mq_text, mq_text_len, msg, msg_len) < 0)
			break;

		char type[4];
		sprintf(type, "%d", MSG_NOCAR_FLOW_INFO);

		if (FLG_MQ_TRUE == mq_producer->send_msg_with_property_text(
		            msg, type, "DeviceID", EP_DEV_ID))
		{
			rc = 0;
		}
		else
		{
			log_debug_storage("Send pedestrian traffic flow message failed!\n");
		}
	}
	while (0);

	safe_free(msg);

	return rc;
}


//dest_mq: 导出MQ会话地址: 0:导出到备用mq会话; 1:导出到管控平台mq会话;
int mq_send_traffic_flow_history(char *msg, int dest_mq, int num_record)
{
	class_mq_producer *mq_producer 	= NULL;
	class_mq_producer *mq_producer2 = NULL;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	if(dest_mq==0)
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_HISTORY_STATISTICS);
	}
	else
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_TRAFFIC_FLOW);
		mq_producer2 = get_mq_producer_instance(MQ_INSTANCE_HISTORY_STATISTICS);
	}

	if (NULL == mq_producer)
	{
		debug("NULL == mq_producer\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_TRAFFICFLOW_INFO);

	if (FLG_MQ_TRUE == mq_producer->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID,
	        "num_record", num_record))
	{
		debug("mq_producer send_msg_text ok\n");


		if (NULL != mq_producer2)
		{
			if (FLG_MQ_TRUE == mq_producer2->send_msg_with_property_text(content, type,
			        "DeviceID", EP_DEV_ID,
			        "num_record", num_record))
			{
				debug("mq_producer2 send_msg_text ok\n");
				//free(content);
				//return 0;
			}
			else
			{
				debug("mq_producer2 send_msg_text failed\n");
				free(content);
				return -1;
			}
		}

		free(content);
		return 0;
	}


	debug("mq_producer send_msg_text fail.\n");
	free(content);
	return -1;
}




/*******************************************************************************
 * 函数名: ftp_get_status_event_alarm
 * 功  能: 获取事件报警FTP连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_get_status_event_alarm(void)
{
	return ftp_get_status(FTP_CHANNEL_ILLEGAL); 	/* 事件报警使用违法FTP */
}


/*******************************************************************************
 * 函数名: mq_get_status_event_alarm
 * 功  能: 获取事件报警MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_event_alarm(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_EVENT_ALARM);

	if (NULL == producer)
	{
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}

	return -1;
}


/*******************************************************************************
 * 函数名: mq_send_event_alarm
 * 功  能: 通过MQ发送事件报警信息
 * 参  数: msg，发送的信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_event_alarm(char *msg)
{
	class_mq_producer *event_alarm;
	//char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	event_alarm = get_mq_producer_instance(MQ_INSTANCE_EVENT_ALARM);
	if (NULL == event_alarm)
	{
		debug("NULL == event_alarm\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	//sprintf(type, "%d", MSG_EVENT_ALARM);
	if (FLG_MQ_TRUE == event_alarm->send_msg_with_property_text(content, NULL,
	        "DeviceID", EP_DEV_ID))
	{
		debug("event_alarm send_msg_text ok\n");
		free(content);
		return 0;
	}

	debug("event_alarm send_msg_text fail.\n");
	free(content);
	return -1;
}



/*******************************************************************************
 * 函数名: mq_send_event_alarm_history
 * 功  能: 通过MQ发送事件报警信息
 * 参  数: msg，发送的信息
 * dest_mq: 导出MQ会话地址: 0:导出到备用mq会话; 1:导出到管控平台mq会话;
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_event_alarm_history(char *msg, int dest_mq, int num_record)
{

	class_mq_producer *mq_producer=NULL;
	class_mq_producer *mq_producer2=NULL;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	if(dest_mq==0)
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_HISTORY_EVENT);
	}
	else
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_EVENT_ALARM);
		mq_producer2 = get_mq_producer_instance(MQ_INSTANCE_HISTORY_EVENT);
	}

	if (NULL == mq_producer)
	{
		debug("NULL == mq_producer\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_EVENT_ALARM);

	if (FLG_MQ_TRUE == mq_producer->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID,
	        "num_record", num_record))
	{
		debug("mq_producer send_msg_text ok\n");


		if (NULL != mq_producer2)
		{
			if (FLG_MQ_TRUE == mq_producer2->send_msg_with_property_text(content, type,
			        "DeviceID", EP_DEV_ID,
			        "num_record", num_record))
			{
				debug("mq_producer2 send_msg_text ok\n");
				//free(content);
				//return 0;
			}
			else
			{
				debug("mq_producer2 send_msg_text failed\n");
				free(content);
				return -1;
			}
		}

		free(content);
		return 0;
	}


	debug("mq_producer send_msg_text fail.\n");
	free(content);
	return -1;
}


/*******************************************************************************
 * 函数名: ftp_get_status_traffic_record
 * 功  能: 获取通行记录FTP连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_get_status_traffic_record(void)
{
	return ftp_get_status(FTP_CHANNEL_PASSCAR);
}


/*******************************************************************************
 * 函数名: ftp_send_traffic_record_pic_buf
 * 功  能: 通过FTP上传通行记录图片缓存
 * 参  数: pic_info，图片信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_send_traffic_record_pic_buf(EP_PicInfo *pic_info)
{
	if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
		return -1;

	return ftp_send_pic_buf(pic_info, FTP_CHANNEL_PASSCAR);
}


/*******************************************************************************
 * 函数名: ftp_send_traffic_record_pic_file
 * 功  能: 通过FTP上传通行记录图片缓存
 * 参  数: pic_info，图片信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_send_traffic_record_pic_file(const char *pic_path, EP_PicInfo *pic_info)
{
	resume_print("In  ftp_send_traffic_record_pic_file\n");
	if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
		return -1;

	return ftp_send_pic_file(pic_path, pic_info, FTP_CHANNEL_PASSCAR_RESUMING);
}


/*******************************************************************************
 * 函数名: mq_get_status_traffic_record
 * 功  能: 获取通行记录MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_traffic_record(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_PASS_CAR);

	if (NULL == producer)
	{
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}

	return -1;
}


/*******************************************************************************
 * 函数名: mq_send_traffic_record
 * 功  能: 通过MQ发送通行记录信息
 * 参  数: msg，发送的信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_traffic_record(char *msg)
{

	class_mq_producer *pass_car;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	pass_car = get_mq_producer_instance(MQ_INSTANCE_PASS_CAR);
	if (NULL == pass_car)
	{
		debug("NULL == pass_car\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_VEHICULAR_TRAFFIC);
	if (FLG_MQ_TRUE == pass_car->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID))
	{
		debug("pass_car send_msg_text ok\n");
		free(content);
		return 0;
	}

	debug("pass_car send_msg_text fail.\n");
	free(content);
	return -1;
}





//dest_mq: 导出MQ会话地址: 0:导出到备用mq会话; 1:导出到管控平台mq会话;
int mq_send_traffic_record_history(char *msg, int dest_mq, int num_record)
{

	class_mq_producer *mq_producer=NULL;
	class_mq_producer *mq_producer2=NULL;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	if(dest_mq==0)
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_HISTORY_PASSCAR);
	}
	else
	{
		mq_producer = get_mq_producer_instance(MQ_INSTANCE_PASS_CAR);
		mq_producer2 = get_mq_producer_instance(MQ_INSTANCE_HISTORY_PASSCAR);
	}

	if (NULL == mq_producer)
	{
		debug("NULL == mq_producer\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_VEHICULAR_TRAFFIC);

	if (FLG_MQ_TRUE == mq_producer->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID,
	        "num_record", num_record))
	{
		debug("mq_producer send_msg_text ok\n");


		if (NULL != mq_producer2)
		{
			if (FLG_MQ_TRUE == mq_producer2->send_msg_with_property_text(content, type,
			        "DeviceID", EP_DEV_ID,
			        "num_record", num_record))
			{
				debug("mq_producer2 send_msg_text ok\n");
				//free(content);
				//return 0;
			}
			else
			{
				debug("mq_producer2 send_msg_text failed\n");
				free(content);
				return -1;
			}
		}

		free(content);
		return 0;
	}


	debug("mq_producer send_msg_text fail.\n");
	free(content);
	return -1;
}


#if 1

/*******************************************************************************
 * 函数名: mq_get_status_park_record
 * 功能:获取泊车记录MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_park_record(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_PARK);

	if (NULL == producer)
	{
		DEBUG("NULL == producer");
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}

	return -1;
}

/*******************************************************************************
 * 函数名: mq_send_park_record
 * 功  能: 通过MQ发送泊车记录信息
 * 参  数: msg，发送的信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_park_record(char *msg)
{

	class_mq_producer *park;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	park = get_mq_producer_instance(MQ_INSTANCE_PARK);
	if (NULL == park)
	{
		debug("NULL == park\n");
		return -1;
	}
	printf("Get park instance success!\n");
	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_PARK);
	printf("goto park->\n");
	if (FLG_MQ_TRUE == park->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID))
	{
		debug("park send_msg_text ok\n");
		free(content);
		return 0;
	}

	debug("park send_msg_text fail.\n");
	free(content);
	return -1;
}

#endif

/*******************************************************************************
 * 函数名: ftp_get_status_violation_records
 * 功  能: 获取违法记录FTP连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_get_status_violation_records(void)
{
	return ftp_get_status(FTP_CHANNEL_ILLEGAL);
}


/*******************************************************************************
 * 函数名: ftp_get_status_violation_video
 * 功  能: 获取违法记录FTP连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_get_status_violation_video(void)
{
	return 0;
}

/*******************************************************************************
 * 函数名: ftp_send_violation_video_buf
 * 功  能: 通过FTP上传违法记录图片缓存
 * 参  数: vid_info，文件信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int ftp_send_violation_video_buf(EP_VidInfo *vid_info)
{
	CLASS_FTP * ftp;
	//char *buf;

	ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL);
	if (NULL == ftp)
	{
		return -1;
	}

	debug("upload video file: /%s/%s/%s/%s/%s/%s/%s, size:%d\n", vid_info->path[0],
	      vid_info->path[1], vid_info->path[2], vid_info->path[3],
	      vid_info->path[4], vid_info->path[5], vid_info->name,
	      vid_info->size[0]+vid_info->size[1]+vid_info->size[2]+vid_info->size[3]+vid_info->size[4]);

	//	buf = (char*) malloc(vid_info->size[0] + vid_info->size[1]);
	//	if (NULL == buf)
	//	{
	//		return -1;
	//	}
	//
	//	if (vid_info->buf_num > 0)
	//	{
	//		memcpy(buf, vid_info->buf[0], vid_info->size[0]);
	//	}
	//	if (vid_info->buf_num > 1)
	//	{
	//		memcpy(buf + vid_info->size[0], vid_info->buf[1], vid_info->size[1]);
	//	}

	//	if (SUCCESS != ftp->ftp_put_data(vid_info->path, vid_info->name, buf,
	//			vid_info->size[0] + vid_info->size[1]))
	if (SUCCESS != ftp->ftp_put_h264(vid_info->path, vid_info->name,
									vid_info))
//	                                 (char*) vid_info->buf[0], vid_info->size[0],
//	                                 (char*) vid_info->buf[1], vid_info->size[1]))
	{
		//free(buf);
		return -1;
	}

	//free(buf);
	return 0;
}

/*******************************************************************************
 * 函数名: mq_get_status_violation_records_motor_vehicle
 * 功  能: 获取违法记录MQ连接状态
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_get_status_violation_records_motor_vehicle(void)
{
	class_mq_producer *producer;
	producer = get_mq_producer_instance(MQ_INSTANCE_ILLEGAL);

	if (NULL == producer)
	{
		return -1;
	}

	if (FLG_MQ_TRUE == producer->get_mq_conn_state())
	{
		return 0;
	}
	return -1;
}

/*******************************************************************************
 * 函数名: mq_send_violation_records_motor_vehicle
 * 功  能: 通过MQ发送机动车违法记录信息
 * 参  数: msg，发送的信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_violation_records_motor_vehicle(char *msg)
{

	class_mq_producer *violation;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	violation = get_mq_producer_instance(MQ_INSTANCE_ILLEGAL);
	if (NULL == violation)
	{
		debug("NULL == violation\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_ILLEGALDATA);
	if (FLG_MQ_TRUE == violation->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID))
	{

		TRACE_LOG_ILLEGAL_PARKING("Bitcom send illegal parking is %s", msg);

		free(content);
		return 0;
	}

	debug("violation send_msg_text fail.\n");
	free(content);
	return -1;
}


/*******************************************************************************
 * 函数名: mq_send_vp_records_hisense
 * 功  能: 通过MQ发送机动车违法记录信息
 * 参  数: msg，发送的信息
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_vp_records_hisense(char *msg)
{

	class_mq_producer *violation;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	violation = get_mq_producer_instance(MQ_INSTANCE_ILLEGAL_HISENSE);
	if (NULL == violation)
	{
		debug("NULL == violation\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_ILLEGALDATA);
	if (FLG_MQ_TRUE == violation->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID))
	{
		TRACE_LOG_ILLEGAL_PARKING("Hisense send illegal parking is %s", msg);
		free(content);
		return 0;
	}

	debug("vp_hisense send_msg_text fail.\n");
	free(content);
	return -1;
}

/*******************************************************************************
 * 函数名: mq_send_violation_records_motor_vehicle_history
 * 功  能: 通过MQ发送机动车违法记录信息
 * 参  数: msg，发送的信息
 * dest_mq: 导出MQ会话地址: 0:导出到备用mq会话; 1:导出到管控平台mq会话;
 * 返回值: 连接，返回0；断开，返回-1
*******************************************************************************/
int mq_send_violation_records_motor_vehicle_history(char *msg, int dest_mq, int num_record)
{

	class_mq_producer *violation=NULL;
	class_mq_producer *violation2=NULL;
	char type[3];

	char* content;
	int len;
	len = strlen(msg) * 2;

	if(dest_mq==0)
	{
		violation = get_mq_producer_instance(MQ_INSTANCE_HISTORY_ILLEGAL);
	}
	else
	{
		violation = get_mq_producer_instance(MQ_INSTANCE_ILLEGAL);
		violation2 = get_mq_producer_instance(MQ_INSTANCE_HISTORY_ILLEGAL);
	}

	if (NULL == violation)
	{
		debug("NULL == violation\n");
		return -1;
	}

	content = (char*) malloc(len);
	if (NULL == content)
	{
		return -1;
	}
	memset(content, 0, len);

	convert_enc("GBK", "UTF-8", msg, strlen(msg), content, len);

	sprintf(type, "%d", MSG_ILLEGALDATA);
	if (FLG_MQ_TRUE == violation->send_msg_with_property_text(content, type,
	        "DeviceID", EP_DEV_ID,
	        "num_record", num_record))
	{
		debug("violation send_msg_text ok\n");


		if (NULL != violation2)
		{
			if (FLG_MQ_TRUE == violation2->send_msg_with_property_text(content, type,
			        "DeviceID", EP_DEV_ID,
			        "num_record", num_record))
			{
				debug("violation2 send_msg_text ok\n");
				//free(content);
				//return 0;
			}
			else
			{
				debug("violation2 send_msg_text failed\n");
				free(content);
				return -1;
			}
		}


		free(content);
		return 0;
	}

	debug("violation send_msg_text fail.\n");
	free(content);
	return -1;
}

#if 0
/*******************************************************************************
 * 函数名: ftp_put_pic_buf
 * 功  能: 通过FTP上传图片缓存
 * 参  数: pic_info，文件信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int ftp_put_pic_buf(const VD_PicInfo *pic_info, int ftp_channel)
{
	log_send(LOG_LEVEL_STATUS,0,"VD:","In ftp_put_pic_buf!");
	CLASS_FTP *ftp = NULL;

	ftp = get_ftp_chanel(ftp_channel);
	if (NULL == ftp)
	{
		log_warn_ftp("%s get FTP channel failed !\n", __func__);
		return -1;
	}

	if(SUCCESS != check_jpeg(pic_info->buf, pic_info->size))
	{
		log_warn_ftp("JPEG check return error !\n");
	}

	resume_print("pic_info->name:%s\n",pic_info->name);
	resume_print("pic_info->path:%s\n",pic_info->path);
	if (SUCCESS != ftp->ftp_put_data(
	            pic_info->path, pic_info->name, pic_info->buf, pic_info->size))
	{
		return -1;
	}

	return 0;
}

/*******************************************************************************
 * 函数名: ftp_put_traffic_record_pic_buf
 * 功  能: 通过FTP上传通行记录图片缓存
 * 参  数: pic_info，图片信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int ftp_put_traffic_record_pic_buf(const VD_PicInfo *pic_info)
{
	log_send(LOG_LEVEL_STATUS,0,"VD:","In ftp_put_traffic_record_pic_buf!");
	if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
		return -1;

	return ftp_put_pic_buf(pic_info, FTP_CHANNEL_PASSCAR);
}

/*******************************************************************************
 * 函数名: ftp_put_park_record_pic_buf
 * 功  能: 通过FTP上传泊车记录图片缓存
 * 参  数: pic_info，图片信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int ftp_put_park_record_pic_buf(const VD_PicInfo *pic_info)
{
	log_send(LOG_LEVEL_STATUS,0,"VD:","In ftp_put_park_record_pic_buf!");
	if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
		return -1;

	return ftp_put_pic_buf(pic_info, FTP_CHANNEL_PASSCAR);
}
#endif
