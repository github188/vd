#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "database_mng.h"
#include "data_process.h"
#include "storage_common.h"

#include "park_records_process.h"
#include "violation_records_process.h"
#include "event_alarm_process.h"
#include "traffic_flow_process.h"
#include "../h264/debug.h"

pthread_mutex_t mutex_db_files_park; 		       	//��������ʻ�����
pthread_mutex_t mutex_db_files_traffic; 			//ͨ�п���ʻ�����
pthread_mutex_t mutex_db_files_violation; 			//Υ������ʻ�����
pthread_mutex_t mutex_db_files_event; 				//�¼�����ʻ�����
pthread_mutex_t mutex_db_files_flow; 				//��������ͨ������ʻ�����
pthread_mutex_t mutex_db_files_flow_pedestrian; 	//���˽�ͨ������ʻ�����

pthread_mutex_t mutex_db_records_park; 			//������¼���ʻ�����
pthread_mutex_t mutex_db_records_traffic; 			//ͨ�м�¼���ʻ�����
pthread_mutex_t mutex_db_records_violation; 		//Υ����¼���ʻ�����
pthread_mutex_t mutex_db_records_event; 			//�¼���¼���ʻ�����
pthread_mutex_t mutex_db_records_flow; 				//��������ͨ����¼���ʻ�����
pthread_mutex_t mutex_db_records_flow_pedestrian; 	//���˽�ͨ����¼���ʻ�����

int flag_stop_history_upload;//ֹͣ��ʷ��¼�ϴ����----���ֽ�ͨ������?


/*******************************************************************************
 * ������: db_init
 * ��  ��: ���ݿ��ʼ�������ø������ݿ�ĳ�ʼ������
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_init(void)
{
	pthread_mutex_init(&mutex_db_files_park, NULL);
	pthread_mutex_init(&mutex_db_files_traffic, NULL);
	pthread_mutex_init(&mutex_db_files_violation, NULL);
	pthread_mutex_init(&mutex_db_files_event, NULL);
	pthread_mutex_init(&mutex_db_files_flow, NULL);
	pthread_mutex_init(&mutex_db_files_flow_pedestrian, NULL);

	pthread_mutex_init(&mutex_db_records_park, NULL);
	pthread_mutex_init(&mutex_db_records_traffic, NULL);
	pthread_mutex_init(&mutex_db_records_violation, NULL);
	pthread_mutex_init(&mutex_db_records_event, NULL);
	pthread_mutex_init(&mutex_db_records_flow, NULL);
	pthread_mutex_init(&mutex_db_records_flow_pedestrian, NULL);

	return 0;
}


/*******************************************************************************
 * ������: db_create_table
 * ��  ��: �������ݱ����ñ��Ѵ��ڣ��򲻻ᴴ��
 * ��  ��: db�����ݿ⣻sql��������SQL
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_create_table(sqlite3 *db, const char *sql)
{
	int rc;
	char *pzErrMsg = NULL;

	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc < 0 || rc > 1)
	{
		debug_print("Create table failed!\n");
		debug_print("Result Code: %d, Error Message: %s\n", rc, pzErrMsg);
		
		sqlite3_close(db);
		return -1;

		
	}
	else 	/* rcֵ��0���ɹ���1���Ѵ��� */
	{
		//debug_print("Result Code: %d, Message: %s\n ", rc, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	return 0;
}


/*******************************************************************************
 * ������: db_write
 * ��  ��: д��¼���ݿ�(��¼�����������Ϣ���������������ݿ�)
 * ��  ��: db_name�����ݿ����ƣ�sql����������SQL
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_write(const char *db_name, const char *sql_insert, const char * sql_create)
{
	printf("In db_write\n");
	int rc;
	sqlite3 *db = NULL;
	char *pzErrMsg = NULL;

	print_time("db_name is :%s.\n",db_name);
	printf("sql_create is :%s.\n",sql_create);
	printf("sql_insert is :%s.\n",sql_insert);
    
	if((db_name==NULL)||(sql_create==NULL)||(sql_insert==NULL))
	{
		printf("input arg is NULL\n");

		return -1;
	}

	rc = sqlite3_open(db_name, &db);//������ݿⲻ���ڻ���д���������������ݿ����ڵ�Ŀ¼�������ڣ���ô�����ᴴ��Ŀ¼
	
	printf("db_write: sqlite3_open  rc=%d.\n",rc);

	if (rc != SQLITE_OK)
	{
		log_warn_storage("sqlite3_open %s failed!\n", db_name);
		return -1;
	}

    //�������ݱ������ַ�ʽ��һ��ʹ��db_create_table,һ��ʹ��sqlite3_exec��������õڶ���
	/*	rc=db_create_table(db, sql_create);
		printf("db_write: db_create_table  rc=%d.\n",rc);
		//usleep(100000);
		if(rc<0)
		{
			printf("db_create_table failed\n");
			return -1;
		}
	*/
	
    //�������ݱ�	
	rc = sqlite3_exec(db, sql_create, 0, 0, &pzErrMsg);
    
	if (rc < 0 || rc > 1)
	{
		log_warn_storage("Create table failed !\n"
		                 "SQL: %s, Result Code: %d, Error Message: %s\n",
		                 sql_create, rc, pzErrMsg);
		sqlite3_close(db);
		return -1;
	}
	else 	/* rcֵ��0���ɹ���1���Ѵ��� */
	{
		printf("Result Code: %d, Message: %s\n ", rc, pzErrMsg);
	}
    
    //��������
	rc = sqlite3_exec(db, sql_insert, 0, 0, &pzErrMsg);
    
	if (rc < 0 || rc > 1)
//	if (rc != SQLITE_OK)//��ʵд�ɹ��ˣ�ȴ����Result Code: 1, Error Message: table traffic_records already exists
	{
		log_warn_storage("Insert data failed !\n"
		                 "SQL: %s, Result Code: %d, Error Message: %s\n",
		                 sql_insert, rc, pzErrMsg);
		sqlite3_close(db);
		return -1;
	}

	printf("db_write: sqlite3_exec  rc=%d.\n",rc);

	sqlite3_free(pzErrMsg);

	sqlite3_close(db);

	return 0;
}
