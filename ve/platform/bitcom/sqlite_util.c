#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "commonfuncs.h"
#include "sqlite_util.h"
#include "logger/log.h"

C_CODE_BEGIN


#define __E(fmt, ...)		ERROR((fmt), ##__VA_ARGS__)


int32_t sqlite_open(sqlite_t *db, const char *path)
{
	int32_t ret;

	ret = sqlite3_open(path, &db->db);
	if(SQLITE_OK != ret){
		__E("%s!", sqlite3_errmsg(db->db));
	}

	return ret;
}

int32_t sqlite_close(sqlite_t *db)
{
	int32_t ret;

	ret = sqlite3_close(db->db);
	if(SQLITE_OK != ret){
		__E("%s!", sqlite3_errmsg(db->db));
	}

	return ret;
}


int32_t sqlite_create_tab(sqlite_t *db, const char *tab, const char *field)
{
	int32_t ret;
	char sql[1024];

	snprintf(sql, sizeof(sql), "create table %s(%s);", tab, field);

	ret = sqlite3_exec(db->db, sql, NULL, 0, NULL);
	if (SQLITE_OK != ret) {
		__E("%s", sqlite3_errmsg(db->db));
	}

	return ret;
}

sqlite3_stmt *sqlite_prepare(sqlite_t *db, const char *sql)
{
	sqlite3_stmt *stmt;
	int32_t ret;

	ret = sqlite3_prepare_v2(db->db, sql, strlen(sql), &stmt, NULL);
	if(SQLITE_OK != ret){
		if(stmt){
			sqlite3_finalize(stmt);
		}

		__E("%s", sqlite3_errmsg(db->db));

		return NULL;
	}

	return stmt;
}

int32_t sqlite_step(sqlite_t *db, sqlite3_stmt *stmt)
{
	return sqlite3_step(stmt);
}

int32_t sqlite_finalize(sqlite_t *sqlite, sqlite3_stmt *stmt)
{
	int32_t ret;

	ret = sqlite3_finalize(stmt);
	if(SQLITE_OK != ret){
		__E("%s.", sqlite3_errmsg(sqlite->db));
	}

	return ret;
}

int32_t sqlite_exec(sqlite_t *db, const char *sql)
{
	int32_t ret;

   	ret = sqlite3_exec(db->db, sql, NULL, 0, NULL);
	if(SQLITE_OK != ret) {
		__E("%s", sqlite3_errmsg(db->db));
	}

	return ret;
}


static int32_t sqlite3_get_cnt_callback(void *arg, int32_t nr_col,
										char *val[], char *hdr[])
{
	*((int32_t *)arg) = atoi(val[0]);
	return SQLITE_OK;
}

int32_t sqlite_get_cnt(sqlite_t *db, const char *tab)
{
	int32_t ret;
	int32_t cnt;
	char sql[128];

	snprintf(sql, sizeof(sql), "select count(*) from %s;", tab);

	ret = sqlite3_exec(db->db, sql, sqlite3_get_cnt_callback, &cnt, NULL);
	if(SQLITE_OK != ret) {
		__E("%s", sqlite3_errmsg(db->db));
		return -1;
	}

	return cnt;
}

int32_t sqlite_del_id(sqlite_t *sqlite, const char *tab, int32_t id)
{
	int ret;
	char sql[128];

	snprintf(sql, sizeof(sql), "delete from %s where id=%d;", tab, id);

	ret = sqlite3_exec(sqlite->db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != ret) {
		__E("%s", sqlite3_errmsg(sqlite->db));
	}

	return ret;
}

int32_t sqlite_del_field(sqlite_t *sqlite, const char *tab,
						 const char *field, const char *value)
{
	int ret;
	char sql[128];

	snprintf(sql, sizeof(sql),
			 "delete from %s where '%s'='%s';",
			 tab, field, value);

	ret = sqlite3_exec(sqlite->db, sql, NULL, NULL, NULL);
	if(SQLITE_OK != ret) {
		__E("%s", sqlite3_errmsg(sqlite->db));
	}

	return ret;
}

C_CODE_END
