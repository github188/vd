#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mq_resend_db.h"
#include "sqlite_util.h"
#include "commonfuncs.h"

C_CODE_BEGIN



int32_t sabitcom_mq_resend_tab_create(sqlite_t *sqlite)
{
	char field[128];

	snprintf(field, sizeof(field),
			 "id integer primary key, text varchar(%d)",
			 SABITCOM_MQ_TEXT_SIZE);

	return sqlite_create_tab(sqlite, SABITCOM_MQ_RESEND_TAB, field);
}

int32_t sabitcom_mq_resend_insert(sqlite_t *sqlite,
								  const sabitcom_mq_resend_t *item)
{
	char sql[128];
	sqlite3_stmt *stmt = NULL;

	TRACE_LOG_SYSTEM("text: %s", item->text);

	sprintf(sql, "insert into %s values(?, ?);", SABITCOM_MQ_RESEND_TAB);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	sqlite3_bind_text(stmt, 2, item->text, strlen(item->text), NULL);

	sqlite_step(sqlite, stmt);
	sqlite_finalize(sqlite, stmt);

	return 0;
}

int32_t sabitcom_mq_resend_select(sqlite_t *sqlite,
								  sabitcom_mq_resend_t *item)
{
	char sql[128];
	sqlite3_stmt *stmt;
	int32_t col, next;
	int32_t id = -1;

	snprintf(sql, sizeof(sql),
			 "select * from %s order by id DESC limit 1;",
			SABITCOM_MQ_RESEND_TAB);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	col = sqlite3_column_count(stmt);
	next = sqlite_step(sqlite, stmt);

	while(SQLITE_ROW == next){
		id = sqlite3_column_int(stmt, 0);
		strncpy(item->text, (char *)sqlite3_column_text(stmt, 1),
				sizeof(item->text));
		next = sqlite_step(sqlite, stmt);
	}

	sqlite_finalize(sqlite, stmt);

	return id;
}

int32_t sabitcom_mq_resend_get_cnt(sqlite_t *sqlite)
{
	return sqlite_get_cnt(sqlite, SABITCOM_MQ_RESEND_TAB);
}

int32_t sabitcom_mq_resend_del_id(sqlite_t *sqlite, int32_t id)
{
	return sqlite_del_id(sqlite, SABITCOM_MQ_RESEND_TAB, id);
}

int32_t sabitcom_mq_resend_del_redundant(sqlite_t *sqlite, int32_t resv)
{
	char sql[128];
	sqlite3_stmt *stmt;
	int32_t col, next;
	int32_t limit;
	int32_t id;

	limit = sabitcom_mq_resend_get_cnt(sqlite) - resv + 1;
	if (limit <= 0) {
		return -1;
	}

	snprintf(sql, sizeof(sql),
			 "select id from %s order by id asc limit %d;",
			 SABITCOM_MQ_RESEND_TAB, limit);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	col = sqlite3_column_count(stmt);
	next = sqlite_step(sqlite, stmt);

	while(SQLITE_ROW == next){
		id = sqlite3_column_int(stmt, 0);
		sabitcom_mq_resend_del_id(sqlite, id);
		TRACE_LOG_SYSTEM("del redundant mq record: %d.", id);
		next = sqlite_step(sqlite, stmt);
	}

	sqlite_finalize(sqlite, stmt);

	return 0;
}


C_CODE_END
