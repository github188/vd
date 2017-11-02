
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mntent.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/hdreg.h>
#include <linux/magic.h>
#include <sys/vfs.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "disk_func.h"
#include "partition_func.h"
#include "sysserver/interface.h"


/*******************************************************************************
 * ������: disk_status_init
 * ��  ��: ��ȡ����״̬��Ϣ��������״̬���д���
 * ����ֵ: �п��ô��̣�����0��δ̽�⵽���ô��̣�����-1
*******************************************************************************/
int disk_status_init(void)
{
	int ret = -1;

	do
	{
		/* �ɹ���ȡ������Ϣ���Ҵ��̴�С�ڳ���֧�ַ�Χ��ʱ������Ϊ̽�⵽���� */
		if ( (disk_get_info(DISK_IS_MMC) == 0) &&
		        (disk_check_size(DISK_IS_MMC) == 0) )
		{
			ret = 0;

			if (is_new_disk(DISK_IS_MMC) < 0)
			{
				if (disk_init(DISK_IS_MMC) < 0)
					break;
			}

			if (partition_status_init(DISK_IS_MMC) == 0)
				mmc_enable();
		}
	}
	while (0);

	int i;
	for (i=0; i<HDD_NUM; i++)
	{
		if (hdd_power_on(i) < 0)
		{
			hdd_power_down(i);
			/* ��ֹ��SATA�߽Ӵ�������ԭ��δ̽�⵽Ӳ��ʱ��Ӳ����ת���� */
			continue;
		}

		/* �ɹ���ȡ������Ϣ���Ҵ��̴�С�ڳ���֧�ַ�Χ��ʱ������Ϊ̽�⵽���� */
		if ( (disk_get_info(i) == 0) && (disk_check_size(i) == 0) )
		{
			ret = 0;

			if (is_new_disk(i) < 0)
			{
				if (disk_init(i) < 0)
					continue;
			}

			if (partition_status_init(i) == 0)
				disk_hdd[i].disk_info.is_usable = DISK_USABLE;
		}
		else
			hdd_power_down(i); 	/* Ӳ�̲�����ʱ�����紦�� */
	}

	hdd_enable();

	return ret;
}


/*******************************************************************************
 * ������: disk_get_info
 * ��  ��: ��ȡ������Ϣ
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_get_info(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	if (disk_get_size(p_disk->dev_name, &(p_disk->size_in_byte)) < 0)
	{
		/* ��ȡ�豸��Сʧ��ʱ����Ϊ���豸�޷�ʹ�� */
		log_warn_storage("Get %s size faild !!!\n", p_disk->dev_name);
		return -1;
	}

	disk_read_flag(p_disk->dev_name, &(p_disk->disk_flag));

	p_disk->is_detected = DISK_DETECTED;

	return 0;
}


/*******************************************************************************
 * ������: disk_check_size
 * ��  ��: �����̴�С�Ƿ��ڱ�����֧�ַ�Χ�ڣ�����ӡ��Ϣ
 * ��  ��: disk_num�����̺�
 * ����ֵ: ֧�֣�����0����֧�֣�����-1
*******************************************************************************/
int disk_check_size(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	char *p_dev_name = p_disk->dev_name;

	unsigned long megabytes = p_disk->size_in_byte/1000000;

	if (megabytes < DISK_MIN_SIZE)
	{
		p_disk->is_size_suitable = DISK_SIZE_SMALL;
		log_warn_storage("Disk %s (%lu < %d MB) seems too small !\n",
		                 p_dev_name, megabytes, DISK_MIN_SIZE);
		return -1;
	}
	else if (megabytes > DISK_MAX_SIZE)
	{
		p_disk->is_size_suitable = DISK_SIZE_LARGE;
		log_warn_storage("Disk %s (%lu > %d MB) seems too large !\n",
		                 p_dev_name, megabytes, DISK_MAX_SIZE);
		return -1;
	}

	if (megabytes < 10000)
	{
		log_state_storage("Disk %s: %lu MB, %lld bytes, "
		                  "total %lld sectors\n",
		                  p_dev_name,
		                  megabytes,
		                  p_disk->size_in_byte,
		                  (p_disk->size_in_byte) >> 9);
	}
	else
	{
		unsigned long hectomega = (megabytes + 50) / 100;
		log_state_storage("Disk %s: %lu.%lu GB, %lld bytes, "
		                  "total %lld sectors\n",
		                  p_dev_name,
		                  hectomega / 10,
		                  hectomega % 10,
		                  p_disk->size_in_byte,
		                  (p_disk->size_in_byte) >> 9);
	}

	return 0;
}


/*******************************************************************************
 * ������: which_disk
 * ��  ��: ���ݴ��̺ż������ĸ�����
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ������ش�����Ϣָ�룻ʧ�ܣ�����NULL
*******************************************************************************/
DiskInfo *which_disk(int disk_num)
{
	if (disk_num == DISK_IS_MMC)
		return &(disk_mmc.disk_info);
	else if ((disk_num >= 0) && (disk_num < HDD_NUM))
		return &(disk_hdd[disk_num].disk_info);
	else
	{
		log_warn_storage("Wrong Disk Number : %d !\n", disk_num);
		return NULL;
	}
}


/*******************************************************************************
 * ������: disk_get_size
 * ��  ��: ��ȡ���̴�С
 * ��  ��: dev_name���豸���ƣ���bytes�����̴�С
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_get_size(const char *dev_name, unsigned long long *bytes)
{
	int rc = -1;

	int disk_fd = open(dev_name, O_RDONLY);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild : %m !\n", dev_name);
		return rc;
	}

	rc = ioctl(disk_fd, BLKGETSIZE64, bytes);
	if (rc < 0)
	{
		*bytes = 0;
	}

	close(disk_fd);

	return rc;
}


/*******************************************************************************
 * ������: disk_read_flag
 * ��  ��: ��ȡ���̱�ʶ��Ϣ
 * ��  ��: dev_name���豸���ƣ�disk_flag�����̱�ʶ��Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_read_flag(const char *dev_name, DiskFlag *disk_flag)
{
	int disk_fd = open(dev_name, O_RDONLY);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild : %m !\n", dev_name);
		return -1;
	}

	lseek(disk_fd, DISK_FLAG_POSITION, SEEK_SET);

	ssize_t read_size = read(disk_fd, disk_flag, sizeof(DiskFlag));

	close(disk_fd);

	if (read_size != sizeof(DiskFlag))
	{
		log_warn_storage("Read DiskFlag failed, read %d bytes.", read_size);
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * ������: is_new_disk
 * ��  ��: �жϴ����Ƿ����µ�
 * ��  ��: disk_num�����̺�
 * ����ֵ: ������µģ�����-1�����򣬷���0
*******************************************************************************/
int is_new_disk(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	/* �����̱�ʶħ���ͷ�������ȷʱ������Ϊ�����Ǿ������������ */
	if ( (p_disk->disk_flag.margic_num == DISK_MAGIC_NUM) &&
	        (partition_table_check(disk_num) == 0) )
		return 0;

	return -1;
}


/*******************************************************************************
 * ������: partition_table_check
 * ��  ��: �жϴ��̷������Ƿ���ȷ
 * ��  ��: disk_num�����̺�
 * ����ֵ: ��ȷ������0�����󣬷���-1
*******************************************************************************/
int partition_table_check(int disk_num)
{
	PartitionTable pt; 		/* ʵ�ʵķ����� */
	PartitionTable expt; 	/* �����ķ����� */
	int i;

	if (partition_table_read(disk_num, &pt) < 0)
		return -1;

	if (partition_table_should_be(disk_num, &expt) < 0)
		return -1;

	for (i=0; i<MBR_PARTITION_NUM; i++)
	{
		/* ����������������һ��ʱ����Ϊ��������ȷ */
		if (pt.partition_entry[i].type != expt.partition_entry[i].type)
		{
			log_state_storage("Disk %d partition %d type is 0x%x, "
			                  "but it should be 0x%x.\n",
			                  disk_num,
			                  i,
			                  pt.partition_entry[i].type,
			                  expt.partition_entry[i].type);
			return -1;
		}

		/* ������С��������һ��ʱ����Ϊ��������ȷ */
		if (pt.partition_entry[i].sectors_num !=
		        expt.partition_entry[i].sectors_num)
		{
			log_state_storage("Disk %d partition %d has %d sectors, "
			                  "but it should be %d.\n",
			                  disk_num,
			                  i,
			                  pt.partition_entry[i].sectors_num,
			                  expt.partition_entry[i].sectors_num);
			return -1;
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: partition_table_read
 * ��  ��: ��ȡ���̷�����
 * ��  ��: disk_num�����̺ţ�p_pt��������ָ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int partition_table_read(int disk_num, PartitionTable *p_pt)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	int i;
	int disk_fd = open(p_disk->dev_name, O_RDONLY);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", p_disk->dev_name);
		return -1;
	}

	ssize_t table_size = sizeof(PartitionTable);

	for (i=0; i<RETRY_TIMES; i++)
	{
		lseek(disk_fd, PARTITION_TABLE_POSITION, SEEK_SET);

		if (read(disk_fd, p_pt, table_size) == table_size)
		{
			close(disk_fd);
			return 0;
		}
	}

	close(disk_fd);
	return -1;
}


/*******************************************************************************
 * ������: partition_table_should_be
 * ��  ��: ������̷�����Ӧ��ֵ
 * ע  ��: �ú������õ��ļ��㷽����ͨ�õģ�
 *         �Ҹ÷����������ڸ߰汾fdisk���������ִ��ķ����������ɵķ�����
 * ��  ��: disk_num���豸�ţ�p_pt��������ָ��
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int partition_table_should_be(int disk_num, PartitionTable *p_pt)
{
	memset(p_pt, 0, sizeof(PartitionTable));

	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	int partition_num;
	int i;

	if (disk_num == DISK_IS_MMC)
		partition_num = MMC_PARTITION_NUM;
	else
		partition_num = HDD_PARTITION_NUM;

	unsigned long long usable_sectors = 	/* ���̿��������� */
	    ((p_disk->size_in_byte) >> 9) - 2048;

	unsigned long long partition_sectors = 	/* ����ǰ���������������� */
	    (((usable_sectors / partition_num) >> 9) << 9);

	for (i=0; i<partition_num; i++)
	{
		p_pt->partition_entry[i].type = LINUX_NATIVE;

		if (i == partition_num - 1)
		{
			p_pt->partition_entry[i].sectors_num =
			    usable_sectors - (partition_sectors * i);
			/* ���һ��������������Ϊʣ�µ������� */
		}
		else
		{
			p_pt->partition_entry[i].sectors_num = partition_sectors;
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: disk_init
 * ��  ��: ���̳�ʼ��
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_init(int disk_num)
{
	if (disk_format(disk_num) < 0)
	{
		log_warn_storage("Format disk %d failed !!!\n", disk_num);
		return -1;
	}

	disk_init_flag(disk_num);
	disk_mount(disk_num);
	mk_disk_info_dir(disk_num);

	return 0;
}


/*******************************************************************************
 * ������: disk_format
 * ��  ��: ���̷�������ʽ��
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_format(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	p_disk->is_format = DISK_FORMATTING;

	disk_umount(disk_num); 		/* ����ǰҪ��ж�ش��� */

	if (disk_partition(disk_num) < 0)
	{
		p_disk->is_format = DISK_NOTFORMATTING;
		return -1;
	}

	int partition_num;
	int i;

	if (disk_num == DISK_IS_MMC)
		partition_num = MMC_PARTITION_NUM;
	else
		partition_num = HDD_PARTITION_NUM;

	for (i=0; i<partition_num; i++)
	{
		partition_format(disk_num, i);
	}

	p_disk->is_format = DISK_NOTFORMATTING;

	return 0;
}


/*******************************************************************************
 * ������: disk_umount
 * ��  ��: ж�ش���
 * ��  ��: disk_num�����̺�
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_umount(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	/* �޸Ĵ��̱�ʶ������Ǹô��̲���ʹ��״̬ */
	p_disk->disk_flag.last_used = 0;
	disk_write_flag(p_disk->dev_name, &(p_disk->disk_flag));

	int i;
	char *mount_point = NULL;

	p_disk->is_usable = DISK_UNUSABLE;

	while ((mount_point = disk_mount_point_find(p_disk->dev_name)) != NULL)
	{
		for (i=0; i<RETRY_TIMES; i++)
		{
			sync();		/* ��ϵͳ������������д����̣���ȷ������ͬ���� */

			log_debug_storage("Try to umount %s\n", mount_point);

			if (umount(mount_point) != 0)
			{
				log_debug_storage("umount %s failed %d time(s) : %m !\n",
				                  mount_point, i);
				u_sleep(2, 0); 	/* ���ܻ��ж�д�����ڽ��У��ȴ�2�����ٳ��� */
			}
			else
			{
				log_debug_storage("Umount %s succeed!\n", mount_point);

				if (rmdir(mount_point) < 0)
				{
					log_debug_storage("rmdir %s : %m !\n", mount_point);
				}

				break;
			}
		}

		if (i == RETRY_TIMES)
		{
			log_state_storage("Umount %s failed!\n", mount_point);
			return -1;
		}
	}

	return 0;
}


/*******************************************************************************
 * ������: disk_mount_point_find
 * ��  ��: ��ѯ���̹��ص�
 * ��  ��: dev_name�������豸����
 * ����ֵ: �ɹ������ع��ص㣻ʧ�ܣ�����NULL
*******************************************************************************/
char *disk_mount_point_find(const char *dev_name)
{
	char original_name[DISK_NAME_LEN];

	if (realpath(dev_name, original_name) == NULL)
	{
		log_debug_storage("realpath %s : %m !\n", dev_name);
		return NULL;
	}

	size_t name_len = strlen(original_name);

	FILE *mtab_fp = NULL;
	struct mntent *mountEntry = NULL;

	mtab_fp = setmntent("/etc/mtab", "r");
	if (!mtab_fp)
	{
		log_warn_storage("setmntent /etc/mtab : %m !\n");
		return NULL;
	}

	while ((mountEntry = getmntent(mtab_fp)) != NULL)
	{
		if (strncmp(original_name, mountEntry->mnt_fsname, name_len) == 0)
		{
			break;
		}
	}

	endmntent(mtab_fp);

	if (mountEntry != NULL)
		return mountEntry->mnt_dir;
	else
		return NULL;
}


/*******************************************************************************
 * ������: reread_partition_table
 * ��  ��: ���¶�ȡ������
 * ��  ��: dev_name���豸����
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int reread_partition_table(const char *dev_name)
{
	int disk_fd;
	int rc = -1;
	disk_fd = open(dev_name, O_RDWR);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", dev_name);
		return rc;
	}

	sync();

	u_sleep(1, 0);

	rc = ioctl(disk_fd, BLKRRPART);
	if (rc < 0)
	{
		log_state_storage("Re-read partition table failed : %m !\n");
	}
	else
	{
		log_debug_storage("Re-read partition table done !\n");
		u_sleep(2, 0);
	}

	close(disk_fd);

	return rc;
}


/*******************************************************************************
* ������: partition_table_clear
* ��  ��: ������̵ķ�����Ϣ���߼���ͷ����������Ϣ
* ��  ��: dev_name���豸����
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int partition_table_clear(const char *dev_name)
{
	int disk_fd;
	ssize_t write_size;
	char zero[PARTITION_TABLE_SIZE];

	disk_fd = open(dev_name, O_RDWR);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", dev_name);
		return -1;
	}

	memset(zero, 0, PARTITION_TABLE_SIZE);

	lseek(disk_fd, PARTITION_TABLE_POSITION, SEEK_SET);
	write_size = write(disk_fd, zero, PARTITION_TABLE_SIZE);

	close(disk_fd);

	if (write_size != PARTITION_TABLE_SIZE)
	{
		log_warn_storage("Clear partition table failed, write %d.\n",
		                 write_size);
		return -1;
	}

	log_debug_storage("Clear partition table done!\n");

	return 0;
}


/*******************************************************************************
* ������: disk_partition
* ��  ��: ���̷���
* ��  ��: disk_num�����̺�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_partition(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	if (partition_table_clear(p_disk->dev_name) < 0)
		return -1;

	reread_partition_table(p_disk->dev_name);

	if (access(DISK_PARTITION, X_OK) < 0)
	{
		log_error_storage("Is %s existing and executable ? Error: %m !\n",
		                  DISK_PARTITION);
		return -1;
	}

	pid_t pid;
	int status;
	int partition_num;
	char argv_partition_num[4];
	char argv_sectors[36];

	if (disk_num == DISK_IS_MMC)
		partition_num = MMC_PARTITION_NUM;
	else
		partition_num = HDD_PARTITION_NUM;

	unsigned long long usable_sectors = 	/* ���̿��������� */
	    ((p_disk->size_in_byte) >> 9) - 2048;

	sprintf(argv_partition_num, "%d", partition_num);
	sprintf(argv_sectors, "%llu",
	        (((usable_sectors / partition_num) >> 9) << 9) - 1);

	char *fdisk_argv[] =
	{
		(char *)DISK_PARTITION,
		p_disk->dev_name,
		argv_partition_num,
		argv_sectors,
		NULL
	}; 	/* �������� disk_partition.sh �ű�ȷ�� */

	pid = vfork();
	if (pid == 0)
	{
		execv(DISK_PARTITION, fdisk_argv);
	}

	waitpid(pid, &status, 0);
	if (!WIFEXITED(status))
	{
		log_warn_storage("disk partition exit abnormally !\n");
		return -1;
	}

	log_state_storage("Disk %s partition done!\n", p_disk->dev_name);

	reread_partition_table(p_disk->dev_name);

	return 0;
}


/*******************************************************************************
* ������: disk_init_flag
* ��  ��: ��ʼ�����̱�ʶ��Ϣ
* ��  ��: disk_num�����̺�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_init_flag(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	p_disk->disk_flag.margic_num 		= DISK_MAGIC_NUM;
	p_disk->disk_flag.id 				= disk_id_create();
	p_disk->disk_flag.last_used 		= 0;
	p_disk->disk_flag.last_partition 	= 0;

	return disk_write_flag(p_disk->dev_name, &(p_disk->disk_flag));
}


/*******************************************************************************
* ������: disk_id_create
* ��  ��: �������һ������ID
* ����ֵ: ����ID
*******************************************************************************/
unsigned int disk_id_create(void)
{
	int succeed_flag = 0; 	/* ID���ɳɹ���ǣ���1ʱ��ʾ�ɹ� */
	unsigned int id = 0;
	DIR *dirp = NULL;
	struct dirent entry;
	struct dirent *result = NULL;

	while (succeed_flag == 0)
	{
		succeed_flag = 1;

		id = random_32();
		id |= 0x80000000; 	/* ȷ������ID�������ļ��еĳ��� */

		/* �ж�������ɵ�ID�Ƿ�����еĳ�ͻ�������ͻ����������һ�� */
		dirp = opendir(DISK_INFO_DIR);
		if (dirp != NULL)
		{
			for(;;)
			{
				readdir_r(dirp, &entry, &result);
				if (result == NULL)
					break;

				if (id == (unsigned int)atoi(entry.d_name))
				{
					succeed_flag = 0;
					break;
				}
			}

			closedir(dirp);
		}
	}

	return id;
}


/*******************************************************************************
* ������: disk_write_flag
* ��  ��: д����̱�ʶ��Ϣ
* ��  ��: dev_name���豸���ƣ�disk_flag�����̱�ʶ��Ϣ
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_write_flag(const char *dev_name, const DiskFlag *disk_flag)
{
	int disk_fd;
	ssize_t write_size;

	disk_fd = open(dev_name, O_RDWR);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild : %m !\n", dev_name);
		return -1;
	}

	lseek(disk_fd, DISK_FLAG_POSITION, SEEK_SET);
	write_size = write(disk_fd, disk_flag, sizeof(DiskFlag));

	if (fdatasync(disk_fd) < 0)
	{
		log_warn_storage("fdatasync %s failed : %m !\n", dev_name);
	}

	close(disk_fd);

	if (write_size != sizeof(DiskFlag))
	{
		log_warn_storage("Write diskflag failed, return %d.\n", write_size);
		return -1;
	}

	return 0;
}


/*******************************************************************************
* ������: disk_mount
* ��  ��: ���ش���
* ��  ��: disk_num�����̺�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int disk_mount(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	int partition_num;
	int i;

	if (disk_num == DISK_IS_MMC)
		partition_num = MMC_PARTITION_NUM;
	else
		partition_num = HDD_PARTITION_NUM;

	for (i=0; i<partition_num; i++)
	{
		if (partition_mount(disk_num, i, NULL) != 0)
		{
			log_warn_storage("Mount disk %d partition %d failed!\n",
			                 disk_num, i);
		}
	}

	p_disk->is_usable = DISK_USABLE;

	return 0;
}


/*******************************************************************************
* ������: disk_status_check
* ��  ��: ���ȫ�����̵�״̬
*******************************************************************************/
void disk_status_check(void)
{
	int i, j;

	if (disk_mmc.disk_info.is_usable == DISK_USABLE)
	{
		for (j=0; j<MMC_PARTITION_NUM; j++)
		{
			partition_status_check(DISK_IS_MMC, j);
		}
	}

	for (i=0; i<HDD_NUM; i++)
	{
		if (disk_hdd[i].disk_info.is_usable == DISK_USABLE)
		{
			for (j=0; j<HDD_PARTITION_NUM; j++)
			{
				partition_status_check(i, j);
			}
		}
	}

	hdd_power_down_others();
}


/*******************************************************************************
* ������: disk_maintain
* ��  ��: ���ݵ�ǰ�Ĵ���״̬���Դ��̽���ά��
*******************************************************************************/
void disk_maintain(void)
{
	int i, j;
	PartitionInfo *p_partition = NULL;
	DiskCmd disk_cmd = {-1, -2, -1};

	/* �ȼ�鵱ǰ�������������������״̬�������л� */
	p_partition = which_partition(cur_disk_num, cur_partition_num);
	if (p_partition == NULL)
	{
		log_error_storage("Current disk number is %d, "
		                  "but partition number is %d",
		                  cur_disk_num, cur_partition_num);
	}
	else
	{
		if ( (p_partition->percent_used > PARTITION_FULL_PERCENT) ||
		        (p_partition->status != PARTITION_OK) )
		{
			log_state_storage("Current partition is full or abnormal, "
			                  "switch now!\n");
			partition_switch();
		}
	}

	/* �ټ��SD�� */
	if (disk_mmc.disk_info.is_usable == DISK_USABLE)
	{
		for (j=0; j<MMC_PARTITION_NUM; j++)
		{
			p_partition = &(disk_mmc.partition_info[j]);

			if (p_partition->status == PARTITION_ABNORMAL)
			{
				disk_cmd.cmd 			= DISK_CMD_RESCURE;
				disk_cmd.disk_num 		= DISK_IS_MMC;
				disk_cmd.partition_num 	= j;
				disk_cmd_mng(&disk_cmd);
			}
		}
	}

	/* �����Ӳ�� */
	for (i=0; i<HDD_NUM; i++)
	{
		if (disk_hdd[i].disk_info.is_usable == DISK_USABLE)
		{
			for (j=0; j<HDD_PARTITION_NUM; j++)
			{
				p_partition = &disk_hdd[i].partition_info[j];

				if (p_partition->status == PARTITION_ABNORMAL)
				{
					disk_cmd.cmd 			= DISK_CMD_RESCURE;
					disk_cmd.disk_num 		= i;
					disk_cmd.partition_num 	= j;
					disk_cmd_mng(&disk_cmd);
				}
			}
		}
	}
}


/*******************************************************************************
* ������: hdd_standby_interval
* ��  ��: ����Ӳ���ڼ��ָ��ʱ��û�в���ʱ����������״̬
* ��  ��: dev_name���豸���ƣ�interval�����
* ע  ��: intervalֵ�ĺ�����ر�
*         ֵΪ 0 ʱ����ʾ�����Զ���������״̬��
*         ֵΪ 1-240 ʱ����ʾ��ʱʱ��Ϊ1��240��5�룬��5�뵽20���ӣ�
*         ֵΪ 241-251 ʱ����ʾ��ʱʱ��Ϊ1-11��30���ӣ���30���ӵ�5.5Сʱ��
*         ֵΪ 252 ʱ����ʾ��ʱʱ��Ϊ21���ӣ�
*         ֵ 253 ��ʾ�ĳ�ʱʱ����Ӳ�������̶��壬��Χ��8��12Сʱ��
*         ֵ 524 ������
*         ֵ 255 ��ʾ21��15�롣
*         ���⣬һЩ�ϵ�Ӳ�̿��ܲ������á�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int hdd_standby_interval(const char *dev_name, int interval)
{
	int disk_fd = open(dev_name, O_RDONLY);
	if (disk_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", dev_name);
		return -1;
	}

	__u8 args[4] = {WIN_SETIDLE1, (__u8)interval, 0, 0};
	int ret = ioctl(disk_fd, HDIO_DRIVE_CMD, args);

	close(disk_fd);

	return ret;
}


/*******************************************************************************
* ������: hdd_enable
* ��  ��: ����Ӳ��
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int hdd_enable(void)
{
	int hdd_num = -2;
	int partition_num = -1;

	if (hdd_choose(&hdd_num, &partition_num) < 0)
		return -1;

	DiskInfo *p_disk = which_disk(hdd_num);
	if (p_disk != NULL)
	{
		if (p_disk->is_usable == DISK_UNUSABLE)
			if (hdd_power_on(hdd_num) < 0)
				hdd_power_down(hdd_num); 	/* ���ϵ�ʧ�ܣ�����紦�� */
	}

	partition_mark_using(hdd_num, partition_num);

	hdd_power_down_others();

	return 0;
}


/*******************************************************************************
* ������: hdd_power_down_others
* ��  ��: ����ʹ�õ�Ӳ�̵���
*******************************************************************************/
void hdd_power_down_others(void)
{
	int i, j;

	for (i=0; i<HDD_NUM; i++)
	{
		if (i == cur_disk_num)
			continue;

		for(j=0; j<HDD_PARTITION_NUM; j++)
		{
			if ( (disk_hdd[i].partition_info[j].status != PARTITION_DAMAGED) &&
			        (disk_hdd[i].partition_info[j].status != PARTITION_OK) )
				break;
		}

		if (j == HDD_PARTITION_NUM) 	/* ˵��4��������δ���ڲ���״̬ */
			hdd_power_down(i);
	}
}


/*******************************************************************************
* ������: hdd_choose
* ��  ��: ѡ��ʹ���ĸ�Ӳ�̵��ĸ�����
* ��  ��: hdd_num��Ӳ�̺ţ�partition_num��������
* ����ֵ: �ɹ�������Ӳ�̺ţ�ʧ�ܣ�����-1
*******************************************************************************/
int hdd_choose(int *hdd_num, int *partition_num)
{
	int i, j;
	int flag; 	/* ��ʶ����������Ӳ�̵��ϴ�ʹ�ñ�ʶһ������� */
	int last_partition;
	DiskInfo *p_disk = NULL;
	PartitionInfo *p_partition = NULL;

	for (flag=0; flag<=1; flag++)
	{
		for (i=0; i<HDD_NUM; i++)
		{
			p_disk = &(disk_hdd[i].disk_info);

			/* �ȼ��������ϴ�ʹ�õ�Ӳ�� */
			if (p_disk->is_detected && ((p_disk->disk_flag.last_used) | flag))
			{
				last_partition = p_disk->disk_flag.last_partition;

				if ( (last_partition >= 0) &&
				        (last_partition < HDD_PARTITION_NUM) )
				{
					p_partition = &(disk_hdd[i].partition_info[last_partition]);

					if (p_partition->status == PARTITION_OK)
					{
						*hdd_num = i;
						*partition_num = last_partition;
						/* ����ϴ�ʹ�õķ�������������ʹ�ø÷��� */

						return 0;
					}
				}

				for (j=0; j<HDD_PARTITION_NUM; j++)
				{
					p_partition = &(disk_hdd[i].partition_info[j]);

					if ( (p_partition->percent_used < PARTITION_FULL_PERCENT) &&
					        (p_partition->status == PARTITION_OK) )
					{
						*hdd_num = i;
						*partition_num = j;

						return 0;
					}
				}
			}
		}
	}

	return -1;
}


/*******************************************************************************
* ������: hdd_wait_for_ready
* ��  ��: �ȴ�Ӳ�̱�ϵͳʶ�����
* ��  ��: hdd_num��Ӳ�̺�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
#define HDD_WAIT_FOR_READY_SLEEP_TIME 3 	/* ���ļ��ʱ�� */

int hdd_wait_for_ready(int hdd_num)
{
	DiskInfo *p_disk = which_disk(hdd_num);
	if (p_disk == NULL)
		return -1;

	int i;
	for (i=(HDD_POWER_ON_TIME/HDD_WAIT_FOR_READY_SLEEP_TIME); i>0; i--)
	{
		if (access(p_disk->dev_name, F_OK) == 0)
		{
			u_sleep(2, 0); 	/* �ٵȴ�2���ӣ���ȷ�������豸������� */
			return 0;
		}

		u_sleep(HDD_WAIT_FOR_READY_SLEEP_TIME, 0);
	}

	log_state_storage("%s apparently does not exist !\n", p_disk->dev_name);

	return -1;
}


/*******************************************************************************
* ������: hdd_power_on
* ��  ��: Ӳ���ϵ�
* ��  ��: hdd_num��Ӳ�̺�
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int hdd_power_on(int hdd_num)
{
//	DiskInfo *p_disk = which_disk(hdd_num);
//	if (p_disk == NULL)
//		return -1;
//
//	log_state_storage("Power on disk %d now.\n", hdd_num);
//
//	if (harddisk_power_onoff(hdd_num, 1) < 0)
//	{
//		log_warn_storage("hard disk %d power on failed!\n", hdd_num);
//		return -1;
//	}
//
//	if (hdd_wait_for_ready(hdd_num) < 0)
//	{
//		log_warn_storage("Wait for disk %d time out !\n", hdd_num);
//		return -1;
//	}
//
//	hdd_standby_interval(p_disk->dev_name, HDD_STANDBY_INTERVAL);
//
//	if (p_disk->is_detected == DISK_DETECTED)
//	{
//		disk_mount(hdd_num); 	/* �״�̽�⵽Ӳ�̣����ڴ˴����й��� */
//	}

	return 0;
}


/*******************************************************************************
* ������: hdd_power_down
* ��  ��: Ӳ�̵���
* ��  ��: hdd_num��Ӳ�̺�
* ����ֵ: �ɹ�������0
*******************************************************************************/
int hdd_power_down(int hdd_num)
{
//	log_debug_storage("Power down disk %d now.\n", hdd_num);
//
//	if ( (hdd_num < 0) || (hdd_num >= HDD_NUM) )
//	{
//		log_error_storage("Wrong hard disk number : %d !\n", hdd_num);
//		return -1;
//	}
//
//	if (disk_hdd[hdd_num].disk_info.is_usable == DISK_USABLE)
//		disk_umount(hdd_num);
//
//	if (harddisk_power_onoff(hdd_num, 0) < 0)
//		return -1;

	return 0;
}


/*******************************************************************************
* ������: mmc_choose
* ��  ��: ѡ��ʹ��SD�����ĸ�����
* ��  ��: partition_num��������
* ����ֵ: �ɹ�������Ӳ�̺ţ�ʧ�ܣ�����-1
*******************************************************************************/
int mmc_choose(int *partition_num)
{
	if (disk_mmc.disk_info.is_detected == DISK_DETECTED)
	{
		int i;
		PartitionInfo *p_partition = NULL;

		for (i=0; i<MMC_PARTITION_NUM; i++)
		{
			p_partition = &(disk_mmc.partition_info[i]);

			if ( (p_partition->status == PARTITION_OK) &&
			        (p_partition->percent_used < PARTITION_FULL_PERCENT) )
			{
				*partition_num = i;

				return 0;
			}
		}
	}

	return -1;
}


/*******************************************************************************
* ������: mmc_enable
* ��  ��: ����SD��
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int mmc_enable(void)
{
	int partition_num = -1;

	if (mmc_choose(&partition_num) < 0)
		return -1;

	disk_mmc.disk_info.is_usable = DISK_USABLE;

	partition_mark_using(DISK_IS_MMC, partition_num);

	return 0;
}


/*******************************************************************************
* ������: mmc_clear
* ��  ��: SD������
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int mmc_clear(void)
{
	int i;
	int partition_num;
	int last, next;
	PartitionInfo *next_partition = NULL;

	if (cur_disk_num == DISK_IS_MMC) 	/* ���Ե�ǰʹ�õķ����������� */
	{
		partition_num 	= MMC_PARTITION_NUM - 1;
		last 			= cur_partition_num;
	}
	else
	{
		partition_num 	= MMC_PARTITION_NUM;
		last 			= disk_mmc.disk_info.disk_flag.last_partition;
	}

	/* �ȳ��ԣ�ֻ�����Ѿ��ϴ������ļ� */
	for (i=0; i<partition_num; i++)
	{
		next 			= (last + i + 1) % MMC_PARTITION_NUM;
		next_partition 	= &(disk_mmc.partition_info[next]);

		if (next_partition->status != PARTITION_OK)
			continue;

		if ( (partition_clear_trash(DISK_IS_MMC, next) == 0) &&
		        (partition_status_check(DISK_IS_MMC, next) == 0) &&
		        (next_partition->percent_used < PARTITION_FULL_PERCENT) )
			return 0;
	}

	/* û���������Ļ����������������� */
	for (i=0; i<partition_num; i++)
	{
		next 			= (last + i + 1) % MMC_PARTITION_NUM;
		next_partition 	= &(disk_mmc.partition_info[next]);

		if (next_partition->status != PARTITION_OK)
			continue;

		if ( (partition_clear_all(DISK_IS_MMC, next) == 0) &&
		        (partition_status_check(DISK_IS_MMC, next) == 0) &&
		        (next_partition->percent_used < PARTITION_EMPTY_PERCENT) )
			return 0;

		if ( (partition_format(DISK_IS_MMC, next) == 0) &&
		        (partition_status_check(DISK_IS_MMC, next) == 0) &&
		        (next_partition->percent_used < PARTITION_EMPTY_PERCENT) )
			return 0;
	}

	return -1;
}


/*******************************************************************************
* ������: hdd_clear
* ��  ��: Ӳ������
* ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int hdd_clear(void)
{
	int i, j;
	int disk_num; 		/* ���̺� */
	int partition_num; 	/* ������������������� */
	int last, next;
	DiskInfo *p_disk;
	PartitionInfo *next_partition;

	for (i=0; i<HDD_NUM; i++)
	{
		if ( (cur_disk_num >= 0) && (cur_disk_num < HDD_NUM) &&
		        (cur_partition_num == (HDD_PARTITION_NUM - 1)) )
			/* �����ǰ�����Ѿ���Ӳ�����һ����������ô�Ȳ����¸�Ӳ�� */
			disk_num = (cur_disk_num + i + 1) % HDD_NUM;
		else
			disk_num = (cur_disk_num + i) % HDD_NUM;

		p_disk = &(disk_hdd[disk_num].disk_info);

		if (p_disk->is_detected != DISK_DETECTED)
			continue;

		if (p_disk->is_usable != DISK_USABLE)
		{
			if (hdd_power_on(disk_num) < 0)
			{
				hdd_power_down(disk_num);
				continue;
			}
		}

		if (cur_disk_num == disk_num)	/* ���Ե�ǰʹ�õķ����������� */
		{
			partition_num 	= HDD_PARTITION_NUM - 1;
			last 			= cur_partition_num;
		}
		else
		{
			partition_num 	= HDD_PARTITION_NUM;
			last 			= p_disk->disk_flag.last_partition;
		}

		/* �ȳ��ԣ�ֻ�����Ѿ��ϴ������ļ� */
		for (j=0; j<partition_num; j++)
		{
			next 			= (last + j + 1) % HDD_PARTITION_NUM;
			next_partition 	= &(disk_hdd[disk_num].partition_info[next]);

			if (next_partition->status != PARTITION_OK)
				continue;

			if ( (partition_clear_trash(disk_num, next) == 0) &&
			        (partition_status_check(disk_num, next) == 0) &&
			        (next_partition->percent_used < PARTITION_FULL_PERCENT) )
				return 0;
		}
	}

	for (i=0; i<HDD_NUM; i++)
	{
		if ( (cur_disk_num >= 0) && (cur_disk_num < HDD_NUM) &&
		        (cur_partition_num == (HDD_PARTITION_NUM - 1)) )
			/* �����ǰ�����Ѿ���Ӳ�����һ����������ô�Ȳ����¸�Ӳ�� */
			disk_num = (cur_disk_num + i + 1) % HDD_NUM;
		else
			disk_num = (cur_disk_num + i) % HDD_NUM;

		p_disk = &(disk_hdd[disk_num].disk_info);

		if (p_disk->is_detected != DISK_DETECTED)
			continue;

		if (p_disk->is_usable != DISK_USABLE)
		{
			if (hdd_power_on(disk_num) < 0)
			{
				hdd_power_down(disk_num);
				continue;
			}
		}

		if (cur_disk_num == disk_num)	/* ���Ե�ǰʹ�õķ����������� */
		{
			partition_num 	= HDD_PARTITION_NUM - 1;
			last 			= cur_partition_num;
		}
		else
		{
			partition_num 	= HDD_PARTITION_NUM;
			last 			= p_disk->disk_flag.last_partition;
		}

		/* û���������Ļ����������������� */
		for (j=0; j<partition_num; j++)
		{
			next 			= (last + j + 1) % HDD_PARTITION_NUM;
			next_partition 	= &(disk_hdd[disk_num].partition_info[next]);

			if (next_partition->status != PARTITION_OK)
				continue;

			if ( (partition_clear_all(disk_num, next) == 0) &&
			        (partition_status_check(disk_num, next) == 0) &&
			        (next_partition->percent_used < PARTITION_EMPTY_PERCENT) )
				return 0;

			if ( (partition_format(disk_num, next) == 0) &&
			        (partition_status_check(disk_num, next) == 0) &&
			        (next_partition->percent_used < PARTITION_EMPTY_PERCENT) )
				return 0;
		}
	}

	return -1;
}
