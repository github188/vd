#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>

#include "commonfuncs.h"
#include "songli_db_api.h"
#include "logger/log.h"

sqlite3 *songli_db;

int songli_db_open(const char *path)
{
	int ret = sqlite3_open(path, &songli_db);
	if(ret != SQLITE_OK)
		ERROR("Create park database failed:%s!", sqlite3_errmsg(songli_db));

	return ret;
}

int songli_db_create_msgreget_tb(sqlite3 *db)
{
	char *perrmsg = NULL;

	/*table : id | message*/
    const char *sql = "create table msgreget(id integer primary key,"
        " message varchar(1024));";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int songli_db_create_picreget_tb(sqlite3 *db)
{
	int ret = 0;
	char *perrmsg = NULL;
    const char *sql = "create table picreget(id integer primary key,"
        " picture_path varchar(128), picture_name varchar(64),"
        " picture_size varchar(12));";

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int songli_db_insert_msgreget_tb(sqlite3 *db, songli_msgreget_tb msgreget_tb)
{
	sqlite3_stmt *stmt = NULL;

	const char *sql = "insert into msgreget values(?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);

		return ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, msgreget_tb.message,
            strlen(msgreget_tb.message), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		ERROR("sqlite3_step failed");
  	}

	return ret;
}

//insert park bitcom picture table data
int songli_db_insert_picreget_tb(sqlite3 *db, songli_picreget_tb picreget_tb)
{
	sqlite3_stmt *stmt = NULL;

    const char* sql = "insert into picreget(id, picture_path, picture_name,"
        " picture_size) values(?, ?, ?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, picreget_tb.pic_path, strlen(picreget_tb.pic_path), NULL);
	sqlite3_bind_text(stmt, 3, picreget_tb.pic_name, strlen(picreget_tb.pic_name), NULL);
	sqlite3_bind_int (stmt, 4, picreget_tb.pic_size);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		ERROR("sqlite3_step failed");
  	}

	return SQLITE_OK; // 0
}

int songli_db_select_msgreget_tb(sqlite3 *db, char *message)
{
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	const char* sql = "select * from msgreget limit 1;";

	//sqlite3 prepare
	ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		ret = sqlite3_column_int(stmt, 0);
  		strcpy(message, (char *)sqlite3_column_text(stmt, 1));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return ret;
}

//select park bitcom picture table data
int songli_db_select_picreget_tb(sqlite3 *db, songli_picreget_tb *picreget_tb)
{
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	const char* sql = "select * from picreget limit 1;";

	//sqlite3 prepare
	ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		ret = sqlite3_column_int(stmt, 0);
  		strcpy(picreget_tb->pic_path, (char *)sqlite3_column_text(stmt, 1));
  		strcpy(picreget_tb->pic_name, (char *)sqlite3_column_text(stmt, 2));
  		picreget_tb->pic_size = sqlite3_column_int(stmt, 3);
		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return ret;
}

//delete park bitcom message records by id
int songli_db_delete_msgreget_tb(sqlite3 *db, int id)
{
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from msgreget where id=%d;", id);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return ret;
}

//delete park bitcom picture records by id
int songli_db_delete_picreget_tb(sqlite3 *db, int id)
{
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from picreget where id=%d;", id);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return ret;
}

//count park bitcom message records
int songli_db_count_msgreget_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int songli_db_count_msgreget_tb(sqlite3 *db)
{
	int ret = 0;
	int li_nrow;
	char *perrmsg;

	const char*sql = "select count(*) from msgreget;";

   	ret = sqlite3_exec(db, sql, songli_db_count_msgreget_callback, (void *)&li_nrow, &perrmsg);

    sqlite3_free(perrmsg);
	return li_nrow;
}

//count park bitcom picture records
int songli_db_count_picreget_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int songli_db_count_picreget_tb(sqlite3 *db)
{
	int ret = 0;
	int li_nrow;
	char *perrmsg;

	const char* sql = "select count(*) from picreget;";

   	ret = sqlite3_exec(db, sql, songli_db_count_picreget_callback, (void *)&li_nrow, &perrmsg);

    sqlite3_free(perrmsg);
	return li_nrow;
}
