
#include "data_process.h"
#include "database_mng.h"
#include "traffic_flow_process.h"
#include "disk_mng.h"
#include "disk_func.h"
#include "storage_common.h"
#include "commonfuncs.h"
#include "upload.h"
#include "h264/debug.h"

#include "data_process_flow.h"

int flag_upload_complete_flow = 0;

//续传交通流记录
int db_upload_flow_records()
{
	if (! EP_REUPLOAD_CFG.is_resume_statistics)
		return -1;

	int ret;
	static DB_File db_file;//逐条记录后移，依赖于ID
	//EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	//EP_VidInfo vid_info;
	static DB_TrafficFlow db_flow_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//由于需要续传完成一个交通记录的数据库，才更新，所以需要存储旧的ID.
	static int ID_record_last = 0;//读取记录时判断条件中的ID，若续传失败，不可读取下一条记录

	//memset(&db_flow_record, 0, sizeof(DB_TrafficFlow));

	debug("in [%s] ... \n", __func__);

	//判断mq状态

	int mq_status = mq_get_status_traffic_flow_motor_vehicle();

	if (mq_status != 0)//mq异常
	{
		debug_print("机动车交通流 mq异常\n");
		return -1;
	}

	//判断是否有数据需要续传
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);//ID_DB_file_cur);//应该与ID 续传标志相关
	ret = db_read_DB_file(DB_NAME_FLOW_RECORDS, &db_file,
	                      &mutex_db_files_flow, sql_cond);
	if (ret == -1)
	{
		debug_print("交通流没有数据需要续传\n");
		flag_upload_complete_flow = 1;
		return -1;
	}
	flag_upload_complete_flow = 0;
	debug_print("db_files event : read ID=%d\n", db_file.ID);

	//读取数据库，读出一条交通流记录
	//－－续传完成后，才删除
	debug_print("db_read_flow_records: %s.\n", db_file.record_path);
	sprintf(sql_cond,
	        "SELECT * FROM traffic_flow WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_record_last);//db_flow_record.ID );//ID_traffic_record_cur);//应该与ID 续传标志相关
	ret = db_read_flow_records(db_file.record_path, &db_flow_record,
	                           &mutex_db_records_flow, sql_cond);
	if (ret != 0)
	{
		debug_print("读出一条交通流记录失败\n");
		//remove(db_file.record_path);
		db_flow_record.ID = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0
		ID_record_last = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0

		//在索引数据库中，删除相应的记录(第一条?)
		//是否实时存储的? 不是，删；是，清续传标志
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_FLOW_RECORDS, &db_file,
		                        &mutex_db_files_flow);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			//避免新产生记录时，不能查询到
			ID_DB_file_last--;//若是第一次有索引数据，无实际通讯记录数据，需要将索引ID 回溯。
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("flow_records : read ID=%d\n", db_flow_record.ID);

	debug("motor_flow. mq state:%d (0 is ok)\n ", mq_status);

	//续传mq消息

	do /* 处理信息 */
	{
		if ((db_flow_record.flag_send == 1) && (mq_status == 0))
		{
			debug_print("send_traffic_flow:\n");
			mq_status = send_traffic_flow_motor_vehicle(&db_flow_record);

			if (mq_status == 0)
				break;
			else
			{
				debug_print("续传交通流消息失败\n");
				return -1;
			}
		}
		else if (db_flow_record.flag_send == 0x01)
		{
			debug_print("there is flow mq info to send ,but mq_status=%d\n",
			            mq_status);
			return -1;

		}
	}
	while (0);

	//删除一条记录－－续传完成后，删除
	debug_print("db_delete_flow_records: %s.\n", db_file.record_path);
	ret = db_delete_flow_records(db_file.record_path, &db_flow_record,
	                             &mutex_db_records_flow);
	if (ret != 0)
	{
		debug_print("删除一条交通流记录失败\n");
		return -1;
	}

	//在续传成功后，更新读取判断条件中的ID
	ID_record_last = db_flow_record.ID;

	return 0;
}


/*******************************************************************************
 * 函数名: traffic_flow_reupload_pedestrian
 * 功  能: 重新上传行人交通流信息
 * 返回值: 上传成功，返回0；未上传或上传失败，返回-1
*******************************************************************************/
int traffic_flow_reupload_pedestrian(void)
{
	if (! EP_REUPLOAD_CFG.is_resume_statistics)
		return -1;

	int ret;
	static DB_File db_file;//逐条记录后移，依赖于ID
	static DB_TrafficFlowPedestrian db_flow_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//由于需要续传完成一个交通记录的数据库，才更新，所以需要存储旧的ID.
	static int ID_record_last = 0;//读取记录时判断条件中的ID，若续传失败，不可读取下一条记录

	int mq_status = mq_get_status_traffic_flow_pedestrian();
	if (mq_status != 0)//mq异常
	{
		debug_print("行人交通流 mq异常\n");
		return -1;
	}

	//判断是否有数据需要续传
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);//ID_DB_file_cur);//应该与ID 续传标志相关
	ret = db_read_DB_file(DB_NAME_FLOW_PEDESTRIAN, &db_file,
	                      &mutex_db_files_flow_pedestrian, sql_cond);
	if (ret == -1)
	{
		debug_print("交通流没有数据需要续传\n");
		flag_upload_complete_flow = 1;
		return -1;
	}
	flag_upload_complete_flow = 0;
	debug_print("db_files event : read ID=%d\n", db_file.ID);

	//读取数据库，读出一条交通流记录
	//－－续传完成后，才删除
	debug_print("db_read_flow_records: %s.\n", db_file.record_path);
	sprintf(sql_cond,
	        "SELECT * FROM traffic_flow WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_record_last);//db_flow_record.ID );//ID_traffic_record_cur);//应该与ID 续传标志相关
	ret = db_read_traffic_flow_pedestrian_info(
	          db_file.record_path, &db_flow_record, sql_cond);
	if (ret != 0)
	{
		debug_print("读出一条交通流记录失败\n");
		//remove(db_file.record_path);
		db_flow_record.ID = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0
		ID_record_last = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0

		//在索引数据库中，删除相应的记录(第一条?)
		//是否实时存储的? 不是，删；是，清续传标志
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_FLOW_PEDESTRIAN, &db_file,
		                        &mutex_db_files_flow_pedestrian);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			//避免新产生记录时，不能查询到
			ID_DB_file_last--;//若是第一次有索引数据，无实际通讯记录数据，需要将索引ID 回溯。
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("flow_records : read ID=%d\n", db_flow_record.ID);

	debug("motor_flow. mq state:%d (0 is ok)\n ", mq_status);

	//续传mq消息

	do /* 处理信息 */
	{
		if ((db_flow_record.flag_send == 1) && (mq_status == 0))
		{
			debug_print("send_traffic_flow_pedestrian:\n");
			mq_status = send_traffic_flow_pedestrian(&db_flow_record);

			if (mq_status == 0)
				break;
			else
			{
				debug_print("续传交通流消息失败\n");
				return -1;
			}
		}
		else if (db_flow_record.flag_send == 0x01)
		{
			debug_print("there is flow mq info to send ,but mq_status=%d\n",
			            mq_status);
			return -1;

		}
	}
	while (0);

	//删除一条记录－－续传完成后，删除
	debug_print("db_delete_flow_records: %s.\n", db_file.record_path);
	ret = db_delete_traffic_flow_pedestrian_info(
	          db_file.record_path, &db_flow_record);
	if (ret != 0)
	{
		debug_print("删除一条交通流记录失败\n");
		return -1;
	}

	//在续传成功后，更新读取判断条件中的ID
	ID_record_last = db_flow_record.ID;

	return 0;
}


int get_record_count_flow(const char *db_name,
                          TYPE_HISTORY_RECORD *type_history_record,
                          pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	sqlite3_stmt *stat;
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

	debug_print("get_record_count_traffic db_file: sql is %s\n", sql);
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
		    "SELECT COUNT(*) FROM traffic_flow WHERE flag_store=1 AND time>='%s' AND time<='%s';",
		    type_history_record->time_start, type_history_record->time_end);
		debug_print("get_count_record: sql is %s\n", sql);
		ret = db_count_records((const char *) db_name, sql,
		                       &mutex_db_records_flow);//函数与数据记录内容无关
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
void *db_upload_history_flow_Thr(void *arg)
{

	TYPE_HISTORY_RECORD *type_history_record = (TYPE_HISTORY_RECORD *) arg;

	int ret;
	DB_File db_file;//逐条记录后移，依赖于ID
	//EP_PicInfo pic_info;
	DB_TrafficFlow db_flow_record={0};
	char sql_cond[1024];
	int ID_DB_file_last = 0;//由于需要续传完成一个通行记录的数据库，才更新，所以需要存储旧的ID.
	int count_send = 0;//本次发送，记录的序号

	debug("in [%s] ... \n", __func__);

	print_time("\ntype_history_record->time_start=%s, dest_mq=%d\n\n",
	           type_history_record->time_start, type_history_record->dest_mq);

	//判断mq、ftp状态
	//int ftp_status = ftp_get_status_();//流量没有图片文件需要上传
	int mq_status = mq_get_status_traffic_flow_motor_vehicle();
	if (mq_status != 0)//mq有异常
	{
		debug_print("mq不是处于可用状态\n");
		return NULL;

	}

	for(;;)
	{
		u_sleep(0, 500000); /* 历史查询，可能工作很长时间，要避免占用过多CPU */

		if (flag_stop_history_upload == 1)//接收到停止命令
		{
			print_time("db_upload_history_event_Thr: 接收到停止命令,退出。\n");
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

		ret=db_read_DB_file(DB_NAME_EVENT_RECORDS, &db_file, &mutex_db_files_event, sql_cond);
		if (ret == -1)
		{
			debug_print("流量没有历史数据需要上传\n");
			return NULL;
		}
		debug_print("db_files flow : read ID=%d\n", db_file.ID);

		//读取数据库，读出一条流量记录
		sprintf(
		    sql_cond,
		    "SELECT * FROM traffic_flow WHERE ID>%d AND flag_store=1 AND time>='%s' AND time<='%s' limit 1;",
		    db_flow_record.ID, type_history_record->time_start,
		    type_history_record->time_end);
		debug_print("db_read_flow_records: %s.\n", db_file.record_path);
		ret = db_read_flow_records(db_file.record_path, &db_flow_record,
		                           &mutex_db_records_flow, sql_cond);
		//sprintf(db_flow_record.description, "历史记录");	//通行描述: 区分续传与实时上传
		if (ret == -1)
		{
			debug_print("读出一条流量记录失败\n");
			//remove(db_file.record_path);//删除记录数据库
			db_flow_record.ID = 0;//一个数据库读取完成，下次查询新的数据库，所以查询的ID 清0

			//在索引数据库中，删除相应的记录(第一条?)
			//是否实时存储的? 不是，删；是，清续传标志
			ID_DB_file_last = db_file.ID;
			//查看历史记录，不做删除操作

			continue;
		}
		debug_print("traffic_flow : read ID=%d\n", db_flow_record.ID);


		//上传mq消息

		do /* 处理信息 */
		{

			//仅依赖于mq的状态。又之前已经判断过了。
			if (1)//((db_traffic_records.flag_send==1)&& (mq_status == 0))
			{

				debug_print("send_traffic_flow_history:\n");
				mq_status = send_traffic_flow_history(
				                &db_flow_record, type_history_record->dest_mq,
				                count_send);
				if (mq_status == 0)
					break;
				else
				{
					debug_print("导出历史流量消息失败\n");
					return NULL;
				}
			}

		}
		while (0);

		//查询，不删除记录


	}

	return NULL;
}
