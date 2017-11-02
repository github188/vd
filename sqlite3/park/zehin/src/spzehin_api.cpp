#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>

#include "commonfuncs.h"
#include "spzehin_api.h"
#include "logger/log.h"

sqlite3 *spzehin_db;

//Create park sqlptie3 database
int spzehin_open(const char *path)
{
	int ret = sqlite3_open(path, &spzehin_db);
	if (ret != SQLITE_OK) {
		DEBUG("Create zehin database failed:%s!", sqlite3_errmsg(spzehin_db));
	}

	return -ret;
}

//Create park zehin message reget table
int spzehin_create_msgreget_table(sqlite3 *asql_db)
{
	char *perrmsg = NULL;

	/*table : id | berth_num | plate_num | plate_color | status | confidence | eltype | eltime | plate_coordinate | uuid */
	const char *sql = "create table msgreget(id integer primary key,\
                       berth_num varchar(32),\
                       plate_num varchar(32),\
                       plate_color varchar(32),\
                       status varchar(32),\
                       confidence varchar(32),\
                       eltype varchar(32),\
                       eltime varchar(32),\
                       plate_coordinate varchar(32),\
                       uuid varchar(128));";

   	int ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if (ret != SQLITE_OK) {
		DEBUG("%s", perrmsg);
	}

	return -ret;
}

//Create park zehin alarm reget table
int spzehin_create_alarmreget_table(sqlite3 *db)
{
	char *perrmsg = NULL;

	/*table : id | berth_num | plate_num | plate_color | status | confidence | eltype | eltime*/
	const char *sql = "create table alarmreget(id integer primary key,\
                       category integer,\
                       level interger,\
                       time varchar(32),\
                       uuid varchar(128));";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if (ret != SQLITE_OK) {
		DEBUG("%s", perrmsg);
	}

	return -ret;
}

//Drop park zehin table
int spzehin_drop_table(sqlite3 *db, char *table_name)
{
	char *perrmsg = NULL;
	char sql[128] = {0};

	/*drop the table*/
	sprintf(sql, "drop table if exists %s", table_name);

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);

	if (ret != SQLITE_OK) {
		DEBUG("%s", perrmsg);
	}

	return -ret;
}


//check the table
int spzehin_check_table(sqlite3 *asql_db, char *ac_tablename,
        char ac_column_name[][32], int ai_column_num)
{
	char **result;
	char *errmsg;
	char sql[128] = {0};
	int nrows = 0;
	int ncols = 0;
	int li_i = 0;

	sprintf(sql, "select * from %s", ac_tablename);

	int ret = sqlite3_get_table(asql_db, sql, &result, &nrows, &ncols, &errmsg);

	if (ret == SQLITE_OK) {
		if (ai_column_num != ncols) {
			DEBUG("%s table: ai_column_num=%d, ncols=%d",
                    ac_tablename, ai_column_num, ncols);

			return -1;
		}

		for (li_i=0; li_i<ncols; li_i++) {
			DEBUG("-------column_name[%d]=%s -------", li_i, result[li_i]);
		}
	}

	return 0;
}

//Create park zehin picture reget table
int spzehin_create_picreget_table(sqlite3 *asql_db)
{
	char *perrmsg = NULL;
	const char *sql = "create table picreget(id integer primary key,\
                       picture_path varchar(128),\
                       picture_name varchar(64),\
                       picture_size varchar(12),\
                       msgrecord_id varchar(32),\
                       msgtable_id integer,\
                       picture_serial integer);";

   	int ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return -ret;
}

//insert park zehin message table data
int spzehin_insert_msgreget_table(sqlite3 *asql_db,
        str_spzehin_msgreget_table as_spzehin_msgreget_table)
{
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into msgreget values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, as_spzehin_msgreget_table.berth_num,
            strlen(as_spzehin_msgreget_table.berth_num), NULL);
	sqlite3_bind_text(stmt, 3, as_spzehin_msgreget_table.plate_num,
            strlen(as_spzehin_msgreget_table.plate_num), NULL);
	sqlite3_bind_text(stmt, 4, as_spzehin_msgreget_table.plate_color,
            strlen(as_spzehin_msgreget_table.plate_color), NULL);
	sqlite3_bind_text(stmt, 5, as_spzehin_msgreget_table.status,
            strlen(as_spzehin_msgreget_table.status), NULL);
	sqlite3_bind_text(stmt, 6, as_spzehin_msgreget_table.confidence,
            strlen(as_spzehin_msgreget_table.confidence), NULL);
	sqlite3_bind_text(stmt, 7, as_spzehin_msgreget_table.eltype,
            strlen(as_spzehin_msgreget_table.eltype), NULL);
	sqlite3_bind_text(stmt, 8, as_spzehin_msgreget_table.eltime,
            strlen(as_spzehin_msgreget_table.eltime), NULL);
	sqlite3_bind_text(stmt, 9, as_spzehin_msgreget_table.plate_position,
            strlen(as_spzehin_msgreget_table.plate_position), NULL);
	sqlite3_bind_text(stmt, 10, as_spzehin_msgreget_table.uuid,
            strlen(as_spzehin_msgreget_table.uuid), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return -ret;
}

//insert park zehin alarm table data
int spzehin_insert_alarmreget_table(sqlite3 *db,
        str_spzehin_alarmreget_table alarm)
{
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into alarmreget values(?, ?, ?, ?, ?);");

	//sqlite3 prepare
	ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -ret;
	}

	//sqlite3 bind *
	sqlite3_bind_int (stmt, 2, alarm.category);
	sqlite3_bind_int (stmt, 3, alarm.level);
	sqlite3_bind_text(stmt, 4, alarm.time, strlen(alarm.time), NULL);
	sqlite3_bind_text(stmt, 5, alarm.uuid, strlen(alarm.uuid), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return -ret;
}

//insert park zehin picture table data
int spzehin_insert_picreget_table(sqlite3 *asql_db,
        str_spzehin_picreget_table as_spzehin_picreget_table)
{
	char *perrmsg = NULL;
	char sql[256] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "insert into picreget(id, picture_path, picture_name,\
        picture_size, msgrecord_id, msgtable_id, picture_serial)\
        values(?, ?, ?, ?, ?, ?, ?);");

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, as_spzehin_picreget_table.pic_path,
            strlen(as_spzehin_picreget_table.pic_path), NULL);
	sqlite3_bind_text(stmt, 3, as_spzehin_picreget_table.pic_name,
            strlen(as_spzehin_picreget_table.pic_name), NULL);
	sqlite3_bind_text(stmt, 4, as_spzehin_picreget_table.pic_size,
            strlen(as_spzehin_picreget_table.pic_size), NULL);
	sqlite3_bind_text(stmt, 5, as_spzehin_picreget_table.msgrecord_id,
            strlen(as_spzehin_picreget_table.msgrecord_id), NULL);
	sqlite3_bind_int(stmt, 6, as_spzehin_picreget_table.msgtable_id);
	sqlite3_bind_int(stmt, 7, as_spzehin_picreget_table.pic_serial);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE)
	{
		sqlite3_finalize(stmt);

		DEBUG("sqlite3_step failed");
  	}

	return 0;
}

//update park zehin picture table data
int spzehin_update_picreget_talbe(sqlite3 *asql_db, int ai_msgtableid,
        char *ac_msgrecordid)
{
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"update picreget set msgrecord_id=%s where msgtable_id=%d;",
            ac_msgrecordid, ai_msgtableid);

   	int ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return -ret;
}

//select park zehin message table data
int spzehin_select_msgreget_table(sqlite3 *asql_db,
        str_spzehin_msgreget_table *as_spzehin_msgreget_table,
        enum SORT_TYPE type)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

    switch (type) {
    case SORT_ASC:
        sprintf(sql, "select * from msgreget order by id limit 1;");
        break;
    case SORT_DESC:
        sprintf(sql, "select * from msgreget order by id DESC limit 1;");
        break;
    default:
        sprintf(sql, "select * from msgreget order by id limit 1;");
        break;
    }

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
  		strcpy(as_spzehin_msgreget_table->berth_num,
                (char *)sqlite3_column_text(stmt, 1));
		strcpy(as_spzehin_msgreget_table->plate_num,
                (char *)sqlite3_column_text(stmt, 2));
		strcpy(as_spzehin_msgreget_table->plate_color,
                (char *)sqlite3_column_text(stmt, 3));
		strcpy(as_spzehin_msgreget_table->status,
                (char *)sqlite3_column_text(stmt, 4));
		strcpy(as_spzehin_msgreget_table->confidence,
                (char *)sqlite3_column_text(stmt, 5));
		strcpy(as_spzehin_msgreget_table->eltype,
                (char *)sqlite3_column_text(stmt, 6));
		strcpy(as_spzehin_msgreget_table->eltime,
                (char *)sqlite3_column_text(stmt, 7));
		strcpy(as_spzehin_msgreget_table->plate_position,
                (char *)sqlite3_column_text(stmt, 8));
		strcpy(as_spzehin_msgreget_table->uuid,
                (char *)sqlite3_column_text(stmt, 9));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}

//select park zehin alarm table data
int spzehin_select_alarmreget_table(sqlite3 *db,
        str_spzehin_alarmreget_table *alarm_tb)
{
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;

	sprintf(sql, "select * from alarmreget order by id DESC limit 1;");

	//sqlite3 prepare
	ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -ret;
	}

	//sqlite3 step
	sqlite3_column_count(stmt);
	int rc = sqlite3_step(stmt);

	while(rc == SQLITE_ROW)
	{
		ret = sqlite3_column_int(stmt, 0);
        alarm_tb->category = sqlite3_column_int(stmt, 1);
        alarm_tb->level = sqlite3_column_int(stmt, 2);
		strcpy(alarm_tb->time, (char *)sqlite3_column_text(stmt, 3));
		strcpy(alarm_tb->uuid, (char *)sqlite3_column_text(stmt, 4));
		rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return ret;
}

/**
 * @brief select item from picreget
 *
 * @param asql_db sql
 * @param pic_tb pic_tb
 * @param type 0 select the picture whose message already send successfully;
 *             1 select one picture in any case.
 *
 * @return return value of sqlite
 */
int spzehin_select_picreget_table(sqlite3 *asql_db,
        str_spzehin_picreget_table *pic_tb, const unsigned char type)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

    if (type == 0)
        sprintf(sql, "select * from picreget where msgrecord_id>0 limit 1;");
    else if (type == 1)
        sprintf(sql, "select * from picreget limit 1;");

	//sqlite3 prepare
	li_ret = sqlite3_prepare_v2(asql_db, sql, strlen(sql), &stmt, NULL);
	if(li_ret != SQLITE_OK)
	{
		if(stmt) sqlite3_finalize(stmt);

		DEBUG("%s", perrmsg);

		return -li_ret;
	}

	//sqlite3 step
	li_ncols = sqlite3_column_count(stmt);
	li_rc = sqlite3_step(stmt);

	while(li_rc == SQLITE_ROW)
	{
		li_ret = sqlite3_column_int(stmt, 0);
  		strcpy(pic_tb->pic_path, (char *)sqlite3_column_text(stmt, 1));
  		strcpy(pic_tb->pic_name, (char *)sqlite3_column_text(stmt, 2));
  		strcpy(pic_tb->pic_size, (char *)sqlite3_column_text(stmt, 3));
		strcpy(pic_tb->msgrecord_id, (char *)sqlite3_column_text(stmt, 4));
		pic_tb->pic_serial = sqlite3_column_int(stmt, 6);
		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return li_ret;
}

//delete park zehin message records by id
int spzehin_delete_msgreget_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from msgreget where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return -li_ret;
}

//delete park zehin alarm records by id
int spzehin_delete_alarmreget_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from alarmreget where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return -li_ret;
}

//delete park zehin picture records by id
int spzehin_delete_picreget_table(sqlite3 *asql_db, int ai_id)
{
	int li_ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from picreget where id=%d;", ai_id);

   	li_ret = sqlite3_exec(asql_db, sql, NULL, 0, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return -li_ret;
}

//count park zehin message records
static int spzehin_count_msgreget_callback(void *data, int ncols,
        char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spzehin_count_msgreget_table(sqlite3 *asql_db)
{
	if (NULL == asql_db) {
		ERROR("Null is param");
		return 0;
	}

	int li_ret = 0;
	int li_nrow;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from msgreget;");

   	li_ret = sqlite3_exec(asql_db, sql, spzehin_count_msgreget_callback,
            (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_nrow;
}

//count park zehin alarm records
int spzehin_count_alarmreget_callback(void *data, int ncols,
        char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spzehin_count_alarmreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	int li_nrow = 0;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from alarmreget;");

   	li_ret = sqlite3_exec(asql_db, sql, spzehin_count_alarmreget_callback,
            (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_nrow;
}

//count park zehin picture records
int spzehin_count_picreget_callback(void *data, int ncols,
        char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int spzehin_count_picreget_table(sqlite3 *asql_db)
{
	int li_ret = 0;
	int li_nrow = 0;
	char *perrmsg;
	char sql[128] = {0};

	sprintf(sql,"select count(*) from picreget;");

   	li_ret = sqlite3_exec(asql_db, sql, spzehin_count_picreget_callback,
            (void *)&li_nrow, &perrmsg);
	if(li_ret != SQLITE_OK)
	{
		DEBUG("%s", perrmsg);
	}

	return li_nrow;
}

