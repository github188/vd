
#ifndef _DATA_PROCESS_VIOLATION_H_
#define _DATA_PROCESS_VIOLATION_H_


#include "data_process.h"

extern int flag_upload_complete_violation;


int analysis_violation_file_path(DB_ViolationRecordsMotorVehicle* db_violation_records, EP_PicInfo pic_info[], EP_VidInfo *vid_info);
int db_upload_violation_records_motor_vehicle();
int get_record_count_violation(char *db_name, TYPE_HISTORY_RECORD *type_history_record, pthread_mutex_t *mutex_db_records);
void *db_upload_history_violation_Thr(void *arg);



#endif 	/* _DATA_PROCESS_H_ */
