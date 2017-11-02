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

pthread_mutex_t mutex_db_files_park; 		       	//泊车库访问互斥锁
pthread_mutex_t mutex_db_files_traffic; 			//通行库访问互斥锁
pthread_mutex_t mutex_db_files_violation; 			//违法库访问互斥锁
pthread_mutex_t mutex_db_files_event; 				//事件库访问互斥锁
pthread_mutex_t mutex_db_files_flow; 				//机动车交通流库访问互斥锁
pthread_mutex_t mutex_db_files_flow_pedestrian; 	//行人交通流库访问互斥锁

pthread_mutex_t mutex_db_records_park; 			//泊车记录访问互斥锁
pthread_mutex_t mutex_db_records_traffic; 			//通行记录访问互斥锁
pthread_mutex_t mutex_db_records_violation; 		//违法记录访问互斥锁
pthread_mutex_t mutex_db_records_event; 			//事件记录访问互斥锁
pthread_mutex_t mutex_db_records_flow; 				//机动车交通流记录访问互斥锁
pthread_mutex_t mutex_db_records_flow_pedestrian; 	//行人交通流记录访问互斥锁

int flag_stop_history_upload;//停止历史记录上传标记----区分交通类型吗?


/*******************************************************************************
 * 函数名: db_init
 * 功  能: 数据库初始化，调用各个数据库的初始化函数
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: db_create_table
 * 功  能: 创建数据表，若该表已存在，则不会创建
 * 参  数: db，数据库；sql，创建表SQL
 * 返回值: 成功，返回0；失败，返回-1
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
	else 	/* rc值：0，成功；1，已存在 */
	{
		//debug_print("Result Code: %d, Message: %s\n ", rc, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	return 0;
}


/*******************************************************************************
 * 函数名: db_write
 * 功  能: 写记录数据库(记录车辆的相关信息，区分与索引数据库)
 * 参  数: db_name，数据库名称；sql，插入数据SQL
 * 返回值: 成功，返回0；失败，返回-1
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

	rc = sqlite3_open(db_name, &db);//如果数据库不存在会进行创建，但是如果数据库所在的目录都不存在，那么并不会创建目录
	
	printf("db_write: sqlite3_open  rc=%d.\n",rc);

	if (rc != SQLITE_OK)
	{
		log_warn_storage("sqlite3_open %s failed!\n", db_name);
		return -1;
	}

    //创建数据表有两种方式，一种使用db_create_table,一种使用sqlite3_exec，这里采用第二种
	/*	rc=db_create_table(db, sql_create);
		printf("db_write: db_create_table  rc=%d.\n",rc);
		//usleep(100000);
		if(rc<0)
		{
			printf("db_create_table failed\n");
			return -1;
		}
	*/
	
    //创建数据表	
	rc = sqlite3_exec(db, sql_create, 0, 0, &pzErrMsg);
    
	if (rc < 0 || rc > 1)
	{
		log_warn_storage("Create table failed !\n"
		                 "SQL: %s, Result Code: %d, Error Message: %s\n",
		                 sql_create, rc, pzErrMsg);
		sqlite3_close(db);
		return -1;
	}
	else 	/* rc值：0，成功；1，已存在 */
	{
		printf("Result Code: %d, Message: %s\n ", rc, pzErrMsg);
	}
    
    //插入数据
	rc = sqlite3_exec(db, sql_insert, 0, 0, &pzErrMsg);
    
	if (rc < 0 || rc > 1)
//	if (rc != SQLITE_OK)//其实写成功了，却报错？Result Code: 1, Error Message: table traffic_records already exists
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
