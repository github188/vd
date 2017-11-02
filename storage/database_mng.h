
#ifndef _DATABASE_MNG_H_
#define _DATABASE_MNG_H_


#include <pthread.h>
#include <sqlite3.h>


#define SQL_BUF_SIZE 	2048 	/* SQL语句缓存大小 */

#define DB_FILES_PATH 				"/var/DB_files"
#define DB_NAME_PARK_RECORD         "/var/DB_files/DB_files_park.db" 
#define DB_NAME_VM_RECORD 		"/var/DB_files/DB_files_vm.db"
#define DB_NAME_VIOLATION_RECORDS 	"/var/DB_files/DB_files_violation.db"
#define DB_NAME_EVENT_RECORDS 		"/var/DB_files/DB_files_event.db"
#define DB_NAME_FLOW_RECORDS 		"/var/DB_files/DB_files_flow.db"
#define DB_NAME_FLOW_PEDESTRIAN 	"/var/DB_files/DB_files_flow_pedestrian.db"


enum
{
	DB_TYPE_TRAFFIC_RECORD, 				/* 通行记录 */
	DB_TYPE_VIOLATION_RECORD, 				/* 违法记录 */
	DB_TYPE_EVENT_ALARM, 					/* 事件报警 */
	DB_TYPE_TRAFFIC_FLOW_MOTOR_VEHICLE, 	/* 机动车交通流 */
	DB_TYPE_TRAFFIC_FLOW_PEDESTRIAN, 		/* 行人交通流 */
};

extern pthread_mutex_t mutex_db_files_park;
extern pthread_mutex_t mutex_db_files_traffic;
extern pthread_mutex_t mutex_db_files_violation;
extern pthread_mutex_t mutex_db_files_event;
extern pthread_mutex_t mutex_db_files_flow;
extern pthread_mutex_t mutex_db_files_flow_pedestrian;

extern pthread_mutex_t mutex_db_records_park;
extern pthread_mutex_t mutex_db_records_traffic;
extern pthread_mutex_t mutex_db_records_violation;
extern pthread_mutex_t mutex_db_records_event;
extern pthread_mutex_t mutex_db_records_flow;
extern pthread_mutex_t mutex_db_records_flow_pedestrian;

extern int flag_stop_history_upload; 				//停止历史记录上传标记


#ifdef  __cplusplus
extern "C"
{
#endif

int db_init(void);
int db_create_table(sqlite3 *db, const char *sql);
//int db_write(const char *db_name, const char *sql);
int db_write(const char *db_name, const char * sql_create, const char *sql_insert);

#ifdef  __cplusplus
}
#endif


#endif 	/* _DATABASE_MNG_H_ */
