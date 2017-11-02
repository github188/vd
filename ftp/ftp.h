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

#define USE_UTF8   1   //定义ftp服务器，上传文件用UTF-8编码格式。
#define STA_FTP_OK    0 //ftp连接正常;
#define STA_FTP_FAIL  1 << 0 //ftp连接异常;
#define STA_FTP_EXP  1 << 1  //ftp服务器响应异常
#define STA_FTP_SET_CHANGED 1 << 2  //ftp服务器设置修改
#define SUCCESS     0
#define SOCKET_ERR 	-1
#define FAILURE  -2

#define MAX_SIZE_SEND				(1024*1024)	//ftp单次发送最大字节数;
#define MAX_DIR_LEVEL 				6			//ftp创建文件夹最多目录级别;
#define TIME_SEC_FAULT_ALARM		(10*60)	//故障报警时间间隔(单位:S秒)


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
    FTP_CHANNEL_ILLEGAL = 0,//违法通道
    FTP_CHANNEL_PASSCAR, //过车通道
    FTP_CHANNEL_H264, //视频通道
    FTP_CHANNEL_CONFIG, //配置通道

    FTP_CHANNEL_ILLEGAL_RESUMING ,//违法通道
    FTP_CHANNEL_PASSCAR_RESUMING, //过车通道
    FTP_CHANNEL_H264_RESUMING, //视频通道

    FTP_CHANNEL_COUNT, 	//通道数量

} FTP_CHANNEL_INDEX;


//定义函数指针
typedef void(*lpFun)(void *, void *);


class CLASS_FTP
{
private:
	TYPE_FTP_CONFIG_PARAM config;

	int fd_ctl_socket; //socket
	char descript[128]; //类描述;
	int ftp_status; //ftp连接状态;
	pthread_mutex_t mutex; //对fd_ctl_socket的操作做同步.
	pthread_t thread_t_ftp;

	int data_port; //pasv模式,数据传输端口
	int fd_data_socket; //数据传输socket

	bool server_is_full;
	bool thread_stop; //线程是否继续运行

	bool started; //start()置位, stop()复位.
	lpFun alarmFunc;

	void* pCallerParam; //上层传下的参数, 回调传回.

public:
	CLASS_FTP();
	CLASS_FTP(const char *des);
	CLASS_FTP(void* pCallerParam);
	virtual ~CLASS_FTP();

	int start();
	void stop();

	TYPE_FTP_CONFIG_PARAM * set_config(TYPE_FTP_CONFIG_PARAM *param);
	TYPE_FTP_CONFIG_PARAM * get_config();

	//在服务器上存储文件;
	int ftp_put_data(const char dir[][80], const char *filename,
	                 char *buf, long size);

	int ftp_put_data(char dir[][80], char * filename, char *buf, long len,
	                 char *buf2, long len2);

	int ftp_put_data(const char *dest_path, const char *dest_name,
	                 const void *buf_ptr, size_t buf_len);

	int ftp_put_h264(char dir[][80], char * filename, EP_VidInfo *pEP_vidinfo);

	//将文件发送到ftp;
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

	int ftp_check_ftp_sta(void); //检测ftp连接状态;

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
