#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>

#include "park_status_db_api.h"
#ifndef FELIXDU_TEST
#include "logger/log.h"
#endif
#include "park_status.h"

sqlite3 *park_state_db = NULL;

int park_state_db_open(const char *path)
{
	int ret = sqlite3_open(path, &park_state_db);
	if(ret != SQLITE_OK)
		ERROR("Create park database failed:%s!", sqlite3_errmsg(park_state_db));

	return ret;
}

int park_state_db_close(void)
{
	int ret = sqlite3_close(park_state_db);
	if(ret != SQLITE_OK)
		ERROR("Close park database failed:%s!", sqlite3_errmsg(park_state_db));

	return ret;
}

int park_state_db_create_tb(void)
{
    sqlite3 *db = park_state_db;
	char *perrmsg = NULL;

	/*table : id | message*/
    const char *sql = "create table if not exists parkhistory(id integer primary key,"
        "time varchar(32), state integer, plate varchar(32),"
        " pic_name1 varchar(128), pic_name2 varchar(128));";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int park_state_db_insert_parkhistory_tb(park_history_t *his)
{
    sqlite3 *db = park_state_db;
	sqlite3_stmt *stmt = NULL;

	const char *sql = "insert into parkhistory values(?, ?, ?, ?, ?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 2, his->time, strlen(his->time), NULL);
	sqlite3_bind_int (stmt, 3, (int)(his->state));
	sqlite3_bind_text(stmt, 4, his->plate, strlen(his->plate), NULL);
	sqlite3_bind_text(stmt, 5, (his->pic_name)[0], strlen((his->pic_name)[0]), NULL);
	sqlite3_bind_text(stmt, 6, (his->pic_name)[1], strlen((his->pic_name)[1]), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return ret;
}

int park_state_db_select_parkhistory_tb(const char* from_time,
                                        const char* end_time,
                                        callback cb)
{
    sqlite3 *db = park_state_db;
    int ret = 0;
    char* errmsg = NULL;
    char sql[128] = {0};
    const char* data = "Callback function called.";

    if (from_time == NULL && end_time == NULL)
        strcpy(sql, "select * from parkhistory;");
    else if (from_time == NULL)
        sprintf(sql, "select * from parkhistory where datetime(time) < '%s' ;", end_time);
    else if (end_time == NULL)
        sprintf(sql, "select * from parkhistory where datetime(time) > '%s' ;", from_time);
    else
        sprintf(sql, "select * from parkhistory where datetime(time) > '%s' and datetime(time) < '%s' ;", from_time, end_time);

    std::cout << sql << std::endl;
    ret = sqlite3_exec(db, sql, cb, (void*)data, &errmsg);
	if(ret != SQLITE_OK)
	{
        ERROR("%s", errmsg);
        sqlite3_free(errmsg);
		return ret;
	}

	return ret;
}

//delete park bitcom message records by id
int park_state_db_delete_parkhistory(int id)
{
    sqlite3 *db = park_state_db;
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from parkhistory where id=%d;", id);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return ret;
}

//delete park bitcom message records before time
int park_state_db_delete_parkhistory_old(const char * time)
{
    sqlite3 *db = park_state_db;
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from parkhistory where datetime(time) < '%s';", time);
    DEBUG(sql);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return ret;
}

//count park bitcom message records
int park_state_db_count_parkhistory_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int park_state_db_count_parkhistory_tb()
{
    sqlite3 *db = park_state_db;
	int ret = 0;
	int li_nrow;
	char *perrmsg;

	const char*sql = "select count(*) from parkhistory;";

   	ret = sqlite3_exec(db, sql, park_state_db_count_parkhistory_callback, (void *)&li_nrow, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return li_nrow;
}
