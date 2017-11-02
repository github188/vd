
#include "data_process.h"
#include "database_mng.h"
#include "event_alarm_process.h"
#include "disk_mng.h"
#include "disk_func.h"
#include "storage_common.h"
#include "commonfuncs.h"
#include "upload.h"
#include "h264/debug.h"
#include "PCIe_api.h"
#include "ftp.h"

#include "data_process_event.h"

int flag_upload_complete_event = 0;


int analysis_event_file_path(DB_EventAlarm* db_records, EP_PicInfo pic_info[])
{
	char file_name_pic[256];
	int num;

	//�����¼�ͼƬ�ļ������ַ�������
	for (num = 0; num < 4; num++)
	{

		sprintf(file_name_pic, "%s/%s/%s",
		        db_records->partition_path,
		        DISK_RECORD_PATH,
		        db_records->image_path[num]);

		debug_print("analysis_event_file_path: file_name_pic=%s\n", file_name_pic);

		char * str = db_records->image_path[num];
		char * str1 = str;
		int pos = 0;
		//����ftp ·��
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

		//���һ����Ϊ�ļ���
		memcpy(pic_info[num].name, str, strlen(str));
		pic_info[num].name[strlen(str)] = 0;

		debug_print("event pic name[%d]=%s\n", num, pic_info[num].name);
	}

	return 0;
}

////////////////////////����/////////////////////////////////////
////////////////////////����/////////////////////////////////////
//�����¼���¼
int db_upload_event_records()
{
	if (! EP_REUPLOAD_CFG.is_resume_event)
		return -1;

	int ret;
	static DB_File db_file;//������¼���ƣ�������ID
	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	//EP_VidInfo vid_info;
	static DB_EventAlarm db_event_record;
	static char sql_cond[1024];
	static int ID_DB_file_last = 0;//������Ҫ�������һ����ͨ��¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	static int ID_record_last = 0;//��ȡ��¼ʱ�ж������е�ID��������ʧ�ܣ����ɶ�ȡ��һ����¼

	memset(pic_info, 0, ILLEGAL_PIC_MAX_NUM * sizeof(EP_PicInfo));

	debug("in [%s] ... \n", __func__);

	//�ж�mq��ftp״̬//�ݶ�����Ҫmq��ftp��ͨ��������
	//��Ҫ���ദ��: ͨ�С��¼�(����������)���¼�����ͨ��
	//sleep(1);
	int ftp_status = ftp_get_status_event_alarm();
	//	debug_print("mq_get_status_event_alarm:\n");
	//sleep(1);
	int mq_status = mq_get_status_event_alarm();

	if ((ftp_status && mq_status) != 0)//ftp��mq���쳣
	{
		debug_print("ftp��mq���쳣\n");
		return -1;
	}

	//�ж��Ƿ���������Ҫ����
	//	debug_print("db_read_DB_file:\n");
	sprintf(sql_cond,
	        "SELECT * FROM DB_files WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_DB_file_last);
	ret = db_read_DB_file((char*) DB_NAME_EVENT_RECORDS, &db_file,
	                      &mutex_db_files_event, sql_cond);
	if (ret == -1)
	{
		debug_print("�¼�û��������Ҫ����\n");
		//char *db_name="DB_files_violation.db";//Ӧ��ʹ�ú꣬����ͳһ   //����һֱ���ڣ���Ӧ��ɾ��
		//remove(db_name);
		flag_upload_complete_event = 1;
		return -1;
	}
	flag_upload_complete_event = 0;
	debug_print("db_files event : read ID=%d\n", db_file.ID);

	//��ȡ���ݿ⣬����һ���¼���¼
	debug_print("db_read_event_records: %s.\n", db_file.record_path);
	sprintf(sql_cond,
	        "SELECT * FROM event_alarm WHERE ID>%d AND flag_send>0 limit 1;",
	        ID_record_last);
	ret = db_read_event_records(db_file.record_path, &db_event_record,
	                            &mutex_db_records_event, sql_cond);
	if (ret == -1)
	{
		debug_print("����һ���¼���¼ʧ��\n");

		db_event_record.ID = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0
		ID_record_last = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0

		//���������ݿ��У�ɾ����Ӧ�ļ�¼(��һ��?)
		//�Ƿ�ʵʱ�洢��? ���ǣ�ɾ���ǣ���������־
		ID_DB_file_last = db_file.ID;
		ret = db_delete_DB_file((char*) DB_NAME_EVENT_RECORDS, &db_file,
		                        &mutex_db_files_event);
		if (ret != 0)
		{
			debug_print("db_delete_DB_file failed\n");
		}
		if (ID_DB_file_last > 0)
		{
			ID_DB_file_last--;//���ǵ�һ�����������ݣ���ʵ��ͨѶ��¼���ݣ���Ҫ������ID ���ݡ�
		}
		ID_DB_file_last = 0;
		return -1;
	}
	debug_print("event_records : read ID=%d\n", db_event_record.ID);

	//��ȡ��Ҫ������ͼƬ������ͼƬ
	int num_pic = 0;
	//	debug_print("analysis_event_file_path:\n");
	//----------��ֱ�������ļ�������Ҫ��ȡ�ļ���������
	//������Ҫ������pic_info��vid_info
	ret = analysis_event_file_path(&db_event_record, pic_info);//, &vid_info);
	if (ret != 0)
	{
		debug_print("��ȡ��Ҫ������ͼƬ·��, ʧ��\n");

		//��ȡ����ͼƬ��ɾ���ü�¼����ʵ���ϲ��ܽ���
		//debug_print("db_delete_violation_records_motor_vehicle: %s.\n",db_file.record_path);
		//ret=db_delete_violation_records_motor_vehicle(db_file.record_path, &db_violation_records, &mutex_db_records_violation);

		//if(ret!=0)
		//{
		//	debug_print("ɾ��һ���¼���¼ʧ��\n");
		//	return -1;
		//}

		return -1;
	}

	//�ж���Ҫ������ͼƬ������
	for (int i = 0; i < 4; i++)
	{
		if (db_event_record.image_path[i][0] != 0)
		{
			num_pic++;
		}
		else
		{
			break;
		}
	}

	debug("motor_vehicles. ftp state: %d, mq state:%d (0 is ok)\n ", ftp_status, mq_status);
	do /* ����ͼƬ */
	{
		if ((ftp_status == 0) && ((db_event_record.flag_send & 0x02) == 0x02))
		{
			ftp_status = send_event_alarm_image_file(&db_event_record,
			             pic_info, num_pic);
			if (ftp_status == 0)
			{
				//����Ϊʹ�õ�ǰ��ftp��ַ��//
				//��mq�ϴ�ʧ�ܣ���Ҫ�������ݿ�
				CLASS_FTP * ftp;
				ftp = get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING);
				if (NULL == ftp)
				{
					return -1;
				}
				TYPE_FTP_CONFIG_PARAM *config = ftp->get_config();
				if (config == NULL)
				{
					return -1;
				}
				sprintf(db_event_record.ftp_user, config->user);
				sprintf(db_event_record.ftp_passwd, config->passwd);
				sprintf(db_event_record.ftp_ip, "%d.%d.%d.%d", config->ip[0],
				        config->ip[1], config->ip[2], config->ip[3]);
				db_event_record.ftp_port = config->port;
				db_event_record.flag_send &= 0xfd;//1111 1101 ��ftp�����ɹ�������Ӧ��־
			}
			else if (ftp_status == -2)
			{
				ret = db_delete_event_records(
				          db_file.record_path, &db_event_record,
				          &mutex_db_records_event);
				if (ret < 0)
				{
					log_warn_storage("Delete record %d form %s failed !\n",
					                 db_event_record.ID, db_file.record_path);
				}

				ID_record_last = db_event_record.ID;

				return 0;
			}
			else
			{
				debug_print("�����¼�ͼƬʧ��\n");
				return -1;
			}
		}
		else if ((db_event_record.flag_send & 0x02) == 0x02)
		{
			debug_print("there is event pic to ftp send ,but ftp_status=%d\n",
			            ftp_status);
			return -1;
		}
	}
	while (0);


	do /* ������Ϣ */
	{
		//���Ѿ��ϴ���ͼƬ�ˣ���˴ν�������Ϣ������Ҫ��ftp��״̬
		//����������ftp״̬
		if ((db_event_record.flag_send == 1) && (mq_status == 0))
		{
			debug_print("send_event_records_motor_vehicle_info:\n");
			mq_status = send_event_alarm_info(&db_event_record);

			if (mq_status == 0)
				break;
			else
			{
				debug_print("�����¼���Ϣʧ��\n");
				return -1;
			}
		}
		else if (db_event_record.flag_send == 0x01)
		{
			debug_print("there is event mq info to send ,but mq_status=%d\n",
			            mq_status);
			return -1;
		}
	}
	while (0);

	//��ftpͨ��mq��ͨ����Ҫ�޸ļ�¼�ı�־������ɾ��


	//ɾ��һ����¼����������ɺ�ɾ��
	debug_print("db_delete_event_records: %s.\n", db_file.record_path);
	ret = db_delete_event_records(db_file.record_path, &db_event_record,
	                              &mutex_db_records_event);
	if (ret != 0)
	{
		log_warn_storage("Delete record %d form %s failed !\n",
		                 db_event_record.ID, db_file.record_path);
	}

	//�������ɹ��󣬸��¶�ȡ�ж������е�ID
	ID_record_last = db_event_record.ID;

	return 0;
}


int get_record_count_event(char *db_name,
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

	debug_print("get_record_count_event db_file: sql is %s\n", sql);
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
		    "SELECT COUNT(*) FROM event_alarm WHERE flag_store=1 AND time>='%s' AND time<='%s';",
		    type_history_record->time_start, type_history_record->time_end);
		debug_print("get_count_record: sql is %s\n", sql);
		ret = db_count_records((const char *) db_name, sql,
		                       &mutex_db_records_event);//���������ݼ�¼�����޹�
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
void *db_upload_history_event_Thr(void *arg)
{

	TYPE_HISTORY_RECORD *type_history_record = (TYPE_HISTORY_RECORD *) arg;

	int ret;
	DB_File db_file;//������¼���ƣ�������ID
	EP_PicInfo pic_info[ILLEGAL_PIC_MAX_NUM];
	DB_EventAlarm db_event_record={0};
	char sql_cond[1024];
	int ID_DB_file_last = 0;//������Ҫ�������һ��ͨ�м�¼�����ݿ⣬�Ÿ��£�������Ҫ�洢�ɵ�ID.
	int count_send = 0;//���η��ͣ���¼�����

	debug("in [%s] ... \n", __func__);

	print_time("\ntype_history_record->time_start=%s, dest_mq=%d\n\n",
	           type_history_record->time_start, type_history_record->dest_mq);

	memset(pic_info, 0, sizeof(EP_PicInfo) * ILLEGAL_PIC_MAX_NUM);

	//�ж�mq��ftp״̬
	int ftp_status = ftp_get_status_event_alarm();
	int mq_status = mq_get_status_event_alarm();
	if ((ftp_status != 0) || (mq_status != 0))//ftp��mq���쳣
	{
		debug_print("ftp��mq���Ƕ����ڿ���״̬\n");
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
			debug_print("�¼�û����ʷ������Ҫ�ϴ�\n");
			return NULL;
		}
		debug_print("db_files event : read ID=%d\n", db_file.ID);

		//��ȡ���ݿ⣬����һ���¼���¼
		sprintf(
		    sql_cond,
		    "SELECT * FROM event_alarm WHERE ID>%d AND flag_store=1 AND time>='%s' AND time<='%s' limit 1;",
		    db_event_record.ID, type_history_record->time_start,
		    type_history_record->time_end);
		debug_print("db_read_event_records: %s.\n", db_file.record_path);
		ret = db_read_event_records(db_file.record_path, &db_event_record,
		                            &mutex_db_records_event, sql_cond);
		//sprintf(db_event_record.description, "��ʷ��¼");	//ͨ������: ����������ʵʱ�ϴ�
		if (ret == -1)
		{
			debug_print("����һ���¼���¼ʧ��\n");
			//remove(db_file.record_path);//ɾ����¼���ݿ�
			db_event_record.ID = 0;//һ�����ݿ��ȡ��ɣ��´β�ѯ�µ����ݿ⣬���Բ�ѯ��ID ��0

			//���������ݿ��У�ɾ����Ӧ�ļ�¼(��һ��?)
			//�Ƿ�ʵʱ�洢��? ���ǣ�ɾ���ǣ���������־
			ID_DB_file_last = db_file.ID;
			//�鿴��ʷ��¼������ɾ������

			continue;
		}
		debug_print("event_records : read ID=%d\n", db_event_record.ID);


		//��������ͼƬ·��
		analysis_event_file_path(&db_event_record, pic_info);

		//�ж���Ҫ������ͼƬ������
		int num_pic = 0;
		for (int i = 0; i < 4; i++)
		{
			if (db_event_record.image_path[i][0] != 0)
			{
				num_pic++;
			}
			else
			{
				break;
			}
		}

		debug("event_alarm. ftp state: %d, mq state:%d (0 is ok)\n ", ftp_status, mq_status);
		do /* ����ͼƬ */
		{
			if(EP_DISK_SAVE_CFG.event_picture== 1)
			{
				ftp_status = send_event_alarm_image_file(
				                 &db_event_record, pic_info, num_pic);

				if (ftp_status == 0)
				{
					//����Ϊʹ�õ�ǰ��ftp��ַ��//
					//��mq�ϴ�ʧ�ܣ���Ҫ�������ݿ�
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
					sprintf(db_event_record.ftp_user, config->user);
					sprintf(db_event_record.ftp_passwd, config->passwd);
					sprintf(db_event_record.ftp_ip, "%d.%d.%d.%d",
					        config->ip[0], config->ip[1], config->ip[2],
					        config->ip[3]);
					db_event_record.ftp_port = config->port;

					break;

				}
				else
				{
					debug_print("������ʷ�¼�ͼƬʧ��\n");
					return NULL;
				}

			}

		}
		while (0);

		//�ϴ�mq��Ϣ

		do /* ������Ϣ */
		{

			//��������mq��״̬����֮ǰ�Ѿ��жϹ��ˡ�
			if (1)//(mq_status == 0)
			{

				debug_print("send_event_alarm_info_history:\n");
				mq_status = send_event_alarm_info_history(
				                &db_event_record, type_history_record->dest_mq,
				                count_send);
				if (mq_status == 0)
					break;
				else
				{
					debug_print("������ʷ�¼���Ϣʧ��\n");
					return NULL;
				}
			}

		}
		while (0);

		//��ѯ����ɾ����¼


	}

	return NULL;
}
