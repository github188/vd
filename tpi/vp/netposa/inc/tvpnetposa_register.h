#ifndef _TVPNETPOSA_REGISTER_H_
#define _TVPNETPOSA_REGISTER_H_



//tvpnetposa register 
typedef struct
{
	char ip[32];
	char company[32];
	int  port;
	int  video_regflag;
	int  heart_cycle;
	char flag;
}str_tvpnetposa_register;


extern str_tvpnetposa_register gstr_tvpnetposa_register;


extern void *tvpnetposa_register_pthread(void *arg);


#endif
