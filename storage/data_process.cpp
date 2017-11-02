
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <dirent.h>

#include "storage_common.h"
#include "data_process.h"
#include "database_mng.h"
#include "violation_records_process.h"
#include "park_records_process.h"
#include "traffic_records_process.h"
#include "event_alarm_process.h"
#include "traffic_flow_process.h"
#include "disk_func.h"
#include "upload.h"
#include "h264/debug.h"
#include "data_process_park.h"
#include "data_process_traffic.h"
#include "data_process_violation.h"
#include "data_process_event.h"
#include "data_process_flow.h"
#include "PCIe_api.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>
#include "logger/log.h"

/*******************************************************************************
 * 函数名: get_base_time
 * 功  能: 获取图像时间戳与当前时间的差值
 * 返回值: 成功，返回时间相差的毫秒值；失败，返回0
*******************************************************************************/
unsigned long long get_base_time(void)
{
	static unsigned long long base_time = 0; 	/* 只获取一次，确保一致性 */

	if (base_time == 0)
	{
		AV_DATA av_data;

		struct timeval now;
		gettimeofday(&now, NULL);

		if (GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data) < 0)
		{
			log_warn_storage("AV_OP_GET_MJPEG_SERIAL failed\n");
			return 0ULL;
		}

		base_time =
		    (now.tv_sec * 1000ULL) + (now.tv_usec / 1000) - av_data.timestamp;
	}

	return base_time;
}


/*******************************************************************************
 * 函数名: get_pic_info_lock
 * 功  能: 获取图片信息并锁定图片
 * 参  数: av_data，图像数据；pic_info，图片信息；pic_id，图片序列号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int get_pic_info_lock(
    AV_DATA *av_data, VD_PicInfo *pic_info, unsigned int pic_id)
{
	if (GetAVData(AV_OP_LOCK_MJPEG, pic_id, av_data) != RET_SUCCESS)
	{
		return -1;
	}

	static unsigned long long base_time = 0;
	if (base_time == 0)
	{
		base_time = get_base_time();
		if (base_time == 0)
		{
			log_warn_storage("get base time failed !\n");
		}
	}

	time_t pic_sec 	= (base_time + av_data->timestamp) / 1000;
	unsigned int pic_usec 	= (base_time + av_data->timestamp) % 1000;

	struct tm pic_time;
	localtime_r(&pic_sec, &pic_time);

	pic_info->tm.year 		= pic_time.tm_year + 1900;
	pic_info->tm.month 		= pic_time.tm_mon + 1;
	pic_info->tm.day 		= pic_time.tm_mday;
	pic_info->tm.hour 		= pic_time.tm_hour;
	pic_info->tm.minute 	= pic_time.tm_min;
	pic_info->tm.second 	= pic_time.tm_sec;
	pic_info->tm.msecond 	= pic_usec;

	pic_info->buf 	= (void *)(av_data->ptr);
	pic_info->size 	= av_data->size;

	return 0;
}


/*******************************************************************************
 * 函数名: get_pic_info_unlock
 * 功  能: 解锁图片；pic_id，图片序列号
 * 参  数: av_data，图像数据；pic_id，图片序列号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int get_pic_info_unlock(AV_DATA *av_data, unsigned int pic_id)
{
	return GetAVData(AV_OP_UNLOCK_MJPEG, pic_id, av_data);
}


//数据处理初始化
//进行数据库使用锁的初始化，及索引数据库的字段数匹配判断
void init_data_process()
{
	db_init();

	if (mkpath(DB_FILES_PATH, 0755) == 0) 	/* 创建索引数据库存储路径 */
	{
		check_DB_File_columns(DB_NAME_PARK_RECORD, NUM_COLUMN_DB_FILES,
		                      &mutex_db_files_park);
		check_DB_File_columns(DB_NAME_VM_RECORD, NUM_COLUMN_DB_FILES,
		                      &mutex_db_files_traffic);
		check_DB_File_columns(DB_NAME_VIOLATION_RECORDS, NUM_COLUMN_DB_FILES,
		                      &mutex_db_files_violation);
//		check_DB_File_columns(DB_NAME_EVENT_RECORDS, NUM_COLUMN_DB_FILES,
//		                      &mutex_db_files_event);
//		check_DB_File_columns(DB_NAME_FLOW_RECORDS, NUM_COLUMN_DB_FILES,
//		                      &mutex_db_files_flow);
	}
}


//检查数据库是否存在，
//若存在，判断字段数是否匹配。
//若不匹配，删数据库。
//复杂处理: 差一个字段，添加字段，差多个字段，删数据库。相应处理记录数据库。硬盘加载上了?
int check_DB_File_columns(const char *db_name, int num_column_now,
                          pthread_mutex_t *mutex_db_files)
{
	if (access(db_name, F_OK) == 0)
	{
		DB_File db_file;

		int count_column = db_column_num_DB_file(db_name, &db_file,
		                   mutex_db_files);
		if (count_column != num_column_now)
		{
			print_time("check_DB_File_columns: %s count_column=%d, num_column_now=%d\n",db_name,count_column,num_column_now);
			remove(db_name);
		}
	}

	return 0;
}

/*******************************************************************************
 * 函数名: db_column_num_DB_file
 * 功  能: 获取索引数据库的字段数目
 * 参  数:
 * 返回值: 成功，返回索引数据库的字段数目；失败，返回-1
*******************************************************************************/
int db_column_num_DB_file(const char *db_name, void *records,
                          pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];

	//DB_File* db_file;
	//db_file = (DB_File*) records;

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

	rc = db_create_table(db, SQL_CREATE_TABLE_DB_FILES);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	sqlite3_stmt *stat;//=new(sqlite3_stmt);
	int nret;
	int count = 0;

	//查询符合添加的记录总条数
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "SELECT * FROM DB_files limit 1");

	nret = sqlite3_prepare_v2(db, sql, -1, &stat, 0);
	if (nret != SQLITE_OK)
	{
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	count = sqlite3_column_count(stat);//获取查询结果中的字段个数(traffic:29)
	printf("db_column_num_traffic_records  finish, count=%d\n", count);

	sqlite3_finalize(stat); //perpare will lock the db, this unlock the db

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return count;
}

/*******************************************************************************
 * 函数名: file_save
 * 功  能: 文件存盘
 * 参  数: dir，保存路径；name，文件名称；buf，文件缓存；buf_size，缓存大小
 * 返回值:
*******************************************************************************/
int file_save(const char *dir_disk, const char dir[][FTP_PATH_LEN],
              const char *name, const void *buf, size_t buf_size)
{
	FILE *file_to_save = NULL;
	int i;
	int ret;
	char dir_temp[PATH_MAX_LEN];
	char file_name[PATH_MAX_LEN];

	sprintf(dir_temp, "%s", dir_disk);
	for (i = 0; i < STORAGE_PATH_DEPTH; i++)
	{
		if (strlen(dir[i]) <= 0)
		{
			break;
		}
		strcat(dir_temp, "/");
		strcat(dir_temp, dir[i]);
		if (access(dir_temp, F_OK) != 0)
		{
			if (mkdir(dir_temp, 0755) == -1)
			{
				debug_print("mkdir %s error\n", dir_temp);
				return -1;
			}
		}
		debug_print("the dir[%d] is %s\n", i, dir[i]);
	}
	sprintf(file_name,"%s/%s", dir_temp, name);

	debug_print("file_save: file_name is %s\n",file_name);
	for (i = 0; i < RETRY_TIMES; i++)
	{
		file_to_save = fopen(file_name, "wb");
		if (file_to_save == NULL)
		{
			u_sleep(0, 10000);
			if (i == RETRY_TIMES - 1)
			{
				remove(name);
				debug_print("\n Couldn't open file %s ...\n", file_name);
				return -1;
			}
		}
		else
			break;
	}

	ret = fwrite(buf, buf_size, 1, file_to_save);

	fflush(file_to_save);
	fclose(file_to_save);

	if (ret != 1)
	{
		debug_print("Write file failed, return: %d\n", ret);
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: file_save_append
 * 功  能: 以追加方式保存文件
 * 参  数: dir，保存路径；name，文件名称；buf，文件缓存；buf_size，缓存大小
 * 返回值:
*******************************************************************************/
int file_save_append(const char *dir_disk, const char dir[][FTP_PATH_LEN],
                     const char *name, const void *buf, size_t buf_size)
{
	FILE *file_to_save = NULL;
	int i;
	int ret;
	char dir_temp[PATH_MAX_LEN];
	char file_name[PATH_MAX_LEN];

	//追加，则文件必然已经存在?

	sprintf(dir_temp, "%s", dir_disk);
	for (i = 0; i < STORAGE_PATH_DEPTH; i++)
	{
		if (strlen(dir[i]) <= 0)
		{
			break;
		}
		strcat(dir_temp, "/");
		strcat(dir_temp, dir[i]);
		if (access(dir_temp, F_OK) != 0)
		{
			if (mkdir(dir_temp, 0755) == -1)
			{
				debug_print("mkdir %s error\n", dir_temp);
				return -1;
			}
		}
		debug_print("the dir[%d] is %s\n", i, dir[i]);
	}

	sprintf(file_name, "%s/%s", dir_temp, name);
	printf("file_save_append: file_name is %s\n", file_name);
	debug_print("file_save_append: file_name is %s\n",file_name);
	for (i = 0; i < RETRY_TIMES; i++)
	{
		file_to_save = fopen(file_name, "ab");
		if (file_to_save == NULL)
		{
			u_sleep(0, 10000);
			if (i == RETRY_TIMES - 1)
			{
				remove(name);
				debug_print("\n Couldn't open file %s ...", file_name);
				return -1;
			}
		}
		else
			break;
	}


	ret = fwrite(buf, buf_size, 1, file_to_save);

	fflush(file_to_save);
	fclose(file_to_save);

	if (ret != 1)
	{
		debug_print("Write file failed, return: %d\n", ret);
		return -1;
	}

	return 0;
}

//进行一级目录创建
int dir_create(char *dir)
{
	if (strlen(dir) <= 0)
	{
		printf("dir is %s  --err\n", dir);
		return -1;
	}
	//sprintf(dir_temp,"%s/%s",dir_temp,dir[i]);
	if (access(dir, F_OK) != 0)
	{
		if (mkdir(dir, 0755) == -1)
		{
			debug_print("mkdir %s error\n", dir);
			return -1;
		}
	}
	debug_print("the dir is %s\n", dir);

	return 0;
}

/*
 * 启动存储数据续传线程。
 */
void start_db_upload_handler_thr()
{
	int ret;
	pthread_t thread_db_upload;
	printf("to create db_upload_Thr\n");
	ret = pthread_create(&thread_db_upload, NULL, db_upload_Thr, NULL);
	if (0 != ret)
	{
		log_error_storage("Create thread_db_upload failed : %m !\n");
	}
}


/********************************************************
 *函数名:db_upload_Thr
 *功能:  交通数据续传线程
 *参数:
 *返回值:
 ********************************************************/
void *db_upload_Thr(void *arg)
{
	int num_loop = 0;
	int ret = 0;
	int sec_wait = 1;

	for (;;)
	{
		if ((num_loop++) % 100 == 0)
		{
			DEBUG("in db_upload_Thr:  num_loop=%d\n",num_loop);
		}

		u_sleep(0, 100000); 	/* 续传线程，不能过多占用cpu */

		/* 控制在连续没有续传数据时，增加等待时间 */
		if (ret != 0)
		{
			if (sec_wait < 30)
				sec_wait++;
		}
		else
		{
			sec_wait = 1;
		}

		u_sleep(sec_wait, 0);

		if(1 == DEV_TYPE)     //   1 泊车
		{
			printf("Enter park_record_reupload\n");
			ret = park_record_reupload();
		}

		if(2 == DEV_TYPE || 3 == DEV_TYPE || 5 == DEV_TYPE)     //2//2,3 卡口
		{
			int disk_status = disk_get_status();
			if (disk_status) //硬盘不可用
			{
				DEBUG("硬盘不可用，不续传\n");
				u_sleep(3, 0);
				continue;
			}

			DEBUG("Enter traffic_record_reupload\n");
			ret = traffic_record_reupload();
			db_record_clr(DB_NAME_VM_RECORD, &mutex_db_records_traffic);

	//		ret &= db_upload_event_records();
	//		ret &= db_upload_flow_records();
	//		ret &= traffic_flow_reupload_pedestrian();

		}

        if(4 == DEV_TYPE)     //2//4 违停
		{
			int disk_status = disk_get_status();
			if (disk_status) //硬盘不可用
			{
				printf("硬盘不可用，不续传\n");
				u_sleep(3, 0);
				continue;
			}

			printf("Enter violation_record_reupload\n");
		    ret = db_upload_violation_records_motor_vehicle();
		}
	 }

	pthread_exit(NULL);
}


//解析索引数据库的一条记录
int db_unformat_read_sql_db_files(char *azResult[], DB_File *buf)
{
	DB_File *db_file = (DB_File *) buf;

	int ncolumn = 0;

	if (db_file == NULL)
	{
		printf("db_unformat_read_sql_db_files: db_file is NULL\n");
		return -1;
	}
	if (azResult == NULL)
	{
		printf("db_unformat_read_sql_db_files: azResult is NULL\n");
		return -1;
	}

	memset(db_file, 0, sizeof(DB_File));

	db_file->ID = atoi(azResult[ncolumn + 0]);
	db_file->record_type = atoi(azResult[ncolumn + 1]);
	sprintf(db_file->time, "%s", azResult[ncolumn + 2]);
	sprintf(db_file->record_path, "%s", azResult[ncolumn + 3]);
	db_file->flag_send = atoi(azResult[ncolumn + 4]);
	db_file->flag_store = atoi(azResult[ncolumn + 5]);

	return 0;
}

//按指定条件读取一条索引记录
int db_read_DB_file(const char *db_name, DB_File *db_file,
                    pthread_mutex_t *mutex_db_files, char * sql_cond)
{

	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
	char **azResult; //二维数组存放结果

	int ID_read = -1;
	//DB_File  db_file;
	//traffic_record = (DB_File *)records;

	pthread_mutex_lock(mutex_db_files);

	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		ERROR("Create database %s failed!\n", db_name);
		ERROR("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_files);
		return -1;
	}
	else
	{
		//printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}

	rc = db_create_table(db, SQL_CREATE_TABLE_DB_FILES);
	if (rc < 0)
	{
		ERROR("db_create_table failed\n");
		pthread_mutex_unlock(mutex_db_files);
		return -1;
	}

	//查询数据
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM DB_files limit 1");//应该与ID 续传标志相关
    // For park platform, I dont want too much log especially the loop msg
    // from reget thread.
#if (DEV_TYPE != 1)
	DEBUG("sql_cond is :%s\n", sql_cond);
#endif
	nrow = 0;
	rc = sqlite3_get_table(db, sql_cond, &azResult, &nrow, &ncolumn, &pzErrMsg);
	if (rc != SQLITE_OK || nrow == 0)
	{
#if (DEV_TYPE != 1)
		ERROR("Can't require data, Error Message: %s\n", pzErrMsg);
		DEBUG("row: %d, column: %d \n", nrow, ncolumn);
#endif
		sqlite3_free(pzErrMsg);
		sqlite3_free_table(azResult);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_files);
		return -1;
	}

	DEBUG(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn][0]);

	ID_read = atoi(azResult[ncolumn + 0]);//关键字，从1开始累加

	db_unformat_read_sql_db_files(&(azResult[ncolumn]), db_file);

	//printf("db_unformat_read_sql_db_files  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);

	pthread_mutex_unlock(mutex_db_files);

	return ID_read;
}



//输入数据库名，及查询条件，返回符合条件的记录条数
int db_count_records(const char *db_name, char *sql_cond,
                     pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	sqlite3_stmt *stat;//=new(sqlite3_stmt);
	int nret;
	int count=0;

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


	//查询符合添加的记录总条数

	nret=sqlite3_prepare_v2(db, sql_cond, -1, &stat, 0);
	if (nret!=SQLITE_OK)
	{
		// sqlite3_free(pzErrMsg);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	count=0;
	if (sqlite3_step(stat)==SQLITE_ROW)
	{
		count=sqlite3_column_int(stat, 0);
	}

	sqlite3_finalize(stat); //perpare will lock the db, this unlock the db

	printf("db_count_records  finish, count=%d\n",count);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return count;
}


/*******************************************************************************
 * 函数名: db_update_records
 * 功  能: 修改记录内容
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_update_records(const char *db_name, char *sql_cond,
                      pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char *pzErrMsg = NULL;
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

	//查询符合添加的记录总条数
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "UPDATE traffic_records SET flag_send=1 WHERE flag_send=0");// ok
	print_time("db_update_records: sql_cond is %s\n",sql_cond);


	rc = sqlite3_exec(db, sql_cond, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		printf("db_update_records failed, Error Message: %s\n", pzErrMsg);
	}
	sqlite3_free(pzErrMsg);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	TRACE_LOG_PACK_PLATFORM("%s, %s", "Records database update successful!", sql_cond);

	return 0;
}

//删除一条记录
int db_delete_DB_file(const char *db_name, DB_File *db_file,
                      pthread_mutex_t *mutex_db_files)
{
	DEBUG("In db_delete_DB_file\n");
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];

	pthread_mutex_lock(mutex_db_files);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		ERROR("Create database %s failed!\n", db_name);
		ERROR("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_files);
		return -1;
	}
	else
	{
		DEBUG("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}
	/*
	 rc=db_create_table(db, SQL_CREATE_TABLE_DB_FILES);
	 if(rc<0)
	 {
	 printf("db_create_table failed\n");
	 pthread_mutex_unlock(mutex_db_files);
	 return -1;
	 }
	 */

	//查询数据
	memset(sql, 0, sizeof(sql));

	if (db_file->flag_store == 1)
	{
		//清理续传标志
		sprintf(sql, "UPDATE DB_files SET flag_send=0 WHERE ID = %d ;",
		        db_file->ID);
	}
	else
	{

		remove(db_file->record_path);//删除记录数据库

		//删除指定ID 的记录	//只有在一个数据库全部续传完成时，才能删除这条对应的记录
		sprintf(sql, "DELETE FROM DB_files WHERE ID = %d ;", db_file->ID);
	}

	DEBUG("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		ERROR("Delete data failed, flag_store=%d,  Error Message: %s\n",
		       db_file->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);
	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_files);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	INFO("db_delete_DB_file  finish\n");

	return 0;
}

int db_format_insert_sql_db_files(char *sql, DB_File *db_file)
{
	int len = 0;

	memset(sql, 0, SQL_BUF_SIZE);

	len += sprintf(sql, "INSERT INTO DB_files VALUES(NULL");
	len += sprintf(sql + len, ",'%d'", db_file->record_type);
	len += sprintf(sql + len, ",'%s'", db_file->time);
	len += sprintf(sql + len, ",'%s'", db_file->record_path);
	len += sprintf(sql + len, ",'%d'", db_file->flag_send);
	len += sprintf(sql + len, ",'%d'", db_file->flag_store);
	len += sprintf(sql + len, ");");

	return len;

}


/*******************************************************************************
 * 函数名: db_write_DB_file
 * 功  能: 写交通通行记录数据库
 * 参  数: records，交通通行记录
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_write_DB_file(const char *db_name, DB_File *db_file,
                     pthread_mutex_t *mutex_db_files)
{
	char sql[SQL_BUF_SIZE];

	db_format_insert_sql_db_files(sql, db_file);

	pthread_mutex_lock(mutex_db_files);
	int ret = db_write(db_name, sql, SQL_CREATE_TABLE_DB_FILES); //TODO 返回值判断
	pthread_mutex_unlock(mutex_db_files);

	if (ret != 0)
	{
		printf("db_write_DB_file failed\n");
		return -1;
	}

	return 0;
}

/*
 获取对应条件的记录总数
 */
int get_record_count(TYPE_HISTORY_RECORD *type_history_record)
{
	int count = -1;

	print_time("in get_record_count:\n");

	if (type_history_record == NULL)
	{
		print_time("get_record_count: type_history_record is NULL \n");
		return -1;
	}

	if (type_history_record->event_type == 0)//过车
	{

		count = get_record_count_traffic(DB_NAME_VM_RECORD,
		                                 type_history_record, &mutex_db_files_traffic);

	}
	else if (type_history_record->event_type == 1)//违法
	{
		count = get_record_count_violation((char*)DB_NAME_VIOLATION_RECORDS,
		                                   type_history_record, &mutex_db_files_violation);

	}
	else if (type_history_record->event_type == 2)//交通流
	{
		count = get_record_count_flow((char*)DB_NAME_FLOW_RECORDS,
		                              type_history_record, &mutex_db_files_flow);

	}
	else if (type_history_record->event_type == 3)//事件
	{
		count = get_record_count_event((char*)DB_NAME_EVENT_RECORDS,
		                               type_history_record, &mutex_db_files_event);

	}

	return count;

}

/*
 启动历史记录上传
 */
int start_upload_history_record(TYPE_HISTORY_RECORD *type_history_record)
{
	int ret = -1;
	pthread_t thread_upload_history_record;

	if (type_history_record == NULL)
	{
		print_time("get_record_count: type_history_record is NULL \n");
		return -1;
	}

	flag_stop_history_upload = 0;

	printf("to create thread_upload_history_record\n");
	if (type_history_record->event_type == 0)//过车
	{
		ret = pthread_create(&thread_upload_history_record, NULL,
		                     db_upload_history_traffic_Thr, type_history_record);

	}
	else if (type_history_record->event_type == 1)//违法
	{
		ret = pthread_create(&thread_upload_history_record, NULL,
		                     db_upload_history_violation_Thr, type_history_record);

	}
	else if (type_history_record->event_type == 2)//交通流
	{
		ret = pthread_create(&thread_upload_history_record, NULL,
		                     db_upload_history_flow_Thr, type_history_record);

	}
	else if (type_history_record->event_type == 3)//事件
	{
		ret = pthread_create(&thread_upload_history_record, NULL,
		                     db_upload_history_event_Thr, type_history_record);

	}

	if (0 != ret)
	{
		log_error_storage("Create thread_upload_history_record failed : %m ! "
		                  "type_history_record : %d\n",
		                  type_history_record->event_type);
	}

	return 0;

}

/*
 停止历史记录上传
 */
int stop_upload_history_record()
{

	flag_stop_history_upload=1;

	return 0;

}




int get_hour_start(const char *time_start, char *history_time_start)
{
	if ((time_start==NULL)||(history_time_start==NULL))
	{
		print_time("get_hour_start: input args is NULL\n");
		return -1;
	}
	const char * str1;
	//memset(history_time_start, 0, sizeof(history_time_start));
	str1=strchr(time_start,	':');
	if (str1==NULL)
	{
		print_time("get_hour_start: search ':' failed . time_start is %s\n",time_start);
		return -1;
	}
	memcpy(history_time_start, time_start , (int)str1 -(int)(time_start));//拷贝':'之前的
	strcat(history_time_start,":00:00");//设置':'之后的
	return 0;
}


int get_hour_end(const char *time_end, char *history_time_end)
{
	if ((time_end==NULL)||(history_time_end==NULL))
	{
		print_time("get_hour_end: input args is NULL\n");
		return -1;
	}
	const char * str1;
	//memset(history_time_end, 0, sizeof(history_time_end));
	str1=strchr(time_end,	':');
	if (str1==NULL)
	{
		print_time("get_hour_end: search ':' failed . time_end is %s\n",time_end);
		return -1;
	}
	memcpy(history_time_end, time_end , (int)str1 -(int)(time_end));//拷贝':'之前的
	strcat(history_time_end,":59:59");//设置':'之后的
	return 0;
}


/*******************************************************************************
 * 函数名: move_record_to_trash
 * 功  能: 将record中的文件移到trash
 * 参  数: partition_path，分区路径；file_path，文件路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int move_record_to_trash(const char *partition_path, const char *file_path)
{
	do 	/* 先确保trash文件夹中存在相应路径 */
	{
		char *parent_path = get_parent_path(file_path);
		if (parent_path == NULL)
		{
			log_debug_storage("Get parent path of %s failed !\n", file_path);
			return -1;
		}

		char new_parent_path[PATH_MAX_LEN];
		snprintf(new_parent_path, PATH_MAX_LEN, "%s/%s/%s",
		         partition_path, DISK_TRASH_PATH, parent_path);

		safe_free(parent_path); 	/* 一定要释放！ */

		if (access(new_parent_path, F_OK) != 0)
		{
			if (mkpath(new_parent_path, 0755) < 0)
			{
				log_debug_storage("mkpath %s failed !\n", new_parent_path);
				return -1;
			}
		}
	}
	while (0);

	char old_path[PATH_MAX_LEN];
	snprintf(old_path, PATH_MAX_LEN, "%s/%s/%s",
	         partition_path, DISK_RECORD_PATH, file_path);

	char new_path[PATH_MAX_LEN];
	snprintf(new_path, PATH_MAX_LEN, "%s/%s/%s",
	         partition_path, DISK_TRASH_PATH, file_path);

	if (rename(old_path, new_path) < 0)
	{
		log_debug_storage("rename %s to %s failed : %m\n", old_path, new_path);
		return -1;
	}

	system("rm /media/park/trash/* -rf");

	return 0;
}

/**
 * db_get_oldest_DB_file - get the oldest DB file
 */
static int32_t db_get_oldest_DB_file(const char *path, DB_File *db_file,
									 pthread_mutex_t *mutex)
{
	char sql[512];

	snprintf(sql, sizeof(sql),
			 "select * from DB_files order by id asc limit 1;");
	return db_read_DB_file(path, db_file, mutex, sql);
}

static int32_t db_get_oldest_record(const char *path, void *record,
									pthread_mutex_t *mutex)
{
	char sql[512];

	snprintf(sql, sizeof(sql),
			 "select * from traffic_records order by id asc limit 1;");
	return db_read_traffic_record(path, record, mutex, sql);
}

static bool dir_is_empty(const char *path)
{
	DIR *dir;
	struct dirent *entry;
	int32_t cnt;

	dir = opendir(path);
	if (!dir) {
		ERROR("open dir %s failed!", path);
		return false;
	}

	cnt = 0;
	while ((entry = readdir(dir)) && (cnt++ < 2));

	closedir(dir);

	return (2 == cnt) ? true : false;
}

int32_t db_record_clr(const char *path, pthread_mutex_t *mutex)
{
	struct statfs stat;
	DB_File db_file;
	DB_TrafficRecord record;
	char buf[1024];
	/* The remain rate of disk configured by user, percent */
	int32_t remain;

	if (-1 == statfs("/mnt/mmc", &stat)) {
		ERROR("Stat /mnt/mmc failed!");
		return -1;
	}

	DEBUG("MMC stat: total %d(KB) available %d(KB) free %d%%",
		  (stat.f_blocks * stat.f_bsize) >> 10,
		  (stat.f_bfree * stat.f_bsize) >> 10,
		  stat.f_bfree * 100 / stat.f_blocks);

	remain = g_arm_config.basic_param.data_save.disk_wri_data.remain_disk;
	if((stat.f_bfree * 100 / stat.f_blocks) > remain){
		return 0;
	}

	INFO("The avaliable blocks is smaller than %d%%", remain);

	/* remove the trash directory */
	system("rm -rf /mnt/mmc/trash/*");

	if(db_get_oldest_DB_file(path, &db_file, mutex) <= 0){
		ERROR("Get the oldest DB file failed!");
		return -1;
	}

	DEBUG("Got the oldest DB file: %s!", db_file.record_path);

	/* remove the pic of record */
	while (-1 != db_get_oldest_record(db_file.record_path, &record, mutex)) {
		snprintf(buf, sizeof(buf),
				 "%s/%s/%s",
				 record.partition_path, DISK_RECORD_PATH, record.image_path);
		remove(buf);
		DEBUG("remove %s", buf);
		db_delete_traffic_records(db_file.record_path, &record, mutex);
	}

	/* delete a DB file */
	db_delete_DB_file(path, &db_file, mutex);

	snprintf(buf, sizeof(buf),
			 "%s/%s",
			 record.partition_path, DISK_RECORD_PATH);
	/* if the directory is empty, then remove it */
	if (dir_is_empty(buf)) {
		DEBUG("%s is empty", buf);
		rmdir(buf);
	}

	return 0;
}
