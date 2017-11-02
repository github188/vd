
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/magic.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <mntent.h>
#include <dirent.h>
#include <fcntl.h>

#include "partition_func.h"
#include "storage_common.h"
#include "disk_func.h"
#include "crc16.h"


/*******************************************************************************
 * 函数名: partition_name_cal
 * 功  能: 计算分区名称
 * 参  数: disk_num，磁盘号；partition_num，分区号；partition_name，分区名称
 * 返回值: 成功，返回0；读取链接失败，返回-1；参数错误，返回-2
*******************************************************************************/
int partition_name_cal(int disk_num, int partition_num, char *partition_name)
{
	if (disk_num == DISK_IS_MMC)
	{
		sprintf(partition_name, "%s-part%d",
		        disk_mmc.disk_info.dev_name, partition_num + 1);
		/* 分区号从0开始，而分区设备号是从1开始的，所以要加1 */
	}
	else if ((disk_num >= 0) && (disk_num < HDD_NUM))
	{
		sprintf(partition_name, "%s-part%d",
		        disk_hdd[disk_num].disk_info.dev_name, partition_num+1);
		/* 分区号从0开始，而分区设备号是从1开始的，所以要加1 */
	}
	else
	{
		memset(partition_name, 0, PARTITION_NAME_LEN);
		log_warn_storage("Wrong Disk Number : %d !\n", disk_num);
		return -2;
	}

	char original_name[PARTITION_NAME_LEN];

	if (realpath(partition_name, original_name) == NULL)
	{
		log_warn_storage("realpath %s : %m !\n", partition_name);
		return -1;
	}

	strncpy(partition_name, original_name, PARTITION_NAME_LEN);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_format
 * 功  能: 分区格式化
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_format(int disk_num, int partition_num)
{
	pid_t pid;
	int status;
	PartitionStatus old_partition_status;
	char partition_name[PARTITION_NAME_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	if (access(partition_name, F_OK) != 0)
	{
		log_state_storage("%s apparently does not exist!\n", partition_name);
		return -1;
	}

	if (access(MKFS, X_OK) < 0)
	{
		log_error_storage("Is %s existing and executable ? Error: %m !\n",
		                  MKFS);
		return -1;
	}

	if (partition_umount(disk_num, partition_num) < 0)
		return -1; 	/* 分区处于挂载状态时无法格式化 */

	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1; 	/* 理论上，不会出现这种情况 */

	old_partition_status = p_partition->status;
	p_partition->status = PARTITION_FORMAT;

	pid = vfork();
	if (pid == 0)
	{
		execl(MKFS, MKFS, partition_name, NULL);
	}

	waitpid(pid, &status, 0);

	p_partition->status = old_partition_status;

	if (!WIFEXITED(status))
	{
		log_warn_storage("Partition format exit abnormally !\n");
		return -1;
	}

	log_debug_storage("Format %s done!\n", partition_name);

	return 0;
}


/*******************************************************************************
 * 函数名: which_partition
 * 功  能: 根据磁盘号和分区号计算是哪个分区
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回分区指针；失败，返回NULL
*******************************************************************************/
PartitionInfo *which_partition(int disk_num, int partition_num)
{
	do
	{
		if (disk_num == DISK_IS_MMC)
		{
			if ( (partition_num >= 0) && (partition_num < MMC_PARTITION_NUM) )
				return &(disk_mmc.partition_info[partition_num]);
			else
				break;
		}
		else if ((disk_num >= 0) && (disk_num < HDD_NUM))
		{
			if ( (partition_num >= 0) && (partition_num < HDD_PARTITION_NUM) )
				return &(disk_hdd[disk_num].partition_info[partition_num]);
			else
				break;
		}
		else
			break;
	}
	while (0);

	log_warn_storage("Wrong Disk Number : %d , or Partition Number: %d !\n",
	                 disk_num, partition_num);
	return NULL;
}


/*******************************************************************************
 * 函数名: partition_umount
 * 功  能: 卸载指定分区
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_umount(int disk_num, int partition_num)
{
	int i;
	char *mount_point = NULL;

	while ((mount_point = partition_mount_point_find(disk_num, partition_num))
	        != NULL)
	{
		for (i=0; i<RETRY_TIMES; i++)
		{
			sync();		/* 将系统缓冲区的内容写入磁盘，以确保数据同步。 */

			log_debug_storage("Try to umount %s\n", mount_point);

			if (umount(mount_point) != 0)
			{
				log_debug_storage("Umount %s failed %d time(s) : %m !\n",
				                  mount_point, i);
				u_sleep(1, 0);
			}
			else
			{
				log_debug_storage("Umount %s succeed!\n", mount_point);

				if (rmdir(mount_point) < 0)
				{
					log_debug_storage("rmdir %s : %m !", mount_point);
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
 * 函数名: partition_mount_point_find
 * 功  能: 查询分区挂载点
 * 参  数: disk_num，分区所在磁盘号；partition_num，分区号
 * 返回值: 成功，返回挂载点；失败，返回NULL
*******************************************************************************/
char *partition_mount_point_find(int disk_num, int partition_num)
{
	FILE *mtab_fp = NULL;
	struct mntent *mountEntry = NULL;
	char partition_name[PARTITION_NAME_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return NULL;

	mtab_fp = setmntent("/etc/mtab", "r");
	if (!mtab_fp)
	{
		log_warn_storage("setmntent /etc/mtab : %m !\n");
		return NULL;
	}

	while ((mountEntry = getmntent(mtab_fp)) != NULL)
	{
		if (strcmp(partition_name, mountEntry->mnt_fsname) == 0)
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
 * 函数名: mount_point_set
 * 功  能: 设置设备挂载点（挂载路径规则: /挂载点路径/设备ID/分区号）
 * 参  数: disk_num，磁盘号；partition_num，分区号；mount_point，设备挂载点
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int mount_point_set(int disk_num, int partition_num, char *mount_point)
{
	if (disk_num == DISK_IS_MMC)
	{
		sprintf(mount_point, "%s/%d",
		        MMC_MOUNT_POINT,
		        partition_num + 1);
		return 0;
	}
	else if ((disk_num >= 0) && (disk_num < HDD_NUM))
	{
		sprintf(mount_point, "%s/%d",
		        HDD_MOUNT_POINT,
		        partition_num + 1);
		*(mount_point + HDD_MNT_DIFF_POS) += disk_num;
		return 0;
	}
	else
	{
		memset(mount_point, 0, MOUNT_POINT_LEN);
		log_warn_storage("Wrong Disk Number : %d !\n", disk_num);
		return -1;
	}
}


/*******************************************************************************
 * 函数名: partition_mount
 * 功  能: 挂载指定分区
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_mount(int disk_num, int partition_num, const void *option)
{
	if (mount_point_check(disk_num, partition_num) == 0)
		return 0;

	char partition_name[PARTITION_NAME_LEN];
	char mount_point[MOUNT_POINT_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	mount_point_set(disk_num, partition_num, mount_point);

	if (access(mount_point, F_OK) == 0)
	{
		/* 删除可能出现的误写入Flash上的文件 */
		remove_recursively(mount_point);
	}

	if (mkpath(mount_point, 0755) < 0)
	{
		log_warn_storage("make %s failed !\n", mount_point);
		return -1;
	}

	if (mount(partition_name, mount_point, FILE_SYSTEM_TYPE, 0, option) < 0)
	{
		log_warn_storage("mount %s at %s : %m !\n",
		                 partition_name, mount_point);
		if (access(mount_point, F_OK) == 0)
		{
			if (rmdir(mount_point) < 0)
			{
				log_debug_storage("rmdir %s : %m !\n", mount_point);
			}
		}
		return -1;
	}

	log_debug_storage("mount %s at %s succeed !\n",
	                  partition_name, mount_point);

	return 0;
}


/*******************************************************************************
 * 函数名: mount_point_check
 * 功  能: 检查设备挂载点是否正确
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 挂载正确，返回0；挂载不正确，返回-1
*******************************************************************************/
int mount_point_check(int disk_num, int partition_num)
{
	int ret = -1;
	char expect_mount_point[MOUNT_POINT_LEN];

	if (mount_point_set(disk_num, partition_num, expect_mount_point) < 0)
	{
		log_warn_storage("Please check disk number !\n");
		return ret;
	}

	FILE *mtab_fp = NULL;
	struct mntent *mountEntry = NULL;
	char partition_name[PARTITION_NAME_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	mtab_fp = setmntent("/etc/mtab", "r");
	if (!mtab_fp)
	{
		log_warn_storage("setmntent /etc/mtab : %m !\n");
		return ret;
	}

	while ((mountEntry = getmntent(mtab_fp)) != NULL)
	{
		if ( (strcmp(partition_name, mountEntry->mnt_fsname) == 0) &&
		        (strcmp(expect_mount_point, mountEntry->mnt_dir) == 0) )
		{
			ret = 0;
			break;
		}
	}

	endmntent(mtab_fp);

	return ret;
}


/*******************************************************************************
 * 函数名: partition_status_init
 * 功  能: 分区状态信息初始化
 * 参  数: disk_num，磁盘号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_status_init(int disk_num)
{
	int i;
	unsigned int sectors_num;
	PartitionTable partition_table;

	partition_table_read(disk_num, &partition_table);

	if (disk_num == DISK_IS_MMC)
	{
		for (i=0; i<MMC_PARTITION_NUM; i++)
		{
			sectors_num = partition_table.partition_entry[i].sectors_num;
			disk_mmc.partition_info[i].block_group_num =
			    ((sectors_num - 1) >> 18) + 1;
			/********************************************************
			 * 块组数 = 分区大小 / (块大小*每个块组的块数)，进一取整
			 * 分区大小 = 扇区数 * 512
			 * 默认块大小为 4096 = 2^12
			 * 默认每个块组的块数为 8*blocksize = 8*4096 = 2^15
			 ********************************************************/

			if (partition_status_check(disk_num, i) == 0)
				partition_sb_backup(disk_num, i);
		}

		return 0;
	}
	else if ((disk_num >= 0) && (disk_num < HDD_NUM))
	{
		for (i=0; i<HDD_PARTITION_NUM; i++)
		{
			sectors_num = partition_table.partition_entry[i].sectors_num;
			disk_hdd[disk_num].partition_info[i].block_group_num =
			    ((sectors_num - 1) >> 18) + 1;

			if (partition_status_check(disk_num, i) == 0)
				partition_sb_backup(disk_num, i);
		}

		return 0;
	}
	else
	{
		log_warn_storage("Wrong Disk Number : %d !\n", disk_num);
		return -1;
	}
}


/*******************************************************************************
 * 函数名: partition_status_check
 * 功  能: 检查指定分区的状态
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 正常，返回0；异常，返回-1
*******************************************************************************/
int partition_status_check(int disk_num, int partition_num)
{
	char mount_point[MOUNT_POINT_LEN];
	struct statfs partition_statfs;
	unsigned long blocks_used = 0;

	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	if (p_partition->status != PARTITION_OK)
	{
		log_state_storage("Disk %d partition %d is abnormal ! "
		                  "Skip over checking it.\n",
		                  disk_num, partition_num);
		return -1;
	}

	log_state_storage("Check disk %d partition %d status now!\n",
	                  disk_num, partition_num);

	if (partition_mount(disk_num, partition_num, NULL) < 0)
	{
		p_partition->status = PARTITION_ABNORMAL;
		return -1;
	}

	mount_point_set(disk_num, partition_num, mount_point);

	if (statfs(mount_point, &partition_statfs) < 0)
	{
		log_state_storage("Get %s statistics failed : %m !\n", mount_point);
		p_partition->status = PARTITION_ABNORMAL;
		return -1;
	}

	/* 计算分区使用率，参考busybox的df */
	blocks_used = partition_statfs.f_blocks - partition_statfs.f_bfree;
	if ((blocks_used + partition_statfs.f_bavail) > 0)
	{
		p_partition->percent_used =
		    (blocks_used * 100ULL + (blocks_used + partition_statfs.f_bavail)/2)
		    / (blocks_used + partition_statfs.f_bavail);
		log_state_storage("%s used %d%%.\n",
		                  mount_point, p_partition->percent_used);
	}
	else
	{
		p_partition->status = PARTITION_ABNORMAL;
		log_state_storage("The size of %s is abnormal !\n", mount_point);
		return -1;
	}

	if (partition_statfs.f_type != EXT3_SUPER_MAGIC)
	{
		p_partition->status = PARTITION_ABNORMAL;
		log_state_storage("Partition type is 0x%X, this is abnormal !\n",
		                  partition_statfs.f_type);
		return -1;
	}

	p_partition->status = PARTITION_OK;
	p_partition->data_to_upload = is_data_to_upload(mount_point);

	return 0;
}


/*******************************************************************************
 * 函数名: is_data_to_upload
 * 功  能: 判断是否有待上传数据
 * 参  数: path_name，路径名称
 * 返回值: 有，返回1；没有，返回0
*******************************************************************************/
int is_data_to_upload(const char *path_name)
{
	/* TODO 由于存储策略更改，需要重写 */
	return 0;
}


/*******************************************************************************
 * 函数名: partition_sb_backup
 * 功  能: 分区超级块和块组描述等信息备份
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_sb_backup(int disk_num, int partition_num)
{
	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	if (p_partition->status != PARTITION_OK)
	{
		log_debug_storage("Disk %d partition %d is abnormal ! "
		                  "Skip over backuping its super block.\n",
		                  disk_num, partition_num);
		return -1;
	}

	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	char sb_bak[32];
	sprintf(sb_bak, "%s/%u/%d",
	        DISK_INFO_DIR, p_disk->disk_flag.id, partition_num);

	FILE *bak_file = fopen(sb_bak, "rb");
	if (bak_file != NULL)
	{
		log_debug_storage("Open %s succeed !\n", sb_bak);
		fclose(bak_file);
		return 0;
	}

	char partition_name[PARTITION_NAME_LEN];
	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	int partition_fd = open(partition_name, O_RDONLY);
	if (partition_fd < 0)
	{
		log_warn_storage("Open %s faild : %m !\n", partition_name);
		return -1;
	}

	ssize_t bak_size =
	    p_partition->block_group_num * GRP_DESC_SIZE + FS_BLOCK_SIZE;

	void *sb_bak_buf = malloc(bak_size);
	if (sb_bak_buf == NULL)
	{
		close(partition_fd);
		log_warn_storage("Allocate memory for sb_bak_buf failed !\n");
		return -1;
	}

	ssize_t read_size = read(partition_fd, sb_bak_buf, bak_size);
	close(partition_fd);

	if (read_size != bak_size)
	{
		safe_free(sb_bak_buf);
		log_warn_storage("Read %s failed, need %d but read %d.",
		                 partition_name, bak_size, read_size);
		return -1;
	}

	mk_disk_info_dir(disk_num);

	bak_file = fopen(sb_bak, "wb");
	if (bak_file == NULL)
	{
		log_warn_storage("Create %s failed : %m !\n", sb_bak);
		safe_free(sb_bak_buf);
		return -1;
	}

	fwrite(sb_bak_buf, bak_size, 1, bak_file);
	fflush(bak_file);
	fclose(bak_file);
	safe_free(sb_bak_buf);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_sb_readbak
 * 功  能: 读取备份的分区超级块和块组描述等信息
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_sb_readbak(int disk_num, int partition_num, void *buf)
{
	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	char sb_bak[32];
	sprintf(sb_bak, "%s/%u/%d",
	        DISK_INFO_DIR, p_disk->disk_flag.id, partition_num);

	FILE *bak_file = fopen(sb_bak, "rb");
	if (bak_file == NULL)
	{
		log_warn_storage("Open %s failed : %m !\n", sb_bak);
		return -1;
	}

	size_t bak_size =
	    p_partition->block_group_num * GRP_DESC_SIZE + FS_BLOCK_SIZE;

	size_t read_size = fread(buf, 1, bak_size, bak_file);
	fclose(bak_file);

	if (read_size != bak_size)
	{
		log_warn_storage("Read %s failed, need %d but read %d.",
		                 sb_bak, bak_size, read_size);
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: ext2fs_group_desc_csum
 * 功  能: 分区块组描述信息校验码计算
 * 参  数: super_block，超级块指针；group，块组号；group_desc，块组描述
 * 返回值: crc，校验结果
*******************************************************************************/
__u16 ext2fs_group_desc_csum(struct ext4_super_block *super_block,
                             int group, struct ext4_group_desc *group_desc)
{
	__u16 crc = 0;

	if (super_block->s_feature_ro_compat & 0x0010)
	{
		int offset = (size_t) &((struct ext4_group_desc *)0)->bg_checksum;

		crc = ext2fs_crc16(~0, super_block->s_uuid,
		                   sizeof(super_block->s_uuid));
		crc = ext2fs_crc16(crc, &group, sizeof(group));
		crc = ext2fs_crc16(crc, group_desc, offset);
		offset += sizeof(group_desc->bg_checksum);

		if (offset < super_block->s_desc_size)
		{
			crc = ext2fs_crc16(crc, (char *)group_desc + offset,
			                   super_block->s_desc_size - offset);
		}
	}

	return crc;
}


/*******************************************************************************
 * 函数名: partition_sb_restore
 * 功  能: 分区超级块和块组描述等信息还原
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_sb_restore(int disk_num, int partition_num)
{
	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	char partition_name[PARTITION_NAME_LEN];
	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	int partition_fd = open(partition_name, O_RDWR);
	if (partition_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", partition_name);
		return -1;
	}

	size_t bak_size =
	    p_partition->block_group_num * GRP_DESC_SIZE + FS_BLOCK_SIZE;

	void *sb_bak_buf = malloc(bak_size);
	if (sb_bak_buf == NULL)
	{
		close(partition_fd);
		log_warn_storage("Allocate memory for sb_bak_buf failed !\n");
		return -1;
	}

	if (partition_sb_readbak(disk_num, partition_num, sb_bak_buf) < 0)
	{
		close(partition_fd);
		safe_free(sb_bak_buf);
		return -1;
	}

	char block_buf[FS_BLOCK_SIZE];
	struct ext4_super_block *super_block = NULL;
	struct ext4_group_desc 	*group_desc  = NULL;
	off_t offset;
	int block_free = 0;
	int inode_free = 0;
	int block_free_total = 0;
	int inode_free_total = 0;
	unsigned int i, j, k;

	super_block = (struct ext4_super_block *)
	              ((char *)sb_bak_buf + PADDING_SIZE);

	for (i=0; i<p_partition->block_group_num; i++)
	{
		block_free = 0;
		inode_free = 0;
		group_desc = (struct ext4_group_desc *)
		             ((char *)sb_bak_buf + FS_BLOCK_SIZE + i*GRP_DESC_SIZE);

		offset = (off_t)(group_desc->bg_block_bitmap_lo) * FS_BLOCK_SIZE;
		lseek(partition_fd, offset, SEEK_SET);
		read(partition_fd, block_buf, FS_BLOCK_SIZE);

		for (j=0; j<(super_block->s_blocks_per_group/8); j++)
		{
			for (k=0; k<8; k++) 	/* 一个字节8位 */
			{
				block_free += 1-((block_buf[j]>>k)&0x01);
			}
		}

		offset = (off_t)(group_desc->bg_inode_bitmap_lo) * FS_BLOCK_SIZE;
		lseek(partition_fd, offset, SEEK_SET);
		read(partition_fd, block_buf, FS_BLOCK_SIZE);

		for (j=0; j<(super_block->s_inodes_per_group/8); j++)
		{
			for (k=0; k<8; k++) 	/* 一个字节8位 */
			{
				inode_free += 1-((block_buf[j]>>k)&0x01);
			}
		}

		group_desc->bg_free_blocks_count_lo = block_free;
		group_desc->bg_free_inodes_count_lo = inode_free;
		group_desc->bg_checksum =
		    ext2fs_group_desc_csum(super_block, i, group_desc);

		block_free_total += block_free;
		inode_free_total += inode_free;
	}

	super_block->s_free_blocks_count_lo = block_free_total;
	super_block->s_free_inodes_count 	= inode_free_total;

	offset = 0;
	lseek(partition_fd, offset, SEEK_SET);
	write(partition_fd, sb_bak_buf, bak_size);
	fdatasync(partition_fd);
	close(partition_fd);

	safe_free(sb_bak_buf);

	return partition_wr_test(disk_num, partition_num);
}


/*******************************************************************************
 * 函数名: partition_sb_copy
 * 功  能: 使用Ext4的首个超级块和组描述信息冗余副本修复超级块和组描述信息
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_sb_copy(int disk_num, int partition_num)
{
	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	if (partition_mount(disk_num, partition_num, SB_COPY_OPTION) < 0)
	{
		log_warn_storage("Mount disk %d with %s failed !\n",
		                 disk_num, SB_COPY_OPTION);
		return -1; 	/* 如果使用备份信息也挂载不上，那就没必要用它了 */
	}

	partition_umount(disk_num, partition_num);

	char partition_name[PARTITION_NAME_LEN];
	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	int partition_fd = open(partition_name, O_RDWR);
	if (partition_fd < 0)
	{
		log_warn_storage("Open %s faild!!!\n", partition_name);
		return -1;
	}

	ssize_t copy_size =
	    p_partition->block_group_num * GRP_DESC_SIZE + FS_BLOCK_SIZE;

	void *block_buf = malloc(copy_size);
	if (block_buf == NULL)
	{
		close(partition_fd);
		log_warn_storage("Allocate memory for block_buf failed !\n");
		return -1;
	}

	lseek(partition_fd, SB_COPY_POSITION, SEEK_SET);
	ssize_t read_size = read(partition_fd, block_buf, copy_size);
	if (read_size != copy_size)
	{
		close(partition_fd);
		safe_free(block_buf);
		log_warn_storage("Read %s failed, need %d but read %d.",
		                 partition_name, copy_size, read_size);
		return -1;
	}

	lseek(partition_fd, PADDING_SIZE, SEEK_SET);
	write(partition_fd, block_buf, SUPER_BLOCK_SIZE);

	lseek(partition_fd, FS_BLOCK_SIZE, SEEK_SET);
	write(partition_fd, (void *)((char *)block_buf+FS_BLOCK_SIZE),
	      (copy_size-FS_BLOCK_SIZE));

	close(partition_fd);

	safe_free(block_buf);

	return partition_wr_test(disk_num, partition_num);
}


/*******************************************************************************
 * 函数名: partition_clear_journal
 * 功  能: 清除分区日志
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_clear_journal(int disk_num, int partition_num)
{
	pid_t pid;
	int status;
	char partition_name[PARTITION_NAME_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	if (access(partition_name, F_OK) != 0)
	{
		log_warn_storage("%s apparently does not exist!\n", partition_name);
		return -1;
	}

	if (access(TUNE2FS, X_OK) < 0)
	{
		log_error_storage("Is %s existing and executable ? Error: %m !\n",
		                  TUNE2FS);
		return -1;
	}

	if (partition_umount(disk_num, partition_num) < 0)
		return -1;

	char *tune2fs_argv[] =
	{
		(char *)TUNE2FS,
		(char *)"-O",
		(char *)"^has_journal",
		partition_name,
		NULL
	};

	pid = vfork();
	if (pid == 0)
	{
		execv(TUNE2FS, tune2fs_argv);
	}

	waitpid(pid, &status, 0);

	if (!WIFEXITED(status))
		return -1;

	log_debug_storage("Clear journal of %s done!\n", partition_name);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_add_journal
 * 功  能: 添加分区日志
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_add_journal(int disk_num, int partition_num)
{
	pid_t pid;
	int status;
	char partition_name[PARTITION_NAME_LEN];

	if (partition_name_cal(disk_num, partition_num, partition_name) < 0)
		return -1;

	if (access(partition_name, F_OK) != 0)
	{
		log_warn_storage("%s apparently does not exist!\n", partition_name);
		return -1;
	}

	if (access(TUNE2FS, X_OK) < 0)
	{
		log_error_storage("Is %s existing and executable ? Error: %m !\n",
		                  TUNE2FS);
		return -1;
	}

	if (partition_umount(disk_num, partition_num) < 0)
		return -1;

	char *tune2fs_argv[] =
	{
		(char *)TUNE2FS,
		(char *)"-j",
		partition_name,
		NULL
	};

	pid = vfork();
	if (pid == 0)
	{
		execv(TUNE2FS, tune2fs_argv);
	}

	waitpid(pid, &status, 0);

	if (!WIFEXITED(status))
		return -1;

	log_debug_storage("Add journal to %s done!\n", partition_name);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_wr_test
 * 功  能: 测试分区文件能否写读成功
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_wr_test(int disk_num, int partition_num)
{
	if (partition_mount(disk_num, partition_num, NULL) < 0)
	{
		log_warn_storage("Mount disk %d partition %d failed !\n",
		                 disk_num, partition_num);
		return -1;
	}

	char mount_point[MOUNT_POINT_LEN];
	char test_file_path[PATH_MAX_LEN];

	mount_point_set(disk_num, partition_num, mount_point);
	sprintf(test_file_path, "%s/%s/", mount_point, DISK_RECORD_PATH);

	if (mkpath(test_file_path, 0755) < 0)
	{
		log_state_storage("mkpath %s failed !\n", test_file_path);
		return -1;
	}

	strcat(test_file_path, "wr_test");

	if ((partition_write_test(test_file_path) == 0) &&
	        (partition_read_test(test_file_path) == 0))
	{
		remove(test_file_path);
		return 0;
	}

	return -1;
}


/*******************************************************************************
 * 函数名: partition_write_test
 * 功  能: 测试分区文件写入是否成功
 * 参  数: test_file_path，文件名
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_write_test(const char *test_file_path)
{
	FILE *file_test = fopen(test_file_path,"wb");
	if (file_test == NULL)
	{
		log_state_storage("Open %s failed : %m !\n", test_file_path);
		return -1;
	}

	int ret = fwrite(PARTITION_WR_TEST_TEXT, 1, sizeof(PARTITION_WR_TEST_TEXT),
	                 file_test);
	if (ret != sizeof(PARTITION_WR_TEST_TEXT))
	{
		log_state_storage("Open %s succeed, but write failed : %m !\n",
		                  test_file_path);
		ret = -1;
	}
	else
	{
		log_debug_storage("Open and write %s succeed!\n", test_file_path);
		ret = 0;
	}

	fflush(file_test);
	fclose(file_test);
	sync();

	return ret;
}


/*******************************************************************************
 * 函数名: partition_read_test
 * 功  能: 测试分区文件读取是否成功
 * 参  数: test_file_path，文件名
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_read_test(const char *test_file_path)
{
	FILE *file_test = fopen(test_file_path,"rb");
	if (file_test == NULL)
	{
		log_state_storage("Open %s failed : %m !\n", test_file_path);
		return -1;
	}

	char buf[sizeof(PARTITION_WR_TEST_TEXT)];

	memset(buf, 0, sizeof(PARTITION_WR_TEST_TEXT));

	fread(buf, sizeof(PARTITION_WR_TEST_TEXT), 1, file_test);

	fclose(file_test);

	if (strcmp(buf, PARTITION_WR_TEST_TEXT) != 0)
	{
		log_state_storage("Open %s succeed, but read get %s \n",
		                  test_file_path, buf);
		return -1;
	}

	log_debug_storage("Open and read %s succeed!\n", test_file_path);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_rescure_superblock
 * 功  能: 修复磁盘分区超级块信息
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_rescure_superblock(int disk_num, int partition_num)
{
	/* 先尝试使用Ext4的首个超级块和组描述信息冗余副本修复超级块和组描述信息 */
	if (partition_sb_copy(disk_num, partition_num) == 0)
		return 0;

	/* 若修复失败，再尝试用备份文件进行修复 */
	if (partition_sb_restore(disk_num, partition_num) == 0)
		return 0;

	return -1;
}


/*******************************************************************************
 * 函数名: partition_rescure_journal
 * 功  能: 修复磁盘分区日志信息
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_rescure_journal(int disk_num, int partition_num)
{
	if ( (partition_clear_journal(disk_num, partition_num) == 0) &&
	        (partition_add_journal(disk_num, partition_num) == 0) )
		return partition_wr_test(disk_num, partition_num);

	return -1;
}


/*******************************************************************************
 * 函数名: partition_rescure
 * 功  能: 修复磁盘分区
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_rescure(int disk_num, int partition_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	if (p_disk->is_usable == DISK_UNUSABLE)
		return -1;

	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return -1;

	if (p_partition->status != PARTITION_ABNORMAL)
		return -1; 	/* 分区处于其他状态时，不允许进行修复操作 */

	do
	{
		char partition_name[PARTITION_NAME_LEN];
		if (partition_name_cal(disk_num, partition_num, partition_name) < -1)
			return -1;

		if (access(partition_name, F_OK) != 0)
		{
			log_warn_storage("%s apparently does not exist!\n", partition_name);
			if (access(p_disk->dev_name, F_OK) != 0)
			{
				log_warn_storage("%s apparently does not exist!\n",
				                 p_disk->dev_name);
				hdd_power_down(disk_num); 	/* 不存在了就掉电 */
				return -1;
			}
			/* TODO 检查分区表（此处逻辑还有问题） */
			if (partition_table_check(disk_num) < 0)
			{
				disk_umount(disk_num);
				disk_partition(disk_num);
				if (disk_mount(disk_num) == 0)
					return 0;
				return disk_format(disk_num);
			}
		}

		p_partition->status = PARTITION_RESCRUE;

		if (partition_wr_test(disk_num, partition_num) == 0)
			break; 	/* 再确认一遍，以应对硬盘连接问题导致的异常 */

		if (partition_rescure_superblock(disk_num, partition_num) == 0)
			break;

		if (partition_rescure_journal(disk_num, partition_num) == 0)
			break;

		if ( (partition_format(disk_num, partition_num) == 0) &&
		        (partition_wr_test(disk_num, partition_num) == 0) )
			break; 	/* TODO 格式化失败呢？ */

		p_partition->status = PARTITION_DAMAGED; 	/* TODO 需要记入磁盘标识 */

		return -1;
	}
	while (0);

	p_partition->status = PARTITION_OK;

	return 0;
}


/*******************************************************************************
 * 函数名: partition_switch
 * 功  能: 切换当前用于存储的分区
*******************************************************************************/
int partition_switch(void)
{
	int j;
	int next;
	PartitionInfo *next_partition = NULL;

	if (cur_disk_num == DISK_IS_MMC)
	{
		/* 先检查SD卡有没有其他正常的空闲分区 */
		for (j=0; j<MMC_PARTITION_NUM; j++)
		{
			next = (disk_mmc.disk_info.disk_flag.last_partition + j)
			       % MMC_PARTITION_NUM;
			next_partition = &(disk_mmc.partition_info[next]);

			if ((next_partition->status == PARTITION_OK) &&
			        (next_partition->percent_used < PARTITION_FULL_PERCENT))
			{
				if (partition_mount(DISK_IS_MMC, next, NULL) == 0)
				{
					partition_mark_using(DISK_IS_MMC, next);
					return 0;
				}
			}
		}

		/* 如果SD卡已经没有正常的空闲分区了，那么启用硬盘 */
		if (hdd_enable() < 0)
		{
			/* 硬盘启用失败，进行磁盘分区清理 */
			if (mmc_clear() == 0)
				partition_switch();
			else
				return -1;
		}
	}
	else if ( (cur_disk_num >= 0) && (cur_disk_num < HDD_NUM) )
	{
		int i;
		int hdd_num = 0;

		for (i=0; i<HDD_NUM; i++)
		{
			hdd_num = (cur_disk_num+i) % HDD_NUM;

			if (disk_hdd[hdd_num].disk_info.is_detected != DISK_DETECTED)
				continue;

			for (j=0; j<HDD_PARTITION_NUM; j++)
			{
				next = (disk_hdd[hdd_num].disk_info.disk_flag.last_partition
				        + j) % HDD_PARTITION_NUM;
				next_partition = &(disk_hdd[hdd_num].partition_info[next]);

				if ((next_partition->status == PARTITION_OK) &&
				        (next_partition->percent_used < PARTITION_FULL_PERCENT))
				{
					if (disk_hdd[hdd_num].disk_info.is_usable == DISK_UNUSABLE)
					{
						if (hdd_power_on(hdd_num) < 0)
						{
							hdd_power_down(hdd_num);
							break;
						}
					}

					if (partition_mount(hdd_num, next, NULL) == 0)
					{
						partition_mark_using(hdd_num, next);

						hdd_power_down_others();

						return 0;
					}
				}
			}
		}

		/* 如果全部硬盘都没有正常的空闲分区了，进行硬盘分区清理 */
		if (hdd_clear() == 0)
			partition_switch(); 	/* ATTN 会不会陷入死循环？ */
		else
			return -1;
	}
	else
	{
		log_error_storage("Current disk number is wrong : %d\n", cur_disk_num);
		return -1;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: partition_mark_using
 * 功  能: 标记用于存储的分区
 * 参  数: disk_num，磁盘号；parititon_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_mark_using(int disk_num, int parititon_num)
{
	DiskInfo *p_disk = which_disk(disk_num);
	if (p_disk == NULL)
		return -1;

	if (partition_mount(disk_num, parititon_num, NULL) < 0)
		return -1;

	log_state_storage("Mark disk %d partition %d for using.\n",
	                  disk_num, parititon_num);

	p_disk->disk_flag.last_used = 1;
	p_disk->disk_flag.last_partition = parititon_num;
	disk_write_flag(p_disk->dev_name, &(p_disk->disk_flag));

	if (cur_disk_num != disk_num)
	{
		DiskInfo *last_disk = which_disk(cur_disk_num);
		if (last_disk != NULL)
		{
			last_disk->disk_flag.last_used = 0;
			cur_disk_num = disk_num;
			disk_write_flag(last_disk->dev_name, &(last_disk->disk_flag));
		}
	}

	set_cur_partition_path(disk_num, parititon_num);

	return 0;
}


/*******************************************************************************
 * 函数名: partition_clear_trash
 * 功  能: 清理分区垃圾箱
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_clear_trash(int disk_num, int partition_num)
{
	int rc = -1;
	char mount_point[MOUNT_POINT_LEN];

	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return rc;

	if (p_partition->status != PARTITION_OK)
		return rc;

	if (partition_mount(disk_num, partition_num, NULL) < 0)
	{
		p_partition->status = PARTITION_ABNORMAL;
		return rc;
	}

	mount_point_set(disk_num, partition_num, mount_point);

	char trash_path[MOUNT_POINT_LEN + NAME_MAX_LEN + 4];
	sprintf(trash_path, "%s/%s", mount_point, DISK_TRASH_PATH);

	p_partition->status = PARTITION_CLEAR;

	rc = remove_recursively(trash_path);

	p_partition->status = PARTITION_OK;

	partition_status_check(disk_num, partition_num);

	return rc;
}


/*******************************************************************************
 * 函数名: partition_clear_all
 * 功  能: 清理分区全部文件
 * 参  数: disk_num，磁盘号；partition_num，分区号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int partition_clear_all(int disk_num, int partition_num)
{
	int rc = -1;
	char mount_point[MOUNT_POINT_LEN];

	PartitionInfo *p_partition = which_partition(disk_num, partition_num);
	if (p_partition == NULL)
		return rc;

	if (p_partition->status != PARTITION_OK)
		return rc;

	if (partition_mount(disk_num, partition_num, NULL) < 0)
	{
		p_partition->status = PARTITION_ABNORMAL;
		return rc;
	}

	mount_point_set(disk_num, partition_num, mount_point);

	p_partition->status = PARTITION_CLEAR;

	DIR *dirp = NULL;
	struct dirent entry;
	struct dirent *result = NULL;
	char clear_path[PATH_MAX_LEN];

	dirp = opendir(mount_point);
	if (dirp != NULL)
	{
		for(;;)
		{
			readdir_r(dirp, &entry, &result);
			if (result == NULL)
				break;
			else if ( (strcmp(entry.d_name, ".") == 0) ||
			          (strcmp(entry.d_name, "..") == 0) ||
			          (strcmp(entry.d_name, "lost+found") == 0) )
				continue;
			else
			{
				sprintf(clear_path, "%s/%s", mount_point, entry.d_name);
				rc = remove_recursively(clear_path);
				if (rc < 0)
					break;
			}
		}

		closedir(dirp);
	}
	else
	{
		log_warn_storage("Open %s : %m !\n", mount_point);
	}

	p_partition->status = PARTITION_OK;

	partition_status_check(disk_num, partition_num);

	return rc;
}


/*******************************************************************************
 * 函数名: remove_recursively
 * 功  能: 递归删除
 * 注  意: 不支持通配符！
 * 参  数: remove_path，删除路径
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int remove_recursively(const char *remove_path)
{
	pid_t pid;
	int status;

	if (access(RM, X_OK) < 0)
	{
		log_error_storage("Is %s existing and executable ? Error: %m !\n", RM);
		return -1;
	}

	pid = vfork();
	if (pid == 0)
	{
		execl(RM, RM, "-rf", remove_path, NULL);
	}

	waitpid(pid, &status, 0);

	if (!WIFEXITED(status))
	{
		log_warn_storage("rm %s exit abnormally !\n", remove_path);
		return -1;
	}

	sync();

	log_debug_storage("Remove %s done!\n", remove_path);

	return 0;
}
