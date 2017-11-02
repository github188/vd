
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <linux/magic.h>
#include <stdlib.h>
 #include <fcntl.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "disk_func.h"

#include "partition_func.h"
#include "commonfuncs.h"
#include "data_process.h"


HDDInfo disk_hdd[HDD_NUM]; 		/* 硬盘信息 */
MMCInfo disk_mmc; 				/* SD卡信息 */

int 	cur_disk_num = -1; 		/* 当前磁盘号，-1表示SD卡，自然数表示hdd[i] */
int 	cur_partition_num = 1; 	/* 当前分区号，从0开始计数 */

static char cur_partition_path[MOUNT_POINT_LEN] = "/mnt/mmc";
/* 默认使用SD卡挂载路径 */

static char park_stroge_path[MOUNT_POINT_LEN] = "/media/park";
/*泊车断网的信息跟图片临时保存路径*/

static pthread_t 		disk_mng_thread;

static pthread_mutex_t 	disk_mng_mutex;
static pthread_cond_t 	disk_mng_cond;

static pthread_mutex_t 	cur_partition_mutex;


/*******************************************************************************
 * 函数名: disk_mng_create
 * 功  能: 创建磁盘分区管理任务线程
 * 返回值: 成功，返回0；失败，返回错误值
*******************************************************************************/
int disk_mng_create(void)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create(&disk_mng_thread, &attr, &disk_mng_tsk, NULL);

	pthread_attr_destroy(&attr);

	if (ret != 0)
	{
		log_error_storage("Create disk manage thread failed !!!\n");
	}

	return ret;
}


/*******************************************************************************
 * 函数名: disk_mng_tsk
 * 功  能: 磁盘分区管理任务线程
*******************************************************************************/
void *disk_mng_tsk(void *argv)
{
	disk_mng_init();

	disk_daemon();

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: disk_mng_init
 * 功  能: 初始化磁盘管理程序
*******************************************************************************/
void disk_mng_init(void)
{
	pthread_mutex_init(&disk_mng_mutex, NULL);
	pthread_cond_init(&disk_mng_cond, NULL);

	pthread_mutex_init(&cur_partition_mutex, NULL);

	do
	{
		memset(&disk_mmc, 0, sizeof(MMCInfo));

		sprintf(disk_mmc.disk_info.dev_name, "%s", MMC_DEV_NAME);
	}
	while (0);

	char hdd_dev_name[DISK_NAME_LEN] = HDD_DEV_NAME;
	/***************************************************************************
	 * 硬盘板上的两个SATA接口与对应的芯片接口是相反的
	 * 第一个SATA接口对应着芯片接口1
	 * 第二个SATA接口对应着芯片接口0
	 * 所以，初始化硬盘设备名称的时候需要特殊处理
	***************************************************************************/
	int i;
	for (i=(HDD_NUM-1); i>=0; i--)
	{
		memset(&disk_hdd[i], 0, sizeof(HDDInfo));

		sprintf(disk_hdd[i].disk_info.dev_name, "%s", hdd_dev_name);

		hdd_dev_name[HDD_DEV_NAME_DIFF_POS]++;
	}

	for (;;)
	{
		if (disk_status_init() < 0)
			u_sleep(DISK_DETECT_TIME, 0);
		else
		{
			log_debug_storage("Disk management initialize done !\n");
			return;
		}
	}
}


/*******************************************************************************
 * 函数名: mk_disk_info_dir
 * 功  能: 创建备份磁盘信息的文件夹
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int mk_disk_info_dir(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	char disk_info_path[32];
	sprintf(disk_info_path, "%s/%u", DISK_INFO_DIR, p_disk->disk_flag.id);

	if (mkpath(disk_info_path, 0755) < 0)
	{
		log_warn_storage("mkdir %s failed !\n", disk_info_path);
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: disk_daemon
 * 功  能: 磁盘管理守护线程，定时检查磁盘状态
*******************************************************************************/
#define DISK_DAEMON_SLEEP_TIME 3 	/* 等待的间隔时间 */

static int disk_daemon_flag = 0; 	/* 标识为0时，中断等待，进行磁盘状态检查 */

void disk_daemon(void)
{
	int i;

	disk_daemon_flag = 1;

	for (;;)
	{
		for (i=(DISK_CHECK_TIME/DISK_DAEMON_SLEEP_TIME);
		        ( (i > 0) && (disk_daemon_flag == 1) ); i--)
		{
			u_sleep(DISK_DAEMON_SLEEP_TIME, 0);
		}

		disk_status_check();
		disk_maintain();

		disk_daemon_flag = 1;
	}
}


/*******************************************************************************
 * 函数名: disk_check_now
 * 功  能: 通知程序立即检查磁盘
*******************************************************************************/
void disk_check_now(void)
{
	if (disk_daemon_flag == 1)
	{
		log_state_storage("Check disk status right now !\n");
		disk_daemon_flag = 0;
	}
}


/*******************************************************************************
 * 函数名: disk_cmd_mng
 * 功  能: 磁盘管理程序内部命令执行接口，用于创建新的线程来执行耗时很长的任务
 * 参  数: disk_cmd，磁盘操作命令指针
 * 返回值: 线程创建成功，返回0；失败，返回-1
*******************************************************************************/
int disk_cmd_mng(DiskCmd *disk_cmd)
{
	pthread_mutex_lock(&disk_mng_mutex);

	pthread_t disk_process_t;
	pthread_attr_t thread_attr;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create(&disk_process_t, &thread_attr,
	                         &disk_cmd_process, (void *)disk_cmd);
	if (ret == 0)
	{
		pthread_cond_wait(&disk_mng_cond, &disk_mng_mutex);
		/* 等待命令执行线程复制完参数，防止命令指针变化造成参数错误 */

		disk_cmd->cmd 			= -1;
		disk_cmd->disk_num 		= -2;
		disk_cmd->partition_num = -1;
		/* 清理命令参数，防止下次收到的命令参数设置不完整 */
	}

	pthread_attr_destroy(&thread_attr);

	pthread_mutex_unlock(&disk_mng_mutex);

	return ret;
}


/*******************************************************************************
 * 函数名: disk_cmd_process
 * 功  能: 磁盘管理命令执行线程
 * 参  数: disk_cmd，磁盘操作命令指针
*******************************************************************************/
void *disk_cmd_process(void *disk_cmd)
{
	int cmd 			= ((DiskCmd *)disk_cmd)->cmd;
	int disk_num 		= ((DiskCmd *)disk_cmd)->disk_num;
	int partition_num 	= ((DiskCmd *)disk_cmd)->partition_num;

	pthread_cond_signal(&disk_mng_cond);

	switch (cmd)
	{
	case DISK_CMD_FDISK:
	{
		log_debug_storage("Receive DISK_CMD_FDISK, disk : %d !\n", disk_num);
		disk_format(disk_num);
		break;
	}
	case DISK_CMD_FORMAT:
	{
		log_debug_storage("Receive command, format disk %d partition %d !\n",
		                  disk_num, partition_num);
		partition_format(disk_num, partition_num);
		break;
	}
	case DISK_CMD_CLEAR:
	{
		log_debug_storage("Receive command, clear disk %d partition %d !\n",
		                  disk_num, partition_num);
		partition_clear_trash(disk_num, partition_num);
		break;
	}
	case DISK_CMD_RESCURE:
	{
		log_debug_storage("Receive command, rescure disk %d partition %d !\n",
		                  disk_num, partition_num);
		partition_rescure(disk_num, partition_num);
		break;
	}
	default:
	{
		log_warn_storage("Unkown disk command: %d!\n", cmd);
		break;
	}
	}

	pthread_exit(NULL);
}


/*******************************************************************************
* 函数名: get_cur_partition_path
* 功  能: 获取当前用于存储的分区路径
* 参  数: path，路径（为NULL时，不设置）
* 返回值: 可用，返回0；不可用，返回-1
*******************************************************************************/
int get_cur_partition_path(char *path)
{
	int fd = 0;
	char lc_buf[128] = {0};
	int li_lens = 0;

	//判断设备是否挂载SD卡
	system("df | grep /mnt/mmc > /tmp/dev_sd.txt ");

	fd = open("/tmp/dev_sd.txt", O_RDONLY);
	if(fd < 0)
	{
		//TRACE_LOG_SYSTEM("Mount SD card is failed!");
	}

	li_lens = read(fd, lc_buf, sizeof(lc_buf));
	if(li_lens <= 0)
	{
		//TRACE_LOG_SYSTEM("Mount SD card is failed!");
	}
	else
	{
		//TRACE_LOG_SYSTEM("Mount SD = %s", lc_buf);

		//strcpy(path, cur_partition_path);
		strncpy(path, cur_partition_path, MOUNT_POINT_LEN);

		//TRACE_LOG_SYSTEM("Mount SD path = %s", path);

		close(fd);

		return 0;
	}

	/* 通知程序检查当前磁盘状态 */
	disk_check_now();

	close(fd);

	return -1;
}

/*******************************************************************************
* 函数名: get_park_enter_db_file_path
* 功  能: 获取当前用于存储泊车信息和图片的路径
* 参  数: path，路径（为NULL时，不设置）
* 返回值: 可用，返回0；不可用，返回-1
*******************************************************************************/

//说明: 因为泊车暂定不适用SD 卡，所以断网的存储暂时存放到tmp目录下

int get_park_storage_path(char *path)
{
	do
	{
		DIR *dirp = NULL;

		dirp = opendir(park_stroge_path);

		if (dirp == NULL)
		{
			dir_create(park_stroge_path);
		}
        else
        {
            closedir(dirp);
        }

		if (path != NULL)
			strncpy(path, park_stroge_path, strlen(park_stroge_path));

		printf("park_db_stroge_path:%s\n",path);

		return 0;
	}
	while (0);
	return -1;
}


/*******************************************************************************
* 函数名: set_cur_partition_path
* 功  能: 设置当前用于存储的分区路径
* 参  数: disk_num，磁盘号；parititon_num，分区号
* 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int set_cur_partition_path(int disk_num, int parititon_num)
{
	pthread_mutex_lock(&cur_partition_mutex);

	cur_disk_num 		= disk_num;
	cur_partition_num 	= parititon_num;

	int rc = mount_point_set(disk_num, parititon_num, cur_partition_path);

	pthread_mutex_unlock(&cur_partition_mutex);

	return rc;
}


/*******************************************************************************
* 函数名: disk_get_status
* 功  能: 获取当前用于存储的分区状态
* 返回值: 可用，返回0；不可用，返回-1
*******************************************************************************/
int disk_get_status(void)
{
	char lc_buf[128] = {0};
	return get_cur_partition_path(lc_buf);
}

/*******************************************************************************
* 函数名: db_path_park_get_status
* 功  能: 获取当前用于存储的分区状态
* 返回值: 可用，返回0；不可用，返回-1
*******************************************************************************/
/*
int db_record_path_park_get_status(void)
{
	return get_park_db_stroge_path(NULL);
}
*/




