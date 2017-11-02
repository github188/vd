
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "partition_func.h"
#include "data_process.h"
#include "database_mng.h"
#include "event_alarm_process.h"
#include "data_process_event.h"
#include "h264/debug.h"
#include "PCIe_api.h"
#include "ftp.h"


#define EVENT_NAME_NUM 	2
#define EVENT_NAME_LEN 	20

const char event_name[EVENT_NAME_NUM][EVENT_NAME_LEN] =
{
	/*
	"违章停车",
	"行人违章",
	"逆向行驶",
	"交通拥堵",
	"车辆溃洒",
	"烟雾报警",
	"火焰报警",
	"超速行驶",
	"低速行驶",
	"违章变线",
	*/
	"溢出报警",
	"行人横穿机动车道",
};


/*******************************************************************************
 * 函数名: process_event_alarm
 * 功  能: 处理事件报警信息
 * 参  数: image_info，图像信息
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int process_event_alarm(const void *image_info)
{
	if (image_info == NULL)
		return -1;

	log_debug_storage("Start to process event alarm.\n");

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);

	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	DB_EventAlarm db_event_alarm;
	int flag_send = 0;

	do /* 处理图片 */
	{
		if (analyze_event_alarm_picture(pic_info, image_info) < 0)
			return -1;

		unsigned int pic_num = ((Pcie_data_head *)image_info)->NumPic;

		if (EP_UPLOAD_CFG.event_picture == 1)
		{
			if (send_event_alarm_image_buf(pic_info, pic_num) < 0)
			{
				flag_send |= 0x02; 	/* 图片上传失败，需要续传 */
				log_debug_storage("Send event alarm image failed !\n");
			}
		}

		if (((flag_send & 0x02) != 0) || (EP_DISK_SAVE_CFG.event_picture == 1))
		{
			if (disk_status == 0)
			{
				disk_status = save_event_alarm_image_buf(
				                  pic_info, pic_num, partition_path);
				if (disk_status < 0)
				{
					log_warn_storage("Save event alarm image failed !\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, "
				                 "event alarm image is discarded.\n");
			}
		}
	}
	while (0);

	do /* 处理信息 */
	{
		if (analyze_event_alarm_info(
		            &db_event_alarm, image_info, pic_info, partition_path) < 0)
			return -1;

		if (EP_UPLOAD_CFG.event_picture == 1)
		{
			flag_send |= 0x01; 		/* 先设置为需要续传 */

			if (((flag_send & 0x02) == 0) &&
			        (send_event_alarm_info(&db_event_alarm) == 0))
			{
				flag_send = 0x00; 	/* 如果上传成功，再恢复为无需上传 */
			}
		}

		if (((flag_send & 0x01) != 0) || (EP_DISK_SAVE_CFG.event_picture == 1))
		{
			if (disk_status == 0)
			{
				db_event_alarm.flag_send 	= flag_send;
				db_event_alarm.flag_store 	= EP_DISK_SAVE_CFG.event_picture;

				if (save_event_alarm_info(&db_event_alarm,
				                          image_info, partition_path) < 0)
				{
					log_error_storage("Save event alarm information failed!\n");
				}
			}
			else
			{
				log_warn_storage("The disk is not available, "
				                 "event alarm information is discarded.\n");
			}
		}
	}
	while (0);

	return 0;
}


/*******************************************************************************
 * 函数名: analyze_event_alarm_picture
 * 功  能: 解析事件报警图片
 * 参  数: pic_info，解析出来的图片信息；image_info，要解析的图像信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_event_alarm_picture(EP_PicInfo pic_info[], const void *image_info)
{
	Pcie_data_head *info = (Pcie_data_head *)image_info;
	SingleEventAlarm *event_alarm = (SingleEventAlarm *)((char *)info + IMAGE_INFO_SIZE);
	unsigned int pic_num = info->NumPic;
	unsigned int i;
	int j;

	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Event alarm picture number error: %d\n", pic_num);
		return -1;
	}

	for (i=0; i<pic_num; i++)
	{
		memset(&pic_info[i], 0, sizeof(EP_PicInfo));

		/* 路径按照用户的配置进行设置 */
		for (j=0; j<EP_FTP_URL_LEVEL.levelNum; j++)
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
				        event_alarm->picInfo[i].year,
				        event_alarm->picInfo[i].month);
				break;
			case DAY: 			//日
				sprintf(pic_info[i].path[j], "%02d",
				        event_alarm->picInfo[i].day);
				break;
			case EVENT_NAME: 	//事件类型
				sprintf(pic_info[i].path[j], "%s", EVENT_ALARM_FTP_DIR);
				break;
			case HOUR: 			//时
				sprintf(pic_info[i].path[j], "%02d",
				        event_alarm->picInfo[i].hour);
				break;
			case FACTORY_NAME: 	//厂商名称
				sprintf(pic_info[i].path[j], "%s", EP_MANUFACTURER);
				break;
			default:
				break;
			}
		}

		/* 文件名以设备编号、时间和序号命名 */
		sprintf(pic_info[i].name, "%s%04d%02d%02d%02d%02d%02d%03d%02d.jpg",
		        EP_EXP_DEV_ID,
		        event_alarm->picInfo[i].year,
		        event_alarm->picInfo[i].month,
		        event_alarm->picInfo[i].day,
		        event_alarm->picInfo[i].hour,
		        event_alarm->picInfo[i].minute,
		        event_alarm->picInfo[i].second,
		        event_alarm->picInfo[i].msecond,
		        i + 1);

		/* 获取图片缓存的位置 */
		pic_info[i].buf = (char *)info + IMAGE_INFO_SIZE +
		                  IMAGE_PADDING_SIZE + info->PosPic[i];

		/* 获取图片缓存的大小 */
		pic_info[i].size = event_alarm->picInfo[i].picSize;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: analyze_event_alarm_info
 * 功  能: 解析事件报警信息
 * 参  数: db_event_alarm，解析出来的信息；image_info，图像信息；
 *         pic_info，图片文件信息；partition_path，分区路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_event_alarm_info(
    DB_EventAlarm *db_event_alarm, const void *image_info,
    const EP_PicInfo pic_info[], const char *partition_path)
{
	SingleEventAlarm *event_alarm =
	    (SingleEventAlarm *)((char *)image_info + IMAGE_INFO_SIZE);
	unsigned int i;
	unsigned int pic_num = ((Pcie_data_head *)image_info)->NumPic;
	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Event alarm picture number error: %d\n", pic_num);
		return -1;
	}

	unsigned int send_pic_num = (pic_num > EVENT_ALARM_IMAGE_NUM)?
	                            EVENT_ALARM_IMAGE_NUM : (pic_num - 1);

	memset(db_event_alarm, 0, sizeof(DB_EventAlarm));

	db_event_alarm->type = event_alarm->eventAlarmInfo.eventType;

	if ((db_event_alarm->type > 0) && (db_event_alarm->type <= EVENT_NAME_NUM))
	{
		i = db_event_alarm->type - 1;
		sprintf(db_event_alarm->description, 	"%s", event_name[i]);
		sprintf(db_event_alarm->name, 			"%s", event_name[i]);
	}
	else
	{
		sprintf(db_event_alarm->description, 	"未知类型");
		sprintf(db_event_alarm->name, 			"未知类型");
		log_warn_storage("Event alarm type error: %d\n", db_event_alarm->type);
	}

	sprintf(db_event_alarm->point_id, 	"%s", EP_POINT_ID);
	sprintf(db_event_alarm->point_name, "%s", EP_POINT_NAME);

	for (i=0; i<pic_num; i++)
	{
		for (int j = 0; j < FTP_PATH_DEPTH; j++)
		{
			if (strlen(pic_info[i].path[j]) == 0)
			{
				break;
			}
			sprintf(db_event_alarm->image_path[i] + strlen(
			            db_event_alarm->image_path[i]), "%s/", pic_info[i].path[j]);
		}
		sprintf(db_event_alarm->image_path[i] + strlen(
		            db_event_alarm->image_path[i]), "%s", pic_info[i].name);
	}

	if (send_pic_num != EVENT_ALARM_IMAGE_NUM)
	{
		strcpy(db_event_alarm->image_path[EVENT_ALARM_IMAGE_NUM],
		       db_event_alarm->image_path[send_pic_num]);
	}

	sprintf(db_event_alarm->partition_path, "%s", partition_path);
	sprintf(db_event_alarm->dev_id, 		"%s", EP_EXP_DEV_ID);

	sprintf(db_event_alarm->time, "%04d-%02d-%02d %02d:%02d:%02d",
	        event_alarm->picInfo[send_pic_num].year,
	        event_alarm->picInfo[send_pic_num].month,
	        event_alarm->picInfo[send_pic_num].day,
	        event_alarm->picInfo[send_pic_num].hour,
	        event_alarm->picInfo[send_pic_num].minute,
	        event_alarm->picInfo[send_pic_num].second);

	sprintf(db_event_alarm->ftp_user, "%s", EP_VIOLATION_FTP.user);
	sprintf(db_event_alarm->ftp_passwd, "%s", EP_VIOLATION_FTP.passwd);
	sprintf(db_event_alarm->ftp_ip, "%d.%d.%d.%d",
	        EP_VIOLATION_FTP.ip[0],
	        EP_VIOLATION_FTP.ip[1],
	        EP_VIOLATION_FTP.ip[2],
	        EP_VIOLATION_FTP.ip[3]);
	db_event_alarm->ftp_port = EP_VIOLATION_FTP.port;

	return 0;
}


/*******************************************************************************
 * 函数名: send_event_alarm_image_buf
 * 功  能: 发送缓存中的事件报警图片
 * 参  数: pic_info，图片文件信息；pic_num，图片张数
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_event_alarm_image_buf(
    const EP_PicInfo pic_info[], unsigned int pic_num)
{
	if (ftp_get_status_event_alarm() < 0)
		return -1;

	if ( (pic_num == 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Event alarm picture number error: %d\n", pic_num);
		return -1;
	}

	unsigned int send_pic_num = (pic_num > EVENT_ALARM_IMAGE_NUM)?
	                            EVENT_ALARM_IMAGE_NUM : (pic_num - 1);
	/* 如果够三张图片，用第三张；否则，用最后一张 */

	if (ftp_send_pic_buf(&pic_info[send_pic_num], FTP_CHANNEL_ILLEGAL) < 0)
	{
		log_debug_storage("Send event alarm image buf failed !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: send_event_alarm_image_file
 * 功  能: 发送存储的事件报警图片
 * 参  数: pic_info，图片文件信息；image_info，图像信息
 * 返回值: 成功，返回0；上传失败，返回-1；文件不存在，返回-2
*******************************************************************************/
int send_event_alarm_image_file(DB_EventAlarm* db_record,
                                EP_PicInfo pic_info[], int pic_num)
{
	if ( (pic_num <= 0) || (pic_num > ILLEGAL_PIC_MAX_NUM) )
	{
		log_error_storage("Event alarm picture number error: %d\n", pic_num);
		return -1;
	}

	unsigned int send_pic_num = (pic_num > EVENT_ALARM_IMAGE_NUM)?
	                            EVENT_ALARM_IMAGE_NUM : (pic_num - 1);
	/* 如果够三张图片，用第三张；否则，用最后一张 */

	char pic_name[NAME_MAX_LEN];
	snprintf(pic_name, NAME_MAX_LEN, "%s/%s/%s",
	         db_record->partition_path,
	         DISK_RECORD_PATH,
	         db_record->image_path[send_pic_num]);

	if (access(pic_name, F_OK) < 0)
	{
		log_debug_storage("%s does not exist !\n", pic_name);
		return -2;
	}

	if (ftp_send_pic_file(pic_name, &pic_info[send_pic_num],
	                      FTP_CHANNEL_ILLEGAL_RESUMING) < 0)
	{
		log_debug_storage("Send event alarm image buf failed !\n");
		return -1;
	}

	if (db_record->flag_store == 0) 	/* 如果不是实时存储，转移文件 */
	{
		int i;
		for (i=0; i<pic_num; i++)
		{
			move_record_to_trash(
			    db_record->partition_path, db_record->image_path[i]);
		}
	}

	return 0;
}


/*******************************************************************************
 * 函数名: send_event_alarm_info
 * 功  能: 发送事件报警信息
 * 参  数: db_event_alarm，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_event_alarm_info(void *db_event_alarm)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	format_mq_text_event_alarm(mq_text, (DB_EventAlarm *) db_event_alarm);
	debug ("text: %s\n", mq_text);

	mq_send_event_alarm(mq_text);

	return 0;
}


/*******************************************************************************
 * 函数名: send_event_alarm_info_history
 * 功  能: 发送历史事件报警信息
 * 参  数: db_event_alarm，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_event_alarm_info_history(void *db_event_alarm, int dest_mq, int num_record)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);
	format_mq_text_event_alarm(mq_text, (DB_EventAlarm *) db_event_alarm);
	debug ("text: %s\n", mq_text);

	mq_send_event_alarm_history(mq_text, dest_mq, num_record);

	return 0;
}


/*******************************************************************************
 * 函数名: format_mq_text_event_alarm
 * 功  能: 格式化事件报警MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；event_alarm，交通流信息
 * 返回值: 字符串长度
*******************************************************************************/
int format_mq_text_event_alarm(char *mq_text, const DB_EventAlarm *event_alarm)
{
	int len = 0;

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	len = sprintf(mq_text, "<HiATMP id=\"ACCIDENTINF\"> <ACCIDENT>");
	/* XML头 */

	len += sprintf(mq_text+len, "<TYPE>%d</TYPE>", event_alarm->type);
	/* 字段1 事件类型 */

	len += sprintf(mq_text+len, "<DESCRIBE>%s</DESCRIBE>",
	               event_alarm->description);
	/* 字段2 事件描述 */

	len += sprintf(mq_text+len, "<NAME>%s</NAME>", event_alarm->name);
	/* 字段3 事件名称 */

	len += sprintf(mq_text+len, "<POINTID>%s</POINTID>", event_alarm->point_id);
	/* 字段4 监测点编号 */

	len += sprintf(mq_text+len, "<POINTNAME>%s</POINTNAME>",
	               event_alarm->point_name);
	/* 字段4 监测点名称 */

	len += sprintf(mq_text+len, "<IMAGEURL>ftp://%s:%s@%s:%d/%s</IMAGEURL>",
	               event_alarm->ftp_user,
	               event_alarm->ftp_passwd,
	               event_alarm->ftp_ip,
	               event_alarm->ftp_port,
	               event_alarm->image_path[EVENT_ALARM_IMAGE_NUM]);
	/* 字段5 图片URL */

	len += sprintf(mq_text+len, "<DEVICEID>%s</DEVICEID>", event_alarm->dev_id);
	/* 字段6 设备编号 */

	len += sprintf(mq_text+len, "<TIME>%s</TIME>", event_alarm->time);
	/* 字段6 检测时间 */

	len += sprintf(mq_text+len, "</ACCIDENT> </HiATMP>");
	/* XML尾 */

	return len;
}


/*******************************************************************************
 * 函数名: db_write_event_alarm
 * 功  能: 写事件报警数据库
 * 参  数: event，事件
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_write_event_alarm(char *db_name, DB_EventAlarm *event,
                         pthread_mutex_t *mutex_db_records)
{
	char sql[SQL_BUF_SIZE];

	db_format_insert_sql_event_alarm(sql, event);
	pthread_mutex_lock(mutex_db_records);
	int ret = db_write(db_name, sql, SQL_CREATE_TABLE_EVENT_ALARM); //TODO 返回值判断
	pthread_mutex_unlock(mutex_db_records);

	if (ret != 0)
	{
		printf("db_write_event_alarm failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: db_format_insert_sql_event_alarm
 * 功  能: 格式化事件报警记录表SQL插入语句
 * 参  数: sql，SQL缓存；event，事件
 * 返回值: SQL长度
*******************************************************************************/
int db_format_insert_sql_event_alarm(char *sql, DB_EventAlarm *event)
{
	int len = 0;

	memset(sql, 0, SQL_BUF_SIZE);

	len += sprintf(sql, "INSERT INTO event_alarm VALUES(NULL");
	len += sprintf(sql + len, ",'%d'", event->type);
	len += sprintf(sql + len, ",'%s'", event->description);
	len += sprintf(sql + len, ",'%s'", event->name);
	len += sprintf(sql + len, ",'%s'", event->point_id);
	len += sprintf(sql + len, ",'%s'", event->point_name);
	len += sprintf(sql + len, ",'%s'", event->image_path[0]);
	len += sprintf(sql + len, ",'%s'", event->image_path[1]);
	len += sprintf(sql + len, ",'%s'", event->image_path[2]);
	len += sprintf(sql + len, ",'%s'", event->image_path[3]);
	len += sprintf(sql + len, ",'%s'", event->partition_path);
	len += sprintf(sql + len, ",'%s'", event->dev_id);
	len += sprintf(sql + len, ",'%s'", event->time);
	len += sprintf(sql + len, ",'%s'", event->ftp_user);
	len += sprintf(sql + len, ",'%s'", event->ftp_passwd);
	len += sprintf(sql + len, ",'%s'", event->ftp_ip);
	len += sprintf(sql + len, ",'%d'", event->ftp_port);
	len += sprintf(sql + len, ",'%d'", event->flag_send);
	len += sprintf(sql + len, ",'%d'", event->flag_store);
	len += sprintf(sql + len, ");");

	return len;
}


/*******************************************************************************
 * 函数名: db_format_insert_sql_event_alarm
 * 功  能: 格式化事件报警记录表SQL插入语句
 * 参  数: sql，SQL缓存；event，事件
 * 返回值: SQL长度
*******************************************************************************/
int db_unformat_read_sql_event_alarm(char *azResult[], DB_EventAlarm *event)
{
	DB_EventAlarm *record;
	record = (DB_EventAlarm *) event;
	int ncolumn = 0;

	if (record == NULL)
	{
		printf("db_unformat_read_sql_event_alarm: record is NULL\n");
		return -1;
	}
	if (azResult == NULL)
	{
		printf("db_unformat_read_sql_event_alarm: azResult is NULL\n");
		return -1;
	}

	memset(record, 0, sizeof(DB_EventAlarm));

	record->ID= atoi(azResult[ncolumn + 0]);
	record->type = atoi(azResult[ncolumn + 1]);

	sprintf(record->description, "%s", azResult[ncolumn + 2]);
	sprintf(record->name, "%s", azResult[ncolumn + 3]);
	sprintf(record->point_id, "%s", azResult[ncolumn + 4]);
	sprintf(record->point_name, "%s", azResult[ncolumn + 5]);
	sprintf(record->image_path[0], "%s", azResult[ncolumn + 6]);
	sprintf(record->image_path[1], "%s", azResult[ncolumn + 7]);
	sprintf(record->image_path[2], "%s", azResult[ncolumn + 8]);
	sprintf(record->image_path[3], "%s", azResult[ncolumn + 9]);
	sprintf(record->partition_path, "%s", azResult[ncolumn + 10]);
	sprintf(record->dev_id, "%s", azResult[ncolumn + 11]);
	sprintf(record->time, "%s", azResult[ncolumn + 12]);
	sprintf(record->ftp_user, "%s", azResult[ncolumn + 13]);
	sprintf(record->ftp_passwd, "%s", azResult[ncolumn + 14]);
	sprintf(record->ftp_ip, "%s", azResult[ncolumn + 15]);

	record->ftp_port = atoi(azResult[ncolumn + 16]);
	record->flag_send = atoi(azResult[ncolumn + 17]);
	record->flag_store = atoi(azResult[ncolumn + 18]);

	return 0;
}


/*******************************************************************************
 * 函数名: db_read_event_records
 * 功  能: 读取数据库中的事件记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_read_event_records(char *db_name, void *records,
                          pthread_mutex_t *mutex_db_records, char * sql_cond)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
	char **azResult; //二维数组存放结果

	DB_EventAlarm* event_record;
	event_record = (DB_EventAlarm *) records;

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

	rc = db_create_table(db, SQL_CREATE_TABLE_EVENT_ALARM);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	//查询数据
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM event_alarm limit 1");
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

	db_unformat_read_sql_event_alarm(&(azResult[ncolumn]), event_record);

	printf("db_read_event_records  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return 0;
}


/*******************************************************************************
 * 函数名: db_delete_event_records
 * 功  能: 删除数据库中的事件记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_delete_event_records(char *db_name, void *records,
                            pthread_mutex_t *mutex_db_records)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	//char **azResult; //二维数组存放结果
	//static char plateNum[16]; //判断同一车牌传送的次数，防止删除数据库失败引起重复传送
	//int samePlateCnt = 0; //续传时判断同一车牌传送的次数

	DB_EventAlarm* event_record;
	event_record = (DB_EventAlarm *) records;

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
		rc = db_create_table(db, SQL_CREATE_TABLE_EVENT_ALARM);
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
	if (event_record->flag_store==1)
	{
		//清理续传标志
		sprintf(sql, "UPDATE event_alarm SET flag_send=0 WHERE ID = %d ;", event_record->ID);
	}
	else
	{

		//删除指定ID 的记录	//只有在一个数据库全部续传完成时，才能删除这条对应的记录
		sprintf(sql, "DELETE FROM event_alarm WHERE ID = %d ;", event_record->ID);
	}

	//printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn]);

	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "DELETE FROM violation_records_motor_vehicle WHERE ID = %s ;",	azResult[ncolumn]);
	printf("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("Delete data failed, flag_store=%d,  Error Message: %s\n", event_record->flag_store, pzErrMsg);
	}


	sqlite3_free(pzErrMsg);

	//sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	printf("db_delete_event_records  finish\n");

	return 0;
}


/*******************************************************************************
 * 函数名: save_event_alarm_image_buf
 * 功  能: 保存缓存中的事件记录图片
 * 参  数: pic_info，图片文件信息；pic_num，图片张数；partition_path，分区路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_event_alarm_image_buf(
    const EP_PicInfo pic_info[], unsigned int pic_num,
    const char *partition_path)
{
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

	for (i=0; i<pic_num; i++)
	{
		debug("===============event pic [%d] ===========================\n", i);
		ret = file_save(record_path, pic_info[i].path, pic_info[i].name,
		                pic_info[i].buf, pic_info[i].size);
		if (ret != 0)
			return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: save_event_alarm_info
 * 功  能: 存储事件告警记录信息
 * 参  数: db_event_alarm，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_event_alarm_info(const DB_EventAlarm *db_event_records,
                          const void *image_info, const char *partition_path)
{

	char db_name[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/violation_records/2013082109.db
	static DB_File db_file;
	//static char db_name_last[PATH_MAX_LEN];
	int flag_record_in_DB_file=0;//是否需要记录到索引数据库

	SingleEventAlarm *event_records =
	    (SingleEventAlarm *)((char *)image_info + IMAGE_INFO_SIZE);

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/event_alarm");

	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	sprintf(db_name, "%s/%04d%02d%02d%02d.db", dir_temp,
	        event_records->picInfo[0].year, event_records->picInfo[0].month,//应该使用第二个索引对应的时间，可是，可能是合并为一张的? 没有第二张
	        event_records->picInfo[0].day, event_records->picInfo[0].hour);

	//判断数据库文件是否已经存在
	//若不存在，需要相应建立一条索引记录
	//续传后，可能没有删记录数据库文件，却删了对应的索引记录。
	if (access(db_name, F_OK) != 0)
	{
		//数据库文件不存在，需要在索引数据库中增加相应的一条记录
		flag_record_in_DB_file=1;
	}
	else
	{
		flag_record_in_DB_file=0;
	}

	//if ((flag_upload_complete_event==1)||(strcasecmp(db_name_last, db_name)))//是新数据库名称
	if (flag_record_in_DB_file==1)//保证，索引记录的删除，同时删对应交通数据库
	{
		//printf("db_name_last is %s, db_name is %s\n", db_name_last, db_name);
		printf("add to DB_files, db_flow_name is %s\n", db_name);

		//写一条记录到数据库名管理
		DB_File db_file;

		db_file.record_type = 2;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_name, strlen(db_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_event_records->time, strlen(
		           db_event_records->time));
		db_file.flag_send = db_event_records->flag_send;
		db_file.flag_store = db_event_records->flag_store;

		ret = db_write_DB_file((char*)DB_NAME_EVENT_RECORDS, &db_file,
		                       &mutex_db_files_event);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf("db_write_DB_file : insert %s  in DB_files_event.db ok\n",
			       db_name);
		}
	}
	else
	{
		// 第一次写索引数据库时，可能是两个条件中任意一个触发，（断网、实时存储）
		// 所以，在另一个条件发生时，需要相应修改索引数据库。

		static int flag_first=1;
		if (flag_first==1)
		{
			//第一次进入，要读取最新的一条索引记录。
			flag_first=0;

			char sql_cond[1024];
			char history_time_start[64];			//时间起点 示例：2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_event_records->time, history_time_start);
			sprintf(sql_cond, "SELECT * FROM DB_files WHERE time>='%s'  limit 1;", history_time_start );
			db_read_DB_file(DB_NAME_EVENT_RECORDS, &db_file, &mutex_db_records_event, sql_cond);
		}

		char sql_cond[1024];
		printf("db_file.flag_send=%d,db_event_records->flag_send=%d\n",db_file.flag_send,db_event_records->flag_send);
		if (( ~(db_file.flag_send) & db_event_records->flag_send)!=0)	//若新记录中包含了新的续传标志信息
		{
			db_file.flag_send |=db_event_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;", db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_EVENT_RECORDS, sql_cond, &mutex_db_records_event);
		}

		if ( (db_file.flag_store==0) && (db_event_records->flag_store!=0))//需要增加存储信息
		{
			db_file.flag_store=db_event_records->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;", db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_EVENT_RECORDS, sql_cond, &mutex_db_records_event);
		}

	}

	//memset(db_name_last, 0, sizeof(db_name_last));
	//memcpy(db_name_last, db_name, sizeof(db_name));

	ret = db_write_event_alarm(db_name, (DB_EventAlarm *) db_event_records,
	                           &mutex_db_records_event);
	if (ret != 0)
	{
		printf("db_write_event_alarm failed\n");
		return -1;
	}

	return ret;

}
