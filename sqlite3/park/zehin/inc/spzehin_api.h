#ifndef _SPZEHIN_API_H_
#define _SPZEHIN_API_H_

#define BUFFERSIZE_16    (16)
#define BUFFERSIZE_32    (32)
#define BUFFERSIZE_64    (64)
#define BUFFERSIZE_128   (128)
#define BUFFERSIZE_512   (512)
#define BUFFERSIZE_1024  (1024)


/****************************typedef struct*********************************/
typedef struct
{
	int  pic_id;
	char pic_path[BUFFERSIZE_512];
	char pic_name[BUFFERSIZE_64];
	char pic_size[BUFFERSIZE_16];
	char pic_serial;
	char msgrecord_id[32];
	int  msgtable_id;
	int  pic_regetflag;
}str_spzehin_picreget_table;


typedef struct
{
	int  msg_id;
	char berth_num[BUFFERSIZE_64];
	char plate_num[BUFFERSIZE_32];
	char plate_color[BUFFERSIZE_16];
   	char status[BUFFERSIZE_32];//状态信息，是否正常：  0，正常；其他，异常；
	char confidence[BUFFERSIZE_32];
	char eltype[BUFFERSIZE_32];//驶入驶离状态。  0，驶离；1，驶入；
	char eltime[BUFFERSIZE_32];
    char plate_position[BUFFERSIZE_32];
    char uuid[BUFFERSIZE_128];
}str_spzehin_msgreget_table;

typedef struct
{
    int category;
    int level;
    char time[32];
    char uuid[128];
}str_spzehin_alarmreget_table;

enum SORT_TYPE
{
    SORT_ASC,
    SORT_DESC
};
/****************************extern varible*********************************/
extern sqlite3 *spzehin_db;


/****************************extern function*********************************/
extern int spzehin_open(const char *ac_path);
extern int spzehin_create_configparam_table(sqlite3 *asql_db);
extern int spzehin_create_lastmsg_table(sqlite3 *asql_db);
extern int spzehin_create_msgreget_table(sqlite3 *asql_db);
extern int spzehin_create_alarmreget_table(sqlite3 *db);
extern int spzehin_create_picreget_table(sqlite3 *asql_db);
extern int spzehin_drop_table(sqlite3 *asql_db, char *ac_table_name);
extern int spzehin_check_table(sqlite3 *asql_db, char *ac_tablename, char ac_column_name[][32], int ai_column_num);
extern int spzehin_insert_configparam_table(sqlite3 *asql_db, char ac_values[][32]);
extern int spzehin_insert_lastmsg_table(sqlite3 *asql_db, str_spzehin_msgreget_table as_spzehin_msgreget_table);
extern int spzehin_insert_msgreget_table(sqlite3 *asql_db, str_spzehin_msgreget_table as_spzehin_msgreget_table);
extern int spzehin_insert_alarmreget_table(sqlite3 *asql_db, str_spzehin_alarmreget_table alarm);
extern int spzehin_insert_picreget_table(sqlite3 *asql_db, str_spzehin_picreget_table as_spzehin_picreget_table);
extern int spzehin_update_picreget_talbe(sqlite3 *asql_db, int ai_picid, char *ac_msgrecordid);
extern int spzehin_select_configparam_table(sqlite3 *asql_db, char ac_values[][32]);
extern int spzehin_select_lastmsg_table(sqlite3 *asql_db, str_spzehin_msgreget_table *as_spzehin_msgreget_table);
extern int spzehin_select_msgreget_table(sqlite3 *asql_db, str_spzehin_msgreget_table *as_spzehin_msgreget_table, enum SORT_TYPE type = SORT_ASC);
extern int spzehin_select_alarmreget_table(sqlite3 *asql_db, str_spzehin_alarmreget_table *alarm);
extern int spzehin_select_picreget_table(sqlite3 *asql_db, str_spzehin_picreget_table *picret_tb, const unsigned char type = 0);
extern int spzehin_delete_configparam_table(sqlite3 *asql_db);
extern int spzehin_delete_lastmsg_table(sqlite3 *asql_db);
extern int spzehin_delete_msgreget_table(sqlite3 *asql_db, int ai_id);
extern int spzehin_delete_alarmreget_table(sqlite3 *asql_db, int ai_id);
extern int spzehin_delete_picreget_table(sqlite3 *asql_db, int ai_id);
extern int spzehin_count_lastmsg_table(sqlite3 *asql_db);
extern int spzehin_count_msgreget_table(sqlite3 *asql_db);
extern int spzehin_count_alarmreget_table(sqlite3 *asql_db);
extern int spzehin_count_picreget_table(sqlite3 *asql_db);


#endif

