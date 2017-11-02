
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

#define FS_CLIENT_MAX_NUM 		10 			/* �������10���ͻ���ͬʱ���� */

#define FS_FILE_SPLIT_SIZE 		1024 		/* �ļ��ָ��С����λ: �ֽ� */

#define FS_PATH_PERMISSION 		"/media" 	/* �ļ����������ڸ�·���� */
#define FS_PATH_PERMISSION_LEN 	6 			/* ����·���ĳ��� */


enum
{
    FS_INIT_ERR_LISTEN 		= -4, 		//����ʧ��
    FS_INIT_ERR_BIND 		= -3, 		//��ʧ��
    FS_INIT_ERR_SOCKOPT 	= -2, 		//�׽�����������ʧ��
    FS_INIT_ERR_SOCKET 		= -1, 		//�׽��ִ���ʧ��
    FS_INIT_SUCCEED 		= 0, 		//��ʼ���ɹ�
}; 	/* fs_server_init ����ֵ */

enum
{
    FS_HEAD_FLAG_WRONG 		= -3, 		//����ͷ��Ǵ���
    FS_HEAD_SIZE_WRONG 		= -2, 		//����ͷ��С����
    FS_HEAD_RECV_ERROR 		= -1, 		//���ճ���
    FS_HEAD_RECV_SHUTDOWN 	= 0, 		//�����ѶϿ�
}; 	/* fs_server_read_cmd_head ����ֵ */

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
