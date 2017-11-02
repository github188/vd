#ifndef _TVPNETPOSA_RECORDS_H_
#define _TVPNETPOSA_RECORDS_H_

#include "violation_records_process.h"


#define VP_PICTURE_BASENUM     (4)
#define VP_PICTURE_BASESIZE    (1024*500)
#define VP_MESSAGE_BASESIZE    (512)

//zehin park platform records struct
typedef struct
{
	unsigned char picture[VP_PICTURE_BASENUM][VP_PICTURE_BASESIZE];
    int  picture_size[VP_PICTURE_BASENUM];
	char picture_path[VP_PICTURE_BASENUM][128];
	char message[VP_MESSAGE_BASESIZE];
	char msgexist_flag;
	char picexist_flag;
	char flag;
	DB_ViolationRecordsMotorVehicle field;
}str_tvpnetposa_records;


extern str_tvpnetposa_records gstr_tvpnetposa_records;


extern void *tvpnetposa_records_pthread(void *arg);


#endif
