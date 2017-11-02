#ifndef _TPZEHIN_RECORDS_H_
#define _TPZEHIN_RECORDS_H_

#include "park_records_process.h"

#define PICTURE_BASENUM     (2)
#define PICTURE_BASESIZE    (1024*500)
#define MESSAGE_BASESIZE    (512)


//zehin park platform records struct
typedef struct
{
	int category;
	int level;
	char time[32];
    char uuid[128];
}str_tpzehin_alarm;

typedef struct
{
	unsigned char picture[PICTURE_BASENUM][PICTURE_BASESIZE];
    int  picture_size[PICTURE_BASENUM];
	char picture_path[PICTURE_BASENUM][128];
	char reget_flag;
	unsigned long long  msgrecord_id;
	str_tpzehin_alarm alarm;
	DB_ParkRecord field;
}str_tpzehin_records;

// type used to notify alarm thread or record thread begin to handle request.
// identity idicate the record in case erase by other request.
typedef struct
{
    long type;
    long identity;
    bool with_picture;
}zehin_msg_t;
/**********************extern varibal************************/
extern str_tpzehin_records gstr_tpzehin_records;
extern pthread_mutex_t g_zehin_record_mutex;
extern pthread_mutex_t g_zehin_alarm_mutex;

/**********************extern function***********************/
extern void *tpzehin_record_pthread(void *arg);
extern void *tpzehin_alarm_pthread(void *arg);
extern void *tpzehin_recordsreget_pthread(void *arg);


#endif
