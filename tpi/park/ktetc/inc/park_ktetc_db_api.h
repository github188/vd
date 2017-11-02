#ifndef __PARK_KTETC_DB_API_H__
#define __PARK_KTETC_DB_API_H__

#include "park_ktetc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <sqlite3.h>

typedef int (*callback)(void* data, int argc, char **argv, char **columnName);

int park_ktetc_db_open(const char *path);
int park_ktetc_db_close(void);
int park_ktetc_db_create_tb(void);
int park_ktetc_db_insert_record(const ktetc_t *k);
int park_ktetc_db_select_record(const char* from_time,
		const char* end_time, callback cb);
int park_ktetc_db_delete_record(const char *id);
int park_ktetc_db_delete_record_old(const char * time);
int park_ktetc_db_count_record();

int park_ktetc_db_insert_alarm(const ktetc_alarm_t *k);
int park_ktetc_db_select_alarm(const char* from_time,
		const char* end_time, callback cb);

int park_ktetc_db_insert_retrans(const char *msg);
int park_ktetc_db_select_retrans(char *msg);
int park_ktetc_db_delete_retrans(int id);
int park_ktetc_db_count_retrans();
#ifdef __cplusplus
}
#endif
#endif /* __PARK_KTETC_DB_API_H__ */
