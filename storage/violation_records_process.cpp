
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#include "tvpuniview_records.h"
#include "baokang_pthread.h"
#include "vd_msg_queue.h"
#include "storage_common.h"
#include "disk_mng.h"
#include "partition_func.h"
#include "data_process.h"
#include "database_mng.h"
#include "violation_records_process.h"
#include "data_process_violation.h"
#include "h264/h264_buffer.h"
#include "h264/debug.h"
//#include "pcie.h"
#include "PCIe_api.h"
#include "ftp.h"
#include "logger/log.h"

#include "interface.h"
//#include "proc_result.h"

#include "mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h"
#include "mcfw/interfaces/ti_media_std.h"
#include "mcfw/interfaces/link_api/systemLink_m3vpss.h"


#include <ApproDrvMsg.h>
#include <Appro_interface.h>

extern int gi_NetPose_vehicle_switch;
extern int gi_Dahua_vehicle_switch;
extern int gi_Bitcom_alleyway_switch;
extern int gi_Uniview_illegally_park_switch;
extern int gi_Baokang_illegally_park_switch;
extern int gi_Zehin_park_switch;

#define LOCK_MP4_VOL			0
#define UNLOCK_MP4_VOL			1
#define LOCK_MP4				2
#define LOCK_MP4_IFRAM			3
#define UNLOCK_MP4				4
#define GET_MPEG4_SERIAL		5
#define WAIT_NEW_MPEG4_SERIAL	6








/*******************************************************************************
 * 函数名: process_violation_records_motor_vehicle
 * 功  能: 处理机动车违法记录
 * 参  数: image_info，图像信息；video_info，视频信息
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int process_violation_records_motor_vehicle(
    const void *image_info, const void *video_info)
{
	if (image_info == NULL)
		return -1;

	log_debug_storage("Start to process motor vehicle violation records.\n");

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);

	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	EP_VidInfo vid_info;
	DB_ViolationRecordsMotorVehicle db_violation_records;
	int flag_send = 0;

	do /* 处理视频 */
	{
		if (video_info == NULL)
		{
			memset(&vid_info, 0, sizeof(EP_VidInfo));
			break;
		}

		analyze_violation_records_video(&vid_info, image_info, video_info);

		if (EP_UPLOAD_CFG.illegal_video == 1)
		{
			if (send_violation_records_video_buf(&vid_info) < 0)
			{
				flag_send |= 0x04; 	/* 视频上传失败，需要续传 */
				log_debug_storage(
				    "Send violation record video of motor vehicle failed !\n");
			}
		}

		if (((flag_send & 0x04) != 0) || (EP_DISK_SAVE_CFG.illegal_video == 1))
		{
			if (disk_status == 0)
			{
				if (save_violation_records_video_buf(&vid_info, partition_path)
				        < 0)
				{
					log_warn_storage("Save violation record video of "
					                 "motor vehicle failed !\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, violation record "
				                 "video of motor vehicle is discarded.\n");
			}
		}
	}
	while (0);

	do /* 处理图片 */
	{
		if (analyze_violation_records_picture(pic_info, image_info) < 0)
			return -1;

		int pic_num = ((Pcie_data_head *)image_info)->NumPic;

		if (EP_UPLOAD_CFG.illegal_picture == 1)
		{
			if (send_violation_records_image_buf(pic_info, pic_num) < 0)
			{
				flag_send |= 0x02; 	/* 图片上传失败，需要续传 */
				log_debug_storage(
				    "Send violation record image of motor vehicle failed !\n");
			}
		}

		if (((flag_send & 0x02) != 0) ||
		        (EP_DISK_SAVE_CFG.illegal_picture == 1))
		{
			if (disk_status == 0)
			{
				disk_status = save_violation_records_image_buf(
				                  pic_info, image_info, partition_path);
				if (disk_status < 0)
				{
					log_warn_storage("Save violation record image of "
					                 "motor vehicle failed !\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, violation record "
				                 "image of motor vehicle is discarded.\n");
			}
		}
	}
	while (0);

	do /* 处理信息 */
	{
		if (analyze_violation_records_motor_vehicle_info(
		            &db_violation_records, image_info, video_info,
		            pic_info, &vid_info, partition_path) < 0)
		{
			log_debug_storage("analyze violation record information of "
			                  "motor vehicle failed !\n");
			return -1;
		}

		if (EP_UPLOAD_CFG.illegal_picture == 1)
		{
			flag_send |= 0x01; 		/* 先设置为需要续传 */

			if (((flag_send & 0x02) == 0) &&
			        (send_violation_records_motor_vehicle_info(
			             &db_violation_records) == 0))
			{
				flag_send = 0x00; 	/* 如果上传成功，再恢复为无需上传 */
			}
		}

		if (((flag_send & 0x01) != 0) ||
		        (EP_DISK_SAVE_CFG.illegal_picture == 1))
		{
			if (disk_status == 0)
			{
				db_violation_records.flag_send = flag_send;

				if (EP_DISK_SAVE_CFG.illegal_picture == 1)
				{
					db_violation_records.flag_store |= 0x01;
				}
				if (EP_DISK_SAVE_CFG.illegal_video == 1)
				{
					db_violation_records.flag_store |= 0x02;
				}

				if (save_violation_records_motor_vehicle_info(
				            &db_violation_records,
				            image_info, partition_path) < 0)
				{
					log_error_storage("Save violation record information of "
					                  "motor vehicle failed !\n");
				}
			}
			else
			{
				log_debug_storage(
				    "The disk is not available, violation record information "
				    "of motor vehicle is discarded.\n");
			}
		}
	}
	while (0);

	return 0;
}


/*******************************************************************************
 * 函数名: analyze_violation_records_video
 * 功  能: 解析违法记录视频
 * 参  数: vid_info，解析出来的视频信息；
 *         image_info，要解析的图像信息；video_info，要解析的H.264流信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_violation_records_video(
    EP_VidInfo *vid_info, const void *image_info, const void *video_info)
{
//	Pcie_data_head *info = (Pcie_data_head *) image_info;
//	illegalRecordVehicle *violation_records =
//	    (illegalRecordVehicle *) ((char *) info + IMAGE_INFO_SIZE);
	IllegalInfoPer *illegalInfoPer = (IllegalInfoPer *)&((SystemVpss_SimcopJpegInfo *)image_info)->algResultInfo.AlgResultInfo;
	H264_Record_Info *h264_info = (H264_Record_Info *) video_info;
	int i;

	memset(vid_info, 0, sizeof(EP_VidInfo));

	/* 路径按照用户的配置进行设置 */
	for (i = 0; i < EP_FTP_URL_LEVEL.levelNum; i++)
	{
		switch (EP_FTP_URL_LEVEL.urlLevel[i])
		{
		case SPORT_ID: 		//地点编号
			sprintf(vid_info->path[i], "%s", EP_POINT_ID);
			break;
		case DEV_ID: 		//设备编号
			sprintf(vid_info->path[i], "%s", EP_DEV_ID);
			break;
		case YEAR_MONTH: 	//年/月
			sprintf(vid_info->path[i], "%04d%02d",
//			        violation_records->picInfo[0].year,
//			        violation_records->picInfo[0].month
			        illegalInfoPer->illegalPics[0].time.tm_year,
			        illegalInfoPer->illegalPics[0].time.tm_mon
					);
			break;
		case DAY: 			//日
			sprintf(vid_info->path[i], "%02d",
//			        violation_records->picInfo[0].day
			        illegalInfoPer->illegalPics[0].time.tm_mday
			        );
			break;
		case EVENT_NAME: 	//事件类型
			sprintf(vid_info->path[i], "%s", VIOLATION_RECORDS_FTP_DIR);
			break;
		case HOUR: 			//时
			sprintf(vid_info->path[i], "%02d",
//			        violation_records->picInfo[0].hour
			        illegalInfoPer->illegalPics[0].time.tm_hour
					);
			break;
		case FACTORY_NAME: 	//厂商名称
			sprintf(vid_info->path[i], "%s", EP_MANUFACTURER);
			break;
		default:
			break;
		}
	}

	/* 文件名以时间和车道号命名 */
	sprintf(vid_info->name, "%s%04d%02d%02d%02d%02d%02d%03d%02d.mp4",
	        EP_EXP_DEV_ID,
//	        violation_records->picInfo[0].year,
//	        violation_records->picInfo[0].month,
//	        violation_records->picInfo[0].day,
//	        violation_records->picInfo[0].hour,
//	        violation_records->picInfo[0].minute,
//	        violation_records->picInfo[0].second,
//	        violation_records->picInfo[0].msecond,
//	        violation_records->illegalVehicle.laneNum
	        illegalInfoPer->illegalPics[0].time.tm_year,
	        illegalInfoPer->illegalPics[0].time.tm_mon,
	        illegalInfoPer->illegalPics[0].time.tm_mday,
   	        illegalInfoPer->illegalPics[0].time.tm_hour,
   	        illegalInfoPer->illegalPics[0].time.tm_min,
   	        illegalInfoPer->illegalPics[0].time.tm_sec,
   	        illegalInfoPer->illegalPics[0].time.tm_msec,
   	        0
			);

	if (h264_info)
	{
		printf("video seg:%d\t"
//		       "seg1:%p,  len:%d\n"
//		       "seg2:%p,  len:%d\n"
		       "time: %ds\n",
		       h264_info->h264_seg,	//h264流的分段数，通常为1段或2段，0表示无效
//		       h264_info->h264_buf_seg[0], 		//指向第一段h264流的起始地址
//		       h264_info->h264_buf_seg_size[0], //第一段h264流的长度
//		       h264_info->h264_buf_seg[1], 		//指向第二段h264流的起始地址
//		       h264_info->h264_buf_seg_size[1], //第二段h264流的长度
		       h264_info->seconds_record_ret 	//违法记录对应的h264视频时间长度
		      );

		vid_info->buf_num = h264_info->h264_seg;

		for (i = 0; i < vid_info->buf_num; i++)
		{
			vid_info->buf[i] = h264_info->h264_buf_seg[i];
			vid_info->size[i] = h264_info->h264_buf_seg_size[i];
			printf("seg%d:%p, len:%d\n ,",i,h264_info->h264_buf_seg[i],h264_info->h264_buf_seg_size[i]);
		}
	}

	return 0;
}


/*******************************************************************************
 * 函数名: analyze_violation_records_picture
 * 功  能: 解析违法记录图片
 * 参  数: pic_info，解析出来的图片信息；image_info，要解析的图像信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_violation_records_picture(
    EP_PicInfo pic_info[], const void *image_info)
{
	Pcie_data_head *info = (Pcie_data_head *) image_info;
	illegalRecordVehicle *violation_records =
	    (illegalRecordVehicle *) ((char *) info + IMAGE_INFO_SIZE);
	unsigned int pic_num = info->NumPic;
	unsigned int i;

	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Violation records picture number error: %d\n",
		                  pic_num);
		return -1;
	}

	for (i=0; i<pic_num; i++)
	{
		memset(&pic_info[i], 0, sizeof(EP_PicInfo));

		/* 路径按照用户的配置进行设置 */
		for (int j = 0; j < EP_FTP_URL_LEVEL.levelNum; j++)
		{
			switch (EP_FTP_URL_LEVEL.urlLevel[j])
			{
			case SPORT_ID: 		//地点编号
				sprintf(pic_info[i].path[j], "%s", EP_POINT_ID);
				break;
			case DEV_ID: 		//设备编号
				sprintf(pic_info[i].path[j], "%s", EP_DEV_ID);
				break;
			case YEAR_MONTH: 	//年/月
				sprintf(pic_info[i].path[j], "%04d%02d",
				        violation_records->picInfo[i].year,
				        violation_records->picInfo[i].month);
				break;
			case DAY: 			//日
				sprintf(pic_info[i].path[j], "%02d",
				        violation_records->picInfo[i].day);
				break;
			case EVENT_NAME: 	//事件类型
				sprintf(pic_info[i].path[j], "%s", VIOLATION_RECORDS_FTP_DIR);
				break;
			case HOUR: 			//时
				sprintf(pic_info[i].path[j], "%02d",
				        violation_records->picInfo[i].hour);
				break;
			case FACTORY_NAME: 	//厂商名称
				sprintf(pic_info[i].path[j], "%s", EP_MANUFACTURER);
				break;
			default:
				break;
			}
		}

		/* 文件名以时间和车道号命名 */
		sprintf(pic_info[i].name, "%s%04d%02d%02d%02d%02d%02d%03d%02d%02d.jpg",
		        EP_EXP_DEV_ID,
		        violation_records->picInfo[i].year,
		        violation_records->picInfo[i].month,
		        violation_records->picInfo[i].day,
		        violation_records->picInfo[i].hour,
		        violation_records->picInfo[i].minute,
		        violation_records->picInfo[i].second,
		        violation_records->picInfo[i].msecond,
		        violation_records->illegalVehicle.laneNum,
		        i + 1);

		/* 获取图片缓存的位置 */
		pic_info[i].buf = (char *) info + IMAGE_INFO_SIZE + IMAGE_PADDING_SIZE
		                  + info->PosPic[i];

		/* 获取图片缓存的大小 */
		pic_info[i].size = violation_records->picInfo[i].picSize;
	}

	return 0;
}


/**
 * 根据优先级, 获得最高优先级对应的用户定义的违法代码和索引.
 *
 *    code[out]: 用户定义的违法代码
 * 返回: 对应的数组下标index
 */
int get_violation_code_index(int type, int *code)
{
	DEBUG("--------------------type: %d\n", type);
	int i, ret;

	//	for (i = 0; i < ILLEGAL_CODE_COUNT; i++)
	//	{
	//		debug("g_arm_config.event_order[%d] : %d, %d\n", i,g_arm_config.illegal_code_info[i].illeagal_type,g_arm_config.illegal_code_info[i].illeagal_num);
	//	}
	for (i = 0; i < ILLEGAL_CODE_COUNT; i++)
	{
		if (type & g_arm_config.illegal_code_info[i].illeagal_type)
		{
			*code = g_arm_config.illegal_code_info[i].illeagal_num;
			break;
		}
	}

	if(i>=ILLEGAL_CODE_COUNT)
	{
		ERROR("%s error : not find illegal code return :  -1",__func__);
		return -1;
	}

	switch (g_arm_config.illegal_code_info[i].illeagal_type)
	{
	case ILLEGALPARK: //非法停车
		ret = ILLEGAL_CODE_ILLEGALPARK;
		break;
	case OVERSAFETYSTRIP: //压安全岛
		ret = ILLEGAL_CODE_OVERSAFETYSTRIP;
		break;
	case ILLEGALLANERUN: //不按道行驶
		ret = ILLEGAL_CODE_ILLEGAL_ANELRUN;
		break;
	case CROSSSTOPLINE: //违法越线
		ret = ILLEGAL_CODE_CROSSSTOPLINE;
		break;
	case COVERLINE://压实线
		ret = ILLEGAL_CODE_COVERLINE;
		break;
	case CHANGELANE: //违法变道
		ret = ILLEGAL_CODE_CHANGELANE;
		break;
	case CONVERSEDRIVE: //逆行
		ret = ILLEGAL_CODE_CONVERSE_DRIVE;
		break;
	case RUNRED://闯红灯
		ret = ILLEGAL_CODE_RUNRED;
		break;
	case EXCEEDSPEED: //超速
		ret = ILLEGAL_CODE_EXCEED_SPEED;
		break;
	case OCCUPYSPECIAL: //非法占用专用车道
		ret = ILLEGAL_CODE_OCCUPYSPECIAL;
		break;
	case OCCUPYNONMOTOR: //非法占用非机动车道
		ret = ILLEGAL_CODE_OCCUPYNONMOTOR;
		break;
	case FORCEDCROSS: //拥堵时强行驶入交叉路口
		ret = ILLEGAL_CODE_FORCEDCROSS;
		break;
	case LIMITTRAVEL: //违反限时限行
		ret = ILLEGAL_CODE_LIMITTRAVEL;
		break;
	case NOTWAYTOPEDES:
		ret = ILLEGAL_CODE_NOTWAYTOPEDES;
		break;
	default:
		ret = 0;
	}

	DEBUG("---------------return :%d",ret);
	return ret;
}

int get_violation_code_index_PD(int type, int *code)
{
	debug("--------------------type: %d\n", type);
	int i, ret;
	int flag_find=0;
	//固定为违法停车，寻找相应的编号
	for (i = 0; i < ILLEGAL_CODE_COUNT; i++)
	{
//		if(g_arm_config.illegal_code_info[i].illeagal_type == ILLEGALPARK)
//		{
//			break;
//		}
		if (type & g_arm_config.illegal_code_info[i].illeagal_type)
		{
			*code = g_arm_config.illegal_code_info[i].illeagal_num;
			flag_find = 1;
			break;
		}

	}
	if(flag_find == 0)//not find
	{
		if(type == BREAK_BLOCKADE)//reuse FORCEDCROSS
		{
			for (i = 0; i < ILLEGAL_CODE_COUNT; i++)
			{
				if(g_arm_config.illegal_code_info[i].illeagal_type == FORCEDCROSS)
				{
					*code = g_arm_config.illegal_code_info[i].illeagal_num;
					flag_find = 1;
					break;
				}
			}
		}
	}


	if(flag_find == 0)//not find
	{
		switch(type)
		{
			case 1:
				*code = 10390;	//违停
				break;
			case 64:
				*code = 13010;	//逆行
				break;
			case 2048:
				*code = 10250; //强行驶入路口
				break;
			case 32768:
				*code = 10780; //未交费闯关   199061  10780
				break;
		}
	}

	if(i>=ILLEGAL_CODE_COUNT)
	{
		ERROR("%s error : not find illegal code return :  -1",__func__);
		return -1;
	}

	switch (g_arm_config.illegal_code_info[i].illeagal_type)
	{
	case ILLEGALPARK: //非法停车
		ret = ILLEGAL_CODE_ILLEGALPARK;
		break;
	case OVERSAFETYSTRIP: //压安全岛
		ret = ILLEGAL_CODE_OVERSAFETYSTRIP;
		break;
	case ILLEGALLANERUN: //不按道行驶
		ret = ILLEGAL_CODE_ILLEGAL_ANELRUN;
		break;
	case CROSSSTOPLINE: //违法越线
		ret = ILLEGAL_CODE_CROSSSTOPLINE;
		break;
	case COVERLINE://压实线
		ret = ILLEGAL_CODE_COVERLINE;
		break;
	case CHANGELANE: //违法变道
		ret = ILLEGAL_CODE_CHANGELANE;
		break;
	case CONVERSEDRIVE: //逆行
		ret = ILLEGAL_CODE_CONVERSE_DRIVE;
		break;
	case RUNRED://闯红灯
		ret = ILLEGAL_CODE_RUNRED;
		break;
	case EXCEEDSPEED: //超速
		ret = ILLEGAL_CODE_EXCEED_SPEED;
		break;
	case OCCUPYSPECIAL: //非法占用专用车道
		ret = ILLEGAL_CODE_OCCUPYSPECIAL;
		break;
	case OCCUPYNONMOTOR: //非法占用非机动车道
		ret = ILLEGAL_CODE_OCCUPYNONMOTOR;
		break;
	case FORCEDCROSS: //拥堵时强行驶入交叉路口
		ret = ILLEGAL_CODE_FORCEDCROSS;
		break;
	case LIMITTRAVEL: //违反限时限行
		ret = ILLEGAL_CODE_LIMITTRAVEL;
		break;
	case NOTWAYTOPEDES:
		ret = ILLEGAL_CODE_NOTWAYTOPEDES;
		break;
	default:
		ret = 0;
	}

	DEBUG("---------------return :%d",ret);
	return ret;
}


/*******************************************************************************
 * 函数名: analyze_violation_records_motor_vehicle_info
 * 功  能: 解析机动车违法记录信息
 * 参  数: image_info，图像信息；video_info，视频信息
 * 返回值:
*******************************************************************************/
int analyze_violation_records_motor_vehicle_info(
    DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const void *video_info,
    const EP_PicInfo pic_info[], const EP_VidInfo *vid_info,
    const char *partition_path)
{
	unsigned int i;
	int index;

	unsigned int pic_num = ((Pcie_data_head *)image_info)->NumPic;
	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("motor vehicle violation record picture number "
		                  "error: %d\n", pic_num);
		return -1;
	}

	illegalRecordVehicle *violation_records =
	    (illegalRecordVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);

	memset(db_violation_records, 0, sizeof(DB_ViolationRecordsMotorVehicle));

	db_violation_records->plate_type =
	    violation_records->illegalVehicle.vehicleFeature.plateType;
	sprintf(db_violation_records->plate_num, "%s",
	        violation_records->illegalVehicle.vehicleFeature.plateNumber);
	index = 0;//violation_records->illegalVehicle.picFlag - 1;

//	if (index < 0)
//	{
//		debug( "illegalVehicle.picFlag = %d. illegal data.\n",
//		       violation_records->illegalVehicle.picFlag);
//		index = 0;
//	}

	sprintf(db_violation_records->time, "%04d-%02d-%02d %02d:%02d:%02d",
	        violation_records->picInfo[index].year,
	        violation_records->picInfo[index].month,
	        violation_records->picInfo[index].day,
	        violation_records->picInfo[index].hour,
	        violation_records->picInfo[index].minute,
	        violation_records->picInfo[index].second);

	index = get_violation_code_index(
	            violation_records->illegalVehicle.illegalType,//  .incidentType,
	            &(db_violation_records->violation_type));

	sprintf(db_violation_records->point_id, "%s", EP_POINT_ID);
	sprintf(db_violation_records->point_name, "%s", EP_POINT_NAME);
	snprintf(db_violation_records->collection_agencies,
	         EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);
	sprintf(db_violation_records->dev_id, "%s", EP_EXP_DEV_ID);

	db_violation_records->direction = EP_DIRECTION;
	db_violation_records->lane_num = violation_records->illegalVehicle.laneNum;

	for (i = 0; i < pic_num; i++)
	{
		get_ftp_path(EP_FTP_URL_LEVEL,
		             violation_records->picInfo[i].year,
		             violation_records->picInfo[i].month,
		             violation_records->picInfo[i].day,
		             violation_records->picInfo[i].hour,
		             (char*) VIOLATION_RECORDS_FTP_DIR,
		             db_violation_records->image_path[i]);

		strcat(db_violation_records->image_path[i], pic_info[i].name);
	}

	if (video_info)
	{
		get_ftp_path(EP_FTP_URL_LEVEL,
		             violation_records->picInfo[0].year,
		             violation_records->picInfo[0].month,
		             violation_records->picInfo[0].day,
		             violation_records->picInfo[0].hour,
		             (char*) VIOLATION_RECORDS_FTP_DIR,
		             db_violation_records->video_path);

		strcat(db_violation_records->video_path, vid_info->name);
		printf("video_path is %s.\n", db_violation_records->video_path);
	}

	sprintf(db_violation_records->partition_path, "%s", partition_path);
	db_violation_records->speed = violation_records->illegalVehicle.speed;


	for(int i=0;i<MAX_CAPTURE_NUM;i++)//车牌框可能出现在任意一张图片上
	{
		printf("analyze_violation_records_motor_vehicle_info: i=%d  plate.(x,y) is (%d,%d)\n",i,db_violation_records->coordinate_x,db_violation_records->coordinate_y);
		if((violation_records->illegalVehicle.illegalPics[i].plateRect.x !=0)
				|| (violation_records->illegalVehicle.illegalPics[i].plateRect.y !=0))
		{
			db_violation_records->coordinate_x = violation_records->illegalVehicle.illegalPics[i].plateRect.x;//  .xPos;
			db_violation_records->coordinate_y = violation_records->illegalVehicle.illegalPics[i].plateRect.y;//.yPos;
			db_violation_records->width = violation_records->illegalVehicle.illegalPics[i].plateRect.width;
			db_violation_records->height = violation_records->illegalVehicle.illegalPics[i].plateRect.height;
			db_violation_records->pic_flag = i+1;//violation_records->illegalVehicle.picFlag;
			break;
		}
	}

	db_violation_records->plate_color = violation_records->illegalVehicle.vehicleFeature.plateColor;//   color;
	db_violation_records->encode_type = ((Pcie_data_head *)image_info)->EncType;

	int first = 0;
	strcpy(db_violation_records->description, "");

	for (int i = 0; i < ILLEGAL_CODE_COUNT; i++)
	{
		if ((violation_records->illegalVehicle.illegalType  //   .incidentType
		        & g_arm_config.illegal_code_info[i].illeagal_type) == 0)
		{
			continue;
		}

		if (first == 1)
		{
			strcat(db_violation_records->description, "||");
		}
		else
		{
			first = 1;
		}

		switch (g_arm_config.illegal_code_info[i].illeagal_type)
		{
		case ILLEGALPARK:
			strcat(db_violation_records->description, "非法停车");
			break;
		case OVERSAFETYSTRIP:
			strcat(db_violation_records->description, "压安全岛");
			break;
		case ILLEGALLANERUN:
			strcat(db_violation_records->description, "不按道行驶");
			break;
		case CROSSSTOPLINE:
			strcat(db_violation_records->description, "违法越线");
			break;
		case COVERLINE:
			strcat(db_violation_records->description, "压实线");
			break;
		case CHANGELANE:
			strcat(db_violation_records->description, "违法变道");
			break;
		case CONVERSEDRIVE:
			strcat(db_violation_records->description, "逆行");
			break;
		case RUNRED:
		{
			strcat(db_violation_records->description, "闯红灯");

			VD_Time *tm =
			    &(violation_records->illegalVehicle.signalCycle.startVD_Time);
			snprintf(db_violation_records->red_lamp_start_time,
			         sizeof(db_violation_records->red_lamp_start_time),
			         "%04d-%02d-%02d %02d:%02d:%02d",
			         tm->tm_year, tm->tm_mon, tm->tm_mday,
			         tm->tm_hour, tm->tm_min, tm->tm_sec);

			db_violation_records->red_lamp_keep_time =
			    violation_records->illegalVehicle.signalCycle.cycleExtent;
		}
		break;
		case EXCEEDSPEED:
			strcat(db_violation_records->description, "超速");
			break;
		case OCCUPYSPECIAL:
			strcat(db_violation_records->description, "非法占用专用车道");
			break;
		case OCCUPYNONMOTOR:
			strcat(db_violation_records->description, "非法占用非机动车道");
			break;
		case FORCEDCROSS:
			strcat(db_violation_records->description, "拥堵时强行驶入");
			break;
		case LIMITTRAVEL:
			strcat(db_violation_records->description, "违反限时限行");
			break;
		case NOTWAYTOPEDES:
			strcat(db_violation_records->description, "不礼让行人");
			break;
		case BREAK_BLOCKADE:
			strcat(db_violation_records->description, "未交费闯关");
			break;
		default:
			strcat(db_violation_records->description, "未知类型");
			break;
		}
	}

	sprintf(db_violation_records->ftp_user, "%s", EP_VIOLATION_FTP.user);
	sprintf(db_violation_records->ftp_passwd, "%s", EP_VIOLATION_FTP.passwd);
	sprintf(db_violation_records->ftp_ip, "%d.%d.%d.%d",
	        EP_VIOLATION_FTP.ip[0],
	        EP_VIOLATION_FTP.ip[1],
	        EP_VIOLATION_FTP.ip[2],
	        EP_VIOLATION_FTP.ip[3]);
	db_violation_records->ftp_port = EP_VIOLATION_FTP.port;

	sprintf(db_violation_records->video_ftp_user, "%s", EP_VIOLATION_FTP.user);
	sprintf(db_violation_records->video_ftp_passwd, "%s",
	        EP_VIOLATION_FTP.passwd);
	sprintf(db_violation_records->video_ftp_ip, "%d.%d.%d.%d",
	        EP_VIOLATION_FTP.ip[0],
	        EP_VIOLATION_FTP.ip[1],
	        EP_VIOLATION_FTP.ip[2],
	        EP_VIOLATION_FTP.ip[3]);
	db_violation_records->video_ftp_port = EP_VIOLATION_FTP.port;

	db_violation_records->vehicle_type =
	    violation_records->illegalVehicle.vehicleFeature.vehicleType;
	db_violation_records->color =
	    violation_records->illegalVehicle.vehicleFeature.vehicleColor;
	db_violation_records->vehicle_logo =
	    violation_records->illegalVehicle.vehicleFeature.vehicleLogo;

	return 0;
}


/*******************************************************************************
 * 函数名: format_mq_text_violation_records_motor_vehicle
 * 功  能: 格式化机动车违法记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；records，违法记录
 * 返回值: 字符串长度
*******************************************************************************/
int format_mq_text_violation_records_motor_vehicle(char *mq_text,
        DB_ViolationRecordsMotorVehicle *records)
{
	int len = 0;
	int i;

	debug("format mq text violation.\n");
	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	len += sprintf(mq_text + len, "%s", EP_DATA_SOURCE);
	/* 字段1 违法数据来源 */

	len += sprintf(mq_text + len, ",%02d", records->plate_type);
	/* 字段2 车牌类型 */

	len += sprintf(mq_text + len, ",%s", records->plate_num);
	/* 字段3 车牌号码 */

	len += sprintf(mq_text + len, ",%s", records->time);
	/* 字段4 采集时间 */

	len += sprintf(mq_text + len, ",%d", records->violation_type);
	/* 字段5 违法代码 */

	len += sprintf(mq_text + len, ",%s", records->point_id);
	/* 字段6 采集点编号 */

	len += sprintf(mq_text + len, ",%s", records->point_name);
	/* 字段7 采集地址 */

	len += sprintf(mq_text + len, ",%s", records->collection_agencies);
	/* 字段8 采集机关 */

	len += sprintf(mq_text + len, ",%s", EP_DATA_SOURCE);
	/* 字段9 数据来源 */

	len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
	/* 字段10 抓拍类型 */

	len += sprintf(mq_text + len, ",%s", records->dev_id);
	/* 字段11 设备编号 */

	len += sprintf(mq_text + len, ",%d", records->direction);
	/* 字段12 方向编号 */

	len += sprintf(mq_text + len, ",%02d", records->lane_num);
	/* 字段13 车道编号 */

	len += sprintf(mq_text + len, ",%s", records->red_lamp_start_time);
	/* 字段14 红灯亮起时间 */

	len += sprintf(mq_text + len, ",%d", records->red_lamp_keep_time);
	/* 字段15 红灯持续时间 */

	for (i = 0; i < 3; i++)
	{
		if (strlen(records->image_path[i]) > 0)
		{
			len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
			               records->ftp_user,
			               records->ftp_passwd,
			               records->ftp_ip,
			               records->ftp_port,
			               records->image_path[i]);
		}
		else
		{
			len += sprintf(mq_text + len, ",");
		}
	}
	/* 字段16到18 前三张违法图片地址 */

#ifndef VP_YANTAI
	if (strlen(records->video_path) > 0)
	{
		len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
		               records->video_ftp_user,
		               records->video_ftp_passwd,
		               records->video_ftp_ip,
		               records->video_ftp_port,
		               records->video_path);
	}
	else
	{
		len += sprintf(mq_text + len, ",");
	}
	/* 字段19 违法视频地址 */
#else
	len += sprintf(mq_text + len, ",");
#endif

	len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d",
	               records->coordinate_x, records->coordinate_y,
	               records->width, records->height,
	               records->pic_flag);
	/* 字段20 车牌定位（X坐标、Y坐标、宽度、高度、第几张图片） */

	len += sprintf(mq_text + len, ",%d", records->plate_color);
	/* 字段21 车牌颜色 */

	len += sprintf(mq_text + len, ",,,,,,,,,");
	/* 字段22到30 未用 */

	len += sprintf(mq_text + len, ",%d", records->plate_color);
	/* 字段31 车牌颜色 */

	len += sprintf(mq_text + len, ",%s", records->description);
	/* 字段32 违法描述 */

	len += sprintf(mq_text + len, ",%d", records->encode_type);
	/* 字段33 编码格式（三合一、四合一、三单张、四单张*/

	if (strlen(records->image_path[3]) > 0)
	{
		len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
		               records->ftp_user,
		               records->ftp_passwd,
		               records->ftp_ip,
		               records->ftp_port,
		               records->image_path[3]);
	}
	else
	{
		len += sprintf(mq_text + len, ",");
	}
	/* 字段34 第四张违法图片地址 */
	len += sprintf(mq_text+len, ",%d", records->vehicle_type);

	/* 字段35 车型 */

	len += sprintf(mq_text+len, ",%d", records->color);
	/* 字段36 车身颜色 */

	len += sprintf(mq_text+len, ",%d", records->vehicle_logo);
	/* 字段37 车标 */

	return len;
}




/*******************************************************************************
 * 函数名: format_mq_text_vp_records_hisense
 * 功  能: 格式化机动车违法记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；records，违法记录
 * 返回值: 字符串长度
 * 特殊说明:此函数专为海信平台发送mq消息使用
*******************************************************************************/
int format_mq_text_vp_records_hisense(char *mq_text_hisense,
        DB_ViolationRecordsMotorVehicle *records)
{
	int len = 0;
	int i;

	debug("format mq_text_hisense vp\n");
	memset(mq_text_hisense, 0, MQ_TEXT_BUF_SIZE);

	len += sprintf(mq_text_hisense + len, "%s", EP_DATA_SOURCE_HISENSE);
	/* 字段1 违法数据来源 */

	len += sprintf(mq_text_hisense + len, ",%02d", records->plate_type);
	/* 字段2 车牌类型 */

	len += sprintf(mq_text_hisense + len, ",%s", records->plate_num);
	/* 字段3 车牌号码 */

	len += sprintf(mq_text_hisense + len, ",%s", records->time);
	/* 字段4 采集时间 */

	len += sprintf(mq_text_hisense + len, ",%d", records->violation_type);
	/* 字段5 违法代码 */

//	len += sprintf(mq_text_hisense + len, ",%d", 123456);
	/* 字段5 违法代码 */

	len += sprintf(mq_text_hisense + len, ",%s", records->point_id);
	/* 字段6 采集点编号 */

	len += sprintf(mq_text_hisense + len, ",%s", records->point_name);
	/* 字段7 采集地址 */

	len += sprintf(mq_text_hisense + len, ",%s", records->collection_agencies);
	/* 字段8 采集机关 */

	len += sprintf(mq_text_hisense + len, ",%s", DATA_SOURCE_HISENSE);
	/* 字段9 数据来源 */

	len += sprintf(mq_text_hisense + len, ",%s", EP_SNAP_TYPE);
	/* 字段10 抓拍类型 */

	len += sprintf(mq_text_hisense + len, ",%s", records->dev_id);
	/* 字段11 设备编号 */

	len += sprintf(mq_text_hisense + len, ",%02d", records->direction);
	/* 字段12 方向编号 */

	for (i = 0; i < 3; i++)
	{
		if (strlen(records->image_path[i]) > 0)
		{
			len += sprintf(mq_text_hisense + len, ",ftp://%s:%s@%s:%d/%s",
			               records->ftp_user,
			               records->ftp_passwd,
			               records->ftp_ip,
			               records->ftp_port,
			               records->image_path[i]);
		}
		else
		{
			len += sprintf(mq_text_hisense + len, ",");
		}
	}
	/* 字段13到15 前三张违法图片地址 */

	len += sprintf(mq_text_hisense + len, ",");

	return len;
}

/*******************************************************************************
 * 函数名: db_write_violation_records_motor_vehicle
 * 功  能: 写机动车违法记录数据库
 * 参  数: records，机动车违法记录
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_write_violation_records_motor_vehicle(char *db_name, void *records,
        pthread_mutex_t *mutex_db_records)
{
	char sql[SQL_BUF_SIZE];

	db_format_insert_sql_violation_records_motor_vehicle(sql, records);

	pthread_mutex_lock(mutex_db_records);
	int ret = db_write(db_name, sql,
	                   SQL_CREATE_TABLE_VIOLATION_RECORDS_MOTOR_VEHICLE);
	pthread_mutex_unlock(mutex_db_records);

	if (ret != 0)
	{
		printf("db_write_violation_records_motor_vehicle failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: db_format_insert_sql_violation_records_motor_vehicle
 * 功  能: 格式化机动车违法记录表SQL插入语句
 * 参  数: sql，SQL缓存；buf，数据缓冲区指针
 * 返回值: SQL长度
*******************************************************************************/
int db_format_insert_sql_violation_records_motor_vehicle(char *sql, void *buf)
{
	DB_ViolationRecordsMotorVehicle *records;
	records = (DB_ViolationRecordsMotorVehicle *) buf;
	int len = 0;

	memset(sql, 0, SQL_BUF_SIZE);

	len += sprintf(sql,
	               "INSERT INTO violation_records_motor_vehicle VALUES(NULL");
	len += sprintf(sql + len, ",'%d'", records->plate_type);
	len += sprintf(sql + len, ",'%s'", records->plate_num);
	len += sprintf(sql + len, ",'%s'", records->time);
	len += sprintf(sql + len, ",'%d'", records->violation_type);
	len += sprintf(sql + len, ",'%s'", records->point_id);
	len += sprintf(sql + len, ",'%s'", records->point_name);
	len += sprintf(sql + len, ",'%s'", records->collection_agencies);
	len += sprintf(sql + len, ",'%s'", records->dev_id);
	len += sprintf(sql + len, ",'%d'", records->direction);
	len += sprintf(sql + len, ",'%02d'", records->lane_num);
	len += sprintf(sql + len, ",'%s'", records->red_lamp_start_time);
	len += sprintf(sql + len, ",'%d'", records->red_lamp_keep_time);
	len += sprintf(sql + len, ",'%s'", records->image_path[0]);
	len += sprintf(sql + len, ",'%s'", records->image_path[1]);
	len += sprintf(sql + len, ",'%s'", records->image_path[2]);
	len += sprintf(sql + len, ",'%s'", records->image_path[3]);
	len += sprintf(sql + len, ",'%s'", records->video_path);
	len += sprintf(sql + len, ",'%s'", records->partition_path);
	len += sprintf(sql + len, ",'%d'", records->speed);
	len += sprintf(sql + len, ",'%d'", records->coordinate_x);
	len += sprintf(sql + len, ",'%d'", records->coordinate_y);
	len += sprintf(sql + len, ",'%d'", records->width);
	len += sprintf(sql + len, ",'%d'", records->height);
	len += sprintf(sql + len, ",'%d'", records->pic_flag);
	len += sprintf(sql + len, ",'%d'", records->plate_color);
	len += sprintf(sql + len, ",'%s'", records->description);
	len += sprintf(sql + len, ",'%s'", records->ftp_user);
	len += sprintf(sql + len, ",'%s'", records->ftp_passwd);
	len += sprintf(sql + len, ",'%s'", records->ftp_ip);
	len += sprintf(sql + len, ",'%d'", records->ftp_port);
	len += sprintf(sql + len, ",'%s'", records->video_ftp_user);
	len += sprintf(sql + len, ",'%s'", records->video_ftp_passwd);
	len += sprintf(sql + len, ",'%s'", records->video_ftp_ip);
	len += sprintf(sql + len, ",'%d'", records->video_ftp_port);
	len += sprintf(sql + len, ",'%d'", records->flag_send);
	len += sprintf(sql + len, ",'%d'", records->flag_store);
	len += sprintf(sql + len, ",'%d'", records->encode_type);
	len += sprintf(sql + len, ",'%d'", records->vehicle_type);
	len += sprintf(sql + len, ",'%d'", records->color);
	len += sprintf(sql + len, ",'%d'", records->vehicle_logo);
	len += sprintf(sql + len, ");");

	return len;
}


/*******************************************************************************
 * 函数名: db_unformat_read_sql_violation_records_motor_vehicle
 * 功  能: 解析从机动车违法记录表中读出的数据
 * 参  数: azResult，数据库表中读出的数据缓存；buf，违法记录结构体指针
 * 返回值: 0正常，其他为异常
*******************************************************************************/
int db_unformat_read_sql_violation_records_motor_vehicle(char *azResult[],
        DB_ViolationRecordsMotorVehicle *buf)
{
	DB_ViolationRecordsMotorVehicle *illegal_record;
	illegal_record = (DB_ViolationRecordsMotorVehicle *) buf;
	int ncolumn = 0;

	if (illegal_record == NULL)
	{
		printf(
		    "db_unformat_read_sql_violation_records_motor_vehicle: illegal_record is NULL\n");
		return -1;
	}
	if (azResult == NULL)
	{
		printf(
		    "db_unformat_read_sql_violation_records_motor_vehicle: azResult is NULL\n");
		return -1;
	}

	memset(buf, 0, sizeof(DB_ViolationRecordsMotorVehicle));

	illegal_record->ID = atoi(azResult[ncolumn + 0]);
	illegal_record->plate_type = atoi(azResult[ncolumn + 1]);

	sprintf(illegal_record->plate_num, "%s", azResult[ncolumn + 2]);
	sprintf(illegal_record->time, "%s", azResult[ncolumn + 3]);

	illegal_record->violation_type = atoi(azResult[ncolumn + 4]);

	sprintf(illegal_record->point_id, "%s", azResult[ncolumn + 5]);
	sprintf(illegal_record->point_name, "%s", azResult[ncolumn + 6]);
	sprintf(illegal_record->collection_agencies, "%s", azResult[ncolumn + 7]);
	sprintf(illegal_record->dev_id, "%s", azResult[ncolumn + 8]);

	illegal_record->direction = atoi(azResult[ncolumn + 9]);
	illegal_record->lane_num = atoi(azResult[ncolumn + 10]);

	sprintf(illegal_record->red_lamp_start_time, "%s", azResult[ncolumn + 11]);

	illegal_record->red_lamp_keep_time = atoi(azResult[ncolumn + 12]);

	sprintf(illegal_record->image_path[0], "%s", azResult[ncolumn + 13]);
	sprintf(illegal_record->image_path[1], "%s", azResult[ncolumn + 14]);
	sprintf(illegal_record->image_path[2], "%s", azResult[ncolumn + 15]);
	sprintf(illegal_record->image_path[3], "%s", azResult[ncolumn + 16]);
	sprintf(illegal_record->video_path, "%s", azResult[ncolumn + 17]);
	sprintf(illegal_record->partition_path, "%s", azResult[ncolumn + 18]);

	illegal_record->speed = atoi(azResult[ncolumn + 19]);
	illegal_record->coordinate_x = atoi(azResult[ncolumn + 20]);
	illegal_record->coordinate_y = atoi(azResult[ncolumn + 21]);
	illegal_record->width = atoi(azResult[ncolumn + 22]);
	illegal_record->height = atoi(azResult[ncolumn + 23]);
	illegal_record->pic_flag = atoi(azResult[ncolumn + 24]);
	illegal_record->plate_color = atoi(azResult[ncolumn + 25]);

	sprintf(illegal_record->description, "%s", azResult[ncolumn + 26]);
	sprintf(illegal_record->ftp_user, "%s", azResult[ncolumn + 27]);
	sprintf(illegal_record->ftp_passwd, "%s", azResult[ncolumn + 28]);
	sprintf(illegal_record->ftp_ip, "%s", azResult[ncolumn + 29]);

	illegal_record->ftp_port = atoi(azResult[ncolumn + 30]);

	sprintf(illegal_record->video_ftp_user, "%s", azResult[ncolumn + 31]);
	sprintf(illegal_record->video_ftp_passwd, "%s", azResult[ncolumn + 32]);
	sprintf(illegal_record->video_ftp_ip, "%s", azResult[ncolumn + 33]);

	illegal_record->video_ftp_port 	= atoi(azResult[ncolumn + 34]);
	illegal_record->flag_send 		= atoi(azResult[ncolumn + 35]);
	illegal_record->flag_store 		= atoi(azResult[ncolumn + 36]);
	illegal_record->encode_type 	= atoi(azResult[ncolumn + 37]);
	illegal_record->vehicle_type 	= atoi(azResult[ncolumn + 38]);
	illegal_record->color 			= atoi(azResult[ncolumn + 39]);
	illegal_record->vehicle_logo 	= atoi(azResult[ncolumn + 40]);

	return 0;
}


/*******************************************************************************
 * 函数名: db_read_violation_records_motor_vehicle
 * 功  能: 读取数据库中的违法记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_read_violation_records_motor_vehicle(char *db_name, void *records,
        pthread_mutex_t *mutex_db_records, char * sql_cond)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
	char **azResult; //二维数组存放结果
	static char plateNum[16]; //判断同一车牌传送的次数，防止删除数据库失败引起重复传送
	int samePlateCnt = 0; //续传时判断同一车牌传送的次数

	DB_ViolationRecordsMotorVehicle * illegal_record;
	illegal_record = (DB_ViolationRecordsMotorVehicle *) records;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}

	rc = db_create_table(db, SQL_CREATE_TABLE_VIOLATION_RECORDS_MOTOR_VEHICLE);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	//查询数据
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM violation_records_motor_vehicle limit 1");
	nrow = 0;
	rc = sqlite3_get_table(db, sql_cond, &azResult, &nrow, &ncolumn, &pzErrMsg);
	if (rc != SQLITE_OK || nrow == 0)
	{
		//		printf("Can't require data, Error Message: %s\n", pzErrMsg);
		//		printf("row:%d column=%d \n", nrow, ncolumn);
		sqlite3_free(pzErrMsg);
		sqlite3_free_table(azResult);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn][0]);

	sprintf(illegal_record->plate_num, "%s", azResult[ncolumn + 2]);
	printf("illegal_record->plate_num is %s\n", illegal_record->plate_num);
	if (strcmp(illegal_record->plate_num, plateNum) == 0)//判断车牌号是否重复
	{
		samePlateCnt++;
		if (samePlateCnt == 5)
		{
			printf("Can't del data %s\n", pzErrMsg);
			sqlite3_free(pzErrMsg);
			sqlite3_free_table(azResult);
			sqlite3_close(db);
			pthread_mutex_unlock(mutex_db_records);
			return -1;
		}
	}
	else
	{
		sprintf(plateNum, "%s", illegal_record->plate_num);
		samePlateCnt = 0;
	}

	db_unformat_read_sql_violation_records_motor_vehicle(&(azResult[ncolumn]),
	        illegal_record);

	printf("db_read_violation_records_motor_vehicle  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return 0;
}


/*******************************************************************************
 * 函数名: db_read_violation_records_motor_vehicle
 * 功  能: 读取数据库中的违法记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_delete_violation_records_motor_vehicle(char *db_name, void *records,
        pthread_mutex_t *mutex_db_records)
//int db_read(char *db_name, Illegal_Records *buf)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];

	//char **azResult; //二维数组存放结果
	//static char plateNum[16]; //判断同一车牌传送的次数，防止删除数据库失败引起重复传送
	//int samePlateCnt = 0; //续传时判断同一车牌传送的次数

	DB_ViolationRecordsMotorVehicle * illegal_record;
	//	Illegal_Records * illegal_record;
	illegal_record = (DB_ViolationRecordsMotorVehicle *) records;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}
	/*
	 rc = db_create_table(db, SQL_CREATE_TABLE_VIOLATION_RECORDS_MOTOR_VEHICLE);
	 if (rc < 0)
	 {
	 printf("db_create_table failed\n");
	 pthread_mutex_unlock(mutex_db_records);
	 return -1;
	 }
	 */
	//查询数据
	memset(sql, 0, sizeof(sql));

	//sprintf(sql, "SELECT * FROM violation_records_motor_vehicle limit 1");
	if (illegal_record->flag_store == 1)
	{
		//清理续传标志
		sprintf(
		    sql,
		    "UPDATE violation_records_motor_vehicle SET flag_send=0 WHERE ID = %d ;",
		    illegal_record->ID);
	}
	else
	{

		//删除指定ID 的记录	//只有在一个数据库全部续传完成时，才能删除这条对应的记录
		sprintf(sql,
		        "DELETE FROM violation_records_motor_vehicle WHERE ID = %d ;",
		        illegal_record->ID);
	}

	//printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn]);

	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "DELETE FROM violation_records_motor_vehicle WHERE ID = %s ;",	azResult[ncolumn]);
	printf("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("Delete data failed, flag_store=%d,  Error Message: %s\n",
		       illegal_record->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	//sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	printf("db_delete_violation_records_motor_vehicle  finish\n");

	return 0;
}


/*******************************************************************************
 * 函数名: send_violation_records_image_file
 * 功  能: 发送违法记录图片文件
 * 参  数: pic_info，图片文件信息；image_info，图像信息
 * 返回值: 成功，返回0；上传失败，返回-1；文件不存在，返回-2
*******************************************************************************/
int send_violation_records_image_file(
    DB_ViolationRecordsMotorVehicle* db_violation_record,
    EP_PicInfo pic_info[], int num_pic)
{
	char file_name_pic[NAME_MAX_LEN];
	int i;
	debug("num_pic:%d\n\n", num_pic);
	for (i = 0; i < num_pic; i++)
	{
		sprintf(file_name_pic, "%s/%s/%s",
		        db_violation_record->partition_path,
		        DISK_RECORD_PATH,
		        db_violation_record->image_path[i]);

		if (access(file_name_pic, F_OK) < 0)
		{
			log_debug_storage("%s does not exist !\n", file_name_pic);
			return -2;
		}

		debug("===============violation pic %d =========================\n", i);
		if (ftp_send_pic_file(file_name_pic, &pic_info[i],
		                      FTP_CHANNEL_ILLEGAL_RESUMING) < 0)
			return -1;

		move_record_to_trash(db_violation_record->partition_path,
		                     db_violation_record->image_path[i]);
	}

	return 0;
}


/*******************************************************************************
 * 函数名: send_violation_records_image_buf
 * 功  能: 发送缓存中的违法记录图片
 * 参  数: pic_info，图片文件信息；image_info，图像信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_violation_records_image_buf(EP_PicInfo pic_info[], int num_pic)
{
	for (int i = 0; i < num_pic; i++)
	{
		if (ftp_send_pic_buf(&pic_info[i], FTP_CHANNEL_ILLEGAL) < 0)
			return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: send_violation_records_video_buf
 * 功  能: 发送缓存中的违法记录视频
 * 参  数: vid_info，视频文件信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_violation_records_video_buf(EP_VidInfo *vid_info)
{
	if (ftp_get_status_violation_video() < 0)
		return -1;

	return ftp_send_violation_video_buf(vid_info);
}


/*******************************************************************************
 * 函数名: send_violation_records_video_file
 * 功  能: 发送存储的违法记录视频
 * 参  数: db_violation_record，记录信息；vid_info，视频文件信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_violation_records_video_file(
    DB_ViolationRecordsMotorVehicle* db_violation_record, EP_VidInfo *vid_info)
{
	char file_name[NAME_MAX_LEN];
	sprintf(file_name, "%s/%s/%s",
	        db_violation_record->partition_path,
	        DISK_RECORD_PATH,
	        db_violation_record->video_path);

	debug("===============violation vid %s =====================\n", file_name);
	if (ftp_send_pic_file(file_name, (EP_PicInfo*) vid_info,
	                      FTP_CHANNEL_ILLEGAL_RESUMING) < 0)
		return -1;

	move_record_to_trash(db_violation_record->partition_path,
	                     db_violation_record->video_path);

	return 0;
}


/*******************************************************************************
 * 函数名: send_violation_records_motor_vehicle_info
 * 功  能: 发送违法记录信息
 * 参  数: records，违法信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_violation_records_motor_vehicle_info(void *records)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	format_mq_text_violation_records_motor_vehicle(mq_text,
	        (DB_ViolationRecordsMotorVehicle *) records);

	printf("violation info:%s\n",mq_text);

	mq_send_violation_records_motor_vehicle(mq_text);

	TRACE_LOG_SYSTEM("illegle info : %s", mq_text);


	#ifdef VP_YANTAI
	//为海信平台专用
	char mq_text_hisense[MQ_TEXT_BUF_SIZE];
	format_mq_text_vp_records_hisense(mq_text_hisense,
	        (DB_ViolationRecordsMotorVehicle *) records);

	printf("vp info for hisense:%s\n",mq_text_hisense);

	mq_send_vp_records_hisense(mq_text_hisense);
	#endif

	return 0;
}


/*******************************************************************************
 * 函数名: send_violation_records_motor_vehicle_info_history
 * 功  能: 发送历史违法记录信息
 * 参  数: records，违法信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_violation_records_motor_vehicle_info_history(void *records,
        int dest_mq, int num_record)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	format_mq_text_violation_records_motor_vehicle(mq_text,
	        (DB_ViolationRecordsMotorVehicle *) records);
	debug("violation info: %s \n", mq_text);
	mq_send_violation_records_motor_vehicle_history(mq_text, dest_mq,
	        num_record);

	return 0;
}


/*******************************************************************************
 * 函数名: save_violation_records_image_buf
 * 功  能: 保存缓存中的违法记录图片
 * 参  数: pic_info，图片信息；image_info，图像信息；partition_path，分区路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_violation_records_image_buf(
    const EP_PicInfo pic_info[],
    const void *image_info, const char *partition_path)
{
	Pcie_data_head *info = (Pcie_data_head *) image_info;
	unsigned int i;

	//保证第一级路径存在(例如: /mnt/sda1/record)
	char record_path[PATH_MAX_LEN];
	sprintf(record_path, "%s/%s", partition_path, DISK_RECORD_PATH);
	int ret = dir_create(record_path);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", record_path);
		return -1;
	}

	debug("num_pic:%d\n\n",info->NumPic);
	for (i = 0; i < info->NumPic; i++)
	{
		debug("===============violation pic [%d] =======================\n", i);
		ret = file_save(record_path, pic_info[i].path, pic_info[i].name,
		                pic_info[i].buf, pic_info[i].size);
		if (ret != 0)
			return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: save_violation_records_video_buf
 * 功  能: 保存缓存中的违法视频
 * 参  数: vid_info，视频文件信息；partition_path，分区路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_violation_records_video_buf(
    const EP_VidInfo *vid_info, const char *partition_path)
{
	int i;

	//保证第一级路径存在(例如: /mnt/sda1/record) --应该不需要每次判断
	char record_path[PATH_MAX_LEN];
	sprintf(record_path, "%s/%s", partition_path, DISK_RECORD_PATH);
	int ret = dir_create(record_path);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", record_path);
		return -1;
	}

	debug("video seg:%d\n\n", vid_info->buf_num);
	for (i = 0; i < vid_info->buf_num; i++)
	{
		debug("===============violation video [%d] =====================\n", i);
		ret = file_save_append(record_path, vid_info->path, vid_info->name,
		                       vid_info->buf[i], vid_info->size[i]);
		if (ret != 0)
			return -1;
	}

	return 0;

}


/*******************************************************************************
 * 函数名: save_violation_records_motor_vehicle_info
 * 功  能: 保存机动车违法记录信息
 * 参  数: db_violation_records，违法记录；
 *         image_info，图像信息；partition_path，分区路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_violation_records_motor_vehicle_info(
    const DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const char *partition_path)
{

	char db_violation_name[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/violation_records/2013082109.db
	static DB_File db_file;
	//static char db_violation_name_last[PATH_MAX_LEN];
	int flag_record_in_DB_file = 0;//是否需要记录到索引数据库
	illegalRecordVehicle *illegal_records =
	    (illegalRecordVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/violation_records");
	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	sprintf(db_violation_name, "%s/%04d%02d%02d%02d.db", dir_temp,
	        illegal_records->picInfo[0].year,
	        illegal_records->picInfo[0].month,//应该使用第二个索引对应的时间，可是，可能是合并为一张的? 没有第二张
	        illegal_records->picInfo[0].day, illegal_records->picInfo[0].hour);

	//判断数据库文件是否已经存在
	if (access(db_violation_name, F_OK) != 0)
	{
		//数据库文件不存在，需要在索引数据库中增加相应的一条记录
		flag_record_in_DB_file = 1;
	}
	else
	{
		flag_record_in_DB_file = 0;
	}

	//判断新生成文件名，是否与上一次的重复
	//以后添加－－也有可能与多次之前的重复，需要循环判断－－
	//重启后，不记得上一次的名称
	//if( (flag_upload_complete_violation==1)||(strcasecmp(db_violation_name_last, db_violation_name)))//是新数据库名称

	if (flag_record_in_DB_file == 1)//需要保证在删除索引记录时，同时删对应交通数据库
	{
		//printf("db_violation_name_last is %s, db_violation_name is %s\n",
		//		db_violation_name_last, db_violation_name);
		printf("add to DB_files, db_violation_name is %s\n", db_violation_name);

		//写一条记录到数据库名管理
		DB_File db_file;

		db_file.record_type = 1;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_violation_name,
		       strlen(db_violation_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_violation_records->time, strlen(
		           db_violation_records->time));
		db_file.flag_send = db_violation_records->flag_send;
		db_file.flag_store = db_violation_records->flag_store;

/*-------------------------创建索引数据库目录---------------------------*/

        sprintf(dir_temp, "%s",DB_FILES_PATH );

        int ret = dir_create( dir_temp);
        if (ret != 0)
        {
            printf("dir_create %s failed\n",  dir_temp);
            return -1;
        }
        else
        {
            resume_print("dir_create %s  success  \n",dir_temp);
        }

		ret = db_write_DB_file((char*) DB_NAME_VIOLATION_RECORDS, &db_file,
		                       &mutex_db_files_violation);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf(
			    "db_write_DB_file : insert %s  in DB_files_violation.db ok\n",
			    db_violation_name);
		}
	}
	else
	{
		// 第一次写索引数据库时，可能是两个条件中任意一个触发，（断网、实时存储）
		// 所以，在另一个条件发生时，需要相应修改索引数据库。

		static int flag_first = 1;
		if (flag_first == 1)
		{
			//第一次进入，要读取最新的一条索引记录。
			flag_first = 0;

			char sql_cond[1024];
			char history_time_start[64];//时间起点 示例：2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_violation_records->time, history_time_start);
			sprintf(sql_cond,
			        "SELECT * FROM DB_files WHERE time>='%s'  limit 1;",
			        history_time_start);
			db_read_DB_file(DB_NAME_VIOLATION_RECORDS, &db_file,
			                &mutex_db_files_violation, sql_cond);

		}

		char sql_cond[1024];
		printf("db_file.flag_send=%d,db_violation_records->flag_send=%d\n",
		       db_file.flag_send, db_violation_records->flag_send);
		if ((~(db_file.flag_send) & db_violation_records->flag_send) != 0) //若新记录中包含了新的续传标志信息
		{
			db_file.flag_send |= db_violation_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;",
			        db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_VIOLATION_RECORDS, sql_cond,
			                  &mutex_db_files_violation);
		}

		if ((db_file.flag_store == 0)
		        && (db_violation_records->flag_store != 0))//需要增加存储信息
		{
			db_file.flag_store = db_violation_records->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;",
			        db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_VIOLATION_RECORDS, sql_cond,
			                  &mutex_db_files_violation);
		}

	}

	ret = db_write_violation_records_motor_vehicle(db_violation_name,
	        (DB_ViolationRecordsMotorVehicle *) db_violation_records,
	        &mutex_db_records_violation);
	if (ret != 0)
	{
		printf("db_write_violation_records_motor_vehicle failed\n");
		return -1;
	}

	return ret;

}


/*******************************************************************************
 * 函数名: db_write_violation_records
 * 功  能: 写非机动车的交通违法记录数据库
 * 参  数: records，交通违法记录
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_write_violation_records_others(char *db_violation_name,
                                      DB_ViolationRecordsMotorVehicle *records)
{
	char sql[SQL_BUF_SIZE] = {0};

	db_format_insert_sql_violation_records_others(sql, records);

	int ret = db_write(db_violation_name, sql,
	                   SQL_CREATE_TABLE_VIOLATION_RECORDS_MOTOR_VEHICLE);
	if (ret != 0)
	{
		printf("db_write_violation_records_others failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: db_format_insert_sql_violation_records_others
 * 功  能: 格式化行人或非机动车违法记录表SQL插入语句
 * 参  数: sql，SQL缓存；buf，数据缓冲区指针
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_format_insert_sql_violation_records_others(char *sql, void *buf)
{
	/* TODO 目前没有该需求 */
	return 0;
}


/*******************************************************************************
 * 函数名: process_violation_records_others
 * 功  能: 处理行人和非机动车违法记录信息
 * 参  数: image_info，图像信息；video_info，视频信息
 * 返回值:
*******************************************************************************/
int process_violation_records_others(
    const void *image_info, const void *video_info)
{
	/* TODO 行人和非机动车违法处理 */
	return 0;
}





//违停记录处理：修改，视频为低优先级，发送失败不影响记录的发送

int process_vp_records(
    const void *image_info, const void *video_info)
{

	printf("process_PD_records_motor_vehicle:\n");

	if (image_info == NULL)
		return -1;

//	log_debug_storage("Start to process motor vehicle violation records.\n");

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);
//
	int disk_status = get_cur_partition_path(partition_path);
	resume_print("get_cur_partition_path:%d\n",disk_status);
	resume_print("the path is%s\n",partition_path);

	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
    memset(pic_info, 0x0, sizeof(EP_PicInfo) * ILLEGAL_PIC_MAX_NUM);

	EP_VidInfo vid_info;
	DB_ViolationRecordsMotorVehicle db_violation_records;
	int flag_send = 0;
	int li_i = 0;

	IllegalInfoPer *illegalInfoPer = (IllegalInfoPer *)&((SystemVpss_SimcopJpegInfo *)image_info)->algResultInfo.AlgResultInfo;

	//SystemVpss_SimcopJpegInfo


	do /* 处理图片 */
	{
		if (analyze_PD_records_picture(pic_info, image_info) < 0)
			return -1;

//		int pic_num = ((IllegalInfoPer *)image_info)->picNum;

		int pic_num = illegalInfoPer->picNum;

		if (EP_UPLOAD_CFG.illegal_picture == 1)
		{
			printf("to send_violation_records_image_buf\n");
			if (send_violation_records_image_buf(pic_info, pic_num) < 0)
			{
				flag_send |= 0x02; 	/* 图片上传失败，需要续传 */
				log_debug_storage(
				    "Send violation record image of motor vehicle failed !\n");
			}
			else //if(send_violation_records_image_buf(pic_info, pic_num)==0)
				{
					resume_print("upload pictures success!\n");
			}
		}

/*--------------------------------------图片存SD-------------------------------------------------------------*/
		if (((flag_send & 0x02) != 0) ||
		        (EP_DISK_SAVE_CFG.illegal_picture == 1))
		{
			resume_print("start   save   picture !\n");
			if (disk_status == 0)
			{
				disk_status = save_violation_records_image_buf(
				                  pic_info, image_info, partition_path);
				resume_print("save   picture  finished ! The disk_status is %d\n",disk_status);
				if (disk_status < 0)
				{
					resume_print("save   picture   failed!\n");
					log_warn_storage("Save violation record image of "
					                 "motor vehicle failed !\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, violation record "
				                 "image of motor vehicle is discarded.\n");
			}
		}

	}
	while (0);


	do /* 处理视频 */
	{
		if (video_info == NULL)
		{
			memset(&vid_info, 0, sizeof(EP_VidInfo));
			break;
		}

		analyze_violation_records_video(&vid_info, image_info, video_info);

		if (EP_UPLOAD_CFG.illegal_video == 1)
		{
			if (send_violation_records_video_buf(&vid_info) < 0)
			{
				resume_print("upload video failed!!\n");
				flag_send |= 0x04; 	/* 视频上传失败，需要续传 */
				log_debug_storage(
				    "Send violation record video of motor vehicle failed !\n");
			}
			else //if(send_violation_records_video_buf(&vid_info) ==0)
			{
				resume_print("upload video success!\n");

			}
		}

/*-------------------------------------视频-存SD-------------------------------------------------------------*/
		resume_print("start store video\n");

		if (((flag_send & 0x04) != 0) || (EP_DISK_SAVE_CFG.illegal_video == 1))
		{
			if (disk_status == 0)
			{
				if (save_violation_records_video_buf(&vid_info, partition_path)
				        < 0)
				{
					log_warn_storage("Save violation record video of "
					                 "motor vehicle failed !\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, violation record "
				                 "video of motor vehicle is discarded.\n");
			}
		}
/*-------------------------------------------------------------------------------------------------*/


	}
	while (0);


	do /* 处理信息 */
	{
		printf("to analyze_PD_records_motor_vehicle_info\n");
		if (analyze_PD_records_motor_vehicle_info(
		            &db_violation_records, illegalInfoPer, video_info,
		            pic_info, &vid_info, NULL) < 0)//partition_path
		{
			printf("analyze violation record information of motor vehicle failed !\n");
//			log_debug_storage("analyze violation record information of "
//			                  "motor vehicle failed !\n");
			return -1;
		}

		if (EP_UPLOAD_CFG.illegal_picture == 1)
		{
			flag_send |= 0x01; 		/* 先设置为需要续传 */
			printf("to send_violation_records_motor_vehicle_info :\n");
			if (((flag_send & 0x02) == 0) &&
			        (send_violation_records_motor_vehicle_info(
			             &db_violation_records) == 0))
			{
				printf("send_violation_records_motor_vehicle_info : successful!\n");
				flag_send = 0x00; 	/* 如果上传成功，再恢复为无需上传 */
			}
			else
				{
				resume_print("upload info failed \n");
			}
		}

        str_vp_msg lstr_vp_msg;
        if(gi_Uniview_illegally_park_switch)
        {
            //send records to vpuniview message queue
            for(li_i=0; li_i<illegalInfoPer->picNum; li_i++)
            {
                gstr_tvpuniview_records.picture_size[li_i] = pic_info[li_i].size;
                //memcpy(gstr_tvpuniview_records.picture[li_i], pic_info[li_i].buf, gstr_tvpuniview_records.picture_size[li_i]);
                gstr_tvpuniview_records.picture[li_i] = (unsigned char *)pic_info[li_i].buf;
            }
            memset(&lstr_vp_msg, 0, sizeof(str_vp_msg));
            strcpy(lstr_vp_msg.uniview.data, "tvpuniview");
            lstr_vp_msg.uniview.data_lens = strlen("tvpuniview");
            memcpy(&gstr_tvpuniview_records.field, &db_violation_records, sizeof(DB_ViolationRecordsMotorVehicle));
            INFO("begin to send pic info to uniview platform.");
            vmq_sendto_tvpuniview(lstr_vp_msg);
		}

        if(gi_Baokang_illegally_park_switch)
        {
            //send records to baokang message queue
            memset(&g_baokang_info, 0, sizeof(baokang_info_s)*VP_BK_PIC_COUNT);
            for(li_i=0; li_i<illegalInfoPer->picNum; li_i++)
            {
                g_baokang_info[li_i].pic_size = pic_info[li_i].size;
                //memcpy(g_baokang_info[li_i].pic, pic_info[li_i].buf, pic_info[li_i].size);
                g_baokang_info[li_i].pic = (char *)pic_info[li_i].buf;
            }

            // send void message just to notify baokang you should receive pic.
            memset(&lstr_vp_msg, 0, sizeof(str_vp_msg));
            TRACE_LOG_SYSTEM("begin to send pic info to baokang platform.");
            vmq_sendto_tvpbaokang(lstr_vp_msg);
        }
/*-------------------------------------上传失败则转为信息存SD---------------------------------------------------*/
		if (((flag_send & 0x01) != 0) ||
		        (EP_DISK_SAVE_CFG.illegal_picture == 1))
		{

			resume_print("save_violation_records_motor_vehicle_info \n");
			if (disk_status == 0)
			{
				db_violation_records.flag_send = flag_send;
				if (EP_DISK_SAVE_CFG.illegal_picture == 1)
				{
					db_violation_records.flag_store |= 0x01;
				}
				if (EP_DISK_SAVE_CFG.illegal_video == 1)
				{
					db_violation_records.flag_store |= 0x02;
				}
				resume_print("db_violation_records.flag_store is %d\n",db_violation_records.flag_store);
				resume_print("save info --path is %s\n",partition_path);

				if (save_violation_records_motor_vehicle_info(
				            &db_violation_records,
				            image_info, partition_path) < 0)
				{
					resume_print("save_violation_records_motor_vehicle_info   failed\n");


					log_error_storage("Save violation record information of "
					                  "motor vehicle failed !\n");
				}
			}
			else
			{
				log_debug_storage(
				    "The disk is not available, violation record information "
				    "of motor vehicle is discarded.\n");
			}
		}


/*--------------------------------------------------------------------------------------------------*/
	}
	while (0);

	printf("process_PD_records_motor_vehicle end.\n");

	return 0;
}






/*******************************************************************************
 * 函数名: analyze_PD_records_motor_vehicle_info
 * 功  能: 解析机动车违停记录信息
 * 参  数: image_info，图像信息；video_info，视频信息
 * 返回值:
*******************************************************************************/
int analyze_PD_records_motor_vehicle_info(
    DB_ViolationRecordsMotorVehicle *db_violation_records,
    const void *image_info, const void *video_info,
    const EP_PicInfo pic_info[], const EP_VidInfo *vid_info,
    const char *partition_path)
{
	unsigned int i;
	int index;

	VDConfigData *p_dsp_cfg = (VDConfigData  *)get_dsp_cfg_pointer();

	IllegalInfoPer * PD_retult = (IllegalInfoPer *)image_info;//(&((VDIllegalReturn *)image_info)->illegalInfo[0]);
	unsigned int pic_num = PD_retult->picNum ;
	printf("[resume]###################in  analyze_PD_records_motor_vehicle_info :the pic_num is %d info############################\n",PD_retult->picNum);
	printf("in analyze_PD_records_motor_vehicle_info:\n");
	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
//		log_error_storage
		printf("motor vehicle violation record picture number "
		                  "error: %d\n", pic_num);
		return -1;
	}

//	illegalRecordVehicle *violation_records =
//	    (illegalRecordVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);

	memset(db_violation_records, 0, sizeof(DB_ViolationRecordsMotorVehicle));

	db_violation_records->plate_type = PD_retult->vehicleFeature.plateType;
	    //violation_records->illegalVehicle.plateType;
	sprintf(db_violation_records->plate_num, "%s",
//	        violation_records->illegalVehicle.strResult);
			PD_retult->vehicleFeature.plateNumber);
	index = 0;//1;//violation_records->illegalVehicle.picFlag - 1;

	for(int i=0;i<MAX_CAPTURE_NUM;i++)//车牌框可能出现在任意一张图片上
	{
		index=i;
//		printf("analyze_violation_records_motor_vehicle_info: i=%d  plate.(w,h) is (%d,%d)\n",i,PD_retult->illegalPics[i].plateRect.width,PD_retult->illegalPics[i].plateRect.height);
		if((PD_retult->illegalPics[i].plateRect.width !=0)
				&& (PD_retult->illegalPics[i].plateRect.height !=0))
		{
			break;
		}
	}

	if ((index < 0) || (index >= MAX_CAPTURE_NUM) )
	{
		debug( "illegalVehicle.picFlag = %d. illegal data.\n",
				index);
		index = 0;
	}

	sprintf(db_violation_records->time, "%04d-%02d-%02d %02d:%02d:%02d",
//	        violation_records->picInfo[index].year,
//	        violation_records->picInfo[index].month,
//	        violation_records->picInfo[index].day,
//	        violation_records->picInfo[index].hour,
//	        violation_records->picInfo[index].minute,
//	        violation_records->picInfo[index].second);
			PD_retult->illegalPics[index].time.tm_year,
			PD_retult->illegalPics[index].time.tm_mon,
			PD_retult->illegalPics[index].time.tm_mday,
			PD_retult->illegalPics[index].time.tm_hour,
			PD_retult->illegalPics[index].time.tm_min,
			PD_retult->illegalPics[index].time.tm_sec);


			get_violation_code_index_PD(
	            PD_retult->illegalType,
	            &(db_violation_records->violation_type));

	printf("in analyze_PD_records_motor_vehicle_info:after get_violation_code_index_PD\n");

	sprintf(db_violation_records->point_id, "%s", EP_POINT_ID);
	sprintf(db_violation_records->point_name, "%s", EP_POINT_NAME);
	snprintf(db_violation_records->collection_agencies,
	         EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);
	sprintf(db_violation_records->dev_id, "%s", EP_EXP_DEV_ID);

	db_violation_records->direction = EP_DIRECTION;
	db_violation_records->lane_num = PD_retult->laneNum;	//没有车道号  //violation_records->illegalVehicle.laneNum;


	for (i = 0; i < pic_num; i++)
	{
		get_ftp_path(EP_FTP_URL_LEVEL,
//		             violation_records->picInfo[i].year,
//		             violation_records->picInfo[i].month,
//		             violation_records->picInfo[i].day,
//		             violation_records->picInfo[i].hour,
		 			 PD_retult->illegalPics[i].time.tm_year,
		 			 PD_retult->illegalPics[i].time.tm_mon,
		 			 PD_retult->illegalPics[i].time.tm_mday,
		 			 PD_retult->illegalPics[i].time.tm_hour,

		             (char*) VIOLATION_RECORDS_FTP_DIR,
		             db_violation_records->image_path[i]);

		strcat(db_violation_records->image_path[i], pic_info[i].name);// zmh: to change

		printf("db_violation_records->image_path[%d]: %s\n",i, db_violation_records->image_path[i]);

	}

	if ( (video_info) && (EP_UPLOAD_CFG.illegal_video== 1) )
	{
		get_ftp_path(EP_FTP_URL_LEVEL,
//		             violation_records->picInfo[0].year,
//		             violation_records->picInfo[0].month,
//		             violation_records->picInfo[0].day,
//		             violation_records->picInfo[0].hour,
		 			 PD_retult->illegalPics[0].time.tm_year,
		 			 PD_retult->illegalPics[0].time.tm_mon,
		 			 PD_retult->illegalPics[0].time.tm_mday,
		 			 PD_retult->illegalPics[0].time.tm_hour,
		             (char*) VIOLATION_RECORDS_FTP_DIR,
		             db_violation_records->video_path);

		strcat(db_violation_records->video_path, vid_info->name);
		printf("video_path is %s.\n", db_violation_records->video_path);
	}
//	printf("in analyze_PD_records_motor_vehicle_info:after 3.1\n");

	sprintf(db_violation_records->partition_path, "%s", MMC_MOUNT_POINT);//partition_path);
//	printf("in analyze_PD_records_motor_vehicle_info:after 3.2\n");
	db_violation_records->speed = 0;//violation_records->illegalVehicle.speed;


	db_violation_records->coordinate_x =// violation_records->illegalVehicle.xPos;
										PD_retult->illegalPics[index].plateRect.x;
	db_violation_records->coordinate_y = //violation_records->illegalVehicle.yPos;
										PD_retult->illegalPics[index].plateRect.y;
	db_violation_records->width = //violation_records->illegalVehicle.width;
									PD_retult->illegalPics[index].plateRect.width;
	db_violation_records->height = //violation_records->illegalVehicle.height;
									PD_retult->illegalPics[index].plateRect.height;
	if(PD_retult->picNum==1)//图片拼接为1张的，车牌位置必然标记在第一张图片上
	{
		db_violation_records->pic_flag = 1;
	}
	else//没有做拼接的，标记在算法指定图片上
	{
		db_violation_records->pic_flag = index+1;//1;//violation_records->illegalVehicle.picFlag;
	}
//	printf("analyze_PD_records_motor_vehicle_info: index=%d\n",index);
//	printf("x=%d,y=%d,width=%d,height=%d\n",
//			db_violation_records->coordinate_x,	db_violation_records->coordinate_y,
//			db_violation_records->width,	db_violation_records->height
//			);


	db_violation_records->plate_color = //violation_records->illegalVehicle.color;
										PD_retult->vehicleFeature.plateColor;
	db_violation_records->encode_type = ((Pcie_data_head *)image_info)->EncType;

//	printf("in analyze_PD_records_motor_vehicle_info:after 3.3\n");
//	int first = 0;
	strcpy(db_violation_records->description, "");

	TRACE_LOG_SYSTEM("PD_retult->illegalType=%d\n", PD_retult->illegalType);

	switch(PD_retult->illegalType)
	{
		case 1:
			strcat(db_violation_records->description, "非法停车");
		break;
		case 64:
			if(p_dsp_cfg != NULL)
			{
				strcat(db_violation_records->description,
						(char *)p_dsp_cfg->vdConfig.vdCS_config.overlay_violate.violate_desc.converse_drive);
			}
			else
			{
				strcat(db_violation_records->description, "逆行");
			}
		break;
		case 2048:
			strcat(db_violation_records->description, "强行驶入路口");
		break;
		case 32768:
			if(p_dsp_cfg != NULL)
			{
				strcat(db_violation_records->description,
						(char *)p_dsp_cfg->vdConfig.vdCS_config.overlay_violate.violate_desc.forced_cross);
			}
			else
			{
				strcat(db_violation_records->description, "未交费闯关");
			}
		break;
	}

//	printf("in analyze_PD_records_motor_vehicle_info:after 4\n");

	sprintf(db_violation_records->ftp_user, "%s", EP_VIOLATION_FTP.user);
	sprintf(db_violation_records->ftp_passwd, "%s", EP_VIOLATION_FTP.passwd);
	sprintf(db_violation_records->ftp_ip, "%d.%d.%d.%d",
	        EP_VIOLATION_FTP.ip[0],
	        EP_VIOLATION_FTP.ip[1],
	        EP_VIOLATION_FTP.ip[2],
	        EP_VIOLATION_FTP.ip[3]);
	db_violation_records->ftp_port = EP_VIOLATION_FTP.port;

	sprintf(db_violation_records->video_ftp_user, "%s", EP_VIOLATION_FTP.user);
	sprintf(db_violation_records->video_ftp_passwd, "%s",
	        EP_VIOLATION_FTP.passwd);
	sprintf(db_violation_records->video_ftp_ip, "%d.%d.%d.%d",
	        EP_VIOLATION_FTP.ip[0],
	        EP_VIOLATION_FTP.ip[1],
	        EP_VIOLATION_FTP.ip[2],
	        EP_VIOLATION_FTP.ip[3]);
	db_violation_records->video_ftp_port = EP_VIOLATION_FTP.port;

	db_violation_records->vehicle_type =
//	    violation_records->illegalVehicle.vehicleType;
			PD_retult->vehicleFeature.vehicleType;
	db_violation_records->color =
//	    violation_records->illegalVehicle.vehicleColor;
			PD_retult->vehicleFeature.vehicleColor;
	db_violation_records->vehicle_logo =
//	    violation_records->illegalVehicle.vehicleLogo;
			PD_retult->vehicleFeature.vehicleLogo;

	return 0;
}

//char buf_jpeg[4][500*1024];

//
//void Init_Interface(int Msg_id)
//{
//	int qid;
//	printf("ApproDrvInit:  ##########\n");
//	if(ApproDrvInit(Msg_id))
//	{
//		printf("ApproDrvInit failed:  ##########\n");
//		exit(1);
//	}
//
//	//printf("func_get_mem:  ##########\n");
//	if (func_get_mem(&qid))
//	{
//		printf("func_get_mem failed:  ##########\n");
//		ApproDrvExit();
//		printf("after ApproDrvExit:  ##########\n");
//		exit(1);
//	}
//
//	printf("after func_get_mem:  ##########\n");
//
//}

//int get_jpeg(char *buf_data,int num)
//{
//
//	memset(buf_data, 0x10, 200*1024);
//
//	return 200*1024;
//
//
//
//	int vType = 0;//STREAM_FRAME_ID;
//	int	SerialBook = -1;
//	int	SerialLock = -1;
//
//	AV_DATA av_data;
//
//	static int flag_first=1;
//	if(flag_first==1)
//	{
//		flag_first=0;
//		ApproDrv_SetProcId(MSG_TYPE_MSG17);
//	//	Init_Interface(MSG_TYPE_MSG17);//15
//		Init_Appro();//MSG_TYPE_MSG17);
//	}
//
//	int num_wait=0;
//	do
//	{
//		printf("GetAVData: Wait for jpeg ......%d\n",num_wait);
//		if(num_wait%200==199)//每分钟打印一次
//		{
//
//			return -1;
////			break;
//		}
//		GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data );
//		usleep(300000);
//		num_wait++;
//		printf("av_data.flags=%d\n",av_data.flags);
//
//	} while (0);//(av_data.flags != AV_FLAGS_MP4_I_FRAME);
//
//	printf("Wait for jpeg done.\n");
//	SerialBook = av_data.serial;
//
////	AV_DATA av_data;
////	int ret = GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data);
//
//	int ret = GetAVData(AV_OP_LOCK_MJPEG, SerialBook, &av_data );
//	if (ret == RET_SUCCESS)
//	{
////		nal_info.nal=(char *)av_data.ptr;
////		nal_info.nal_size=av_data.size;
////		nal_info.nal_IDC_type=av_data.frameType;
////		nal_info.timestamp=((long long)av_data.timestamp)*90;  //对于一个已经越界过的数，随时都会回卷//时间戳本身就可能越界
//		printf("get jpeg ,memcpy size=%.\n",av_data.size);
//		memcpy(buf_data, (char *)av_data.ptr, av_data.size);
//
//		if (SerialLock >= 0)
//		{
//			GetAVData(AV_OP_UNLOCK_MJPEG, SerialLock, &av_data);
//		}
//
//		SerialLock = SerialBook;
//		SerialBook++;
//		return av_data.size;
//
//
//	}
//	else
//	{
//		GetAVData(AV_OP_UNLOCK_MJPEG, SerialLock, &av_data);
//		return -1;
//	}
//
//
//
//}

//分析违停的图片信息
int analyze_PD_records_picture(
    EP_PicInfo pic_info[], const void *image_info)
{

	IllegalInfoPer * PD_retult = (IllegalInfoPer *)&((SystemVpss_SimcopJpegInfo *)image_info)->algResultInfo.AlgResultInfo;//(IllegalInfoPer *)image_info;//(&((VDIllegalReturn *)image_info)->illegalInfo[0]);
	unsigned int pic_num = PD_retult->picNum ;

//	Pcie_data_head *info = (Pcie_data_head *) image_info;
//	illegalRecordVehicle *violation_records =
//	    (illegalRecordVehicle *) ((char *) info + IMAGE_INFO_SIZE);
//	unsigned int pic_num = info->NumPic;
	unsigned int i;
	static int count = 0;//增加一个记录计数器，避免不同记录的图片名称重复
	count++;

	printf("analyze_PD_records_picture: pic_num=%d\n",pic_num);

	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Violation records picture number error: %d\n",
		                  pic_num);
		return -1;
	}

	for (i=0; i<pic_num; i++)
	{
		printf("analyze_PD_records_picture: i=%d\n",i);
		memset(&pic_info[i], 0, sizeof(EP_PicInfo));

		/* 路径按照用户的配置进行设置 */
		for (int j = 0; j < EP_FTP_URL_LEVEL.levelNum; j++)
		{
			switch (EP_FTP_URL_LEVEL.urlLevel[j])
			{
			case SPORT_ID: 		//地点编号
				sprintf(pic_info[i].path[j], "%s", EP_POINT_ID);
				break;
			case DEV_ID: 		//设备编号
				sprintf(pic_info[i].path[j], "%s", EP_DEV_ID);
				break;
			case YEAR_MONTH: 	//年/月
				sprintf(pic_info[i].path[j], "%04d%02d",
//				        violation_records->picInfo[i].year,
//				        violation_records->picInfo[i].month);
						PD_retult->illegalPics[i].time.tm_year,
						PD_retult->illegalPics[i].time.tm_mon);
				break;
			case DAY: 			//日
				sprintf(pic_info[i].path[j], "%02d",
//				        violation_records->picInfo[i].day);
						PD_retult->illegalPics[i].time.tm_mday);
				break;
			case EVENT_NAME: 	//事件类型
				sprintf(pic_info[i].path[j], "%s", VIOLATION_RECORDS_FTP_DIR);
				break;
			case HOUR: 			//时
				sprintf(pic_info[i].path[j], "%02d",
//				        violation_records->picInfo[i].hour);
						PD_retult->illegalPics[i].time.tm_hour);
				break;
			case FACTORY_NAME: 	//厂商名称
				sprintf(pic_info[i].path[j], "%s", EP_MANUFACTURER);
				break;
			default:
				break;
			}
		}

		/* 文件名以时间和车道号命名 */
		sprintf(pic_info[i].name, "%s%04d%02d%02d%02d%02d%02d%03d%02d%02d%d.jpg",
		        EP_EXP_DEV_ID,
//		        violation_records->picInfo[i].year,
//		        violation_records->picInfo[i].month,
//		        violation_records->picInfo[i].day,
//		        violation_records->picInfo[i].hour,
//		        violation_records->picInfo[i].minute,
//		        violation_records->picInfo[i].second,
//		        violation_records->picInfo[i].msecond,
//		        violation_records->illegalVehicle.laneNum,

				PD_retult->illegalPics[i].time.tm_year,
				PD_retult->illegalPics[i].time.tm_mon,
				PD_retult->illegalPics[i].time.tm_mday,
				PD_retult->illegalPics[i].time.tm_hour,
				PD_retult->illegalPics[i].time.tm_min,
				PD_retult->illegalPics[i].time.tm_sec,
				PD_retult->illegalPics[i].time.tm_msec,//500,//ms
				1,

		        i + 1,
		        count);



		/* 获取图片缓存的位置 */

		printf("to get_jpeg\n");
//		int ret=get_jpeg(buf_jpeg[i],0);
//		pic_info[i].buf = buf_jpeg[i];//(char *)av_data.ptr;

		//获取jpeg存放地址		---怎样保证jpeg的匹配，需要按照违法信息寻找相应jpeg！！！！！！ ****************************
		//循环遍历叠加完成的jpeg缓冲，寻找ID匹配的。

		char * buf_jpeg_start=(char *)((unsigned int)image_info- ((SystemVpss_SimcopJpegInfo *)image_info)->jpeg_bufid * SIZE_JPEG_BUFFER);
		printf("image_info addr=0x%x, buf_jpeg_start=0x%x",(unsigned int)image_info, (unsigned int)buf_jpeg_start);
		int num_offset_jpeg;

		for(num_offset_jpeg=NUM_JPEG_SPLIT_BUFFER; num_offset_jpeg<NUM_JPEG_BUFFER_TOTAL; num_offset_jpeg++)
		{
			printf("find jpeg %d :\n",num_offset_jpeg);
			SystemVpss_SimcopJpegInfo * jpeg_info=(SystemVpss_SimcopJpegInfo *)(buf_jpeg_start+num_offset_jpeg*SIZE_JPEG_BUFFER);
			printf("PD_retult->illegalPics[i].picID=%d, jpeg_info->jpeg_numid=%d\n",PD_retult->illegalPics[i].picID, jpeg_info->jpeg_numid);

//			if(PD_retult->illegalPics[i].picID==jpeg_info->jpeg_numid)
			//避免不同违法寻找到同一张jpeg，通过车牌位置来进一步的区分
			IllegalInfoPer * PD_retult_buf = (IllegalInfoPer *)&((SystemVpss_SimcopJpegInfo *)jpeg_info)->algResultInfo.AlgResultInfo;
			if((PD_retult->illegalPics[i].picID==jpeg_info->jpeg_numid)
					&&(PD_retult->illegalPics[i].plateRect.x  ==PD_retult_buf->illegalPics[i].plateRect.x)
					&&(PD_retult->illegalPics[i].plateRect.y  ==PD_retult_buf->illegalPics[i].plateRect.y)
					)
			{
				pic_info[i].buf = //(void *)(image_info + SIZE_JPEG_INFO);//(unsigned int)
								  (void *)((char *)jpeg_info + SIZE_JPEG_INFO);

				/* 获取图片缓存的大小 */
				pic_info[i].size = //ret;//av_data.size ;//violation_records->picInfo[i].picSize;
//									((SystemVpss_SimcopJpegInfo *)image_info)->jpeg_buf_size;
									jpeg_info->jpeg_buf_size;
				printf("find jpeg: PD_retult->illegalPics[%d].picID=%d, jpeg_buf_size=%d\n",i ,PD_retult->illegalPics[i].picID, jpeg_info->jpeg_buf_size);
				break;
			}
		}
		if(num_offset_jpeg>=NUM_JPEG_BUFFER_TOTAL)
		{
//			printf("find jpeg %d failed\n",i);
			log_error("PD_proc","%s:find jpeg %d failed, discard record\n",__func__,i);
			return -1;//找不到jpeg，不进行记录发送
		}
	}

	return 0;
}



/*******************************************************************************
 * 函数名: analyze_PD_records_video
 * 功  能: 解析违停视频
 * 参  数: vid_info，解析出来的视频信息；
 *         image_info，要解析的图像信息；video_info，要解析的H.264流信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_PD_records_video(
    EP_VidInfo *vid_info, const void *image_info, const void *video_info)
{
//	Pcie_data_head *info = (Pcie_data_head *) image_info;
//	illegalRecordVehicle *violation_records =
//	    (illegalRecordVehicle *) ((char *) info + IMAGE_INFO_SIZE);
	H264_Record_Info *h264_info = (H264_Record_Info *) video_info;
	int i;

	IllegalInfoPer * PD_retult = (IllegalInfoPer *)&((SystemVpss_SimcopJpegInfo *)image_info)->algResultInfo.AlgResultInfo;//(IllegalInfoPer *)image_info;//(&((VDIllegalReturn *)image_info)->illegalInfo[0]);
//	unsigned int pic_num = PD_retult->picNum ;

	memset(vid_info, 0, sizeof(EP_VidInfo));

	/* 路径按照用户的配置进行设置 */
	for (i = 0; i < EP_FTP_URL_LEVEL.levelNum; i++)
	{
		switch (EP_FTP_URL_LEVEL.urlLevel[i])
		{
		case SPORT_ID: 		//地点编号
			sprintf(vid_info->path[i], "%s", EP_POINT_ID);
			break;
		case DEV_ID: 		//设备编号
			sprintf(vid_info->path[i], "%s", EP_DEV_ID);
			break;
		case YEAR_MONTH: 	//年/月
			sprintf(vid_info->path[i], "%04d%02d",
					PD_retult->illegalPics[0].time.tm_year,
					PD_retult->illegalPics[0].time.tm_mon
//			        violation_records->picInfo[0].year,
//			        violation_records->picInfo[0].month
			        );
			break;
		case DAY: 			//日
			sprintf(vid_info->path[i], "%02d",
					PD_retult->illegalPics[0].time.tm_mday);
			break;
		case EVENT_NAME: 	//事件类型
			sprintf(vid_info->path[i], "%s", VIOLATION_RECORDS_FTP_DIR);
			break;
		case HOUR: 			//时
			sprintf(vid_info->path[i], "%02d",
					PD_retult->illegalPics[0].time.tm_hour);
			break;
		case FACTORY_NAME: 	//厂商名称
			sprintf(vid_info->path[i], "%s", EP_MANUFACTURER);
			break;
		default:
			break;
		}
	}

	/* 文件名以时间和车道号命名 */
	sprintf(vid_info->name, "%s%04d%02d%02d%02d%02d%02d%03d%02d.mp4",
	        EP_EXP_DEV_ID,
//	        violation_records->picInfo[0].year,
//	        violation_records->picInfo[0].month,
//	        violation_records->picInfo[0].day,
//	        violation_records->picInfo[0].hour,
//	        violation_records->picInfo[0].minute,
//	        violation_records->picInfo[0].second,
//	        violation_records->picInfo[0].msecond,
	        PD_retult->illegalPics[0].time.tm_year,
	        PD_retult->illegalPics[0].time.tm_mon,
	        PD_retult->illegalPics[0].time.tm_mday,
	        PD_retult->illegalPics[0].time.tm_hour,
	        PD_retult->illegalPics[0].time.tm_min,
	        PD_retult->illegalPics[0].time.tm_sec,
	        PD_retult->illegalPics[0].time.tm_msec,

//	        violation_records->illegalVehicle.laneNum
	        0
	        );

	if (h264_info)
	{
		printf("video seg:%d\t"
		       "seg1:%p,  len:%d\n"
		       "seg2:%p,  len:%d\n"
		       "time: %ds\n",
		       h264_info->h264_seg,	//h264流的分段数，通常为1段或2段，0表示无效
		       h264_info->h264_buf_seg[0], 		//指向第一段h264流的起始地址
		       h264_info->h264_buf_seg_size[0], //第一段h264流的长度
		       h264_info->h264_buf_seg[1], 		//指向第二段h264流的起始地址
		       h264_info->h264_buf_seg_size[1], //第二段h264流的长度
		       h264_info->seconds_record_ret 	//违法记录对应的h264视频时间长度
		      );

		vid_info->buf_num = h264_info->h264_seg;

		for (i = 0; i < vid_info->buf_num; i++)
		{
			vid_info->buf[i] = h264_info->h264_buf_seg[i];
			vid_info->size[i] = h264_info->h264_buf_seg_size[i];
		}
	}

	return 0;
}





