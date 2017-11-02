
#include "storage_common.h"
#include "file_system_server.h"


static pthread_t 			fs_server_thread;

static int 					fs_server_run_flag;

static int 					fs_server_socket;

static pthread_mutex_t 		fs_server_mutex;
static pthread_cond_t 		fs_server_cond;

static int 					fs_client_socket[FS_CLIENT_MAX_NUM];
static struct sockaddr_in 	fs_client_addr[FS_CLIENT_MAX_NUM];

static pthread_mutex_t 		fs_client_mutex[FS_CLIENT_MAX_NUM];
static pthread_cond_t 		fs_client_cond[FS_CLIENT_MAX_NUM];


/*******************************************************************************
 * 函数名: close_fs_client_socket
 * 功  能: 关闭客户端套接字
 * 参  数: client_num，连接的客户端序号
*******************************************************************************/
void close_fs_client_socket(int client_num)
{
	if (fs_client_socket[client_num] > 2)
	{
		close(fs_client_socket[client_num]);
		fs_client_socket[client_num] = -1;
		log_debug_storage("Close client socket %d.\n", client_num);
	}
}


/*******************************************************************************
 * 函数名: fs_server_create
 * 功  能: 创建磁盘文件服务任务线程
 * 返回值: 成功，返回0；失败，其他
*******************************************************************************/
int fs_server_create(void)
{
	pthread_attr_t thread_attr;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create(&fs_server_thread, &thread_attr,
	                         &fs_server_task, NULL);

	pthread_attr_destroy(&thread_attr);

	if (ret != 0)
	{
		log_error_storage("Create fs server thread failed !!!\n");
	}

	return ret;
}


/*******************************************************************************
 * 函数名: fs_server_task
 * 功  能: 磁盘文件服务任务线程
*******************************************************************************/
void *fs_server_task(void *argv)
{
	int ret = fs_server_init();
	if (ret < 0)
	{
		log_error_storage("fs server initialize failed ! return %d.\n", ret);
		pthread_exit(NULL);
	}

	fs_server_start();

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: fs_server_init
 * 功  能: 初始化磁盘文件服务
 * 返回值: 成功，0
 *         套接字创建失败: -1
 *         套接字属性设置失败: -2
 *         绑定失败: -3
 *         侦听失败: -4
*******************************************************************************/
int fs_server_init(void)
{
	int i;
	struct sockaddr_in server_addr;
	int optval = 1;

	for (i=0; i<FS_CLIENT_MAX_NUM; i++)
	{
		fs_client_socket[i] = -1;
		pthread_mutex_init(&fs_client_mutex[i], NULL);
		pthread_cond_init(&fs_client_cond[i], NULL);
	}

	pthread_mutex_init(&fs_server_mutex, NULL);
	pthread_cond_init(&fs_server_cond, NULL);

	fs_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (fs_server_socket < 0)
	{
		log_warn_storage("fs server socket create failed : %m !\n");
		return FS_INIT_ERR_SOCKET;
	}

	if (setsockopt(fs_server_socket, SOL_SOCKET, SO_REUSEADDR,
	               &optval, sizeof(optval)) < 0)
	{
		log_warn_storage("set options on fs server socket failed : %m !\n");
		close(fs_server_socket);
		return FS_INIT_ERR_SOCKOPT;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(FS_SERVER_PORT);

	if (bind(fs_server_socket, (struct sockaddr*)&server_addr,
	         sizeof(server_addr)) < 0)
	{
		log_warn_storage("fs server socket bind failed : %m !\n");
		close(fs_server_socket);
		return FS_INIT_ERR_BIND;
	}

	if (listen(fs_server_socket, FS_SERVER_BACKLOG) < 0)
	{
		log_warn_storage("fs server socket listen failed : %m !\n");
		close(fs_server_socket);
		return FS_INIT_ERR_LISTEN;
	}

	return FS_INIT_SUCCEED;
}


/*******************************************************************************
 * 函数名: fs_server_start
 * 功  能: 启动磁盘文件服务
*******************************************************************************/
void fs_server_start(void)
{
	int client_socket;
	int client_num;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	fs_server_run_flag = 1;

	while (fs_server_run_flag)
	{
		client_socket =
		    accept(fs_server_socket, (struct sockaddr*)&client_addr, &addrlen);
		if (client_socket > 0)
		{
			log_debug_storage("new client connected !\n");

			client_num =
			    alloc_new_connection(client_socket, &client_addr);
			if (client_num < 0)
			{
				process_failed_new_connection(client_socket);
			}
			else
			{
				process_succeed_new_connection(client_num);
			}
		}
	}
}


/*******************************************************************************
 * 函数名: alloc_new_connection
 * 功  能: 给新连接的客户端分配资源
 * 参  数: sockfd，套接字文件描述符；addr，地址
 * 返回值: 分配成功，返回分配的编号；分配失败，返回-1
*******************************************************************************/
int alloc_new_connection(int sockfd, struct sockaddr_in *addr)
{
	int i;

	for (i=0; i<FS_CLIENT_MAX_NUM; i++)
	{
		if (fs_client_socket[i] == -1)
		{
			fs_client_socket[i] = sockfd;
			memcpy(&fs_client_addr[i], addr, sizeof(struct sockaddr_in));
			log_debug_storage("Allocate client %d to the new connection.\n", i);
			return i;
		}
	}

	log_state_storage("Maximum number of client connections "
	                  "has been reached !\n");

	return -1;
}


/*******************************************************************************
 * 函数名: process_failed_new_connection
 * 功  能: 处理失败的新客户端连接，告诉客户端连接数量达到上限
 * 参  数: sockfd，套接字描述符
*******************************************************************************/
void process_failed_new_connection(int sockfd)
{
	FileServCmdHead cmd_head;

	memset(&cmd_head, 0, sizeof(FileServCmdHead));
	cmd_head.flag 	= FS_CMD_HEAD_FLAG;
	cmd_head.reply 	= RE_FS_CMD_MAX_CONNECT;

	if (send(sockfd, &cmd_head, sizeof(FileServCmdHead), 0) < 0)
	{
		log_state_storage("process failed new connection send : %m !\n");
	}

	close(sockfd);
}


/*******************************************************************************
 * 函数名: process_succeed_new_connection
 * 功  能: 处理成功的新客户端连接，告诉客户端连接成功，创建服务子线程
 * 参  数: client_num，客户端序号
 * 返回值: 成功，返回0；失败，返回线程创建失败返回值
*******************************************************************************/
int process_succeed_new_connection(int client_num)
{
	FileServCmdHead cmd_head;
	int ret;
	int socket_fd = fs_client_socket[client_num];

	memset(&cmd_head, 0, sizeof(FileServCmdHead));
	cmd_head.flag 	= FS_CMD_HEAD_FLAG;
	cmd_head.reply 	= RE_FS_CMD_CONN_SUCCEED;

	struct timeval timeout = { 600, 0 };
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
	           &timeout, sizeof(struct timeval));

	ret = send(socket_fd, &cmd_head, sizeof(FileServCmdHead), 0);
	if (ret < 0)
	{
		log_state_storage("Send client %d succeed connect message failed : "
		                  "%m !\n", client_num);
		close_fs_client_socket(client_num);
		return ret;
	}

	pthread_mutex_lock(&fs_server_mutex);

	pthread_t thread_run;
	pthread_attr_t thread_attr;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&thread_run, &thread_attr,
	                     &fs_server_run, (void *)&client_num);
	if (ret == 0)
	{
		pthread_cond_wait(&fs_server_cond, &fs_server_mutex);
	}
	else
	{
		close_fs_client_socket(client_num);
		log_error_storage("Create fs server run thread failed !\n");
	}

	pthread_attr_destroy(&thread_attr);

	pthread_mutex_unlock(&fs_server_mutex);

	return ret;
}


/*******************************************************************************
 * 函数名: fs_server_run
 * 功  能: 磁盘文件服务子线程，处理单个客户端的连接
 * 参  数: 客户端序号指针
*******************************************************************************/
void *fs_server_run(void *argv)
{
	int client_num = *(int *)argv;
	pthread_cond_signal(&fs_server_cond);

	while (fs_server_run_flag)
	{
		if (fs_server_process_connection(client_num) < 0)
		{
			log_warn_storage("fs server process connection failed !\n");

			close_fs_client_socket(client_num);

			pthread_exit(NULL);
		}
	}

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: fs_server_process_connection
 * 功  能: 处理连接
 * 参  数: client_num，客户端序号
 * 返回值:
*******************************************************************************/
int fs_server_process_connection(int client_num)
{
	FileServCmdHead cmd_head;
	void *cmd_body = NULL;

	switch (fs_server_read_cmd_head(client_num, &cmd_head))
	{
	case FS_HEAD_RECV_SHUTDOWN:
	case FS_HEAD_RECV_ERROR:
		log_state_storage("fs server read command head failed !\n");
		return -1;
	case FS_HEAD_SIZE_WRONG:
	case FS_HEAD_FLAG_WRONG:
		process_wrong_head_connection(client_num, &cmd_head);
		return -1;
	default:
		break;
	}

	if (cmd_head.body_length > 0)
	{
		cmd_body = malloc(cmd_head.body_length + 1);
		if (cmd_body == NULL)
		{
			log_warn_storage("Allocate memory for cmd_body failed !\n");
			return -2;
		}

		memset(cmd_body, 0, cmd_head.body_length + 1);

		if (fs_server_read_cmd_body(client_num, cmd_head.body_length,
		                            cmd_body) < 0)
		{
			log_warn_storage("fs server read command body failed !\n");
			safe_free(cmd_body);
			return -2;
		}
	}

	fs_server_process_cmd(client_num, &cmd_head, cmd_body);

	safe_free(cmd_body);

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_read_cmd_head
 * 功  能: 读取命令头
 * 参  数: client_num，客户端序号；cmd_head，命令头指针
 * 返回值: 成功，返回接受到的字节数
 *         连接断开，返回0
 *         recv出错，返回-1
 *         命令头大小错误，返回-2
 *         命令头标记错误，返回-3
*******************************************************************************/
int fs_server_read_cmd_head(int client_num, FileServCmdHead *cmd_head)
{
	int ret = 0;
	int socket_fd = fs_client_socket[client_num];

	ret = recv(socket_fd, cmd_head, sizeof(FileServCmdHead), 0);
	if (ret < 0)
	{
		log_state_storage("Receive client %d socket failed : %m !\n",
		                  client_num);
		return FS_HEAD_RECV_ERROR;
	}

	if (ret == 0)
	{
		log_state_storage("Can't connect client %d !\n", client_num);
		return FS_HEAD_RECV_SHUTDOWN;
	}

	if ((unsigned)ret < sizeof(FileServCmdHead))
	{
		log_state_storage("Wrong command head size : %d. Maybe not a client.\n",
		                  ret);
		return FS_HEAD_SIZE_WRONG;
	}

	if (cmd_head->flag != FS_CMD_HEAD_FLAG)
	{
		log_state_storage("Command head flag is wrong : %llx. "
		                  "Maybe not a client.\n",
		                  cmd_head->flag);
		return FS_HEAD_FLAG_WRONG;
	}

	return ret;
}


/*******************************************************************************
 * 函数名: process_wrong_head_connection
 * 功  能: 处理错误的命令头，通知上位机错误类型
 * 参  数: client_num，客户端序号
*******************************************************************************/
void process_wrong_head_connection(int client_num, FileServCmdHead *cmd_head)
{
	int socket_fd = fs_client_socket[client_num];
	FileServCmdHead reply_cmd;
	memcpy(&reply_cmd, cmd_head, sizeof(FileServCmdHead));

	cmd_head->reply 		= RE_FS_CMD_HEAD_ERROR;
	cmd_head->body_length 	= 0;

	if (send(socket_fd, &cmd_head, sizeof(FileServCmdHead), 0) < 0)
	{
		log_state_storage("process wrong head connection send : %m !\n");
	}
}


/*******************************************************************************
 * 函数名: fs_server_read_cmd_body
 * 功  能: 读取命令体
 * 参  数: client_num，客户端序号；length，命令体长度；cmd_body，命令体指针
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int fs_server_read_cmd_body(int client_num, size_t length, void *cmd_body)
{
	int socket_fd = fs_client_socket[client_num];

	if (recv(socket_fd, cmd_body, length, 0) <= 0)
	{
		log_state_storage("Disconnected the client !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_process_cmd
 * 功  能: 处理上位机发送的命令
 * 参  数: client_num，客户端序号；cmd_head，命令消息头；cmd_body，命令消息体
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int fs_server_process_cmd(int client_num,
                          FileServCmdHead *cmd_head, void *cmd_body)
{
	int ret = 0;

	switch (cmd_head->cmd)
	{
	case FS_CMD_GET_STATUS:
		log_debug_storage("Received command: FS_CMD_GET_STATUS\n");
		ret = fs_server_reply_status(client_num, cmd_head);
		break;
	case FS_CMD_LIST_DIR:
		log_debug_storage("Received command: FS_CMD_LIST_DIR\n");
		ret = fs_server_reply_list(client_num, cmd_head, (char *)cmd_body);
		break;
	case FS_CMD_DOWNLOAD_FILE:
		log_debug_storage("Received command: FS_CMD_DOWNLOAD_FILE\n");
		ret = fs_server_reply_download(client_num, cmd_head, (char *)cmd_body);
		break;
	case FS_CMD_MOUNT_HDD:
		break;
	default:
		log_state_storage("Received unknown command: %d\n", cmd_head->cmd);
		ret = fs_server_unknown_cmd(client_num, cmd_head);
		break;
	}

	return ret;
}


/*******************************************************************************
 * 函数名: fs_server_unknown_cmd
 * 功  能: 通知上位机命令不支持
 * 参  数: client_num，客户端序号；reply_cmd，返回的消息指针
 * 返回值: 成功回应，0；失败，-1
*******************************************************************************/
int fs_server_unknown_cmd(int client_num, FileServCmdHead *cmd_head)
{
	int socket_fd = fs_client_socket[client_num];
	FileServCmdHead reply_cmd;
	memcpy(&reply_cmd, cmd_head, sizeof(FileServCmdHead));

	reply_cmd.reply = RE_FS_CMD_NOT_SUPPORT;
	reply_cmd.body_length = 0;

	if (send(socket_fd, &reply_cmd, sizeof(FileServCmdHead), 0) < 0)
	{
		log_state_storage("unknown command send : %m !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_reply_status
 * 功  能: 回应磁盘状态信息
 * 参  数: client_num，客户端序号；cmd_head，命令消息头指针
 * 返回值: 成功回应，0；失败，-1
*******************************************************************************/
int fs_server_reply_status(int client_num, FileServCmdHead *cmd_head)
{
	int socket_fd = fs_client_socket[client_num];
	FileServCmdHead reply_cmd;
	memcpy(&reply_cmd, cmd_head, sizeof(FileServCmdHead));
	reply_cmd.body_length = sizeof(FileServDiskStatus);

	FileServDiskStatus disk_status;
	fs_server_get_disk_status(&disk_status);

	if (send(socket_fd, &reply_cmd, sizeof(FileServCmdHead), 0) !=
	        sizeof(FileServCmdHead))
	{
		log_state_storage("reply status head send: %m !\n");
		return -1;
	}

	if (send(socket_fd, &disk_status, sizeof(FileServDiskStatus), 0) !=
	        sizeof(FileServDiskStatus))
	{
		log_state_storage("reply status body send: %m !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_get_disk_status
 * 功  能: 获取磁盘状态信息
 * 参  数: disk_status，磁盘状态信息
*******************************************************************************/
void fs_server_get_disk_status(FileServDiskStatus *disk_status)
{
	int i,j;

	memset(disk_status, 0, sizeof(FileServDiskStatus));

	do
	{
		disk_status->mmc_info.disk_info.disk_id
		    = disk_mmc.disk_info.disk_flag.id;
		disk_status->mmc_info.disk_info.is_detected
		    = disk_mmc.disk_info.is_detected;
		disk_status->mmc_info.disk_info.is_usable
		    = disk_mmc.disk_info.is_usable;
		disk_status->mmc_info.disk_info.size_in_byte
		    = disk_mmc.disk_info.size_in_byte;

		for (j=0; j<MMC_PARTITION_NUM; j++)
		{
			disk_status->mmc_info.partition_info[j].percent_used
			    = disk_mmc.partition_info[j].percent_used;
			disk_status->mmc_info.partition_info[j].status
			    = disk_mmc.partition_info[j].status;
		}
	}
	while (0);

	for (i=0; i<HDD_NUM; i++)
	{
		disk_status->hdd_info[i].disk_info.disk_id
		    = disk_hdd[i].disk_info.disk_flag.id;
		disk_status->hdd_info[i].disk_info.is_detected
		    = disk_hdd[i].disk_info.is_detected;
		disk_status->hdd_info[i].disk_info.is_usable
		    = disk_hdd[i].disk_info.is_usable;
		disk_status->hdd_info[i].disk_info.size_in_byte
		    = disk_hdd[i].disk_info.size_in_byte;

		for (j=0; j<HDD_PARTITION_NUM; j++)
		{
			disk_status->hdd_info[i].partition_info[j].percent_used
			    = disk_hdd[i].partition_info[j].percent_used;
			disk_status->hdd_info[i].partition_info[j].status
			    = disk_hdd[i].partition_info[j].status;
		}
	}
}


/*******************************************************************************
 * 函数名: fs_server_reply_list
 * 功  能: 回应目录列表命令
 * 参  数: client_num，客户端序号；dir_path，绝对路径
 * 返回值: 成功回应，0；失败，-1
*******************************************************************************/
int fs_server_reply_list(int client_num,
                         FileServCmdHead *cmd_head, char *dir_path)
{
	int socket_fd = fs_client_socket[client_num];
	FileServCmdHead reply_cmd;
	memcpy(&reply_cmd, cmd_head, sizeof(FileServCmdHead));
	reply_cmd.body_length = 0;

	if (is_path_dir(dir_path) > 0)
	{
		ListArgv list_argv;
		list_argv.client_num 	= client_num;
		list_argv.cmd_head 		= cmd_head;
		list_argv.cmd_body 		= dir_path;

		pthread_mutex_lock(&(fs_client_mutex[client_num]));

		pthread_t thread_list;
		pthread_attr_t thread_attr;

		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

		if (0 == pthread_create(&thread_list, &thread_attr,
		                        &thread_list_dir, (void *)&list_argv))
		{
			reply_cmd.reply = RE_FS_CMD_RECV_SUCCEED;
			pthread_cond_wait(&(fs_client_cond[client_num]),
			                  &(fs_client_mutex[client_num]));
		}
		else
		{
			log_error_storage("create thread_list_dir failed !\n");
			reply_cmd.reply = RE_FS_CMD_HANDLE_ERROR;
		}

		pthread_attr_destroy(&thread_attr);

		pthread_mutex_unlock(&(fs_client_mutex[client_num]));
	}
	else
	{
		log_debug_storage("%s may not be a accessible directory !\n", dir_path);
		reply_cmd.reply = RE_FS_CMD_DATA_ERROR;
	}

	if (send(socket_fd, &reply_cmd, sizeof(FileServCmdHead), 0) !=
	        sizeof(FileServCmdHead))
	{
		log_state_storage("reply list send: %m !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: is_path_dir
 * 功  能: 判断路径是否是可进入的文件夹
 * 参  数: path，路径名称
 * 返回值: 是，返回1；否，返回0；不存在或没有可执行权限，返回-1
*******************************************************************************/
int is_path_dir(const char *path)
{
	char resolved_path[PATH_MAX_LEN];

	/* 获取标准化的绝对路径 */
	if (realpath(path, resolved_path) == NULL)
		return -1;

	/* 超出访问权限的路径不允许进入 */
	if (strncmp(resolved_path, FS_PATH_PERMISSION, FS_PATH_PERMISSION_LEN) != 0)
		return -1;

	if (access(path, X_OK) < 0)
		return -1;

	struct stat dstat;

	if (stat(path, &dstat) < 0)
	{
		log_debug_storage("stat %s failed : %m !\n", path);
		return -1;
	}

	return S_ISDIR(dstat.st_mode);
}


/*******************************************************************************
 * 函数名: thread_list_dir
 * 功  能: 目录列表线程
 * 参  数: ListArgv结构体指针
*******************************************************************************/
void *thread_list_dir(void *argv)
{
	ListArgv *p_list_argv = (ListArgv *)argv;
	int client_num = p_list_argv->client_num;

	FileServCmdHead cmd_head;
	memcpy(&cmd_head, p_list_argv->cmd_head, sizeof(FileServCmdHead));

	char *path = strdup(p_list_argv->cmd_body);
	if (path == NULL)
	{
		log_warn_storage("Duplicate list argv %s failed !\n",
		                 p_list_argv->cmd_body);
		pthread_cond_signal(&(fs_client_cond[client_num]));
		pthread_exit(NULL);
	}

	pthread_cond_signal(&(fs_client_cond[client_num]));

	int list_socket = connect_client_server(client_num, FS_LIST_PORT);
	if (list_socket > 0)
	{
		send_list(list_socket, &cmd_head, path);
	}

	close(list_socket);
	safe_free(path);

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: connect_client_server
 * 功  能: 连接客户端的服务端口
 * 参  数: client_num，客户端序号；port，端口号
 * 返回值: 成功，返回套接字描述符；失败，返回-1
*******************************************************************************/
int connect_client_server(int client_num, int port)
{
	struct sockaddr_in list_server_addr;
	memcpy(&list_server_addr, &(fs_client_addr[client_num]),
	       sizeof(struct sockaddr_in));
	list_server_addr.sin_port = htons(port);

	int list_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (0 == connect(list_socket, (struct sockaddr*)&list_server_addr,
	                 sizeof(struct sockaddr)))
		return list_socket;

	log_state_storage("Connect server %x failed !\n",
	                  list_server_addr.sin_addr.s_addr);

	return -1;
}


/*******************************************************************************
 * 函数名: send_list
 * 功  能: 发送目录列表
 * 参  数: list_socket，套接字；cmd_head，命令头；path，绝对路径
 * 返回值: 执行成功，返回0；执行失败，返回-1
*******************************************************************************/
int send_list(int list_socket, FileServCmdHead *cmd_head, char *path)
{
	DIR *dirp = NULL;
	struct dirent entry;
	struct dirent *result = NULL;
	struct dnode node;
	void *full_name = NULL;

	do
	{
		dirp = opendir(path);
		if (dirp == NULL)
		{
			log_debug_storage("Open %s failed : %m !", path);
			cmd_head->reply 	= RE_FS_CMD_HANDLE_ERROR;
			break;
		}

		full_name = malloc(cmd_head->body_length + NAME_MAX_LEN + 4);
		if (full_name == NULL)
		{
			log_warn_storage("Allocate memory for full_name failed !\n");
			cmd_head->reply 	= RE_FS_CMD_MALLOC_ERROR;
			break;
		}

		cmd_head->reply 		= RE_FS_CMD_HANDLE_SUCCEED;
		cmd_head->body_length 	= sizeof(struct dnode);

		for(;;)
		{
			readdir_r(dirp, &entry, &result);
			if (result == NULL)
				break;

			sprintf(node.dname, "%s", entry.d_name);
			sprintf((char *)full_name, "%s/%s", path, entry.d_name);

			stat((char *)full_name, &(node.dstat));

			if (send(list_socket, cmd_head, sizeof(FileServCmdHead), 0) < 0)
			{
				log_state_storage("send list command head error : %m !\n");
				break;
			}

			if (send(list_socket, &node, sizeof(struct dnode), 0) < 0)
			{
				log_state_storage("send list entry error : %m !\n");
				break;
			}
		}

		safe_free(full_name);

		closedir(dirp);

		return 0;
	}
	while (0);

	cmd_head->body_length 	= 0;
	if (send(list_socket, cmd_head, sizeof(FileServCmdHead), 0) < 0)
	{
		log_state_storage("send list command head : %m !\n");
	}

	return -1;
}


/*******************************************************************************
 * 函数名: fs_server_reply_download
 * 功  能: 回应文件下载命令
 * 参  数: client_num，客户端序号；file_path，文件的绝对路径
 * 返回值: 成功回应，0；失败，-1
*******************************************************************************/
int fs_server_reply_download(int client_num,
                             FileServCmdHead *cmd_head, char *file_path)
{
	int socket_fd = fs_client_socket[client_num];
	FileServCmdHead reply_cmd;
	memcpy(&reply_cmd, cmd_head, sizeof(FileServCmdHead));
	reply_cmd.body_length = 0;

	if (is_path_regular_file(file_path) > 0)
	{
		DownloadArgv download_argv;
		download_argv.client_num 	= client_num;
		download_argv.cmd_head 		= cmd_head;
		download_argv.cmd_body 		= file_path;

		pthread_mutex_lock(&(fs_client_mutex[client_num]));

		pthread_t thread_download;
		pthread_attr_t thread_attr;

		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

		if (0 == pthread_create(&thread_download, &thread_attr,
		                        &thread_download_file, (void *)&download_argv))
		{
			reply_cmd.reply = RE_FS_CMD_RECV_SUCCEED;
			pthread_cond_wait(&(fs_client_cond[client_num]),
			                  &(fs_client_mutex[client_num]));
		}
		else
		{
			log_error_storage("create thread_download_file failed !\n");
			reply_cmd.reply = RE_FS_CMD_HANDLE_ERROR;
		}

		pthread_attr_destroy(&thread_attr);

		pthread_mutex_unlock(&(fs_client_mutex[client_num]));
	}
	else
	{
		log_debug_storage("%s may not be a accessible file !\n", file_path);
		reply_cmd.reply = RE_FS_CMD_DATA_ERROR;
	}

	if (send(socket_fd, &reply_cmd, sizeof(FileServCmdHead), 0) !=
	        sizeof(FileServCmdHead))
	{
		log_state_storage("reply download send: %m !\n");
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: is_path_regular_file
 * 功  能: 判断路径是否是可读文件
 * 参  数: file_path，文件的绝对路径
 * 返回值: 是，返回1；否，返回0；不存在或没有可读权限，返回-1
*******************************************************************************/
int is_path_regular_file(const char *file_path)
{
	char resolved_path[PATH_MAX_LEN];

	/* 获取标准化的绝对路径 */
	if (realpath(file_path, resolved_path) == NULL)
		return -1;

	/* 超出访问权限的路径不允许进入 */
	if (strncmp(resolved_path, FS_PATH_PERMISSION, FS_PATH_PERMISSION_LEN) != 0)
		return -1;

	if (access(file_path, R_OK) < 0)
		return -1;

	struct stat dstat;

	if (stat(file_path, &dstat) < 0)
	{
		log_debug_storage("stat %s failed : %m !\n", file_path);
		return -1;
	}

	return S_ISREG(dstat.st_mode);
}


/*******************************************************************************
 * 函数名: thread_download_file
 * 功  能: 目录列表线程
 * 参  数: DownloadArgv结构体指针
*******************************************************************************/
void *thread_download_file(void *argv)
{
	DownloadArgv *p_download_argv = (DownloadArgv *)argv;
	int client_num = p_download_argv->client_num;

	FileServCmdHead cmd_head;
	memcpy(&cmd_head, p_download_argv->cmd_head, sizeof(FileServCmdHead));

	char *file_path = strdup(p_download_argv->cmd_body);
	if (file_path == NULL)
	{
		log_warn_storage("Duplicate download argv %s failed !\n",
		                 p_download_argv->cmd_body);
		pthread_cond_signal(&(fs_client_cond[client_num]));
		pthread_exit(NULL);
	}

	pthread_cond_signal(&(fs_client_cond[client_num]));

	int download_socket = connect_client_server(client_num, FS_DOWNLOAD_PORT);
	if (download_socket > 0)
	{
		send_file(download_socket, &cmd_head, file_path);
	}

	close(download_socket);
	safe_free(file_path);

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: send_file
 * 功  能: 发送文件
 * 参  数: download_socket，套接字；file_path，文件的绝对路径；cmd_head，命令头
 * 返回值: 执行成功，返回0；执行失败，返回-1
*******************************************************************************/
int send_file(int download_socket, FileServCmdHead *cmd_head, char *file_path)
{
	int ret = -1;
	struct stat dstat;
	stat(file_path, &dstat);
	off_t file_size = dstat.st_size;

	FILE *file = NULL;
	file = fopen(file_path, "rb");
	if (file == NULL)
	{
		log_warn_storage("Open %s failed : %m !\n", file_path);

		cmd_head->reply 		= RE_FS_CMD_HANDLE_ERROR;
		cmd_head->body_length 	= 0;
		if (send(download_socket, cmd_head, sizeof(FileServCmdHead), 0) < 0)
		{
			log_state_storage("send download command head error: %m !\n");
		}

		return ret;
	}

	do
	{
		ret = 0;

		cmd_head->reply 		= RE_FS_CMD_HANDLE_SUCCEED;
		cmd_head->body_length 	= file_size;
		if (send(download_socket, cmd_head, sizeof(FileServCmdHead), 0) < 0)
		{
			log_state_storage("send download command head error : %m !\n");
			ret = -1;
			break;
		}

		unsigned int split_count 	= file_size / FS_FILE_SPLIT_SIZE;
		size_t file_tail_size 		= file_size % FS_FILE_SPLIT_SIZE;
		char buffer[FS_FILE_SPLIT_SIZE];

		for (; split_count>0; split_count--)
		{
			if (1 != fread(buffer, FS_FILE_SPLIT_SIZE, 1, file))
			{
				log_warn_storage("fread file %s error : %m !\n", file_path);
				ret = -1;
				break;
			}

			if (send(download_socket, buffer, FS_FILE_SPLIT_SIZE, 0) < 0)
			{
				log_state_storage("send file %s error : %m !\n", file_path);
				ret = -1;
				break;
			}

			if ((split_count % 200) == 0)
			{
				u_sleep(0, 2000); 	/* 每200个包，等待2毫秒 */
			}
		}

		if ( (file_tail_size > 0) && (ret == 0) )
		{
			if (1 != fread(buffer, file_tail_size, 1, file))
			{
				log_warn_storage("fread file %s error : %m !\n", file_path);
				ret = -1;
				break;
			}

			if (send(download_socket, buffer, file_tail_size, 0) < 0)
			{
				log_state_storage("send file %s error : %m !\n", file_path);
				ret = -1;
				break;
			}
		}
	}
	while (0);

	fclose(file);

	return ret;
}


/*******************************************************************************
 * 函数名: fs_server_delete
 * 功  能: 删除磁盘文件服务任务线程
 * 返回值: 成功，返回0；失败，其他
*******************************************************************************/
int fs_server_delete(void)
{
	log_debug_storage("Delete file system server now.\n");

	fs_server_destory();

	pthread_cancel(fs_server_thread);

	log_debug_storage("Delete file system server done.\n");

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_destory
 * 功  能: 销毁磁盘文件服务
 * 返回值: 成功，返回0
*******************************************************************************/
int fs_server_destory(void)
{
	log_debug_storage("Destory file system server now.\n");

	fs_server_stop();

	if (fs_server_socket > 2)
	{
		close(fs_server_socket);
		fs_server_socket = -1;
	}

	pthread_cond_signal(&fs_server_cond);
	pthread_cond_destroy(&fs_server_cond);

	pthread_mutex_trylock(&fs_server_mutex);
	pthread_mutex_unlock(&fs_server_mutex);
	pthread_mutex_destroy(&fs_server_mutex);

	log_debug_storage("Destory file system server done.\n");

	return 0;
}


/*******************************************************************************
 * 函数名: fs_server_stop
 * 功  能: 停止磁盘文件服务
*******************************************************************************/
void fs_server_stop(void)
{
	log_debug_storage("Stop file system server now.\n");

	fs_server_run_flag = 0;

	int i;
	for (i=0; i<FS_CLIENT_MAX_NUM; i++)
	{
		close_fs_client_socket(i);

		pthread_cond_signal(&fs_client_cond[i]);
		pthread_cond_destroy(&fs_client_cond[i]);

		pthread_mutex_trylock(&fs_client_mutex[i]);
		pthread_mutex_unlock(&fs_client_mutex[i]);
		pthread_mutex_destroy(&fs_client_mutex[i]);
	}

	log_debug_storage("Stop file system server done.\n");
}
