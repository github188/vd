#ifndef _TPZEHIN_PTHREAD_H_
#define _TPZEHIN_PTHREAD_H_

#include "../lib/libsignalling/vPaas_DataType.h"
#include "../lib/libsignalling/vPaasT_SDK.h"

/******************login info********************/
typedef struct
{
    char ip[32];
    int  cmd_port;
    int  heart_port;
}str_center;

typedef struct
{
    char ip[128];
    int  port;
}str_sf;

typedef struct
{
    int user_stat;
    int type;
    int port;
    char ip[32];
    char user_name[64];
    char user_passwd[64];
}str_local;


typedef struct
{
    str_center center;
	str_sf    sf;
    str_local local;
    stZehinCamInfo_T_ camera;
	stZehinDevInfo_T_ device;
	stZehinDataPointInfo_T_ index;
	int index_num;
}str_tpzehin_login;

enum ZEHIN_CAP_STATE
{
    ZEHIN_CAP_START,
    ZEHIN_CAP_SEND2DSP,
    ZEHIN_CAP_FINISH
};

extern str_tpzehin_login gstr_tpzehin_login;

extern void *tpzehin_pthread(void *arg);

#endif



