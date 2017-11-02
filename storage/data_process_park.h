
#ifndef _DATA_PROCESS_PARK_H_
#define _DATA_PROCESS_PARK_H_

//#include "data_process.h"
//extern int flag_upload_complete_traffic;

//int analysis_traffic_file_path(DB_TrafficRecord* db_records, EP_PicInfo  *pic_info);
int park_record_reupload(void);
//int get_record_count_traffic(const char *db_name, TYPE_HISTORY_RECORD *type_history_record, pthread_mutex_t *mutex_db_records);
//void *db_upload_history_traffic_Thr(void *arg);
int analysis_park_file_path(DB_ParkRecord* db_records,
                               EP_PicInfo pic_info[]);

#endif 	/* _DATA_PROCESS_TRAFFIC_H_ */
