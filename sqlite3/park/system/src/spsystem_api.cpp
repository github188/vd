#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>

#include "commonfuncs.h"
#include "spsystem_api.h"
#include "logger/log.h"

sqlite3 *spsystem_db;

//Create park sqlptie3 database
int spsystem_open(const char *ac_path)
{
	int li_ret = 0;

	li_ret = sqlite3_open(ac_path, &spsystem_db);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("Create park sqlite3 database failed:%s!", sqlite3_errmsg(spsystem_db));
	}

	return li_ret;
}

//Create park last message record table
int spsystem_create_lastmsg_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;

	/*table : id | plate_num | status | eltype | eltime*/
	const char *sql = "create table lastmsg(id integer primary key, plate_num varchar(32), status varchar(32), eltype varchar(32), eltime varchar(32));";

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}

//Create park zehin last message record table
int spsystem_create_picture_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;

	/*table : id | picture_name | picture_path | bitcom_flag | zehin_flag */
    const char *sql = "create table picture(id integer primary key,"
                      "picture_name varchar(32), picture_path varchar(128),"
                      "bitcom_flag varchar(8), zehin_flag varchar(8),"
                      " songli_flag varchar(8), http_flag varchar(8));";

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);

		return -1;
	}

	return li_ret;
}


//Create park config table
int spsystem_create_config_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;

	/*table : id | bitcom | zehin */
	const char *sql = "create table config(id integer primary key, bitcom varchar(8), zehin varchar(8));";

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}


//insert park zehin lastmsg table data
int spsystem_insert_lastmsg_table(sqlite3 *asql_db, char ac_values[][128])
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into lastmsg values(?, ?, ?, ?, ?);");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, ac_values[1], strlen(ac_values[1]), NULL);
	sqlite3_bind_text(stmt, 3, ac_values[2], strlen(ac_values[2]), NULL);
	sqlite3_bind_text(stmt, 4, ac_values[3], strlen(ac_values[3]), NULL);
	sqlite3_bind_text(stmt, 5, ac_values[4], strlen(ac_values[4]), NULL);

	//sqlite3 step
	li_ret = sqlite3_step(stmt);
	if(li_ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return li_ret;
}


//insert park picture table data
int spsystem_insert_picture_table(sqlite3 *asql_db, char ac_values[][128])
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into picture values(?, ?, ?, ?, ?, ?, ?);");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, ac_values[1], strlen(ac_values[1]), NULL);
	sqlite3_bind_text(stmt, 3, ac_values[2], strlen(ac_values[2]), NULL);
	sqlite3_bind_text(stmt, 4, ac_values[3], strlen(ac_values[3]), NULL);
	sqlite3_bind_text(stmt, 5, ac_values[4], strlen(ac_values[4]), NULL);
	sqlite3_bind_text(stmt, 6, ac_values[5], strlen(ac_values[5]), NULL);
	sqlite3_bind_text(stmt, 7, ac_values[6], strlen(ac_values[6]), NULL);

	//sqlite3 step
	li_ret = sqlite3_step(stmt);
	if(li_ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return li_ret;
}

//insert park zehin message table data
int spsystem_insert_config_table(sqlite3 *asql_db, char *ac_values)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into config values(?, ?, ?);");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 bind *

	sqlite3_bind_text(stmt, 2, ac_values, strlen(ac_values), NULL);
	sqlite3_bind_text(stmt, 3, ac_values, strlen(ac_values), NULL);

	//sqlite3 step
	li_ret = sqlite3_step(stmt);
	if(li_ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return li_ret;
}

//check park picture exsist
int spsystem_check_picture_exist(sqlite3 *asql_db, const char *ac_values)
{
	int li_ret = 0;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	sprintf(sql, "select * from picture where picture_name=\"%s\" limit 1;", ac_values);

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);


		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}



//update park picture table data
int spsystem_update_picture_table(sqlite3 *asql_db, const char *ac_name, int ai_values, int ai_type)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	switch(ai_type)
	{
	    case PARK_BITCOM:
			sprintf(sql,"update picture set bitcom_flag=%d where picture_name=\"%s\";", ai_values, ac_name);
		break;
		case PARK_ZEHIN:
			sprintf(sql,"update picture set zehin_flag=%d where picture_name=\"%s\";", ai_values, ac_name);
		break;
		case PARK_SONGLI:
			sprintf(sql,"update picture set songli_flag=%d where picture_name=\"%s\";", ai_values, ac_name);
            break;
		case PARK_HTTP:
			sprintf(sql,"update picture set http_flag=%d where picture_name=\"%s\";", ai_values, ac_name);
            break;
        default:
            break;
	}

    DEBUG("%s", sql);
   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}

//select park system table data
int spsystem_select_picture_table(sqlite3 *asql_db, char ac_values[][128], int ai_type)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	if(ai_type == 1)
	{
        sprintf(sql, "select * from picture where bitcom_flag=\"1\""
                     " and zehin_flag=\"1\" and songli_flag=\"1\""
                     " and http_flag=\"1\" limit 1;");
	}
	else if(ai_type == 2)
	{
		sprintf(sql, "select * from picture limit 1;");
	}

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
		strcpy(ac_values[1], (char *)sqlite3_column_text(stmt, 1));
		strcpy(ac_values[2], (char *)sqlite3_column_text(stmt, 2));
		strcpy(ac_values[3], (char *)sqlite3_column_text(stmt, 3));
		strcpy(ac_values[4], (char *)sqlite3_column_text(stmt, 4));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}


//select park system lastmsg table data
int spsystem_select_lastmsg_table(sqlite3 *asql_db, char ac_values[][128])
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	sprintf(sql, "select * from lastmsg order by id DESC limit 1;");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
		strcpy(ac_values[1], (char *)sqlite3_column_text(stmt, 1));
		strcpy(ac_values[2], (char *)sqlite3_column_text(stmt, 2));
		strcpy(ac_values[3], (char *)sqlite3_column_text(stmt, 3));
		strcpy(ac_values[4], (char *)sqlite3_column_text(stmt, 4));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}


//select park config table data
int spsystem_select_config_table(sqlite3 *asql_db, char *ac_values[128])
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	sprintf(sql, "select * from config limit 1;");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
		strcpy(ac_values[1], (char *)sqlite3_column_text(stmt, 1));
		strcpy(ac_values[2], (char *)sqlite3_column_text(stmt, 2));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}


//delete park system lastmsg records
int spsystem_delete_lastmsg_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from lastmsg;");

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}



//delete
int spsystem_delete_picture_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from picture where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}

//delete
int spsystem_delete_config_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from config where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_ret;
}


//count
int spsystem_count_picture_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spsystem_count_picreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	int li_nrow;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from picture;");

   	li_ret = sqlite3_exec(asql_db, sql, spsystem_count_picture_callback, (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
	}

	return li_nrow;
}

