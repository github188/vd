#include "ftp_resend_db.h"
#include "sqlite_util.h"
#include "commonfuncs.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


C_CODE_BEGIN


int32_t sabitcom_ftp_resend_tab_create(sqlite_t *sqlite)
{
	char field[256];

	snprintf(field, sizeof(field),
			 "id integer primary key,"
			 "name varchar(%d),"
			 "path varchar(%d),"
			 "size integer",
			 SABITCOM_FTP_NAME_SIZE,
			 SABITCOM_FTP_PATH_SIZE);

	return sqlite_create_tab(sqlite, SABITCOM_FTP_RESEND_TAB, field);
}

int32_t sabitcom_ftp_resend_insert(sqlite_t *sqlite,
								   const sabitcom_ftp_resend_t *item)
{
	char sql[128];
	sqlite3_stmt *stmt = NULL;

	TRACE_LOG_SYSTEM("name: %s", item->name);
	TRACE_LOG_SYSTEM("path: %s", item->path);
	TRACE_LOG_SYSTEM("size: %d", item->size);

	snprintf(sql, sizeof(sql),
			"insert into %s values(?, ?, ?, ?);",
			SABITCOM_FTP_RESEND_TAB);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	sqlite3_bind_text(stmt, 2, item->name, strlen(item->name), NULL);
	sqlite3_bind_text(stmt, 3, item->path, strlen(item->path), NULL);
	sqlite3_bind_int(stmt, 4, item->size);

	sqlite_step(sqlite, stmt);
	return sqlite_finalize(sqlite, stmt);
}

int32_t sabitcom_ftp_resend_select(sqlite_t *sqlite,
								   sabitcom_ftp_resend_t *item)
{
	char sql[128];
	sqlite3_stmt *stmt;
	int32_t col, next;
	int32_t id = -1;

	snprintf(sql, sizeof(sql),
			 "select * from %s order by id DESC limit 1;",
			SABITCOM_FTP_RESEND_TAB);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	col = sqlite3_column_count(stmt);
	next = sqlite_step(sqlite, stmt);

	while(SQLITE_ROW == next){
		id = sqlite3_column_int(stmt, 0);
		strncpy(item->name, (char *)sqlite3_column_text(stmt, 1),
				sizeof(item->name));
		strncpy(item->path, (char *)sqlite3_column_text(stmt, 2),
				sizeof(item->path));
		item->size = sqlite3_column_int(stmt, 3);

		next = sqlite_step(sqlite, stmt);
	}

	sqlite_finalize(sqlite, stmt);

	return id;
}

int32_t sabitcom_ftp_resend_get_cnt(sqlite_t *sqlite)
{
	return sqlite_get_cnt(sqlite, SABITCOM_FTP_RESEND_TAB);
}

int32_t sabitcom_ftp_resend_del_id(sqlite_t *sqlite, int32_t id)
{
	return sqlite_del_id(sqlite, SABITCOM_FTP_RESEND_TAB, id);
}

int32_t sabitcom_ftp_resend_del_redundant(sqlite_t *sqlite, int32_t nr_resv,
										  sabitcom_ftp_name_t *name,
										  int32_t nr_name_max)
{
	char sql[128];
	sqlite3_stmt *stmt;
	int32_t col, next, limit, id;
	int32_t cnt = 0;

	limit = sabitcom_ftp_resend_get_cnt(sqlite) - nr_resv + 1;
	if (limit <= 0) {
		return -1;
	}

	snprintf(sql, sizeof(sql),
			 "select id,name from %s order by id asc limit %d;",
			 SABITCOM_FTP_RESEND_TAB, limit);

	stmt = sqlite_prepare(sqlite, sql);
	if (!stmt) {
		return -1;
	}

	col = sqlite3_column_count(stmt);
	next = sqlite_step(sqlite, stmt);

	while((SQLITE_ROW == next) && (cnt < nr_name_max)){
		/* 读记录 */
		id = sqlite3_column_int(stmt, 0);

		strncpy((char *)name, (char *)sqlite3_column_text(stmt, 1),
				sizeof(sabitcom_ftp_name_t));

		/* 删除记录 */
		sabitcom_ftp_resend_del_id(sqlite, id);
		TRACE_LOG_SYSTEM("del ftp record: %d.", id);

		++cnt;
		++name;

		/* 读下一个 */
		next = sqlite_step(sqlite, stmt);
	}

	sqlite_finalize(sqlite, stmt);

	return cnt;
}



C_CODE_END
