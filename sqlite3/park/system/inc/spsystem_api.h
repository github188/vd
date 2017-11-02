#ifndef _SPSYSTEM_API_H_
#define _SPSYSTEM_API_H_

#define BUFFERSIZE_16    (16)
#define BUFFERSIZE_32    (32)
#define BUFFERSIZE_64    (64)
#define BUFFERSIZE_512   (512)
#define BUFFERSIZE_1024  (1024)


/****************************typedef struct*********************************/

enum
{
	PARK_BITCOM = 1,
	PARK_ZEHIN = 2,
    PARK_SONGLI = 3,
    PARK_HTTP = 4
};


/****************************extern varible*********************************/
extern sqlite3 *spsystem_db;


/****************************extern function*********************************/
extern int spsystem_open(const char *ac_path);
extern int spsystem_create_lastmsg_table(sqlite3 *asql_db);
extern int spsystem_create_picture_table(sqlite3 *asql_db);
extern int spsystem_create_config_table(sqlite3 *asql_db);
extern int spsystem_insert_lastmsg_table(sqlite3 *asql_db, char ac_values[][128]);
extern int spsystem_insert_picture_table(sqlite3 *asql_db, char ac_values[][128]);
extern int spsystem_insert_config_table(sqlite3 *asql_db, char *ac_values);
extern int spsystem_check_picture_exist(sqlite3 *asql_db, const char *ac_values);
extern int spsystem_update_picture_table(sqlite3 *asql_db, const char *ac_name, int ai_values, int ai_type);
extern int spsystem_select_picture_table(sqlite3 *asql_db, char ac_values[][128], int ai_type);
extern int spsystem_select_config_table(sqlite3 *asql_db, char *ac_values[128]);
extern int spsystem_select_lastmsg_table(sqlite3 *asql_db, char ac_values[][128]);
extern int spsystem_delete_lastmsg_table(sqlite3 *asql_db);
extern int spsystem_delete_picture_table(sqlite3 *asql_db, int ai_id);
extern int spsystem_delete_config_table(sqlite3 *asql_db, int ai_id);
extern int spsystem_count_picreget_table(sqlite3 *asql_db);

#endif

