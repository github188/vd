#ifndef _VD_MSG_QUEUE_H_
#define _VD_MSG_QUEUE_H_

#include "types.h"

C_CODE_BEGIN

#define MSG_PLATFORM_INFO_ID			gi_msg_platform_info_id
#define MSG_ALLEYWAY_ID					gi_msg_alleyway_id
#define MSG_ALLEYWAY_BITCOM_TYPE		(1)

#define MSG_VM_ID						gi_msg_vm_id

#define MSG_VP_ID						gi_msg_vp_id
#define MSG_VP_UNIVIEW_TYPE		        (30)
#define MSG_VP_NETPOSA_TYPE		        (31)
#define MSG_VP_BAOKANG_TYPE		        (32)

#define MSG_PARK_ID						gi_msg_park_id

#define MSG_PARK_SYSTEM_TYPE			( 9)
#define MSG_PARK_ZEHIN_TYPE		        (10)
#define MSG_PARK_BITCOM_TYPE            (11)
#define MSG_PARK_HTTP_TYPE              (19)
#define MSG_PARK_SONGLI_TYPE            (20)
#define MSG_PARK_KTETC_TYPE             (22)

#define MSG_PARK_ALARM                  (12)
#define MSG_PARK_RECORD                 (13)
#define MSG_PARK_INFO                   (14)
#define MSG_PARK_CONF                   (15)
#define MSG_PARK_STATUS                 (16)
#define MSG_PARK_ZEHIN_CB               (17)
#define MSG_PARK_SNAP                   (18)
#define MSG_PARK_LIGHT                  (21)

/*************************** platform configurateion message queue ************/
typedef struct
{
	long dst;
	long src;
	long type;
	long data_lens;
	char data[1024];
}str_platform_info;

typedef struct
{
	long type;
	str_platform_info info;
}str_platform_info_msg;

/****************************alleyway message queue struct*********************/
typedef struct
{
	char func;
	char *data;
	size_t data_lens;
	struct {
		uint8_t *frm;
		size_t frm_len;
		uint8_t *pic;
		size_t pic_len;
	}http;
}str_alleybitom;

typedef struct
{
	long type;
	str_alleybitom bitcom;
}str_alleyway_msg;



typedef struct
{
}str_vm_msg;



/****************************vp message queue struct***************************/
typedef struct
{
    long type;
	char data[512];
	int  data_lens;
}str_vpuniview;

typedef struct
{
    long type;
    char data[512];
    int data_lens;
}str_baokang;

typedef struct
{
	long type;
	str_vpuniview uniview;
    str_baokang baokang;
}str_vp_msg;

/****************************park message queue struct*************************/
typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO; MSG_PARK_CONF;
    char *data;
    int lens;
}str_parksystem;

typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO; MSG_PARK_CONF;
    char *data;
    int lens;
}str_parkzehin;

typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO; MSG_PARK_CONF;
    void* data;
    int lens;
}parkhttp;

typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO
    void* data;
    int lens;
}parksongli;

typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO; MSG_PARK_CONF;
    char *data;
    int lens;
}str_parkbitcom;

typedef struct
{
    int type; // MSG_PARK_ALARM; MSG_PARK_RECORD; MSG_PARK_INFO; MSG_PARK_CONF;
    void *data;
    int lens;
}parkktetc;

typedef struct
{
	long type; // PLATFORM TYPE ZEHIN BITCOM HTTP SONGLI
	union {
		str_parkzehin zehin;
		str_parkbitcom bitcom;
		str_parksystem system;
        parkhttp http;
        parksongli songli;
		parkktetc ktetc;
	}data;
}str_park_msg;

//-------------------global variable-----------------------
extern int gi_msg_platform_info_id;
extern int gi_msg_alleyway_id;
extern int gi_msg_park_id;
extern int gi_msg_vp_id;



//-------------------function----------------------------
extern int vd_msgueue_init(void);
extern int32_t ve_msg_del(void);


extern int vmq_sendto_tabitcom(str_alleyway_msg as_alleyway_msg, char ac_func);
extern int vmq_sendto_tpzehin(str_park_msg as_park_msg);
extern int vmq_sendto_tpbitcom(str_park_msg as_park_msg);
extern int vmq_sendto_http(str_park_msg park_msg);
extern int vmq_sendto_songli(str_park_msg park_msg);
extern int vmq_sendto_ktetc(str_park_msg park_msg);

extern int vmq_sendto_tvpuniview(str_vp_msg as_vp_msg);
extern int vmq_sendto_tvpbaokang(str_vp_msg as_vp_msg);
extern int vmq_sendto_tvpnetposa(str_vp_msg as_vp_msg);



C_CODE_END







#endif

