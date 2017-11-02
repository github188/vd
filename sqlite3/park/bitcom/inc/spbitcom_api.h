#ifndef _SPBITCOM_API_H_
#define _SPBITCOM_API_H_

#define BUFFERSIZE_16    (16)
#define BUFFERSIZE_32    (32)
#define BUFFERSIZE_64    (64)
#define BUFFERSIZE_512   (512)
#define BUFFERSIZE_1024  (1024)


/****************************typedef struct*********************************/
typedef struct
{
	int id;
	char pic_path[BUFFERSIZE_512];
	char pic_name[BUFFERSIZE_64];
	char pic_size[BUFFERSIZE_16];
	int  pic_regetflag;
}str_spbitcom_picreget_table;


typedef struct
{
	int id;
	char message[BUFFERSIZE_1024];
}str_spbitcom_msgreget_table;


/****************************extern varible*********************************/
extern sqlite3 *spbitcom_db;


/****************************extern function*********************************/
extern int spbitcom_open(const char *ac_path);
extern int spbitcom_create_msgreget_table(sqlite3 *asql_db);
extern int spbitcom_create_picreget_table(sqlite3 *asql_db);
extern int spbitcom_insert_msgreget_table(sqlite3 *asql_db, str_spbitcom_msgreget_table as_spbitcom_msgreget_table);
extern int spbitcom_insert_picreget_table(sqlite3 *asql_db, str_spbitcom_picreget_table as_spbitcom_picreget_table);
extern int spbitcom_select_msgreget_table(sqlite3 *asql_db, char *ac_message);
extern int spbitcom_select_picreget_table(sqlite3 *asql_db, str_spbitcom_picreget_table *as_spbitcom_picreget_table);
extern int spbitcom_delete_msgreget_table(sqlite3 *asql_db, int ai_id);
extern int spbitcom_delete_picreget_table(sqlite3 *asql_db, int ai_id);
extern int spbitcom_count_msgreget_table(sqlite3 *asql_db);
extern int spbitcom_count_picreget_table(sqlite3 *asql_db);


#endif

