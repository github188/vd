#ifndef __SQLITE_UTIL_H
#define __SQLITE_UTIL_H

#include "types.h"
#include "sqlite3.h"

C_CODE_BEGIN


typedef struct sqlite{
	sqlite3 *db;
}sqlite_t;

int32_t sqlite_open(sqlite_t *db, const char *path);
int32_t sqlite_close(sqlite_t *db);
int32_t sqlite_create_tab(sqlite_t *db, const char *tab, const char *field);
sqlite3_stmt *sqlite_prepare(sqlite_t *db, const char *sql);
int32_t sqlite_step(sqlite_t *db, sqlite3_stmt *stmt);
int32_t sqlite_del_id(sqlite_t *db, const char *tab, int32_t id);
int32_t sqlite_del_field(sqlite_t *sqlite, const char *tab,
						 const char *field, const char *value);
int32_t sqlite_get_cnt(sqlite_t *db, const char *tab);
int32_t sqlite_finalize(sqlite_t *sqlite, sqlite3_stmt *stmt);


C_CODE_END

#endif

