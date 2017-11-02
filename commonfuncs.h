
/****************************************
 *Filename:    commonfuncs.h
 *
 *Created on:  2013-4-8
 *Author:      shanhongwei
****************************************/

#ifndef COMMONFUNCS_H_
#define COMMONFUNCS_H_

#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "commontypes.h"
#include "ep_type.h"

#include "dsp_config.h"
#include "log_interface.h"


#define SHANHEPING  1

#define log_debug(mod_name, msg...)	\
	log_send(LOG_LEVEL_DEBUG, 0, mod_name, msg)

#define log_state(mod_name, msg...)	\
	log_send(LOG_LEVEL_STATUS, 0, mod_name, msg)

#define log_warn(mod_name, msg...)	\
	log_send(LOG_LEVEL_WARNING, 0, mod_name, msg)

#define log_error(mod_name, msg...)	\
	log_send(LOG_LEVEL_FAULT, 0, mod_name, msg)

#define _DEBUG_FUNCS_

#ifdef _DEBUG_FUNCS_
#define debug(msg...) \
	do{\
		printf("(%s|%s|%d) ", __FILE__, __func__, __LINE__);\
		printf(msg);\
	} while (0);
#else
#define debug(msg...)
#endif

#define MAX_NUM_EVENT		20	//事件优先级个数;
#define Ksize		(1024)
#define Msize 		(Ksize * Ksize)
#define Gsize 		(Ksize * Msize)

//#########################################//#########################################//
/* Function error codes */
#define SUCCESS             0
#define FAILURE             -2

/* Thread error codes */
#define THREAD_SUCCESS      (Void *) 0
#define THREAD_FAILURE      (Void *) -1

#define TRACE_LOG_ILLEGAL_PARKING(string...) 
#define TRACE_LOG_PLATFROM_INTERFACE(string...) 
#define TRACE_LOG_PACK_PLATFORM(string...)
#define TRACE_LOG_SYSTEM(string...) 

#define printf_with_ms(...)  do { struct timeval tv; \
                              struct tm *tmnow;\
                              char tmbuffer[128];\
                              gettimeofday(&tv, NULL);\
                              tmnow = localtime(&(tv.tv_sec));\
                      strftime (tmbuffer,sizeof(tmbuffer),"%Y-%m-%d %H:%M:%S",tmnow);\
                              printf("\n\r[%s::%d] ", tmbuffer,(int)tv.tv_usec/1000);\
                              printf("[vd_ms] " __VA_ARGS__); fflush(stdout); } while(0)


//#########################################//#########################################//
extern struct timeval g_time_start;
extern struct timeval g_time_end;


#ifdef  __cplusplus
extern "C"
{
#endif

/* This function is in sys/file.h */
extern ssize_t get_file_size(const char *path);

void trace_log(const char *file, int32_t line, const char *func,
			   const char *fmt, ...);


u32 check_sum(u8 *data, int len);

void spend_ms_start(void);

void spend_ms_end(const char *descp);

int save_file(const char *file_name, const char *buff, int len);

int vpot_int_256_to_254(int a);

int vpot_int_254_to_256(int b);

void covert_256_to_254(char *src, int len, char *dest);

int covert_254_to_256(char *src, int len, char *dest);

char * get_date_time(void);

int set_systime(int year, int mon, int day, int hour, int min, int sec);

int create_mul_dir(const char *path);

int convert_enc(const char* fromcode, const char* tocode,
                const char *buf_in, int inlen, char *buf_out, int outlen);
ssize_t convert_enc_s(const char *fromcode, const char *tocode,
					  const char *inbuf, size_t inlen,
					  char *outbuf, size_t outlen);

int mk_time(int year, int mon, int day, int hour, int min, int sec);

int check_jpeg(const void *ptr_pic, size_t len);

int get_ftp_path(FTP_URL_Level param, int year, int month, int day, int hour,
                 char* event_name, char *filepath);

int get_thread_name(char *name);
void set_thread(const char *thread_name);

int check_file_correct(const char * fpName);

int clear_socket_recvbuf(int sockfd);

int get_time_from_log(const char * fpName, time_t *rec_time_ret );

#ifdef  __cplusplus
}
#endif


#endif /* COMMONFUNCS_H_ */
