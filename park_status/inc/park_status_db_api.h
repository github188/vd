#ifndef __PARK_STATUS_DB_API_H__
#define __PARK_STATUS_DB_API_H__

#include "park_status.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PARK_STATE_DB_PATH "/var/www/parkjson/park_state.db"

typedef int (*callback)(void* data, int argc, char **argv, char **columnName);

int park_state_db_open(const char *path);
int park_state_db_close(void);

int park_state_db_create_tb(void);

int park_state_db_insert_parkhistory_tb(park_history_t *his);

int park_state_db_select_parkhistory_tb(const char* from_time,
        const char* end_time, callback cb);

//delete park bitcom message records by id
int park_state_db_delete_parkhistory(int id);

//delete park bitcom message records before time
int park_state_db_delete_parkhistory_old(const char * time);

//count park bitcom message records
int park_state_db_count_parkhistory_callback(void *data, int ncols,
        char **values, char **headers);

int park_state_db_count_parkhistory_tb(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PARK_STATUS_DB_API_H__ */
