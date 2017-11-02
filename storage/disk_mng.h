
#ifndef _DISK_MNG_H_
#define _DISK_MNG_H_

#include <linux/types.h>


#define DISK_DETECT_TIME	120			//探测磁盘设备的时间间隔，单位：秒
#define DISK_CHECK_TIME		600			//检查磁盘状态的时间间隔，单位：秒


#ifdef 	_STORAGE_DEBUG_

#define PARTITION_FULL_PERCENT	85		//分区使用率达到85%时，认为分区已满

#else 	/* _STORAGE_DEBUG_ */

#include "arm_config.h"

extern ARM_config g_arm_config;

#define PARTITION_FULL_PERCENT 	\
	(100 - \
	 ((((g_arm_config.basic_param.data_save.disk_wri_data.remain_disk) < 10) ||\
	   ((g_arm_config.basic_param.data_save.disk_wri_data.remain_disk) > 50)) ?\
	  15 : (g_arm_config.basic_param.data_save.disk_wri_data.remain_disk)))

#endif 	/* _STORAGE_DEBUG_ */

#define PARTITION_EMPTY_PERCENT	10		//分区使用率小于10%时，认为分区为空


#define HDD_NUM				2			//硬盘数量
#define MBR_PARTITION_NUM 	4			//MBR支持的磁盘分区数量
#define HDD_PARTITION_NUM	4			//硬盘分区数量
#define MMC_PARTITION_NUM	2			//SD卡分区数量
#define DISK_NAME_LEN		48			//磁盘设备名称长度

#define PATH_MAX_LEN 		4096 		//本程序允许的最长绝对路径
#define NAME_MAX_LEN 		256 		//文件名称的最大长度

#define DISK_INFO_DIR 		"/var/disk_info" 	//存储磁盘信息的文件夹

#define DISK_RECORD_PATH 	"record" 		//档案箱，图像和视频记录放在这里
#define DISK_TRASH_PATH 	"trash" 		//垃圾箱，临时删除的记录放在这里
#define DISK_DB_PATH 		"DB" 			//数据库目录，所有数据库放在这里


typedef struct
{
	__u32 	margic_num; 		//魔数，用于判断磁盘是否经过本程序处理
	__u32 	id;					//本程序定义的磁盘ID，用于区分不同磁盘
	__u8 	last_used;			//上次用的是该磁盘则标记为1
	__u8 	last_partition;		//上次使用的分区号
	__u8 	damaged_partition; 	//已经损坏了的分区号
} DiskFlag;

typedef struct
{
	__u8 	status;							//分区活动状态
	__u8 	chs_first_head;					//CHS起始磁头号
	__u16 	chs_first_sector_cylinder;		//CHS起始扇区号和柱面号
	__u8 	type;							//分区类型
	__u8 	chs_last_head;					//CHS结束磁头号
	__u16 	chs_last_sector_cylinder;		//CHS结束扇区号和柱面号
	__u32 	lba_first_sector;				//LBA起始扇区号
	__u32 	sectors_num;					//分区扇区数
} PartitionEntry;

typedef struct
{
	PartitionEntry partition_entry[MBR_PARTITION_NUM];
} PartitionTable;

typedef enum
{
    PARTITION_DAMAGED = -1,		//分区损坏，修复失败
    PARTITION_OK,				//分区正常
    PARTITION_ABNORMAL,			//分区异常
    PARTITION_RESCRUE,			//分区正在被尝试修复
    PARTITION_FORMAT,			//分区正在被格式化
    PARTITION_CLEAR,			//分区正在被清理
} PartitionStatus;

typedef struct
{
	PartitionStatus		status;				//分区状态
	unsigned short		data_to_upload;		//分区是否有数据等待续传
	unsigned short		percent_used;		//分区使用率
	unsigned int 		block_group_num;	//分区块组数量
} PartitionInfo;


#define DISK_DETECTED 		1 		//探测到磁盘
#define DISK_UNDETECTED 	0 		//未探测到磁盘
#define DISK_SIZE_SUITABLE 	0 		//磁盘容量适合
#define DISK_SIZE_SMALL 	1 		//磁盘容量太小
#define DISK_SIZE_LARGE 	2 		//磁盘容量太大
#define DISK_USABLE 		1 		//磁盘处于可用状态
#define DISK_UNUSABLE 		0 		//磁盘处于不可用状态
#define DISK_FORMATTING 	1 		//磁盘正在被格式化
#define DISK_NOTFORMATTING 	0 		//磁盘没有正在被格式化


typedef struct
{
	DiskFlag disk_flag; 				//磁盘标识信息
	char dev_name[DISK_NAME_LEN]; 		//磁盘对应的设备名称，如/dev/sda
	unsigned long long size_in_byte;	//磁盘大小，单位：字节
	short is_detected;					//磁盘设备能否被探测到
	short is_size_suitable; 			//磁盘大小是否在本程序支持范围内
	short is_usable;					//磁盘是否处于可用状态
	short is_format;					//磁盘是否正在格式化
} DiskInfo;

typedef struct
{
	DiskInfo 		disk_info;							//SD卡信息
	PartitionInfo 	partition_info[MMC_PARTITION_NUM];	//SD卡分区的信息
} MMCInfo;

typedef struct
{
	DiskInfo 		disk_info;							//硬盘信息
	PartitionInfo 	partition_info[HDD_PARTITION_NUM];	//硬盘分区的信息
} HDDInfo;

enum
{
    DISK_CMD_FDISK,
    DISK_CMD_FORMAT,
    DISK_CMD_CLEAR,
    DISK_CMD_RESCURE,
}; 							//磁盘管理内部命令

typedef struct
{
	int 	cmd; 			//指令
	int 	disk_num; 		//磁盘号
	int 	partition_num; 	//分区号
} DiskCmd; 					//磁盘管理内部命令结构体


extern HDDInfo 	disk_hdd[HDD_NUM];
extern MMCInfo 	disk_mmc;

#define DISK_IS_MMC 	-1 	/* SD卡的编号 */

extern int 	cur_disk_num;
extern int 	cur_partition_num;


#ifdef  __cplusplus
extern "C"
{
#endif

int disk_mng_create(void);
void *disk_mng_tsk(void *argv);
void disk_mng_init(void);
int mk_disk_info_dir(int disk_num);
void disk_daemon(void);
void disk_check_now(void);
int disk_cmd_mng(DiskCmd *disk_cmd);
void *disk_cmd_process(void *disk_cmd);
int get_cur_partition_path(char *path);
int set_cur_partition_path(int disk_num, int parititon_num);
int disk_get_status(void);
//int db_record_path_park_get_status(void);    //add by lxd

int get_park_storage_path(char *path);
#ifdef  __cplusplus
}
#endif


#endif 	/* _DISK_MNG_H_ */


