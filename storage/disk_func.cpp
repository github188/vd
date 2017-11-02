
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
 * 函数名: disk_status_init
 * 功  能: 获取磁盘状态信息，并根据状态进行处理
 * 返回值: 有可用磁盘，返回0；未探测到可用磁盘，返回-1
*******************************************************************************/
int disk_status_init(void)
{
	int ret = -1;

	do
	{
		/* 成功获取磁盘信息，且磁盘大小在程序支持范围内时，才认为探测到磁盘 */
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
			/* 防止因SATA线接触不良等原因未探测到硬盘时，硬盘运转过热 */
			continue;
		}

		/* 成功获取磁盘信息，且磁盘大小在程序支持范围内时，才认为探测到磁盘 */
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
			hdd_power_down(i); 	/* 硬盘不可用时，掉电处理 */
	}

	hdd_enable();

	return ret;
}


/*******************************************************************************
 * 函数名: disk_get_info
 * 功  能: 获取磁盘信息
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int disk_get_info(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	if (disk_get_size(p_disk->dev_name, &(p_disk->size_in_byte)) < 0)
	{
		/* 获取设备大小失败时，认为该设备无法使用 */
		log_warn_storage("Get %s size faild !!!\n", p_disk->dev_name);
		return -1;
	}

	disk_read_flag(p_disk->dev_name, &(p_disk->disk_flag));

	p_disk->is_detected = DISK_DETECTED;

	return 0;
}


/*******************************************************************************
 * 函数名: disk_check_size
 * 功  能: 检查磁盘大小是否在本程序支持范围内，并打印信息
 * 参  数: disk_num，磁盘号
 * 返回值: 支持，返回0；不支持，返回-1
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
 * 函数名: which_disk
 * 功  能: 根据磁盘号计算是哪个磁盘
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回磁盘信息指针；失败，返回NULL
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
 * 函数名: disk_get_size
 * 功  能: 获取磁盘大小
 * 参  数: dev_name，设备名称；；bytes，磁盘大小
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: disk_read_flag
 * 功  能: 读取磁盘标识信息
 * 参  数: dev_name，设备名称；disk_flag，磁盘标识信息
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: is_new_disk
 * 功  能: 判断磁盘是否是新的
 * 参  数: disk_num，磁盘号
 * 返回值: 如果是新的，返回-1；否则，返回0
*******************************************************************************/
int is_new_disk(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	/* 当磁盘标识魔数和分区表都正确时，才认为磁盘是经过本程序处理的 */
	if ( (p_disk->disk_flag.margic_num == DISK_MAGIC_NUM) &&
	        (partition_table_check(disk_num) == 0) )
		return 0;

	return -1;
}


/*******************************************************************************
 * 函数名: partition_table_check
 * 功  能: 判断磁盘分区表是否正确
 * 参  数: disk_num，磁盘号
 * 返回值: 正确，返回0；错误，返回-1
*******************************************************************************/
int partition_table_check(int disk_num)
{
	PartitionTable pt; 		/* 实际的分区表 */
	PartitionTable expt; 	/* 期望的分区表 */
	int i;

	if (partition_table_read(disk_num, &pt) < 0)
		return -1;

	if (partition_table_should_be(disk_num, &expt) < 0)
		return -1;

	for (i=0; i<MBR_PARTITION_NUM; i++)
	{
		/* 分区类型与期望不一致时，认为分区表不正确 */
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

		/* 分区大小与期望不一致时，认为分区表不正确 */
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
 * 函数名: partition_table_read
 * 功  能: 读取磁盘分区表
 * 参  数: disk_num，磁盘号；p_pt，分区表指针
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: partition_table_should_be
 * 功  能: 计算磁盘分区表应该值
 * 注  意: 该函数中用到的计算方法不通用的！
 *         且该方法不适用于高版本fdisk或其他更现代的分区工具生成的分区表！
 * 参  数: disk_num，设备号；p_pt，分区表指针
 * 返回值: 成功，返回0；失败，返回-1
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

	unsigned long long usable_sectors = 	/* 磁盘可用扇区数 */
	    ((p_disk->size_in_byte) >> 9) - 2048;

	unsigned long long partition_sectors = 	/* 磁盘前几个分区的扇区数 */
	    (((usable_sectors / partition_num) >> 9) << 9);

	for (i=0; i<partition_num; i++)
	{
		p_pt->partition_entry[i].type = LINUX_NATIVE;

		if (i == partition_num - 1)
		{
			p_pt->partition_entry[i].sectors_num =
			    usable_sectors - (partition_sectors * i);
			/* 最后一个分区的扇区数为剩下的扇区数 */
		}
		else
		{
			p_pt->partition_entry[i].sectors_num = partition_sectors;
		}
	}

	return 0;
}


/*******************************************************************************
 * 函数名: disk_init
 * 功  能: 磁盘初始化
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
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
 * 函数名: disk_format
 * 功  能: 磁盘分区并格式化
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int disk_format(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	p_disk->is_format = DISK_FORMATTING;

	disk_umount(disk_num); 		/* 分区前要先卸载磁盘 */

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
 * 函数名: disk_umount
 * 功  能: 卸载磁盘
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int disk_umount(int disk_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	/* 修改磁盘标识符，标记该磁盘不在使用状态 */
	p_disk->disk_flag.last_used = 0;
	disk_write_flag(p_disk->dev_name, &(p_disk->disk_flag));

	int i;
	char *mount_point = NULL;

	p_disk->is_usable = DISK_UNUSABLE;

	while ((mount_point = disk_mount_point_find(p_disk->dev_name)) != NULL)
	{
		for (i=0; i<RETRY_TIMES; i++)
		{
			sync();		/* 将系统缓冲区的内容写入磁盘，以确保数据同步。 */

			log_debug_storage("Try to umount %s\n", mount_point);

			if (umount(mount_point) != 0)
			{
				log_debug_storage("umount %s failed %d time(s) : %m !\n",
				                  mount_point, i);
				u_sleep(2, 0); 	/* 可能还有读写操作在进行，等待2秒钟再尝试 */
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
 * 函数名: disk_mount_point_find
 * 功  能: 查询磁盘挂载点
 * 参  数: dev_name，磁盘设备名称
 * 返回值: 成功，返回挂载点；失败，返回NULL
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
 * 函数名: reread_partition_table
 * 功  能: 重新读取分区表
 * 参  数: dev_name，设备名称
 * 返回值: 成功，返回0；失败，返回-1
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
* 函数名: partition_table_clear
* 功  能: 清理磁盘的分区信息及逻辑磁头柱面数等信息
* 参  数: dev_name，设备名称
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: disk_partition
* 功  能: 磁盘分区
* 参  数: disk_num，磁盘号
* 返回值: 成功，返回0；失败，返回-1
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

	unsigned long long usable_sectors = 	/* 磁盘可用扇区数 */
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
	}; 	/* 参数根据 disk_partition.sh 脚本确定 */

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
* 函数名: disk_init_flag
* 功  能: 初始化磁盘标识信息
* 参  数: disk_num，磁盘号
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: disk_id_create
* 功  能: 随机生成一个磁盘ID
* 返回值: 磁盘ID
*******************************************************************************/
unsigned int disk_id_create(void)
{
	int succeed_flag = 0; 	/* ID生成成功标记，置1时表示成功 */
	unsigned int id = 0;
	DIR *dirp = NULL;
	struct dirent entry;
	struct dirent *result = NULL;

	while (succeed_flag == 0)
	{
		succeed_flag = 1;

		id = random_32();
		id |= 0x80000000; 	/* 确保根据ID命名的文件夹的长度 */

		/* 判断随机生成的ID是否跟现有的冲突，如果冲突，重新生成一个 */
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
* 函数名: disk_write_flag
* 功  能: 写入磁盘标识信息
* 参  数: dev_name，设备名称；disk_flag，磁盘标识信息
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: disk_mount
* 功  能: 挂载磁盘
* 参  数: disk_num，磁盘号
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: disk_status_check
* 功  能: 检查全部磁盘的状态
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
* 函数名: disk_maintain
* 功  能: 根据当前的磁盘状态，对磁盘进行维护
*******************************************************************************/
void disk_maintain(void)
{
	int i, j;
	PartitionInfo *p_partition = NULL;
	DiskCmd disk_cmd = {-1, -2, -1};

	/* 先检查当前分区，如果不处于正常状态，进行切换 */
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

	/* 再检查SD卡 */
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

	/* 最后检查硬盘 */
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
* 函数名: hdd_standby_interval
* 功  能: 设置硬盘在间隔指定时长没有操作时，进入休眠状态
* 参  数: dev_name，设备名称；interval，间隔
* 注  意: interval值的含义很特别。
*         值为 0 时，表示不会自动进入休眠状态；
*         值为 1-240 时，表示超时时间为1到240个5秒，即5秒到20分钟；
*         值为 241-251 时，表示超时时间为1-11个30分钟，即30分钟到5.5小时；
*         值为 252 时，表示超时时间为21分钟；
*         值 253 表示的超时时间由硬盘生产商定义，范围是8到12小时；
*         值 524 保留；
*         值 255 表示21分15秒。
*         此外，一些老的硬盘可能并不适用。
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: hdd_enable
* 功  能: 启用硬盘
* 返回值: 成功，返回0；失败，返回-1
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
				hdd_power_down(hdd_num); 	/* 若上电失败，需掉电处理 */
	}

	partition_mark_using(hdd_num, partition_num);

	hdd_power_down_others();

	return 0;
}


/*******************************************************************************
* 函数名: hdd_power_down_others
* 功  能: 给不使用的硬盘掉电
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

		if (j == HDD_PARTITION_NUM) 	/* 说明4个分区都未处于操作状态 */
			hdd_power_down(i);
	}
}


/*******************************************************************************
* 函数名: hdd_choose
* 功  能: 选择使用哪个硬盘的哪个分区
* 参  数: hdd_num，硬盘号；partition_num，分区号
* 返回值: 成功，返回硬盘号；失败，返回-1
*******************************************************************************/
int hdd_choose(int *hdd_num, int *partition_num)
{
	int i, j;
	int flag; 	/* 标识，处理两个硬盘的上次使用标识一样的情况 */
	int last_partition;
	DiskInfo *p_disk = NULL;
	PartitionInfo *p_partition = NULL;

	for (flag=0; flag<=1; flag++)
	{
		for (i=0; i<HDD_NUM; i++)
		{
			p_disk = &(disk_hdd[i].disk_info);

			/* 先检查存在且上次使用的硬盘 */
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
						/* 如果上次使用的分区正常，继续使用该分区 */

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
* 函数名: hdd_wait_for_ready
* 功  能: 等待硬盘被系统识别出来
* 参  数: hdd_num，硬盘号
* 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
#define HDD_WAIT_FOR_READY_SLEEP_TIME 3 	/* 检测的间隔时间 */

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
			u_sleep(2, 0); 	/* 再等待2秒钟，以确保分区设备都已设别到 */
			return 0;
		}

		u_sleep(HDD_WAIT_FOR_READY_SLEEP_TIME, 0);
	}

	log_state_storage("%s apparently does not exist !\n", p_disk->dev_name);

	return -1;
}


/*******************************************************************************
* 函数名: hdd_power_on
* 功  能: 硬盘上电
* 参  数: hdd_num，硬盘号
* 返回值: 成功，返回0；失败，返回-1
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
//		disk_mount(hdd_num); 	/* 首次探测到硬盘，不在此处进行挂载 */
//	}

	return 0;
}


/*******************************************************************************
* 函数名: hdd_power_down
* 功  能: 硬盘掉电
* 参  数: hdd_num，硬盘号
* 返回值: 成功，返回0
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
* 函数名: mmc_choose
* 功  能: 选择使用SD卡的哪个分区
* 参  数: partition_num，分区号
* 返回值: 成功，返回硬盘号；失败，返回-1
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
* 函数名: mmc_enable
* 功  能: 启用SD卡
* 返回值: 成功，返回0；失败，返回-1
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
* 函数名: mmc_clear
* 功  能: SD卡清理
* 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int mmc_clear(void)
{
	int i;
	int partition_num;
	int last, next;
	PartitionInfo *next_partition = NULL;

	if (cur_disk_num == DISK_IS_MMC) 	/* 不对当前使用的分区进行清理 */
	{
		partition_num 	= MMC_PARTITION_NUM - 1;
		last 			= cur_partition_num;
	}
	else
	{
		partition_num 	= MMC_PARTITION_NUM;
		last 			= disk_mmc.disk_info.disk_flag.last_partition;
	}

	/* 先尝试，只清理已经上传过的文件 */
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

	/* 没有理想结果的话，再清理整个分区 */
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
* 函数名: hdd_clear
* 功  能: 硬盘清理
* 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int hdd_clear(void)
{
	int i, j;
	int disk_num; 		/* 磁盘号 */
	int partition_num; 	/* 分区数，命名容易误解 */
	int last, next;
	DiskInfo *p_disk;
	PartitionInfo *next_partition;

	for (i=0; i<HDD_NUM; i++)
	{
		if ( (cur_disk_num >= 0) && (cur_disk_num < HDD_NUM) &&
		        (cur_partition_num == (HDD_PARTITION_NUM - 1)) )
			/* 如果当前分区已经是硬盘最后一个分区，那么先测试下个硬盘 */
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

		if (cur_disk_num == disk_num)	/* 不对当前使用的分区进行清理 */
		{
			partition_num 	= HDD_PARTITION_NUM - 1;
			last 			= cur_partition_num;
		}
		else
		{
			partition_num 	= HDD_PARTITION_NUM;
			last 			= p_disk->disk_flag.last_partition;
		}

		/* 先尝试，只清理已经上传过的文件 */
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
			/* 如果当前分区已经是硬盘最后一个分区，那么先测试下个硬盘 */
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

		if (cur_disk_num == disk_num)	/* 不对当前使用的分区进行清理 */
		{
			partition_num 	= HDD_PARTITION_NUM - 1;
			last 			= cur_partition_num;
		}
		else
		{
			partition_num 	= HDD_PARTITION_NUM;
			last 			= p_disk->disk_flag.last_partition;
		}

		/* 没有理想结果的话，再清理整个分区 */
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
