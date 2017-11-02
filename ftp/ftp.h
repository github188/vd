/*
 * CLASS_FTP.h
 *
 *  Created on: 2013-3-15
 *      Author: shanhongwei
 */

#ifndef FTP_H_
#define FTP_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <string.h>        // for bzero
#include <arpa/inet.h>
#include <sys/stat.h>

#include "commonfuncs.h"
#include "ep_type.h"
#include "arm_config.h"
#include "upload.h"

using namespace std;

#define USE_UTF8   1   //����ftp���������ϴ��ļ���UTF-8�����ʽ��
#define STA_FTP_OK    0 //ftp��������;
#define STA_FTP_FAIL  1 << 0 //ftp�����쳣;
#define STA_FTP_EXP  1 << 1  //ftp��������Ӧ�쳣
#define STA_FTP_SET_CHANGED 1 << 2  //ftp�����������޸�
#define SUCCESS     0
#define SOCKET_ERR 	-1
#define FAILURE  -2

#define MAX_SIZE_SEND				(1024*1024)	//ftp���η�������ֽ���;
#define MAX_DIR_LEVEL 				6			//ftp�����ļ������Ŀ¼����;
#define TIME_SEC_FAULT_ALARM		(10*60)	//���ϱ���ʱ����(��λ:S��)


#define FTP_PASSWORD_REQUIRED 	"331"
#define FTP_LOGGED_ON 			"230"


//#define  _DEBUG_FTP_
#ifdef _DEBUG_FTP_
#define debug_ftp(msg...) \
	do{\
		printf("(%s|%s|%d)",__FILE__,__func__,__LINE__);\
		printf(msg);\
		fflush(stdout);\
	}while (0);
#else
#define debug_ftp(msg...)
#endif

#if(1)
#define log_debug_ftp(msg...) 	printf(msg)
#define log_state_ftp(msg...) 	printf(msg)
#define log_warn_ftp(msg...) 	printf(msg)
#define log_error_ftp(msg...) 	printf(msg)
#else
#define log_debug_ftp(msg...) 	log_debug("[FTP]", msg)
#define log_state_ftp(msg...) 	log_state("[FTP]", msg)
#define log_warn_ftp(msg...) 	log_warn("[FTP]", msg)
#define log_error_ftp(msg...) 	log_error("[FTP]", msg)
#endif


typedef enum
{
    FTP_CHANNEL_ILLEGAL = 0,//Υ��ͨ��
    FTP_CHANNEL_PASSCAR, //����ͨ��
    FTP_CHANNEL_H264, //��Ƶͨ��
    FTP_CHANNEL_CONFIG, //����ͨ��

    FTP_CHANNEL_ILLEGAL_RESUMING ,//Υ��ͨ��
    FTP_CHANNEL_PASSCAR_RESUMING, //����ͨ��
    FTP_CHANNEL_H264_RESUMING, //��Ƶͨ��

    FTP_CHANNEL_COUNT, 	//ͨ������

} FTP_CHANNEL_INDEX;


//���庯��ָ��
typedef void(*lpFun)(void *, void *);


class CLASS_FTP
{
private:
	TYPE_FTP_CONFIG_PARAM config;

	int fd_ctl_socket; //socket
	char descript[128]; //������;
	int ftp_status; //ftp����״̬;
	pthread_mutex_t mutex; //��fd_ctl_socket�Ĳ�����ͬ��.
	pthread_t thread_t_ftp;

	int data_port; //pasvģʽ,���ݴ���˿�
	int fd_data_socket; //���ݴ���socket

	bool server_is_full;
	bool thread_stop; //�߳��Ƿ��������

	bool started; //start()��λ, stop()��λ.
	lpFun alarmFunc;

	void* pCallerParam; //�ϲ㴫�µĲ���, �ص�����.

public:
	CLASS_FTP();
	CLASS_FTP(const char *des);
	CLASS_FTP(void* pCallerParam);
	virtual ~CLASS_FTP();

	int start();
	void stop();

	TYPE_FTP_CONFIG_PARAM * set_config(TYPE_FTP_CONFIG_PARAM *param);
	TYPE_FTP_CONFIG_PARAM * get_config();

	//�ڷ������ϴ洢�ļ�;
	int ftp_put_data(const char dir[][80], const char *filename,
	                 char *buf, long size);

	int ftp_put_data(char dir[][80], char * filename, char *buf, long len,
	                 char *buf2, long len2);

	int ftp_put_data(const char *dest_path, const char *dest_name,
	                 const void *buf_ptr, size_t buf_len);

	int ftp_put_h264(char dir[][80], char * filename, EP_VidInfo *pEP_vidinfo);

	//���ļ����͵�ftp;
	int ftp_put_file(const char * src_file_name, char dst_dir[][80],
	                 char * des_file_name);

	int ftp_get_file(char *ftp_path_file, const char *save_path_file);

	int ftp_dele(char* filename);

	void setDescript(char* descript);
	int getDescript(char* buf);

	void setAlarmFxn(lpFun alarmFunc);
	lpFun getAlarmFxn() const;

	int get_status();
	bool getServer_is_full() const;

	int open_server();
	void close_server();
private:
	int ftp_set_trans_pasv_mode();

	int ftp_mk_ctl_socket();
	void ftp_close_ctl_socket();

	int ftp_login();
	int ftp_logout();

	bool getThread_stop() const;
	void setThread_stop(bool thread_stop);

	static void* thread_ftp_run(void *arg);

	void set_server_full(bool server_is_full);

	void set_status(int sta, bool set);
	void set_status(int sta);

	void *getCallerParam() const;
	void setCallerParam(void *pCallerParam);

	int ftp_mk_data_socket();
	void ftp_close_data_socket();

	int ftp_check_server_full();

	int ftp_check_ftp_sta(void); //���ftp����״̬;

	int ftp_send_cmd(char* cmd, int len);

	int ftp_recv_cmd_response(char* buf, int len);

	int ftp_mk_dir(const char dir[][80]);
	int ftp_mk_path(const char *path);
	int ftp_cd_dir(const char* dir);

	int ftp_mkd(const char *dir);
	int ftp_pwd();

	int ftp_store(const char* dir, const char* filename);
	int ftp_retr(char* path_file);
	int ftp_set_trans_type_I();

	int ftp_send_file(const char* file_path);
	int ftp_send_data(const void *buf, long size);
};

bool ftp_module_initialized();

int ftp_module_init(void);
void ftp_module_exit(void);

CLASS_FTP* get_ftp_chanel(int index);


#endif /* FTP_H_ */
