#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "park_ktetc_db_api.h"
#ifndef FELIXDU_TEST
#include "logger/log.h"
#endif
#include "park_status.h"

sqlite3 *park_ktetc_db = NULL;

static int park_ktetc_db_create_record_tb(void);
static int park_ktetc_db_create_alarm_tb(void);
static int park_ktetc_db_create_retrans_tb(void);

int park_ktetc_db_open(const char *path)
{
	int ret = sqlite3_open(path, &park_ktetc_db);
	if(ret != SQLITE_OK)
		ERROR("Create park database failed:%s!", sqlite3_errmsg(park_ktetc_db));

	return ret;
}

int park_ktetc_db_close(void)
{
	int ret = sqlite3_close(park_ktetc_db);
	if(ret != SQLITE_OK)
		ERROR("Close park database failed:%s!", sqlite3_errmsg(park_ktetc_db));

	return ret;
}

int park_ktetc_db_create_tb(void)
{
	park_ktetc_db_create_record_tb();
	park_ktetc_db_create_alarm_tb();
	park_ktetc_db_create_retrans_tb();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Record
////////////////////////////////////////////////////////////////////////////////
int park_ktetc_db_create_record_tb(void)
{
    sqlite3 *db = park_ktetc_db;
	char *perrmsg = NULL;

    //const char *sql = "create table if not exists ktetc(id integer primary key,"
	//	"comType varchar(32), IDdataTime varchar(32), flowId varchar(32),"
	//	" parkCode varchar(10), devCode varchar(7), psCode varchar(32),"
	//	" inOutState integer, vehPlate varchar(32), confidence integer,"
	//	" plateColor varchar(16), vehColor varchar(16), vehType interger,"
	//	" Image1 varchar(256), Image2 varchar(256), Image3 varchar(256),"
	//	" Image4 varchar(256), FiledataTime varchar(32), inTime varchar(32));";

    const char *sql = "create table if not exists ktetc("
		"comType varchar(32), IDdataTime varchar(32), flowId varchar(32),"
		" parkCode varchar(10), devCode varchar(7), psCode varchar(32),"
		" inOutState integer, vehPlate varchar(32), confidence integer,"
		" plateColor varchar(16), vehColor varchar(16), vehType interger,"
		" Image1 varchar(256), Image2 varchar(256), Image3 varchar(256),"
		" Image4 varchar(256), FiledataTime varchar(32), inTime varchar(32));";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int park_ktetc_db_insert_record(const ktetc_t *k)
{
    sqlite3 *db = park_ktetc_db;
	sqlite3_stmt *stmt = NULL;

	//const char *sql = "insert into ktetc values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	const char *sql = "insert into ktetc values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 bind *
	//sqlite3_bind_text(stmt, 2, k->comType.c_str(), k->comType.size(), NULL);
	//sqlite3_bind_text(stmt, 3, k->IDdataTime.c_str(), k->IDdataTime.size(), NULL);
	//sqlite3_bind_text(stmt, 4, k->flowId.c_str(), k->flowId.size(), NULL);
	//sqlite3_bind_text(stmt, 5, k->parkCode.c_str(), k->parkCode.size(), NULL);
	//sqlite3_bind_text(stmt, 6, k->devCode.c_str(), k->devCode.size(), NULL);
	//sqlite3_bind_text(stmt, 7, k->psCode.c_str(), k->psCode.size(), NULL);
	//sqlite3_bind_int(stmt, 8, k->inOutState);
	//sqlite3_bind_text(stmt, 9, k->vehPlate.c_str(), k->vehPlate.size(), NULL);
	//sqlite3_bind_int(stmt, 10, k->confidence);
	//sqlite3_bind_text(stmt, 11, k->plateColor.c_str(), k->plateColor.size(), NULL);
	//sqlite3_bind_text(stmt, 12, k->vehColor.c_str(), k->vehColor.size(), NULL);
	//sqlite3_bind_int(stmt, 13, k->vehType);
	//sqlite3_bind_text(stmt, 14, k->Image1.c_str(), k->Image1.size(), NULL);
	//sqlite3_bind_text(stmt, 15, k->Image2.c_str(), k->Image2.size(), NULL);
	//sqlite3_bind_text(stmt, 16, k->Image3.c_str(), k->Image3.size(), NULL);
	//sqlite3_bind_text(stmt, 17, k->Image4.c_str(), k->Image4.size(), NULL);
	//sqlite3_bind_text(stmt, 18, k->FiledataTime.c_str(), k->FiledataTime.size(), NULL);
	//sqlite3_bind_text(stmt, 19, k->inTime.c_str(), k->inTime.size(), NULL);
	sqlite3_bind_text(stmt, 1, k->comType.c_str(), k->comType.size(), NULL);
	sqlite3_bind_text(stmt, 2, k->IDdataTime.c_str(), k->IDdataTime.size(), NULL);
	sqlite3_bind_text(stmt, 3, k->flowId.c_str(), k->flowId.size(), NULL);
	sqlite3_bind_text(stmt, 4, k->parkCode.c_str(), k->parkCode.size(), NULL);
	sqlite3_bind_text(stmt, 5, k->devCode.c_str(), k->devCode.size(), NULL);
	sqlite3_bind_text(stmt, 6, k->psCode.c_str(), k->psCode.size(), NULL);
	sqlite3_bind_int(stmt, 7, k->inOutState);
	sqlite3_bind_text(stmt, 8, k->vehPlate.c_str(), k->vehPlate.size(), NULL);
	sqlite3_bind_int(stmt, 9, k->confidence);
	sqlite3_bind_text(stmt, 10, k->plateColor.c_str(), k->plateColor.size(), NULL);
	sqlite3_bind_text(stmt, 11, k->vehColor.c_str(), k->vehColor.size(), NULL);
	sqlite3_bind_int(stmt, 12, k->vehType);
	sqlite3_bind_text(stmt, 13, k->Image1.c_str(), k->Image1.size(), NULL);
	sqlite3_bind_text(stmt, 14, k->Image2.c_str(), k->Image2.size(), NULL);
	sqlite3_bind_text(stmt, 15, k->Image3.c_str(), k->Image3.size(), NULL);
	sqlite3_bind_text(stmt, 16, k->Image4.c_str(), k->Image4.size(), NULL);
	sqlite3_bind_text(stmt, 17, k->FiledataTime.c_str(), k->FiledataTime.size(), NULL);
	sqlite3_bind_text(stmt, 18, k->inTime.c_str(), k->inTime.size(), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return ret;
}

int park_ktetc_db_select_record(const char* from_time,
		const char* end_time, callback cb)
{
    sqlite3 *db = park_ktetc_db;
    int ret = 0;
    char* errmsg = NULL;
    char sql[128] = {0};
    const char *data = "Callback function called.";

    if (from_time == NULL && end_time == NULL)
        strcpy(sql, "select * from ktetc;");
    else if (from_time == NULL)
        sprintf(sql, "select * from ktetc where datetime(FiledataTime) < '%s' ;", end_time);
    else if (end_time == NULL)
        sprintf(sql, "select * from ktetc where datetime(FiledataTime) > '%s' ;", from_time);
    else
        sprintf(sql, "select * from ktetc where datetime(FiledataTime) > '%s' and datetime(FiledataTime) < '%s' ;", from_time, end_time);

    //std::cout << sql << std::endl;
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
int park_ktetc_db_delete_record(const char *id)
{
    sqlite3 *db = park_ktetc_db;
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from ktetc where flowId=%s;", id);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return ret;
}

//delete park bitcom message records before time
int park_ktetc_db_delete_record_old(const char * time)
{
    sqlite3 *db = park_ktetc_db;
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from ktetc where datetime(alarmTime) < '%s';", time);
    DEBUG(sql);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return ret;
}

//count park bitcom message records
int park_ktetc_db_count_record_callback(void *data, int ncols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int park_ktetc_db_count_record()
{
    sqlite3 *db = park_ktetc_db;
	int ret = 0;
	int li_nrow;
	char *perrmsg;

	const char*sql = "select count(*) from ktetc;";

   	ret = sqlite3_exec(db, sql, park_ktetc_db_count_record_callback, (void *)&li_nrow, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return li_nrow;
}

////////////////////////////////////////////////////////////////////////////////
// Retrans
////////////////////////////////////////////////////////////////////////////////
int park_ktetc_db_create_retrans_tb(void)
{
    sqlite3 *db = park_ktetc_db;
	char *perrmsg = NULL;

    const char *sql = "create table if not exists retrans(id integer primary key,"
		"message varchar(1024));";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int park_ktetc_db_insert_retrans(const char *msg)
{
	if (msg == NULL) {
		ERROR("msg is NULL.");
		return -1;
	}

    sqlite3 *db = park_ktetc_db;
	sqlite3_stmt *stmt = NULL;

	const char *sql = "insert into retrans values(?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	sqlite3_bind_text(stmt, 2, msg, strlen(msg), NULL);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return ret;
}

int park_ktetc_db_select_retrans(char *msg)
{
	int ret = 0;
    sqlite3 *db = park_ktetc_db;
	sqlite3_stmt *stmt = NULL;
	int li_ncols = 0;
	int li_rc = 0;

	const char* sql = "select * from retrans limit 1;";

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
  		strcpy(msg, (char *)sqlite3_column_text(stmt, 1));

		li_rc = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return ret;
}

//delete park bitcom message records by id
int park_ktetc_db_delete_retrans(int id)
{
    sqlite3 *db = park_ktetc_db;
	int ret = 0;
	char *perrmsg = NULL;
	char sql[128] = {0};

	sprintf(sql,"delete from retrans where id=%d;", id);

   	ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return ret;
}

static int count_retrans_cb(void *data, int cols, char **values, char **headers)
{
	*(int *)data = atoi(values[0]);

	return 0;
}

int park_ktetc_db_count_retrans()
{
    sqlite3 *db = park_ktetc_db;
	int ret = 0;
	int row;
	char *perrmsg = NULL;

	const char *sql = "select count(*) from retrans;";

   	ret = sqlite3_exec(db, sql, count_retrans_cb, (void *)&row, &perrmsg);
	if(ret != SQLITE_OK) {
		ERROR("%s", perrmsg);
	}

	return row;
}

////////////////////////////////////////////////////////////////////////////////
// Alarm
////////////////////////////////////////////////////////////////////////////////
int park_ktetc_db_create_alarm_tb(void)
{
    sqlite3 *db = park_ktetc_db;
	char *perrmsg = NULL;

    //const char *sql = "create table if not exists alarm(id integer primary key,"
	//	" comType varchar(32), flowId varchar(32), parkCode varchar(10),"
	//	" devCode varchar(7), psCode varchar(32),"
	//	" alarmCode integer, alarmTime varchar(32), alarmLevel integer);";
    const char *sql = "create table if not exists alarm("
		" comType varchar(32), flowId varchar(32), parkCode varchar(10),"
		" devCode varchar(7), psCode varchar(32),"
		" alarmCode integer, alarmTime varchar(32), alarmLevel integer);";

   	int ret = sqlite3_exec(db, sql, NULL, 0, &perrmsg);
	if(ret != SQLITE_OK)
		ERROR("%s", perrmsg);

    sqlite3_free(perrmsg);
	return ret;
}

int park_ktetc_db_insert_alarm(const ktetc_alarm_t *k)
{
    sqlite3 *db = park_ktetc_db;
	sqlite3_stmt *stmt = NULL;

	//const char *sql = "insert into ktetc values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	const char *sql = "insert into alarm values(?, ?, ?, ?, ?, ?, ?, ?);";

	//sqlite3 prepare
	int ret = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(ret != SQLITE_OK) {
		if(stmt)
            sqlite3_finalize(stmt);
		return ret;
	}

	//sqlite3 bind *
	sqlite3_bind_text(stmt, 1, k->comType.c_str(), k->comType.size(), NULL);
	sqlite3_bind_text(stmt, 2, k->flowId.c_str(), k->flowId.size(), NULL);
	sqlite3_bind_text(stmt, 3, k->parkCode.c_str(), k->parkCode.size(), NULL);
	sqlite3_bind_text(stmt, 4, k->devCode.c_str(), k->devCode.size(), NULL);
	sqlite3_bind_text(stmt, 5, k->psCode.c_str(), k->psCode.size(), NULL);
	sqlite3_bind_int(stmt, 6, k->alarmCode);
	sqlite3_bind_text(stmt, 7, k->alarmTime.c_str(), k->alarmTime.size(), NULL);
	sqlite3_bind_int(stmt, 8, k->alarmLevel);

	//sqlite3 step
	ret = sqlite3_step(stmt);
	if(ret != SQLITE_DONE) {
		sqlite3_finalize(stmt);

		ERROR("sqlite3_step failed");
  	}

	return ret;
}

int park_ktetc_db_select_alarm(const char* from_time,
		const char* end_time, callback cb)
{
    sqlite3 *db = park_ktetc_db;
    int ret = 0;
    char* errmsg = NULL;
    char sql[128] = {0};
    const char *data = "Callback function called.";

    if (from_time == NULL && end_time == NULL)
        strcpy(sql, "select * from alarm;");
    else if (from_time == NULL)
        sprintf(sql, "select * from alarm where datetime(alarmTime) < '%s' ;", end_time);
    else if (end_time == NULL)
        sprintf(sql, "select * from alarm where datetime(alarmTime) > '%s' ;", from_time);
    else
        sprintf(sql, "select * from alarm where datetime(alarmTime) > '%s' and datetime(alarmTime) < '%s' ;", from_time, end_time);

    //std::cout << sql << std::endl;
    ret = sqlite3_exec(db, sql, cb, (void*)data, &errmsg);
	if(ret != SQLITE_OK)
	{
        ERROR("%s", errmsg);
        sqlite3_free(errmsg);
		return ret;
	}

	return ret;
}

