
#include "data_process.h"
#include "database_mng.h"
#include "violation_records_process.h"
#include "disk_mng.h"
#include "disk_func.h"
#include "storage_common.h"
#include "commonfuncs.h"
#include "upload.h"
#include "h264/debug.h"
#include "PCIe_api.h"
#include "ftp.h"

#include "data_process_violation.h"

int flag_upload_complete_violation = 0;


/****************************************
函数:   analysis_violation_file_path
功能:   获取EP_PicInfo结构体所需要的信息
返回值:
*****************************************/
int analysis_violation_file_path(
    DB_ViolationRecordsMotorVehicle* db_violation_record,
    EP_PicInfo pic_info[], EP_VidInfo * vid_info)
{

	//db_traffic_records->image_path  //反解析为path[FTP_PATH_DEPTH][FTP_PATH_LEN];

	char file_name_pic[256];
	int num;

	//解析违法图片文件名至字符数组中
	for (num = 0; num < 4; num++)
	{
		sprintf(file_name_pic, "%s/%s/%s",
		        db_violation_record->partition_path,
		        DISK_RECORD_PATH,
		        db_violation_record->image_path[num]);

		debug_print("analysis_violation_file_path: file_name_pic=%s\n",
		            file_name_pic);

		char * str = db_violation_record->image_path[num];
		char * str1 = str;
		int pos = 0;
		//解析ftp 路径
		for (int i = 0; i < STORAGE_PATH_DEPTH; i++)
		{
			//pos=0;
			str1 = strchr(str, '/');
			if (str1 == 0)
			{
				break;
			}
			pos = (unsigned int) str1 - (unsigned int) str;

			//if(pos!=0)
			{
				memcpy(pic_info[num].path[i], str, pos);
				pic_info[num].path[i][pos] = 0;
				//str+=pos;
				debug_print("dir[%d]=%s\n", i, pic_info[num].path[i]);
			}
			str = str1 + 1;

		}

		//最后一部分为文件名
		memcpy(pic_info[num].name, str, strlen(str));
		pic_info[num].name[strlen(str)] = 0;

		debug_print("violation pic name[%d]=%s\n", num, pic_info[num].name);
	}

	//解析违法视频文件名至字符数组中

	char * str = db_violation_record->video_path;
	char * str1 = str;
	int pos = 0;
	//解析ftp 路径
	for (int i = 0; i < STORAGE_PATH_DEPTH; i++)
	{
		//pos=0;
		str1 = strchr(str, '/');
		if (str1 == 0)
		{
			break;
		}
		pos = (unsigned int) str1 - (unsigned int) str;

		//if(pos!=0)
		{
			memcpy(vid_info->path[i], str, pos);
			vid_info->path[i][pos] = 0;
			//str+=pos;
			debug_print("dir[%d]=%s\n", i, vid_info->path[i]);
		}
		str = str1 + 1;

	}

	//最后一部分为文件名
	memcpy(vid_info->name, str, strlen(str));
	vid_info->name[strlen(str)] = 0;

	debug_print("violation video file name=%s\n", vid_info->name);

	return 0;
}


//续传违法记录
int db_upload_violation_records_motor_vehicle()
{
	if (! EP_REUPLOAD_CFG.is_resume_illegal)
		return -1;

	int ret;
	static DB_File db_file;//逐条记录后移，依赖于ID
	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	EP_VidInfo vid_info;
	static DB_ViolationRecordsMotorVehicle db_violation_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//由于需要续传完成一个交通记录的数据库，才更新，所以需要存储旧的ID.
	static int ID_record_last = 0;//读取记录时判断条件中的ID，若续传失败，不可读取下一条记录

	//	memset(&db_violation_record, 0, sizeof(DB_ViolationRecordsMotorVehicle));
	memset(pic_info, 0, ILLEGAL_PIC_MAX_NUM * sizeof(EP_PicInfo));

	//debug("in [%s] ... \n", __func__);


	//判断mq、ftp状态//暂定，需要mq与ftp都通，才续传
	//需要分类处理: 通行、违法(机动车、非)、事件、交通流

	int ftp_status = ftp_get_status_violation_records();
	int mq_status = mq_get_status_violation_records_motor_vehicle();

	if ((ftp_status && mq_status) != 0)//ftp与mq都异常
	{
		debug_print("ftp与mq都异常\n");
		return -1;
	}

	//判断是否有数据需要续传
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);//ID_DB_file_cur);//应该与ID 续传标志相关
	ret = db_read_DB_file((char*) DB_NAME_VIOLATION_RECORDS, &db_file,
	                      &mutex_db_files_violation, sql_cond);
	if (ret == -1)
	{
		//debug_print("违法没有数据需要续传\n");
		//char *db_name="DB_files_violation.db";//应该使用宏，进行统一   //可以一直存在，不应该删除
		//remove(db_name);
		flag_upload_complete_violation = 1;
		return -1;
	}
	flag_upload_complete_violation = 0;
	debug_print("db_files violation : read ID=%d\n", db_file.ID);

	//读取数据库，读出一条违法记录
	debug_print("db_read_violation_records_motor_vehicle: %s.\n",
	            db_file.record_path);
	sprintf(
	    sql_cond,
	    "SELECT * FROM violation_records_motor_vehicle WHERE ID>%d AND flag_send>0 limit 1;",
	    ID_record_last);//db_violation_record.ID );//ID_traffic_record_cur);//应该与ID 续传标志相关
	ret = db_read_violation_records_motor_vehicle(db_file.record_path,
	        &db_violation_record, &mutex_db_records_violation, sql_cond);
	if (ret == -1)
	{
		debug_print("读出一条违法记录失败\n");
		//remove(db_file.record_path);
		db_violation_record.ID = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0
		ID_record_last = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0

		//在索引数据库中，删除相应的记录(第一条?)
		//是否实时存储的? 不是，删；是，清续传标志
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_VIOLATION_RECORDS, &db_file,
		                        &mutex_db_files_violation);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			ID_DB_file_last--;//若是第一次有索引数据，无实际通讯记录数据，需要将索引ID 回溯。
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("violation_records : read ID=%d\n", db_violation_record.ID);

	//读取需要续传的图片，续传图片
	int num_pic = 0;
	//	debug_print("analysis_violation_file_path:\n");

	//解析出违法文件路径pic_info，vid_info
	analysis_violation_file_path(&db_violation_record, pic_info, &vid_info);

	//判断需要续传的图片的张数
	for (int i = 0; i < 4; i++)
	{
		if (db_violation_record.image_path[i][0] != 0)
		{
			num_pic++;
		}
		else
		{
			break;
		}
	}

	debug("motor_vehicles. ftp state: %d, mq state:%d (0 is ok)\n ", ftp_status, mq_status);
	do /* 处理图片 */
	{
		if ((ftp_status == 0) && ((db_violation_record.flag_send & 0x02)
		                          == 0x02))
		{
			ftp_status = send_violation_records_image_file(
			                 &db_violation_record, pic_info, num_pic);
			if (ftp_status == 0)
			{
				//更换为使用当前的ftp地址。//
				//若mq上传失败，需要更新数据库
				CLASS_FTP * ftp;
				ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING);//FTP_CHANNEL_PASSCAR);
				if (NULL == ftp)
				{
					return -1;
				}
				TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
				if (config == NULL)
				{
					return -1;
				}
				sprintf(db_violation_record.ftp_user, config->user);
				sprintf(db_violation_record.ftp_passwd, config->passwd);
				sprintf(db_violation_record.ftp_ip, "%d.%d.%d.%d",
				        config->ip[0], config->ip[1], config->ip[2],
				        config->ip[3]);
				db_violation_record.ftp_port = config->port;
				db_violation_record.flag_send &= 0xfd;//1111 1101 ，ftp续传成功，清相应标志(忽略了违法视频，即违法视频是可以丢失的)

				break;

			}
			else if (ftp_status == -2)
			{
				ret = db_delete_violation_records_motor_vehicle(
				          db_file.record_path, &db_violation_record,
				          &mutex_db_records_violation);
				if (ret != 0)
				{
					log_warn_storage(
					    "Delete record %d form %s failed !\n",
					    db_violation_record.ID, db_file.record_path);
				}

				ID_record_last = db_violation_record.ID;

				return 0;
			}
			else
			{
				debug_print("续传违法图片失败\n");
				return -1;
			}

		}
		else if ((db_violation_record.flag_send & 0x02) == 0x02)
		{
			debug_print("there is violation pic to ftp send ,but ftp_status=%d\n",
			            ftp_status);
			return -1;
		}
	}
	while (0);

	do /* 处理视频 */
	{
		if ((ftp_status == 0) &&
		        ((db_violation_record.flag_send & 0x04) == 0x04))
		{
			//ftp_status = send_violation_records_image_buf(&pic_info,  num_pic, FTP_CHANNEL_ILLEGAL_RESUMING);

			ftp_status = send_violation_records_video_file(
			                 &db_violation_record, &vid_info);

			if (ftp_status == 0)
			{
				//更换为使用当前的ftp地址。//
				//若mq上传失败，需要更新数据库
				CLASS_FTP * ftp;
				ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING);//FTP_CHANNEL_PASSCAR);
				if (NULL == ftp)
				{
					return -1;
				}
				TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
				if (config == NULL)
				{
					return -1;
				}
				sprintf(db_violation_record.video_ftp_user, config->user);
				sprintf(db_violation_record.video_ftp_passwd, config->passwd);
				sprintf(db_violation_record.video_ftp_ip, "%d.%d.%d.%d",
				        config->ip[0], config->ip[1], config->ip[2],
				        config->ip[3]);
				//for(int i=0;i<4;i++)
				{
					//	db_violation_record.ftp_ip[i]= config->ip[i];
				}
				db_violation_record.video_ftp_port = config->port;
				db_violation_record.flag_send &= 0xfb;//1111 1011 ，图片与视频都ftp续传成功，清相应标志

				break;

			}
			else
			{
				//续传失败，继续执行--- 违法视频用于辅助执法，不是必须的，可以丢失。

				db_violation_record.flag_send &= 0xfb;//1111 1011 ，图片与视频都ftp续传成功，清相应标志
				debug_print("续传违法视频失败\n");
				//return -1;
			}

		}
		else if ((db_violation_record.flag_send & 0x02) == 0x02)
		{
			debug_print("there is violation vid to ftp send ,but ftp_status=%d\n",
			            ftp_status);
			return -1;
		}
	}
	while (0);

	do /* 处理信息 */
	{
		//若已经上传了图片了，则此次仅续传消息，不需要管ftp的状态
		//否则，依赖于ftp状态
		if ((db_violation_record.flag_send == 1) && (mq_status == 0))
			//		||((db_traffic_records.flag_send==3)&& (mq_status == 0)&&(ftp_status == 0) ))
		{

			debug_print("send_violation_records_motor_vehicle_info:\n");
			mq_status = send_violation_records_motor_vehicle_info(
			                &db_violation_record);
			if (mq_status == 0)
				break;
			else
			{
				debug_print("续传违法消息失败\n");
				return -1;
			}
		}
		else if (db_violation_record.flag_send == 0x01)
		{
			debug_print("there is violation mq info to send ,but mq_status=%d\n",
			            mq_status);
			return -1;

		}
	}
	while (0);

	//若ftp通，mq不通，需要修改记录的标志，不能删除


	//删除一条记录－－续传完成后，删除
	debug_print("db_delete_violation_records_motor_vehicle: %s.\n",
	            db_file.record_path);
	ret = db_delete_violation_records_motor_vehicle(db_file.record_path,
	        &db_violation_record, &mutex_db_records_violation);
	if (ret != 0)
	{
		log_warn_storage("Delete record %d form %s failed !\n",
		                 db_violation_record.ID, db_file.record_path);
	}

	//在续传成功后，更新读取判断条件中的ID
	ID_record_last = db_violation_record.ID;

	return 0;
}

//统计历史查询的违法记录数目
int get_record_count_violation(char *db_name,
                               TYPE_HISTORY_RECORD *type_history_record,
                               pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	sqlite3_stmt *stat;//=new(sqlite3_stmt);
	int ret;
	int count_total = 0;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		debug_print("Create database %s failed!\n", db_name);
		debug_print("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		debug_print("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}


	//查询符合添加的记录总条数
	memset(sql, 0, sizeof(sql));

	//索引数据库一条记录对应的时间，是一个小时
	char history_time_start[64];//时间起点 示例：2013-06-01 12:01:02
	char history_time_end[64];  //时间终点
	memset(history_time_start, 0, sizeof(history_time_start));
	memset(history_time_end, 0, sizeof(history_time_end));
	get_hour_start(type_history_record->time_start, history_time_start);
	get_hour_end(type_history_record->time_end, history_time_end);

	sprintf(sql, "SELECT * FROM DB_files WHERE flag_store=1 AND time>='%s' AND time<='%s';",
	        history_time_start,//type_history_record->time_start,
	        history_time_end//type_history_record->time_end
	       );

	debug_print("get_record_count_violation db_file: sql is %s\n",sql);
	ret = sqlite3_prepare_v2(db, sql, -1, &stat, 0);
	if (ret != SQLITE_OK)
	{
		// sqlite3_free(pzErrMsg);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	count_total = 0;

	while (sqlite3_step(stat) == SQLITE_ROW)
	{
		debug_print("db_query_db_records 3 , ID_read=%d, time_read is %s\n",
		            sqlite3_column_int(stat, 0), sqlite3_column_text(stat, 2));

		const unsigned char *db_name = sqlite3_column_text(stat, 3);
		debug_print("db_query_db_records : db_name %s\n", db_name);

		//int count_records=0;
		sprintf(
		    sql,
		    "SELECT COUNT(*) FROM violation_records_motor_vehicle WHERE flag_store=1 AND time>='%s' AND time<='%s';",
		    type_history_record->time_start, type_history_record->time_end);
		debug_print("get_count_record: sql is %s\n", sql);
		ret = db_count_records((const char *) db_name, sql,
		                       &mutex_db_records_violation);//函数与数据记录内容无关
		if (ret >= 0)
		{
			count_total += ret;
			print_time("db_count_records ret=%d,  count_total=%d\n",ret, count_total);
		}
		else
		{
			print_time("db_count_records ret failed\n");

		}

	}

	sqlite3_finalize(stat); //perpare will lock the db, this unlock the db
	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return count_total;

}

//上传历史记录的线程
void *db_upload_history_violation_Thr(void *arg)
{
	TYPE_HISTORY_RECORD *type_history_record = (TYPE_HISTORY_RECORD *) arg;

	int ret;
	DB_File db_file;//逐条记录后移，依赖于ID
	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	EP_VidInfo vid_info;
	DB_ViolationRecordsMotorVehicle db_violation_record = {0};
	char sql_cond[1024];
	int ID_DB_file_last = 0;//由于需要续传完成一个通行记录的数据库，才更新，所以需要存储旧的ID.
	int count_send = 0;//本次发送，记录的序号

	debug("in [%s] ... \n", __func__);

	print_time("\ntype_history_record->time_start=%s, dest_mq=%d\n\n",
	           type_history_record->time_start, type_history_record->dest_mq);

	memset(pic_info, 0, sizeof(EP_PicInfo) * ILLEGAL_PIC_MAX_NUM);

	//判断mq、ftp状态
	int ftp_status = ftp_get_status_violation_records();
	int mq_status = mq_get_status_violation_records_motor_vehicle();
	if ((ftp_status != 0) || (mq_status != 0))//ftp与mq有异常
	{
		debug_print("ftp与mq不是都处于可用状态\n");
		return NULL;

	}

	for(;;)
	{
		u_sleep(0, 500000); /* 历史查询，可能工作很长时间，要避免占用过多CPU */

		if (flag_stop_history_upload == 1)//接收到停止命令
		{
			print_time("db_upload_history_traffic_Thr: 接收到停止命令,退出。\n");
			return NULL;
		}

		count_send++;

		//判断是否有数据需要续传
		char history_time_start[64];//时间起点 示例：2013-06-01 12:01:02
		char history_time_end[64];  //时间终点
		//索引数据库一条记录对应的时间，是一个小时
		memset(history_time_start, 0, sizeof(history_time_start));
		memset(history_time_end, 0, sizeof(history_time_end));
		get_hour_start(type_history_record->time_start, history_time_start);
		get_hour_end(type_history_record->time_end, history_time_end);

		sprintf(sql_cond, "SELECT * FROM DB_files WHERE ID>%d AND flag_store=1 AND time>='%s' AND time<='%s' limit 1;",
		        ID_DB_file_last,
		        history_time_start,//type_history_record->time_start,
		        history_time_end//type_history_record->time_end
		       );

		ret=db_read_DB_file( DB_NAME_VIOLATION_RECORDS, &db_file, &mutex_db_files_violation, sql_cond);
		if (ret == -1)
		{
			debug_print("违法没有历史数据需要上传\n");
			return NULL;
		}
		debug_print("db_files violation : read ID=%d\n", db_file.ID);

		//读取数据库，读出一条违法记录
		sprintf(
		    sql_cond,
		    "SELECT * FROM violation_records_motor_vehicle WHERE ID>%d AND flag_store=1 AND time>='%s' AND time<='%s' limit 1;",
		    db_violation_record.ID, type_history_record->time_start,
		    type_history_record->time_end);
		debug_print("db_read_violation_records_motor_vehicle: %s.\n",
		            db_file.record_path);
		ret = db_read_violation_records_motor_vehicle(db_file.record_path,
		        &db_violation_record, &mutex_db_records_violation, sql_cond);
		////////////////////
		if (ret == -1)
		{
			debug_print("读出一条违法记录失败\n");
			//remove(db_file.record_path);
			db_violation_record.ID = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0

			ID_DB_file_last = db_file.ID;
			//查看历史记录，不做删除操作
			//ret=db_delete_DB_file(DB_NAME_VIOLATION_RECORDS, &db_file, &mutex_db_files_violation);
			//if(ret!=0)
			//{
			//	debug_print("db_delete_DB_file failed\n");
			//}
			continue;
		}
		debug_print("violation_records : read ID=%d\n", db_violation_record.ID);

		//读取需要续传的图片，续传图片
		int num_pic = 0;
		//	debug_print("analysis_violation_file_path:\n");

		//解析出违法文件路径pic_info，vid_info
		analysis_violation_file_path(&db_violation_record, pic_info, &vid_info);

		//判断需要续传的图片的张数
		for (int i = 0; i < 4; i++)
		{
			if (db_violation_record.image_path[i][0] != 0)
			{
				num_pic++;
			}
			else
			{
				break;
			}
		}

		debug("motor_vehicles. ftp state: %d, mq state:%d (0 is ok)\n ", ftp_status, mq_status);
		do /* 处理图片 */
		{
			//if(1)//( (ftp_status == 0) && ( (db_violation_record.flag_send&0x02)==0x02 ) )
			if(EP_DISK_SAVE_CFG.illegal_picture== 1)
			{
				ftp_status = send_violation_records_image_file(
				                 &db_violation_record, pic_info, num_pic);
				if (ftp_status == 0)
				{
					//更换为使用当前的ftp地址。//
					//若mq上传失败，需要更新数据库
					CLASS_FTP * ftp;
					ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING);//FTP_CHANNEL_PASSCAR);
					if (NULL == ftp)
					{
						return NULL;
					}
					TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
					if (config == NULL)
					{
						return NULL;
					}
					sprintf(db_violation_record.ftp_user, config->user);
					sprintf(db_violation_record.ftp_passwd, config->passwd);
					sprintf(db_violation_record.ftp_ip, "%d.%d.%d.%d",
					        config->ip[0], config->ip[1], config->ip[2],
					        config->ip[3]);
					db_violation_record.ftp_port = config->port;
					//db_violation_record.flag_send&=0xfd;//1111 1101 ，ftp续传成功，清相应标志(忽略了违法视频，即违法视频是可以丢失的)

					break;

				}
				else
				{

					//若是读取图片失败，需要删除记录。－－需要传递读取文件的结果出来
					//若仅是ftp状态时通时断，不可删除记录。//////////////////////////

					//ret=db_delete_violation_records_motor_vehicle(db_file.record_path, &db_violation_record, &mutex_db_records_violation);
					//if(ret!=0)
					//{
					//	debug_print("删除一条违法记录失败\n");
					//	return NULL;
					//}


					debug_print("导出历史违法图片失败\n");
					return NULL;

				}

			}
			else //if  ((db_violation_record.flag_send&0x02)==0x02 )
			{
				//debug_print("there is violation pic to ftp send ,but ftp_status=%d\n",ftp_status);
				//return NULL;
			}

		}
		while (0);

		do /* 处理视频 */
		{
			//if(1)//( (ftp_status == 0) && ( (db_violation_record.flag_send&0x04)==0x04 ) )
			if(EP_DISK_SAVE_CFG.illegal_video== 1)
			{
				//ftp_status = send_violation_records_image_buf(&pic_info,  num_pic, FTP_CHANNEL_ILLEGAL_RESUMING);
				//若上传ftp失败，不影响整个流程的运行
				ret = send_violation_records_video_file(
				          &db_violation_record, &vid_info);

				if (ret == 0)
				{
					//更换为使用当前的ftp地址。//
					//若mq上传失败，需要更新数据库
					CLASS_FTP * ftp;
					ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING);//FTP_CHANNEL_PASSCAR);
					if (NULL == ftp)
					{
						return NULL;
					}
					TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
					if (config == NULL)
					{
						return NULL;
					}
					sprintf(db_violation_record.video_ftp_user, config->user);
					sprintf(db_violation_record.video_ftp_passwd,
					        config->passwd);
					sprintf(db_violation_record.video_ftp_ip, "%d.%d.%d.%d",
					        config->ip[0], config->ip[1], config->ip[2],
					        config->ip[3]);
					//for(int i=0;i<4;i++)
					{
						//	db_violation_record.ftp_ip[i]= config->ip[i];
					}
					db_violation_record.video_ftp_port = config->port;
					//db_violation_record.flag_send&=0xfb;//1111 1011 ，图片与视频都ftp续传成功，清相应标志

					break;

				}
				else
				{
					//续传失败，继续执行--- 违法视频用于辅助执法，不是必须的，可以丢失。

					//db_violation_record.flag_send&=0xfb;//1111 1011 ，图片与视频都ftp续传成功，清相应标志
					debug_print("导出历史违法视频失败\n");
					//return -1;
				}

			}
			else //if  ((db_violation_record.flag_send&0x02)==0x02 )
			{
				//debug_print("there is violation vid to ftp send ,but ftp_status=%d\n",ftp_status);
				//return NULL;
			}

		}
		while (0);

		//续传mq消息

		do /* 处理信息 */
		{
			//若已经上传了图片了，则此次仅续传消息，不需要管ftp的状态
			//否则，依赖于ftp状态
			if (1)//((db_violation_record.flag_send==1)&& (mq_status == 0))
				//		||((db_traffic_records.flag_send==3)&& (mq_status == 0)&&(ftp_status == 0) ))
			{

				debug_print("send_violation_records_motor_vehicle_info:\n");
				mq_status=send_violation_records_motor_vehicle_info_history(&db_violation_record, type_history_record->dest_mq, count_send);
				////////////////////

				if (mq_status == 0)
					break;
				else
				{
					debug_print("导出历史违法消息失败\n");
					return NULL;
				}
			}
			else //if (db_violation_record.flag_send==0x01)
			{
				debug_print(
				    "there is violation mq info to send ,but mq_status=%d\n",
				    mq_status);
				return NULL;

			}
		}
		while (0);

		//查询，不删除记录


	}

	return NULL;
}
