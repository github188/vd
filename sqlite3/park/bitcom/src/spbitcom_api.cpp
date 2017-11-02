#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>

#include "commonfuncs.h"
#include "spbitcom_api.h"
#include "logger/log.h"

sqlite3 *spbitcom_db;

//Create park sqlptie3 database
int spbitcom_open(const char *ac_path)
{
	int li_ret = 0;

	li_ret = sqlite3_open(ac_path, &spbitcom_db);
	if(li_ret != SQLITE_OK)
	{
		ERROR("Create park sqlite3 database failed:%s!",
                sqlite3_errmsg(spbitcom_db));
	}

	return li_ret;
}

//Create park bitcom message reget table
int spbitcom_create_msgreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;

	/*table : id | message*/
    const char *sql = "create table msgreget(id integer primary key,"
        " message varchar(1024));";

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_ret;
}

//Create park bitcom picture reget table
int spbitcom_create_picreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;
    const char *sql = "create table picreget(id integer primary key,"
        " picture_path varchar(128), picture_name varchar(64),"
        "picture_size varchar(12));";

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_ret;
}


//insert park bitcom message table data
int spbitcom_insert_msgreget_table(sqlite3 *asql_db, str_spbitcom_msgreget_table as_spbitcom_msgreget_table)
{
	int li_ret = 0;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into msgreget values(?, ?);");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return li_ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, as_spbitcom_msgreget_table.message, strlen(as_spbitcom_msgreget_table.message), NULL);

	//sqlite3 step
	li_ret = sqlite3_step(stmt);
	if(li_ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return li_ret;
}

//insert park bitcom picture table data
int spbitcom_insert_picreget_table(sqlite3 *asql_db, str_spbitcom_picreget_table as_spbitcom_picreget_table)
{
	int li_ret = 0;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into picreget(id, picture_path, picture_name, picture_size) values(?, ?, ?, ?);");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return li_ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, as_spbitcom_picreget_table.pic_path, strlen(as_spbitcom_picreget_table.pic_path), NULL);
	sqlite3_bind_text(stmt, 3, as_spbitcom_picreget_table.pic_name, strlen(as_spbitcom_picreget_table.pic_name), NULL);
	sqlite3_bind_text(stmt, 4, as_spbitcom_picreget_table.pic_size, strlen(as_spbitcom_picreget_table.pic_size), NULL);

	//sqlite3 step
	li_ret = sqlite3_step(stmt);
	if(li_ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return SQLITE_OK; // 0
}



//select park bitcom message table data
int spbitcom_select_msgreget_table(sqlite3 *asql_db, char *ac_message)
{
	int li_ret = 0;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	sprintf(sql, "select * from msgreget limit 1;");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
  		strcpy(ac_message, (char *)sqlite3_column_text(stmt, 1));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}

//select park bitcom picture table data
int spbitcom_select_picreget_table(sqlite3 *asql_db, str_spbitcom_picreget_table *as_spbitcom_picreget_table)
{
	int li_ret = 0;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	sprintf(sql, "select * from picreget limit 1;");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt)
            sqlite3_finalize(stmt);
		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
  		strcpy(as_spbitcom_picreget_table->pic_path, (char *)sqlite3_column_text(stmt, 1));
  		strcpy(as_spbitcom_picreget_table->pic_name, (char *)sqlite3_column_text(stmt, 2));
  		strcpy(as_spbitcom_picreget_table->pic_size, (char *)sqlite3_column_text(stmt, 3));
		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}

//delete park bitcom message records by id
int spbitcom_delete_msgreget_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from msgreget where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_ret;
}

//delete park bitcom picture records by id
int spbitcom_delete_picreget_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from picreget where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_ret;
}

//count park bitcom message records
int spbitcom_count_msgreget_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spbitcom_count_msgreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	int li_nrow;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from msgreget;");

   	li_ret = sqlite3_exec(asql_db, sql, spbitcom_count_msgreget_callback, (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_nrow;
}

//count park bitcom picture records
int spbitcom_count_picreget_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spbitcom_count_picreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	int li_nrow;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from picreget;");

   	li_ret = sqlite3_exec(asql_db, sql, spbitcom_count_picreget_callback, (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		ERROR("%s", perrmsg);
	}

    sqlite3_free(perrmsg);
	return li_nrow;
}

