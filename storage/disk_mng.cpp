
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


HDDInfo disk_hdd[HDD_NUM]; 		/* Ӳ����Ϣ */
MMCInfo disk_mmc; 				/* SD����Ϣ */

int 	cur_disk_num = -1; 		/* ��ǰ���̺ţ�-1��ʾSD������Ȼ����ʾhdd[i] */
int 	cur_partition_num = 1; 	/* ��ǰ�����ţ���0��ʼ���� */

static char cur_partition_path[MOUNT_POINT_LEN] = "/mnt/mmc";
/* Ĭ��ʹ��SD������·�� */

static char park_stroge_path[MOUNT_POINT_LEN] = "/media/park";
/*������������Ϣ��ͼƬ��ʱ����·��*/

static pthread_t 		disk_mng_thread;

static pthread_mutex_t 	disk_mng_mutex;
static pthread_cond_t 	disk_mng_cond;

static pthread_mutex_t 	cur_partition_mutex;


/*******************************************************************************
 * ������: disk_mng_create
 * ��  ��: �������̷������������߳�
 * ����ֵ: �ɹ�������0��ʧ�ܣ����ش���ֵ
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
 * ������: disk_mng_tsk
 * ��  ��: ���̷������������߳�
*******************************************************************************/
void *disk_mng_tsk(void *argv)
{
	disk_mng_init();

	disk_daemon();

	pthread_exit(NULL);
}


/*******************************************************************************
 * ������: disk_mng_init
 * ��  ��: ��ʼ�����̹������
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
	 * Ӳ�̰��ϵ�����SATA�ӿ����Ӧ��оƬ�ӿ����෴��
	 * ��һ��SATA�ӿڶ�Ӧ��оƬ�ӿ�1
	 * �ڶ���SATA�ӿڶ�Ӧ��оƬ�ӿ�0
	 * ���ԣ���ʼ��Ӳ���豸���Ƶ�ʱ����Ҫ���⴦��
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
 * ������: mk_disk_info_dir
 * ��  ��: �������ݴ�����Ϣ���ļ���
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: disk_daemon
 * ��  ��: ���̹����ػ��̣߳���ʱ������״̬
*******************************************************************************/
#define DISK_DAEMON_SLEEP_TIME 3 	/* �ȴ��ļ��ʱ�� */

static int disk_daemon_flag = 0; 	/* ��ʶΪ0ʱ���жϵȴ������д���״̬��� */

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
 * ������: disk_check_now
 * ��  ��: ֪ͨ��������������
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
 * ������: disk_cmd_mng
 * ��  ��: ���̹�������ڲ�����ִ�нӿڣ����ڴ����µ��߳���ִ�к�ʱ�ܳ�������
 * ��  ��: disk_cmd�����̲�������ָ��
 * ����ֵ: �̴߳����ɹ�������0��ʧ�ܣ�����-1
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
		/* �ȴ�����ִ���̸߳������������ֹ����ָ��仯��ɲ������� */

		disk_cmd->cmd 			= -1;
		disk_cmd->disk_num 		= -2;
		disk_cmd->partition_num = -1;
		/* ���������������ֹ�´��յ�������������ò����� */
	}

	pthread_attr_destroy(&thread_attr);

	pthread_mutex_unlock(&disk_mng_mutex);

	return ret;
}


/*******************************************************************************
 * ������: disk_cmd_process
 * ��  ��: ���̹�������ִ���߳�
 * ��  ��: disk_cmd�����̲�������ָ��
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
* ������: get_cur_partition_path
* ��  ��: ��ȡ��ǰ���ڴ洢�ķ���·��
* ��  ��: path��·����ΪNULLʱ�������ã�
* ����ֵ: ���ã�����0�������ã�����-1
*******************************************************************************/
int get_cur_partition_path(char *path)
{
	int fd = 0;
	char lc_buf[128] = {0};
	int li_lens = 0;

	//�ж��豸�Ƿ����SD��
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

	/* ֪ͨ�����鵱ǰ����״̬ */
	disk_check_now();

	close(fd);

	return -1;
}

/*******************************************************************************
* ������: get_park_enter_db_file_path
* ��  ��: ��ȡ��ǰ���ڴ洢������Ϣ��ͼƬ��·��
* ��  ��: path��·����ΪNULLʱ�������ã�
* ����ֵ: ���ã�����0�������ã�����-1
*******************************************************************************/

//˵��: ��Ϊ�����ݶ�������SD �������Զ����Ĵ洢��ʱ��ŵ�tmpĿ¼��

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
* ������: set_cur_partition_path
* ��  ��: ���õ�ǰ���ڴ洢�ķ���·��
* ��  ��: disk_num�����̺ţ�parititon_num��������
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
* ������: disk_get_status
* ��  ��: ��ȡ��ǰ���ڴ洢�ķ���״̬
* ����ֵ: ���ã�����0�������ã�����-1
*******************************************************************************/
int disk_get_status(void)
{
	char lc_buf[128] = {0};
	return get_cur_partition_path(lc_buf);
}

/*******************************************************************************
* ������: db_path_park_get_status
* ��  ��: ��ȡ��ǰ���ڴ洢�ķ���״̬
* ����ֵ: ���ã�����0�������ã�����-1
*******************************************************************************/
/*
int db_record_path_park_get_status(void)
{
	return get_park_db_stroge_path(NULL);
}
*/




