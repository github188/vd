
#ifndef _DISK_FUNC_H_
#define _DISK_FUNC_H_

#include "disk_mng.h"

#define DISK_MIN_SIZE 		2000		//本程序支持的最小磁盘容量，单位：MB
#define DISK_MAX_SIZE 		2600000		//本程序支持的最大磁盘容量，单位：MB

#define DISK_FLAG_POSITION 	0x0200		//DiskFlag的位置：磁盘第二个扇区
#define DISK_MAGIC_NUM 		0x4853		//磁盘标识魔数，十六进制的“HS”

#define MMC_DEV_NAME 		"/dev/disk/by-path/platform-mmci-omap-hs.1"
#define HDD_DEV_NAME 		"/dev/disk/by-path/platform-ahci.0-scsi-0:0:0:0"
#define HDD_DEV_NAME_DIFF_POS 	41 		//硬盘设备名称差异位的位置

#define HDD_POWER_ON_TIME 		30 		//硬盘上电后循环检测的时间，单位：秒
#define HDD_STANDBY_INTERVAL 	120 	//硬盘进入休眠状态的超时时间间隔，10分钟

#define PARTITION_TABLE_POSITION 	0x01BE 	//磁盘分区表起始位置
#define PARTITION_TABLE_SIZE 		64 		//磁盘分区表大小，单位：字节
#define LINUX_NATIVE 				0x83	//Linux分区类型

#define DISK_PARTITION 		"/opt/ipnc/disk_partition.sh" 	//磁盘分区脚本


int disk_status_init(void);
int disk_get_info(int disk_num);
int disk_check_size(int disk_num);
DiskInfo *which_disk(int disk_num);
int disk_get_size(const char *dev_name, unsigned long long *bytes);
int disk_read_flag(const char *dev_name, DiskFlag *disk_flag);
int is_new_disk(int disk_num);
int partition_table_check(int disk_num);
int partition_table_read(int disk_num, PartitionTable *p_table);
int partition_table_should_be(int disk_num, PartitionTable *p_table);
int partition_table_clear(const char *dev_name);
int disk_init(int disk_num);
int disk_format(int disk_num);
int disk_umount(int disk_num);
char *disk_mount_point_find(const char *dev_name);
int disk_partition(int disk_num);
int disk_init_flag(int disk_num);
unsigned int disk_id_create(void);
int disk_write_flag(const char *dev_name, const DiskFlag *disk_flag);
int disk_mount(int disk_num);
void disk_status_check(void);
void disk_maintain(void);
int hdd_standby_interval(const char *dev_name, int interval);
int hdd_choose(int *hdd_num, int *partition_num);
int hdd_enable(void);
int hdd_wait_for_ready(int hdd_num);
int hdd_power_on(int hdd_num);
int hdd_power_down(int hdd_num);
void hdd_power_down_others(void);
int mmc_enable(void);
int mmc_choose(int *partition_num);
int reread_partition_table(const char *dev_name);
int mmc_clear(void);
int hdd_clear(void);

#endif 	/* _DISK_FUNC_H_ */
