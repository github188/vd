
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
 * ������: close_fs_client_socket
 * ��  ��: �رտͻ����׽���
 * ��  ��: client_num�����ӵĿͻ������
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
 * ������: fs_server_create
 * ��  ��: ���������ļ����������߳�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����
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
 * ������: fs_server_task
 * ��  ��: �����ļ����������߳�
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
 * ������: fs_server_init
 * ��  ��: ��ʼ�������ļ�����
 * ����ֵ: �ɹ���0
 *         �׽��ִ���ʧ��: -1
 *         �׽�����������ʧ��: -2
 *         ��ʧ��: -3
 *         ����ʧ��: -4
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
 * ������: fs_server_start
 * ��  ��: ���������ļ�����
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
 * ������: alloc_new_connection
 * ��  ��: �������ӵĿͻ��˷�����Դ
 * ��  ��: sockfd���׽����ļ���������addr����ַ
 * ����ֵ: ����ɹ������ط���ı�ţ�����ʧ�ܣ�����-1
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
 * ������: process_failed_new_connection
 * ��  ��: ����ʧ�ܵ��¿ͻ������ӣ����߿ͻ������������ﵽ����
 * ��  ��: sockfd���׽���������
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
 * ������: process_succeed_new_connection
 * ��  ��: ����ɹ����¿ͻ������ӣ����߿ͻ������ӳɹ��������������߳�
 * ��  ��: client_num���ͻ������
 * ����ֵ: �ɹ�������0��ʧ�ܣ������̴߳���ʧ�ܷ���ֵ
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
 * ������: fs_server_run
 * ��  ��: �����ļ��������̣߳��������ͻ��˵�����
 * ��  ��: �ͻ������ָ��
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
 * ������: fs_server_process_connection
 * ��  ��: ��������
 * ��  ��: client_num���ͻ������
 * ����ֵ:
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
 * ������: fs_server_read_cmd_head
 * ��  ��: ��ȡ����ͷ
 * ��  ��: client_num���ͻ�����ţ�cmd_head������ͷָ��
 * ����ֵ: �ɹ������ؽ��ܵ����ֽ���
 *         ���ӶϿ�������0
 *         recv��������-1
 *         ����ͷ��С���󣬷���-2
 *         ����ͷ��Ǵ��󣬷���-3
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
 * ������: process_wrong_head_connection
 * ��  ��: ������������ͷ��֪ͨ��λ����������
 * ��  ��: client_num���ͻ������
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
 * ������: fs_server_read_cmd_body
 * ��  ��: ��ȡ������
 * ��  ��: client_num���ͻ�����ţ�length�������峤�ȣ�cmd_body��������ָ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: fs_server_process_cmd
 * ��  ��: ������λ�����͵�����
 * ��  ��: client_num���ͻ�����ţ�cmd_head��������Ϣͷ��cmd_body��������Ϣ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: fs_server_unknown_cmd
 * ��  ��: ֪ͨ��λ�����֧��
 * ��  ��: client_num���ͻ�����ţ�reply_cmd�����ص���Ϣָ��
 * ����ֵ: �ɹ���Ӧ��0��ʧ�ܣ�-1
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
 * ������: fs_server_reply_status
 * ��  ��: ��Ӧ����״̬��Ϣ
 * ��  ��: client_num���ͻ�����ţ�cmd_head��������Ϣͷָ��
 * ����ֵ: �ɹ���Ӧ��0��ʧ�ܣ�-1
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
 * ������: fs_server_get_disk_status
 * ��  ��: ��ȡ����״̬��Ϣ
 * ��  ��: disk_status������״̬��Ϣ
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
 * ������: fs_server_reply_list
 * ��  ��: ��ӦĿ¼�б�����
 * ��  ��: client_num���ͻ�����ţ�dir_path������·��
 * ����ֵ: �ɹ���Ӧ��0��ʧ�ܣ�-1
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
 * ������: is_path_dir
 * ��  ��: �ж�·���Ƿ��ǿɽ�����ļ���
 * ��  ��: path��·������
 * ����ֵ: �ǣ�����1���񣬷���0�������ڻ�û�п�ִ��Ȩ�ޣ�����-1
*******************************************************************************/
int is_path_dir(const char *path)
{
	char resolved_path[PATH_MAX_LEN];

	/* ��ȡ��׼���ľ���·�� */
	if (realpath(path, resolved_path) == NULL)
		return -1;

	/* ��������Ȩ�޵�·����������� */
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
 * ������: thread_list_dir
 * ��  ��: Ŀ¼�б��߳�
 * ��  ��: ListArgv�ṹ��ָ��
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
 * ������: connect_client_server
 * ��  ��: ���ӿͻ��˵ķ���˿�
 * ��  ��: client_num���ͻ�����ţ�port���˿ں�
 * ����ֵ: �ɹ��������׽�����������ʧ�ܣ�����-1
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
 * ������: send_list
 * ��  ��: ����Ŀ¼�б�
 * ��  ��: list_socket���׽��֣�cmd_head������ͷ��path������·��
 * ����ֵ: ִ�гɹ�������0��ִ��ʧ�ܣ�����-1
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
 * ������: fs_server_reply_download
 * ��  ��: ��Ӧ�ļ���������
 * ��  ��: client_num���ͻ�����ţ�file_path���ļ��ľ���·��
 * ����ֵ: �ɹ���Ӧ��0��ʧ�ܣ�-1
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
 * ������: is_path_regular_file
 * ��  ��: �ж�·���Ƿ��ǿɶ��ļ�
 * ��  ��: file_path���ļ��ľ���·��
 * ����ֵ: �ǣ�����1���񣬷���0�������ڻ�û�пɶ�Ȩ�ޣ�����-1
*******************************************************************************/
int is_path_regular_file(const char *file_path)
{
	char resolved_path[PATH_MAX_LEN];

	/* ��ȡ��׼���ľ���·�� */
	if (realpath(file_path, resolved_path) == NULL)
		return -1;

	/* ��������Ȩ�޵�·����������� */
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
 * ������: thread_download_file
 * ��  ��: Ŀ¼�б��߳�
 * ��  ��: DownloadArgv�ṹ��ָ��
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
 * ������: send_file
 * ��  ��: �����ļ�
 * ��  ��: download_socket���׽��֣�file_path���ļ��ľ���·����cmd_head������ͷ
 * ����ֵ: ִ�гɹ�������0��ִ��ʧ�ܣ�����-1
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
				u_sleep(0, 2000); 	/* ÿ200�������ȴ�2���� */
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
 * ������: fs_server_delete
 * ��  ��: ɾ�������ļ����������߳�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����
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
 * ������: fs_server_destory
 * ��  ��: ���ٴ����ļ�����
 * ����ֵ: �ɹ�������0
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
 * ������: fs_server_stop
 * ��  ��: ֹͣ�����ļ�����
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
