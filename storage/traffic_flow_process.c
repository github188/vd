
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "partition_func.h"
#include "data_process.h"
#include "database_mng.h"
#include "traffic_flow_process.h"


/*******************************************************************************
 * ������: process_traffic_flow
 * ��  ��: ����ͨ����Ϣ
 * ��  ��: traffic_flow����ͨ����Ϣ
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int process_traffic_flow(const TrafficFlowStatistics *traffic_flow)
{
	if (traffic_flow == NULL)
		return -1;

	int rc = 0;

	rc |=
	    process_traffic_flow_motor_vehicles(&(traffic_flow->vehicleFlowReturn));

	rc |= process_traffic_flow_pedestrian(&(traffic_flow->PFlowReturn));

	return rc;
}


/*******************************************************************************
 * ������: process_traffic_flow_motor_vehicles
 * ��  ��: �����������ͨ����Ϣ
 * ��  ��: statistics����������ͨ��ͳ����Ϣ
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int process_traffic_flow_motor_vehicles(const VehicleFlow *statistics)
{
	if (statistics->statisticReady == 0)
		return 0;

	log_debug_storage("Start to process motor vehicles traffic flow.\n");

	DB_TrafficFlow db_traffic_flow;

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);
	int lane_num 	= statistics->laneNum;
	int flag_send;
	int i;

	if ( (lane_num <= 0) || (lane_num > MAX_LANE_NUM) )
	{
		log_error_storage("traffic flow lane number error: %d\n", lane_num);
		return -1;
	}

	/* ���ܰ����������򰴶������ */
	for (i=0; i<lane_num; i++)
	{
		flag_send = 0;

		analyze_traffic_flow_motor_vehicle_info(
		    &db_traffic_flow, statistics, i);

		if (EP_UPLOAD_CFG.flow_statistics == 1)
		{
			if (send_traffic_flow_motor_vehicle(&db_traffic_flow) < 0)
			{
				flag_send |= 0x01; 	/* �ϴ�ʧ�ܣ���Ҫ���� */

				log_debug_storage(
				    "Send traffic flow motor vehicle information failed !\n");
			}
		}

		if (((flag_send & 0x01) != 0) ||
		        (EP_DISK_SAVE_CFG.flow_statistics == 1))
		{
			if (disk_status == 0)
			{
				db_traffic_flow.flag_send 	= flag_send;
				db_traffic_flow.flag_store 	= EP_DISK_SAVE_CFG.flow_statistics;

				if (save_traffic_flow_info(&db_traffic_flow,
				                           statistics, partition_path) < 0)
				{
					log_warn_storage("Save traffic flow "
					                 "motor vehicle information failed !\n");
				}
			}
			else
			{
				log_debug_storage("The disk is not available, traffic flow "
				                  "motor vehicle information is discarded.\n");
			}
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: process_traffic_flow_pedestrian
 * ��  ��: �������˽�ͨ����Ϣ
 * ��  ��: statistics�����˽�ͨ��ͳ����Ϣ
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int process_traffic_flow_pedestrian(const PedestrianFlow *statistics)
{
	if (statistics->isReady == 0)
		return 0;

	log_debug_storage("Start to process pedestrian traffic flow.\n");

	DB_TrafficFlowPedestrian db_traffic_flow;

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);
	int flag_send = 0;

	analyze_traffic_flow_pedestrian_info(&db_traffic_flow, statistics);

	if (EP_UPLOAD_CFG.flow_statistics == 1)
	{
		if (send_traffic_flow_pedestrian(&db_traffic_flow) < 0)
		{
			flag_send |= 0x01; 	/* �ϴ�ʧ�ܣ���Ҫ���� */

			log_debug_storage(
			    "Send traffic flow pedestrian information failed !\n");
		}
	}

	if (((flag_send & 0x01) != 0) || (EP_DISK_SAVE_CFG.flow_statistics == 1))
	{
		if (disk_status == 0)
		{
			db_traffic_flow.flag_send 	= flag_send;
			db_traffic_flow.flag_store 	= EP_DISK_SAVE_CFG.flow_statistics;

			if (save_traffic_flow_pedestrian_info(
			            &db_traffic_flow, statistics, partition_path) < 0)
			{
				log_warn_storage(
				    "Save traffic flow pedestrian information failed !\n");
			}
		}
		else
		{
			log_debug_storage("The disk is not available, traffic flow "
			                  "pedestrian information is discarded.\n");
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: analyze_traffic_flow_motor_vehicle_info
 * ��  ��: ������������ͨ����Ϣ
 * ��  ��: db_traffic_flow���������������ݣ�
 *         statistics����������ͨ����Ϣ��lane_num������
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int analyze_traffic_flow_motor_vehicle_info(
    DB_TrafficFlow *db_traffic_flow,
    const VehicleFlow *statistics, int lane_num)
{
	log_debug_storage("Start to analyze traffic flow.\n");

	const VehicleFlowLane *traffic_flow =
	    &(statistics->vehicleFlowLane[lane_num]);
	const VD_Time *statistic_time = &(statistics->statisticTime);

	memset(db_traffic_flow, 0, sizeof(DB_TrafficFlow));

	sprintf(db_traffic_flow->section_id, "%s", EP_SECTION_ID);
	sprintf(db_traffic_flow->section_name, "%s", EP_SECTION_NAME);
	db_traffic_flow->large_count 	= traffic_flow->largeCount;
	db_traffic_flow->small_count 	= traffic_flow->smallCount;
	db_traffic_flow->comm_count 	= traffic_flow->commCount;
	db_traffic_flow->avg_volume 	= traffic_flow->avgVolume;
	db_traffic_flow->avg_speed 		= traffic_flow->avgSpeed;
	db_traffic_flow->occupancy 		= traffic_flow->occupancy;
	db_traffic_flow->queue 			= traffic_flow->queue;
	db_traffic_flow->traval_time 	= traffic_flow->travelTime;
	sprintf(db_traffic_flow->point_id, 		"%s", EP_POINT_ID);
	sprintf(db_traffic_flow->point_name, 	"%s", EP_POINT_NAME);
	sprintf(db_traffic_flow->dev_id, 		"%s", EP_EXP_DEV_ID);
	sprintf(db_traffic_flow->dev_name, 		"%s", EP_DEV_NAME);
	sprintf(db_traffic_flow->uptime,
	        "%04d-%02d-%02d %02d:%02d:%02d",
	        statistic_time->tm_year,
	        statistic_time->tm_mon,
	        statistic_time->tm_mday,
	        statistic_time->tm_hour,
	        statistic_time->tm_min,
	        statistic_time->tm_sec);
	db_traffic_flow->lan_num 		= traffic_flow->laneNo;
	db_traffic_flow->direction 		= EP_DIRECTION;
	db_traffic_flow->time_headway 	= traffic_flow->timeHeadWay;
	db_traffic_flow->density 		= traffic_flow->density;
	db_traffic_flow->car_len 		= traffic_flow->carlen;

	return 0;
}


/*******************************************************************************
 * ������: analyze_traffic_flow_pedestrian_info
 * ��  ��: �������˽�ͨ����Ϣ
 * ��  ��: db_traffic_flow���������������ݣ�statistics�����˽�ͨ��ͳ����Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int analyze_traffic_flow_pedestrian_info(
    DB_TrafficFlowPedestrian *db_traffic_flow,
    const PedestrianFlow *statistics)
{
	const PedestrianFlow *traffic_flow = statistics;

	memset(db_traffic_flow, 0, sizeof(DB_TrafficFlowPedestrian));

	db_traffic_flow->flow_left2right = traffic_flow->flowL2R;
	db_traffic_flow->flow_right2left = traffic_flow->flowR2L;

	sprintf(db_traffic_flow->point_id, 		"%s", EP_POINT_ID);
	sprintf(db_traffic_flow->point_name, 	"%s", EP_POINT_NAME);
	sprintf(db_traffic_flow->dev_id, 		"%s", EP_EXP_DEV_ID);
	sprintf(db_traffic_flow->dev_name, 		"%s", EP_DEV_NAME);
	sprintf(db_traffic_flow->uptime,
	        "%04d-%02d-%02d %02d:%02d:%02d",
	        traffic_flow->startTime.tm_year,
	        traffic_flow->startTime.tm_mon,
	        traffic_flow->startTime.tm_mday,
	        traffic_flow->startTime.tm_hour,
	        traffic_flow->startTime.tm_min,
	        traffic_flow->startTime.tm_sec);

	return 0;
}


/*******************************************************************************
 * ������: send_traffic_flow_motor_vehicle
 * ��  ��: ���ͽ�ͨ����Ϣ
 * ��  ��: db_traffic_flow����ͨ����Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int send_traffic_flow_motor_vehicle(const DB_TrafficFlow *db_traffic_flow)
{
	if (mq_get_status_traffic_flow_motor_vehicle() < 0)
		return -1;

	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	format_mq_text_traffic_flow_motor_vehicle(mq_text, db_traffic_flow);

	return mq_send_traffic_flow_motor_vehicle(mq_text);
}


/*******************************************************************************
 * ������: send_traffic_flow_pedestrian
 * ��  ��: ���ͽ�ͨ����Ϣ
 * ��  ��: db_traffic_flow����ͨ����Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int send_traffic_flow_pedestrian(
    const DB_TrafficFlowPedestrian *db_traffic_flow)
{
	if (mq_get_status_traffic_flow_pedestrian() < 0)
		return -1;

	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	format_mq_text_traffic_flow_pedestrian(mq_text, db_traffic_flow);

	return mq_send_traffic_flow_pedestrian(mq_text);
}


/*******************************************************************************
 * ������: send_traffic_flow_history
 * ��  ��: ���ͽ�ͨ����Ϣ
 * ��  ��: db_traffic_flow����ͨ����Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int send_traffic_flow_history(
    const DB_TrafficFlow *db_traffic_flow, int dest_mq, int num_record)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	format_mq_text_traffic_flow_motor_vehicle(mq_text, db_traffic_flow);

	debug_print("text: %s\n", mq_text);

	return mq_send_traffic_flow_history(mq_text, dest_mq, num_record);
}


/*******************************************************************************
 * ������: format_mq_text_traffic_flow_motor_vehicle
 * ��  ��: ��ʽ����������ͨ��MQ�ı���Ϣ
 * ��  ��: mq_text��MQ�ı���Ϣ��������traffic_flow����ͨ����Ϣ
 * ����ֵ: �ַ�������
*******************************************************************************/
int format_mq_text_traffic_flow_motor_vehicle(
    char *mq_text, const DB_TrafficFlow *traffic_flow)
{
	int len = 0;

	len += sprintf(mq_text+len, "%s", EP_DATA_SOURCE);
	/* �ֶ�1 ������Դ */

	len += sprintf(mq_text+len, ",%s", traffic_flow->section_id);
	/* �ֶ�2 ·�α�� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->section_name);
	/* �ֶ�3 ·������ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->large_count);
	/* �ֶ�4 ����ƽ�����ͳ����� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->small_count);
	/* �ֶ�5 ����ƽ��С�ͳ����� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->comm_count);
	/* �ֶ�6 ������ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->avg_volume);
	/* �ֶ�7 ����ƽ������ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->avg_speed);
	/* �ֶ�8 ����ƽ������ */

	len += sprintf(mq_text+len, ",%d.%04d",
	               traffic_flow->occupancy/10000,
	               traffic_flow->occupancy%10000);
	/* �ֶ�9 ����ƽ��ʱ��ռ���� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->queue);
	/* �ֶ�10 ����ƽ���ŶԳ��� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->traval_time);
	/* �ֶ�11 ����ƽ������ʱ�� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->point_id);
	/* �ֶ�12 �ɼ����� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->point_name);
	/* �ֶ�13 �ɼ������� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->dev_id);
	/* �ֶ�14 ����豸��� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->dev_name);
	/* �ֶ�15 ����豸���� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->uptime);
	/* �ֶ�16 ���ʱ�� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->lan_num);
	/* �ֶ�17 ������ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->direction);
	/* �ֶ�18 ������ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->time_headway);
	/* �ֶ�19 ƽ����ͷʱ�� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->density);
	/* �ֶ�20 �����ܶ� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->car_len);
	/* �ֶ�21 ����ƽ������ */

	return len;
}


/*******************************************************************************
 * ������: format_mq_text_traffic_flow_pedestrian
 * ��  ��: ��ʽ�����˽�ͨ��MQ�ı���Ϣ
 * ��  ��: mq_text��MQ�ı���Ϣ��������traffic_flow����ͨ����Ϣ
 * ����ֵ: �ַ�������
*******************************************************************************/
int format_mq_text_traffic_flow_pedestrian(
    char *mq_text, const DB_TrafficFlowPedestrian *traffic_flow)
{
	int len = 0;

	len += sprintf(mq_text+len, "%s", EP_DATA_SOURCE);
	/* �ֶ�1 ������Դ */

	len += sprintf(mq_text+len, ",%d", traffic_flow->flow_left2right);
	/* �ֶ�2 ���������� */

	len += sprintf(mq_text+len, ",%d", traffic_flow->flow_right2left);
	/* �ֶ�3 ���������� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->point_id);
	/* �ֶ�4 �ɼ����� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->point_name);
	/* �ֶ�5 �ɼ������� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->dev_id);
	/* �ֶ�6 ����豸��� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->dev_name);
	/* �ֶ�7 ����豸���� */

	len += sprintf(mq_text+len, ",%s", traffic_flow->uptime);
	/* �ֶ�8 ���ʱ�� */

	return len;
}


/*******************************************************************************
 * ������: db_write_flow_records
 * ��  ��: д��������ͨ�����ݿ�
 * ��  ��: db_name�����ݿ����ƣ�traffic_flow����ͨ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_write_flow_records(char *db_flow_name, DB_TrafficFlow *records,
                          pthread_mutex_t *mutex_db_records)
{
	char sql[SQL_BUF_SIZE];

	memset(sql, 0, SQL_BUF_SIZE);

	db_format_insert_sql_traffic_flow(sql, records);

	pthread_mutex_lock(mutex_db_records);
	int ret = db_write(db_flow_name, sql, SQL_CREATE_TABLE_TRAFFIC_FLOW);
	pthread_mutex_unlock(mutex_db_records);

	if (ret != 0)
	{
		printf("db_write_flow_records failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * ������: db_write_traffic_flow_pedestrian
 * ��  ��: д���˽�ͨ�����ݿ�
 * ��  ��: db_name�����ݿ����ƣ�traffic_flow����ͨ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_write_traffic_flow_pedestrian(
    const char *db_name, const DB_TrafficFlowPedestrian *record)
{
	char sql[SQL_BUF_SIZE];

	memset(sql, 0, SQL_BUF_SIZE);

	db_format_insert_sql_traffic_flow_pedestrian(sql, record);

	pthread_mutex_lock(&mutex_db_records_flow_pedestrian);

	int ret = db_write(db_name, sql, SQL_CREATE_TABLE_TRAFFIC_FLOW_PEDESTRIAN);

	pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);

	if (ret != 0)
	{
		printf("db_write_flow_records failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * ������: db_read_flow_records
 * ��  ��: ��ȡ���ݿ��еĽ�ͨ����¼
 * ��  ��:
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_read_flow_records(char *db_name, void *records,
                         pthread_mutex_t *mutex_db_records, char * sql_cond)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //��ѯ�����������������
	char **azResult; //��ά�����Ž��

	DB_TrafficFlow* flow_record;
	flow_record = (DB_TrafficFlow*) records;

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

	rc = db_create_table(db, SQL_CREATE_TABLE_TRAFFIC_FLOW);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	//��ѯ����
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM traffic_flow limit 1");
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

	db_unformat_read_sql_traffic_flow(&(azResult[ncolumn]), flow_record);

	printf("db_read_flow_records  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return 0;
}


/*******************************************************************************
 * ������: db_read_traffic_flow_pedestrian_info
 * ��  ��: ��ȡ���ݿ��е����˽�ͨ����¼
 * ��  ��: db_name�����ݿ����ƣ�record����¼��Ϣ��sql��SQL���
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_read_traffic_flow_pedestrian_info(
    const char *db_name, DB_TrafficFlowPedestrian *record, const char *sql)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	int nrow = 0, ncolumn = 0; //��ѯ�����������������
	char **azResult; //��ά�����Ž��

	pthread_mutex_lock(&mutex_db_records_flow_pedestrian);

	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}

	rc = db_create_table(db, SQL_CREATE_TABLE_TRAFFIC_FLOW_PEDESTRIAN);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		sqlite3_close(db);
		pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);
		return -1;
	}

	//��ѯ����
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM traffic_flow limit 1");
	nrow = 0;
	rc = sqlite3_get_table(db, sql, &azResult, &nrow, &ncolumn, &pzErrMsg);
	if (rc != SQLITE_OK || nrow == 0)
	{
		//		printf("Can't require data, Error Message: %s\n", pzErrMsg);
		//		printf("row:%d column=%d \n", nrow, ncolumn);
		sqlite3_free(pzErrMsg);
		sqlite3_free_table(azResult);
		sqlite3_close(db);
		pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);
		return -1;
	}

	printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn][0]);

	db_unformat_traffic_flow_pedestrian_info(&(azResult[ncolumn]), record);

	printf("db_read_flow_records  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);

	pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);

	return 0;
}


/*******************************************************************************
 * ������: db_delete_flow_records
 * ��  ��: ɾ�����ݿ��еĽ�ͨ����¼
 * ��  ��:
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_delete_flow_records(const char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	//char **azResult; //��ά�����Ž��
	//static char plateNum[16]; //�ж�ͬһ���ƴ��͵Ĵ�������ֹɾ�����ݿ�ʧ�������ظ�����
	//int samePlateCnt = 0; //����ʱ�ж�ͬһ���ƴ��͵Ĵ���

	DB_TrafficFlow* flow_record;
	flow_record = (DB_TrafficFlow*) records;

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

	//��ѯ����
	memset(sql, 0, sizeof(sql));

	if (flow_record->flag_store == 1)
	{
		//����������־
		sprintf(sql, "UPDATE traffic_flow SET flag_send=0 WHERE ID = %d ;",
		        flow_record->ID);
	}
	else
	{
		//ɾ��ָ��ID �ļ�¼	//ֻ����һ�����ݿ�ȫ���������ʱ������ɾ��������Ӧ�ļ�¼
		sprintf(sql, "DELETE FROM traffic_flow WHERE ID = %d ;",
		        flow_record->ID);
	}

	printf("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("Delete data failed, flag_store=%d,  Error Message: %s\n",
		       flow_record->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	//sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	printf("db_delete_flow_records  finish\n");

	return 0;
}


/*******************************************************************************
 * ������: db_delete_flow_records
 * ��  ��: ɾ�����ݿ��еĽ�ͨ����¼
 * ��  ��:
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_delete_traffic_flow_pedestrian_info(
    const char *db_name, const DB_TrafficFlowPedestrian *record)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];

	pthread_mutex_lock(&mutex_db_records_flow_pedestrian);

	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}

	//��ѯ����
	memset(sql, 0, sizeof(sql));

	if (record->flag_store == 1)
	{
		//����������־
		sprintf(sql, "UPDATE traffic_flow SET flag_send=0 WHERE ID = %d ;",
		        record->ID);
	}
	else
	{
		//ɾ��ָ��ID �ļ�¼	//ֻ����һ�����ݿ�ȫ���������ʱ������ɾ��������Ӧ�ļ�¼
		sprintf(sql, "DELETE FROM traffic_flow WHERE ID = %d ;", record->ID);
	}

	printf("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("Delete data failed, flag_store=%d,  Error Message: %s\n",
		       record->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	sqlite3_close(db);

	pthread_mutex_unlock(&mutex_db_records_flow_pedestrian);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	printf("db_delete_flow_records  finish\n");

	return 0;
}


/*******************************************************************************
 * ������: save_traffic_flow_info
 * ��  ��: �����������ͨ����Ϣ�����ݿ�����ʱ�䶯̬�仯
 * ��  ��: db_flow_records����������ͨ����Ϣ��
 *         statistics����������ͨ����Ϣ��partition_path������·��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int save_traffic_flow_info(
    const DB_TrafficFlow *db_flow_records,
    const VehicleFlow *statistics, const char *partition_path)
{
	char db_flow_name[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	static DB_File db_file;
	//static char db_flow_name_last[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	int flag_record_in_DB_file = 0;//�Ƿ���Ҫ��¼���������ݿ�

	const VD_Time *statisticTime = &(statistics->statisticTime);//����ͳ�ƿ�ʼʱ��

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/traffic_flow");
	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	sprintf(db_flow_name, "%s/%04d%02d%02d%02d.db", dir_temp,
	        statisticTime->tm_year, statisticTime->tm_mon,
	        statisticTime->tm_mday, statisticTime->tm_hour);

	//�ж����ݿ��ļ��Ƿ��Ѿ�����
	//�������ڣ���Ҫ��Ӧ����һ��������¼
	//�����󣬿���û��ɾ��¼���ݿ��ļ���ȴɾ�˶�Ӧ��������¼��
	if (access(db_flow_name, F_OK) != 0)
	{
		//���ݿ��ļ������ڣ���Ҫ���������ݿ���������Ӧ��һ����¼
		flag_record_in_DB_file = 1;
	}
	else
	{
		flag_record_in_DB_file = 0;
	}

	if (flag_record_in_DB_file == 1)//��֤��������¼��ɾ����ͬʱɾ��Ӧ��ͨ���ݿ�
	{
		//printf("db_traffic_name_last is %s, db_traffic_name is %s\n",
		//		db_flow_name_last, db_flow_name);
		printf("add to DB_files, db_flow_name is %s\n", db_flow_name);

		//дһ����¼�����ݿ�������
		DB_File db_file;

		db_file.record_type = DB_TYPE_TRAFFIC_FLOW_MOTOR_VEHICLE;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_flow_name, strlen(db_flow_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_flow_records->uptime, strlen(
		           db_flow_records->uptime));
		db_file.flag_send = db_flow_records->flag_send;
		db_file.flag_store = db_flow_records->flag_store;

		ret = db_write_DB_file((char*) DB_NAME_FLOW_RECORDS, &db_file,
		                       &mutex_db_files_flow);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf("db_write_DB_file %s in %s ok.\n", db_flow_name,
			       DB_NAME_FLOW_RECORDS);
		}
	}
	else
	{
		// ��һ��д�������ݿ�ʱ����������������������һ����������������ʵʱ�洢��
		// ���ԣ�����һ����������ʱ����Ҫ��Ӧ�޸��������ݿ⡣

		static int flag_first = 1;
		if (flag_first == 1)
		{
			//��һ�ν��룬Ҫ��ȡ���µ�һ��������¼��
			flag_first = 0;

			char sql_cond[1024];
			char history_time_start[64]; //ʱ����� ʾ����2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_flow_records->uptime, history_time_start);
			sprintf(sql_cond,
			        "SELECT * FROM DB_files WHERE time>='%s'  limit 1;",
			        history_time_start);
			db_read_DB_file(DB_NAME_FLOW_RECORDS, &db_file,
			                &mutex_db_files_flow, sql_cond);
		}

		char sql_cond[1024];
		printf("db_file.flag_send=%d,db_flow_records->flag_send=%d\n",
		       db_file.flag_send, db_flow_records->flag_send);
		if ((~(db_file.flag_send) & db_flow_records->flag_send) != 0) //���¼�¼�а������µ�������־��Ϣ
		{
			db_file.flag_send |= db_flow_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;",
			        db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_FLOW_RECORDS, sql_cond,
			                  &mutex_db_records_flow);
		}

		if ((db_file.flag_store == 0) && (db_flow_records->flag_store != 0))//��Ҫ���Ӵ洢��Ϣ
		{
			db_file.flag_store = db_flow_records->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;",
			        db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_FLOW_RECORDS, sql_cond,
			                  &mutex_db_records_flow);
		}

	}

	//memset(db_flow_name_last, 0, sizeof(db_flow_name_last));
	//memcpy(db_flow_name_last, db_flow_name, sizeof(db_flow_name));

	ret = db_write_flow_records(db_flow_name,
	                            (DB_TrafficFlow*) db_flow_records, &mutex_db_records_flow);
	if (ret != 0)
	{
		printf("db_write_flow_records failed\n");
		return -1;
	}

	return ret;
}


/*******************************************************************************
 * ������: save_traffic_flow_pedestrian_info
 * ��  ��: �������˽�ͨ����Ϣ
 * ��  ��: db_flow_records�����˽�ͨ����Ϣ
 *         statistics�����˽�ͨ��ͳ����Ϣ��partition_path������·��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int save_traffic_flow_pedestrian_info(
    const DB_TrafficFlowPedestrian *db_flow_records,
    const PedestrianFlow *statistics, const char *partition_path)
{
	char db_path[PATH_MAX_LEN];
	char db_full_name[PATH_MAX_LEN];
	static DB_File db_file;

	const VD_Time *statistic_time = &(statistics->startTime);

	snprintf(db_path, PATH_MAX_LEN, "%s/%s/%s",
	         partition_path, DISK_DB_PATH, DB_TRAFFIC_FLOW_PEDESTRIAN_DIR);
	snprintf(db_full_name, PATH_MAX_LEN, "%s/%04d%02d%02d%02d.db",
	         db_path,
	         statistic_time->tm_year,
	         statistic_time->tm_mon,
	         statistic_time->tm_mday,
	         statistic_time->tm_hour);

	if (access(db_full_name, F_OK) < 0)
	{
		/* ���ݿ��ļ������ڣ���Ҫ���������ݿ������һ����Ӧ�ļ�¼ */
		log_debug_storage("Add %s to Index DataBase.\n", db_full_name);

		if (access(db_path, F_OK) < 0)
		{
			if (mkpath(db_path, 0755) < 0)
			{
				log_warn_storage("mkpath %s failed !\n", db_path);
				return -1;
			}
		}

		memset(&db_file, 0, sizeof(DB_File));

		db_file.record_type = DB_TYPE_TRAFFIC_FLOW_PEDESTRIAN;
		snprintf(db_file.record_path, sizeof(db_file.record_path),
		         "%s", db_full_name);
		snprintf(db_file.time, sizeof(db_file.time),
		         "%s", db_flow_records->uptime);
		db_file.flag_send 	= db_flow_records->flag_send;
		db_file.flag_store 	= db_flow_records->flag_store;

		if (db_write_DB_file(DB_NAME_FLOW_RECORDS, &db_file,
		                     &mutex_db_files_flow_pedestrian) < 0)
		{
			log_warn_storage("Add %s to database failed !\n", db_full_name);
		}
	}
	else 	/* �������ݿ��������� */
	{
		char sql_cond[1024];

		static int flag_first = 1;
		if (flag_first == 1) 	/* ��һ�ν��룬Ҫ��ȡ���µ�һ��������¼ */
		{
			flag_first = 0;

			char history_time_start[64]; //ʱ����� ʾ����2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_flow_records->uptime, history_time_start);
			sprintf(sql_cond,
			        "SELECT * FROM DB_files WHERE time>='%s'  limit 1;",
			        history_time_start);
			db_read_DB_file(DB_NAME_FLOW_PEDESTRIAN, &db_file,
			                &mutex_db_files_flow_pedestrian, sql_cond);
		}

		printf("db_file.flag_send=%d,db_flow_records->flag_send=%d\n",
		       db_file.flag_send, db_flow_records->flag_send);
		if ((~(db_file.flag_send) & db_flow_records->flag_send) != 0) //���¼�¼�а������µ�������־��Ϣ
		{
			db_file.flag_send |= db_flow_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;",
			        db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_FLOW_PEDESTRIAN, sql_cond,
			                  &mutex_db_records_flow_pedestrian);
		}

		if ((db_file.flag_store == 0) && (db_flow_records->flag_store != 0))//��Ҫ���Ӵ洢��Ϣ
		{
			db_file.flag_store = db_flow_records->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;",
			        db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_FLOW_PEDESTRIAN, sql_cond,
			                  &mutex_db_records_flow_pedestrian);
		}
	}

	if (db_write_traffic_flow_pedestrian(db_full_name, db_flow_records) < 0)
	{
		log_warn_storage(
		    "Write pedestrian traffic flow information to database failed\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * ������: db_format_insert_sql_traffic_flow
 * ��  ��: ��ʽ���¼�������¼��SQL�������
 * ��  ��: sql��SQL��traffic_flow����ͨ��
 * ����ֵ: SQL����
*******************************************************************************/
int db_format_insert_sql_traffic_flow(char *sql, DB_TrafficFlow *traffic_flow)
{
	int len = 0;

	len += sprintf(sql, "INSERT INTO traffic_flow VALUES(NULL");
	len += sprintf(sql+len, ",'%s'", traffic_flow->section_id);
	len += sprintf(sql+len, ",'%s'", traffic_flow->section_name);
	len += sprintf(sql+len, ",'%d'", traffic_flow->large_count);
	len += sprintf(sql+len, ",'%d'", traffic_flow->small_count);
	len += sprintf(sql+len, ",'%d'", traffic_flow->comm_count);
	len += sprintf(sql+len, ",'%d'", traffic_flow->avg_volume);
	len += sprintf(sql+len, ",'%d'", traffic_flow->avg_speed);
	len += sprintf(sql+len, ",'%d'", traffic_flow->occupancy);
	len += sprintf(sql+len, ",'%d'", traffic_flow->queue);
	len += sprintf(sql+len, ",'%d'", traffic_flow->traval_time);
	len += sprintf(sql+len, ",'%s'", traffic_flow->point_id);
	len += sprintf(sql+len, ",'%s'", traffic_flow->point_name);
	len += sprintf(sql+len, ",'%s'", traffic_flow->dev_id);
	len += sprintf(sql+len, ",'%s'", traffic_flow->dev_name);
	len += sprintf(sql+len, ",'%s'", traffic_flow->uptime);
	len += sprintf(sql+len, ",'%d'", traffic_flow->lan_num);
	len += sprintf(sql+len, ",'%d'", traffic_flow->direction);
	len += sprintf(sql+len, ",'%d'", traffic_flow->time_headway);
	len += sprintf(sql+len, ",'%d'", traffic_flow->density);
	len += sprintf(sql+len, ",'%d'", traffic_flow->car_len);
	len += sprintf(sql+len, ",'%d'", traffic_flow->flag_send);
	len += sprintf(sql+len, ",'%d'", traffic_flow->flag_store);
	len += sprintf(sql+len, ");");

	return len;
}


/*******************************************************************************
 * ������: db_format_insert_sql_traffic_flow
 * ��  ��: ��ʽ���¼�������¼��SQL�������
 * ��  ��: sql��SQL��traffic_flow����ͨ��
 * ����ֵ: SQL����
*******************************************************************************/
int db_format_insert_sql_traffic_flow_pedestrian(
    char *sql, const DB_TrafficFlowPedestrian *traffic_flow)
{
	int len = 0;

	len += sprintf(sql, "INSERT INTO traffic_flow_pedestrian VALUES(NULL");
	len += sprintf(sql+len, ",'%d'", traffic_flow->flow_left2right);
	len += sprintf(sql+len, ",'%d'", traffic_flow->flow_right2left);
	len += sprintf(sql+len, ",'%s'", traffic_flow->point_id);
	len += sprintf(sql+len, ",'%s'", traffic_flow->point_name);
	len += sprintf(sql+len, ",'%s'", traffic_flow->dev_id);
	len += sprintf(sql+len, ",'%s'", traffic_flow->dev_name);
	len += sprintf(sql+len, ",'%s'", traffic_flow->uptime);
	len += sprintf(sql+len, ",'%d'", traffic_flow->flag_send);
	len += sprintf(sql+len, ",'%d'", traffic_flow->flag_store);
	len += sprintf(sql+len, ");");

	return len;
}


/*******************************************************************************
 * ������: db_unformat_read_sql_traffic_flow
 * ��  ��: �����ӻ�����ͨ�м�¼���ж���������
 * ��  ��: azResult�����ݿ���ж��������ݻ��棻buf��ͨ�м�¼�ṹ��ָ��
 * ����ֵ: 0����������Ϊ�쳣
*******************************************************************************/
int db_unformat_read_sql_traffic_flow(char *azResult[], DB_TrafficFlow* buf)
{
	DB_TrafficFlow* flow_record;
	flow_record = (DB_TrafficFlow *) buf;
	int ncolumn = 0;

	if (flow_record == NULL)
	{
		printf("db_unformat_read_sql_traffic_flow: flow_record is NULL\n");
		return -1;
	}
	if (azResult == NULL)
	{
		printf("db_unformat_read_sql_traffic_flow: azResult is NULL\n");
		return -1;
	}

	memset(buf, 0, sizeof(DB_TrafficFlow));

	flow_record->ID = atoi(azResult[ncolumn + 0]);

	sprintf(flow_record->section_id, "%s", azResult[ncolumn + 1]);
	sprintf(flow_record->section_name, "%s", azResult[ncolumn + 2]);

	flow_record->large_count = atoi(azResult[ncolumn + 3]);
	flow_record->small_count = atoi(azResult[ncolumn + 4]);
	flow_record->comm_count = atoi(azResult[ncolumn + 5]);
	flow_record->avg_volume = atoi(azResult[ncolumn + 6]);
	flow_record->avg_speed = atoi(azResult[ncolumn + 7]);
	flow_record->occupancy = atoi(azResult[ncolumn + 8]);
	flow_record->queue = atoi(azResult[ncolumn + 9]);
	flow_record->traval_time = atoi(azResult[ncolumn + 10]);

	sprintf(flow_record->point_id, "%s", azResult[ncolumn + 11]);
	sprintf(flow_record->point_name, "%s", azResult[ncolumn + 12]);
	sprintf(flow_record->dev_id, "%s", azResult[ncolumn + 13]);
	sprintf(flow_record->dev_name, "%s", azResult[ncolumn + 14]);
	sprintf(flow_record->uptime, "%s", azResult[ncolumn + 15]);

	flow_record->lan_num = atoi(azResult[ncolumn + 16]);
	flow_record->direction = atoi(azResult[ncolumn + 17]);
	flow_record->time_headway = atoi(azResult[ncolumn + 18]);
	flow_record->density = atoi(azResult[ncolumn + 19]);
	flow_record->car_len = atoi(azResult[ncolumn + 20]);
	flow_record->flag_send = atoi(azResult[ncolumn + 21]);
	flow_record->flag_store = atoi(azResult[ncolumn + 22]);

	return 0;
}


/*******************************************************************************
 * ������: db_unformat_traffic_flow_pedestrian_info
 * ��  ��: �����ӻ�����ͨ�м�¼���ж���������
 * ��  ��: azResult�����ݿ���ж��������ݻ��棻buf��ͨ�м�¼�ṹ��ָ��
 * ����ֵ: 0����������Ϊ�쳣
*******************************************************************************/
int db_unformat_traffic_flow_pedestrian_info(
    char *azResult[], DB_TrafficFlowPedestrian *record)
{
	int ncolumn = 0;

	if (record == NULL)
	{
		printf("db_unformat_read_sql_traffic_flow: flow_record is NULL\n");
		return -1;
	}
	if (azResult == NULL)
	{
		printf("db_unformat_read_sql_traffic_flow: azResult is NULL\n");
		return -1;
	}

	memset(record, 0, sizeof(DB_TrafficFlowPedestrian));

	record->ID = atoi(azResult[ncolumn + 0]);
	record->flow_left2right = atoi(azResult[ncolumn + 1]);
	record->flow_right2left = atoi(azResult[ncolumn + 2]);

	sprintf(record->point_id, "%s", azResult[ncolumn + 3]);
	sprintf(record->point_name, "%s", azResult[ncolumn + 4]);
	sprintf(record->dev_id, "%s", azResult[ncolumn + 5]);
	sprintf(record->dev_name, "%s", azResult[ncolumn + 6]);
	sprintf(record->uptime, "%s", azResult[ncolumn + 7]);

	record->flag_send = atoi(azResult[ncolumn + 8]);
	record->flag_store = atoi(azResult[ncolumn + 9]);

	return 0;
}
