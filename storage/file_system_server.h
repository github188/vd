
#ifndef _FILE_SYSTEM_SERVER_H_
#define _FILE_SYSTEM_SERVER_H_

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#include "file_system_server_api.h"


#define FS_SERVER_BACKLOG 		10

#define FS_CLIENT_MAX_NUM 		10 			/* 最多允许10个客户端同时连接 */

#define FS_FILE_SPLIT_SIZE 		1024 		/* 文件分割大小，单位: 字节 */

#define FS_PATH_PERMISSION 		"/media" 	/* 文件服务限制在该路径内 */
#define FS_PATH_PERMISSION_LEN 	6 			/* 上面路径的长度 */


enum
{
    FS_INIT_ERR_LISTEN 		= -4, 		//侦听失败
    FS_INIT_ERR_BIND 		= -3, 		//绑定失败
    FS_INIT_ERR_SOCKOPT 	= -2, 		//套接字属性设置失败
    FS_INIT_ERR_SOCKET 		= -1, 		//套接字创建失败
    FS_INIT_SUCCEED 		= 0, 		//初始化成功
}; 	/* fs_server_init 返回值 */

enum
{
    FS_HEAD_FLAG_WRONG 		= -3, 		//命令头标记错误
    FS_HEAD_SIZE_WRONG 		= -2, 		//命令头大小错误
    FS_HEAD_RECV_ERROR 		= -1, 		//接收出错
    FS_HEAD_RECV_SHUTDOWN 	= 0, 		//连接已断开
}; 	/* fs_server_read_cmd_head 返回值 */

typedef struct
{
	int 				client_num;
	FileServCmdHead 	*cmd_head;
	char 				*cmd_body;
} ListArgv, DownloadArgv;


void close_fs_client_socket(int client_num);
int fs_server_create(void);
void *fs_server_task(void *argv);
int fs_server_init(void);
void fs_server_start(void);
int alloc_new_connection(int sockfd, struct sockaddr_in *addr);
void process_failed_new_connection(int sockfd);
int process_succeed_new_connection(int client_num);
void *fs_server_run(void *argv);
int fs_server_process_connection(int client_num);
int fs_server_read_cmd_head(int client_num, FileServCmdHead *cmd_head);
void process_wrong_head_connection(int client_num, FileServCmdHead *cmd_head);
int fs_server_read_cmd_body(int client_num, size_t length, void *buf);
int fs_server_process_cmd(int client_num,
                          FileServCmdHead *cmd_head, void *cmd_body);
int fs_server_unknown_cmd(int client_num, FileServCmdHead *reply_cmd);
int fs_server_reply_list(int client_num,
                         FileServCmdHead *cmd_head, char *dir_path);
int is_path_dir(const char *path);
void *thread_list_dir(void *argv);
int connect_client_server(int client_num, int port);
int send_list(int list_socket, FileServCmdHead *cmd_head, char *path);
int fs_server_reply_download(int client_num,
                             FileServCmdHead *cmd_head, char *file_path);
int is_path_regular_file(const char *file_path);
void *thread_download_file(void *argv);
int send_file(int list_socket, FileServCmdHead *cmd_head, char *file_path);
int fs_server_reply_status(int client_num, FileServCmdHead *cmd_head);
void fs_server_get_disk_status(FileServDiskStatus *disk_status);
int fs_server_delete(void);
int fs_server_destory(void);
void fs_server_stop(void);

#endif 	/* _FILE_SYSTEM_SERVER_H_ */
