#ifndef _TPBITCOM_RECORDS_H_
#define _TPBITCOM_RECORDS_H_

#include "park_records_process.h"
#include "upload.h"

#define PICTURE_BASENUM     (2)
#define PICTURE_BASESIZE    (1024*500)
#define MESSAGE_BASESIZE    (512)


//bitcom park platform records struct
typedef struct
{
	unsigned char picture[PICTURE_BASENUM][PICTURE_BASESIZE];
    int  picture_size[PICTURE_BASENUM];
	char picture_path[PICTURE_BASENUM][80];
	char picture_name[PICTURE_BASENUM][256];
	char message[MESSAGE_BASESIZE];
	char reget_flag;
	char msgexist_flag;
	char picexist_flag;
	char pic_flag;
	DB_ParkRecord field;
	EP_PicInfo picture_info[PICTURE_BASENUM];
}str_tpbitcom_records;


/**********************extern varibal************************/
extern str_tpbitcom_records gstr_tpbitcom_records;


/**********************extern function***********************/
extern int tpbitcom_save_picture(unsigned char *ac_picbuff, int ai_picbuff_size);
extern void *tpbitcom_message_pthread(void *arg);
extern void *tpbitcom_picture_pthread(void *arg);
extern void *tpbitcom_recordsreget_pthread(void *arg);


#endif
