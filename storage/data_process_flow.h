
#ifndef _DATA_PROCESS_FLOW_H_
#define _DATA_PROCESS_FLOW_H_


#include "data_process.h"


extern int flag_upload_complete_flow;


int db_upload_flow_records();
int traffic_flow_reupload_pedestrian(void);
int get_record_count_flow(const char *db_name, TYPE_HISTORY_RECORD *type_history_record, pthread_mutex_t *mutex_db_records);
void *db_upload_history_flow_Thr(void *arg);


#endif 	/* _DATA_PROCESS_H_ */
