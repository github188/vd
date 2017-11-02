
#ifndef _DATA_PROCESS_EVENT_H_
#define _DATA_PROCESS_EVENT_H_


#include "data_process.h"



extern int flag_upload_complete_event;

int analysis_event_file_path(DB_EventAlarm* db_records, EP_PicInfo pic_info[]);//, EP_VidInfo *vid_info);
int db_upload_event_records();
int get_record_count_event(char *db_name, TYPE_HISTORY_RECORD *type_history_record, pthread_mutex_t *mutex_db_records);
void *db_upload_history_event_Thr(void *arg);


#endif 	/* _DATA_PROCESS_EVENT_H_ */
