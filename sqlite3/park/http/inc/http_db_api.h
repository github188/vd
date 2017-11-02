#ifndef _HTTP_DB_API_H_
#define _HTTP_DB_API_H_

#include <sqlite3.h>

#define MSG_LENGTH      (1024)
#define PIC_PATH_LENGTH (512)
#define PIC_NAME_LENGTH (64)

/****************************typedef struct*********************************/
typedef struct
{
	int id;
	char pic_path[PIC_PATH_LENGTH];
	char pic_name[PIC_NAME_LENGTH];
	size_t pic_size;
}http_picreget_tb;

typedef struct
{
	int id;
	char message[MSG_LENGTH];
}http_msgreget_tb;

/****************************extern varible*********************************/
extern sqlite3 *http_db;

/****************************extern function*********************************/
int http_db_open(const char *path);
int http_db_create_msgreget_tb(sqlite3 *db);
int http_db_create_picreget_tb(sqlite3 *db);
int http_db_insert_msgreget_tb(sqlite3 *db, http_msgreget_tb msgreget_tb);
int http_db_insert_picreget_tb(sqlite3 *db, http_picreget_tb picreget_tb);
int http_db_select_msgreget_tb(sqlite3 *db, char *message);
int http_db_select_picreget_tb(sqlite3 *db, http_picreget_tb *picreget_tb);
int http_db_delete_msgreget_tb(sqlite3 *db, int id);
int http_db_delete_picreget_tb(sqlite3 *db, int id);
int http_db_delete_picreget_tb(sqlite3 *db, const char* pic_name);
int http_db_count_msgreget_tb (sqlite3 *db);
int http_db_count_picreget_tb (sqlite3 *db);
#endif
