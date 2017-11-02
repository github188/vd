
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

//������ͨ����¼
int db_upload_flow_records()
{
	if (! EP_REUPLOAD_CFG.is_resume_statistics)
		return -1;

	int ret;
	static DB_File db_file;//������¼���ƣ�������ID
	//EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	//EP_VidInfo vid_info;
	static DB_TrafficFlow db_flow_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//������Ҫ�������һ����ͨ��¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	static int ID_record_last = 0;//��ȡ��¼ʱ�ж������е�ID��������ʧ�ܣ����ɶ�ȡ��һ����¼

	//memset(&db_flow_record, 0, sizeof(DB_TrafficFlow));

	debug("in [%s] ... \n", __func__);

	//�ж�mq״̬

	int mq_status = mq_get_status_traffic_flow_motor_vehicle();

	if (mq_status != 0)//mq�쳣
	{
		debug_print("��������ͨ�� mq�쳣\n");
		return -1;
	}

	//�ж��Ƿ���������Ҫ����
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);//ID_DB_file_cur);//Ӧ����ID ������־���
	ret = db_read_DB_file(DB_NAME_FLOW_RECORDS, &db_file,
	                      &mutex_db_files_flow, sql_cond);
	if (ret == -1)
	{
		debug_print("��ͨ��û��������Ҫ����\n");
		flag_upload_complete_flow = 1;
		return -1;
	}
	flag_upload_complete_flow = 0;
	debug_print("db_files event : read ID=%d\n", db_file.ID);

	//��ȡ���ݿ⣬����һ����ͨ����¼
	//����������ɺ󣬲�ɾ��
	debug_print("db_read_flow_records: %s.\n", db_file.record_path);
	sprintf(sql_cond,
	        "SELECT * FROM traffic_flow WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_record_last);//db_flow_record.ID );//ID_traffic_record_cur);//Ӧ����ID ������־���
	ret = db_read_flow_records(db_file.record_path, &db_flow_record,
	                           &mutex_db_records_flow, sql_cond);
	if (ret != 0)
	{
		debug_print("����һ����ͨ����¼ʧ��\n");
		//remove(db_file.record_path);
		db_flow_record.ID = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0
		ID_record_last = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0

		//���������ݿ��У�ɾ����Ӧ�ļ�¼(��һ��?)
		//�Ƿ�ʵʱ�洢��? ���ǣ�ɾ���ǣ���������־
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_FLOW_RECORDS, &db_file,
		                        &mutex_db_files_flow);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			//�����²�����¼ʱ�����ܲ�ѯ��
			ID_DB_file_last--;//���ǵ�һ�����������ݣ���ʵ��ͨѶ��¼���ݣ���Ҫ������ID ���ݡ�
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("flow_records : read ID=%d\n", db_flow_record.ID);

	debug("motor_flow. mq state:%d (0 is ok)\n ", mq_status);

	//����mq��Ϣ

	do /* ������Ϣ */
	{
		if ((db_flow_record.flag_send == 1) && (mq_status == 0))
		{
			debug_print("send_traffic_flow:\n");
			mq_status = send_traffic_flow_motor_vehicle(&db_flow_record);

			if (mq_status == 0)
				break;
			else
			{
				debug_print("������ͨ����Ϣʧ��\n");
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

	//ɾ��һ����¼����������ɺ�ɾ��
	debug_print("db_delete_flow_records: %s.\n", db_file.record_path);
	ret = db_delete_flow_records(db_file.record_path, &db_flow_record,
	                             &mutex_db_records_flow);
	if (ret != 0)
	{
		debug_print("ɾ��һ����ͨ����¼ʧ��\n");
		return -1;
	}

	//�������ɹ��󣬸��¶�ȡ�ж������е�ID
	ID_record_last = db_flow_record.ID;

	return 0;
}


/*******************************************************************************
 * ������: traffic_flow_reupload_pedestrian
 * ��  ��: �����ϴ����˽�ͨ����Ϣ
 * ����ֵ: �ϴ��ɹ�������0��δ�ϴ����ϴ�ʧ�ܣ�����-1
*******************************************************************************/
int traffic_flow_reupload_pedestrian(void)
{
	if (! EP_REUPLOAD_CFG.is_resume_statistics)
		return -1;

	int ret;
	static DB_File db_file;//������¼���ƣ�������ID
	static DB_TrafficFlowPedestrian db_flow_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//������Ҫ�������һ����ͨ��¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	static int ID_record_last = 0;//��ȡ��¼ʱ�ж������е�ID��������ʧ�ܣ����ɶ�ȡ��һ����¼

	int mq_status = mq_get_status_traffic_flow_pedestrian();
	if (mq_status != 0)//mq�쳣
	{
		debug_print("���˽�ͨ�� mq�쳣\n");
		return -1;
	}

	//�ж��Ƿ���������Ҫ����
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);//ID_DB_file_cur);//Ӧ����ID ������־���
	ret = db_read_DB_file(DB_NAME_FLOW_PEDESTRIAN, &db_file,
	                      &mutex_db_files_flow_pedestrian, sql_cond);
	if (ret == -1)
	{
		debug_print("��ͨ��û��������Ҫ����\n");
		flag_upload_complete_flow = 1;
		return -1;
	}
	flag_upload_complete_flow = 0;
	debug_print("db_files event : read ID=%d\n", db_file.ID);

	//��ȡ���ݿ⣬����һ����ͨ����¼
	//����������ɺ󣬲�ɾ��
	debug_print("db_read_flow_records: %s.\n", db_file.record_path);
	sprintf(sql_cond,
	        "SELECT * FROM traffic_flow WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_record_last);//db_flow_record.ID );//ID_traffic_record_cur);//Ӧ����ID ������־���
	ret = db_read_traffic_flow_pedestrian_info(
	          db_file.record_path, &db_flow_record, sql_cond);
	if (ret != 0)
	{
		debug_print("����һ����ͨ����¼ʧ��\n");
		//remove(db_file.record_path);
		db_flow_record.ID = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0
		ID_record_last = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0

		//���������ݿ��У�ɾ����Ӧ�ļ�¼(��һ��?)
		//�Ƿ�ʵʱ�洢��? ���ǣ�ɾ���ǣ���������־
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_FLOW_PEDESTRIAN, &db_file,
		                        &mutex_db_files_flow_pedestrian);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			//�����²�����¼ʱ�����ܲ�ѯ��
			ID_DB_file_last--;//���ǵ�һ�����������ݣ���ʵ��ͨѶ��¼���ݣ���Ҫ������ID ���ݡ�
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("flow_records : read ID=%d\n", db_flow_record.ID);

	debug("motor_flow. mq state:%d (0 is ok)\n ", mq_status);

	//����mq��Ϣ

	do /* ������Ϣ */
	{
		if ((db_flow_record.flag_send == 1) && (mq_status == 0))
		{
			debug_print("send_traffic_flow_pedestrian:\n");
			mq_status = send_traffic_flow_pedestrian(&db_flow_record);

			if (mq_status == 0)
				break;
			else
			{
				debug_print("������ͨ����Ϣʧ��\n");
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

	//ɾ��һ����¼����������ɺ�ɾ��
	debug_print("db_delete_flow_records: %s.\n", db_file.record_path);
	ret = db_delete_traffic_flow_pedestrian_info(
	          db_file.record_path, &db_flow_record);
	if (ret != 0)
	{
		debug_print("ɾ��һ����ͨ����¼ʧ��\n");
		return -1;
	}

	//�������ɹ��󣬸��¶�ȡ�ж������е�ID
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

	//��ѯ������ӵļ�¼������
	memset(sql, 0, sizeof(sql));

	//�������ݿ�һ����¼��Ӧ��ʱ�䣬��һ��Сʱ
	char history_time_start[64];//ʱ����� ʾ����2013-06-01 12:01:02
	char history_time_end[64];  //ʱ���յ�
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
		                       &mutex_db_records_flow);//���������ݼ�¼�����޹�
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



//�ϴ���ʷ��¼���߳�
void *db_upload_history_flow_Thr(void *arg)
{

	TYPE_HISTORY_RECORD *type_history_record = (TYPE_HISTORY_RECORD *) arg;

	int ret;
	DB_File db_file;//������¼���ƣ�������ID
	//EP_PicInfo pic_info;
	DB_TrafficFlow db_flow_record={0};
	char sql_cond[1024];
	int ID_DB_file_last = 0;//������Ҫ�������һ��ͨ�м�¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	int count_send = 0;//���η��ͣ���¼�����

	debug("in [%s] ... \n", __func__);

	print_time("\ntype_history_record->time_start=%s, dest_mq=%d\n\n",
	           type_history_record->time_start, type_history_record->dest_mq);

	//�ж�mq��ftp״̬
	//int ftp_status = ftp_get_status_();//����û��ͼƬ�ļ���Ҫ�ϴ�
	int mq_status = mq_get_status_traffic_flow_motor_vehicle();
	if (mq_status != 0)//mq���쳣
	{
		debug_print("mq���Ǵ��ڿ���״̬\n");
		return NULL;

	}

	for(;;)
	{
		u_sleep(0, 500000); /* ��ʷ��ѯ�����ܹ����ܳ�ʱ�䣬Ҫ����ռ�ù���CPU */

		if (flag_stop_history_upload == 1)//���յ�ֹͣ����
		{
			print_time("db_upload_history_event_Thr: ���յ�ֹͣ����,�˳���\n");
			return NULL;
		}

		count_send++;

		//�ж��Ƿ���������Ҫ����

		char history_time_start[64];//ʱ����� ʾ����2013-06-01 12:01:02
		char history_time_end[64];  //ʱ���յ�
		//�������ݿ�һ����¼��Ӧ��ʱ�䣬��һ��Сʱ
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
			debug_print("����û����ʷ������Ҫ�ϴ�\n");
			return NULL;
		}
		debug_print("db_files flow : read ID=%d\n", db_file.ID);

		//��ȡ���ݿ⣬����һ��������¼
		sprintf(
		    sql_cond,
		    "SELECT * FROM traffic_flow WHERE ID>%d AND flag_store=1 AND time>='%s' AND time<='%s' limit 1;",
		    db_flow_record.ID, type_history_record->time_start,
		    type_history_record->time_end);
		debug_print("db_read_flow_records: %s.\n", db_file.record_path);
		ret = db_read_flow_records(db_file.record_path, &db_flow_record,
		                           &mutex_db_records_flow, sql_cond);
		//sprintf(db_flow_record.description, "��ʷ��¼");	//ͨ������: ����������ʵʱ�ϴ�
		if (ret == -1)
		{
			debug_print("����һ��������¼ʧ��\n");
			//remove(db_file.record_path);//ɾ����¼���ݿ�
			db_flow_record.ID = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0

			//���������ݿ��У�ɾ����Ӧ�ļ�¼(��һ��?)
			//�Ƿ�ʵʱ�洢��? ���ǣ�ɾ���ǣ���������־
			ID_DB_file_last = db_file.ID;
			//�鿴��ʷ��¼������ɾ������

			continue;
		}
		debug_print("traffic_flow : read ID=%d\n", db_flow_record.ID);


		//�ϴ�mq��Ϣ

		do /* ������Ϣ */
		{

			//��������mq��״̬����֮ǰ�Ѿ��жϹ��ˡ�
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
					debug_print("������ʷ������Ϣʧ��\n");
					return NULL;
				}
			}

		}
		while (0);

		//��ѯ����ɾ����¼


	}

	return NULL;
}
