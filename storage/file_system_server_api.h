
#ifndef _FILE_SYSTEM_SERVER_API_H_
#define _FILE_SYSTEM_SERVER_API_H_

#ifdef WIN32

#include "linux_stat.h"

#define NAME_MAX_LEN 		256

struct dnode
{
	char 				dname[NAME_MAX_LEN];
	struct linux_stat 	dstat;
};

#define HDD_NUM				2	//硬盘数量
#define HDD_PARTITION_NUM	4	//硬盘分区数量
#define MMC_PARTITION_NUM 	2	//SD卡分区数量

#define DISK_DETECTED 		1 	//磁盘存在
#define DISK_UNDETECTED 	0 	//磁盘不存在
#define DISK_USABLE 		1 	//磁盘可用
#define DISK_UNUSABLE 		0 	//磁盘不可用

typedef enum
{
    PARTITION_DAMAGED = -1,		//分区损坏，修复失败
    PARTITION_OK,				//分区正常
    PARTITION_ABNORMAL,			//分区异常
    PARTITION_RESCRUE,			//分区正在被尝试修复
    PARTITION_FORMAT,			//分区正在被格式化
    PARTITION_CLEAR,			//分区正在被清理
} PartitionStatus;

#else 	/* WIN32 */

#include <sys/stat.h>
#include "disk_mng.h"
struct dnode
{
	char 		dname[NAME_MAX_LEN];
	struct stat dstat;
};

#endif 	/* WIN32 */


#define FS_SERVER_PORT 		34561
#define FS_LIST_PORT 		34562
#define FS_DOWNLOAD_PORT 	34563

#define FS_CMD_HEAD_FLAG 	0x426974636f6d00ULL 	//字符串"Bitcom"

typedef enum
{
    FS_CMD_GET_STATUS, 		//获取状态命令
    FS_CMD_LIST_DIR, 		//列出目录命令
    FS_CMD_DOWNLOAD_FILE, 	//下载文件命令
    FS_CMD_MOUNT_HDD, 		//挂载硬盘命令
} FileServCmd;

typedef enum
{
    RE_FS_CMD_CONN_ERROR 		= -8, 		//连接上位机Server失败
    RE_FS_CMD_MALLOC_ERROR 		= -7, 		//内存申请失败
    RE_FS_CMD_MAX_CONNECT 		= -6, 		//达到最大连接数
    RE_FS_CMD_HEAD_ERROR 		= -5, 		//命令头错误
    RE_FS_CMD_HANG_IGNORE 		= -4, 		//命令中止忽略
    RE_FS_CMD_HANDLE_ERROR 		= -3, 		//命令执行出错
    RE_FS_CMD_NOT_SUPPORT 		= -2, 		//命令不被支持
    RE_FS_CMD_DATA_ERROR 		= -1, 		//命令数据错误

    RE_FS_CMD_CONN_SUCCEED 		= 10, 		//连接创建成功
    RE_FS_CMD_RECV_SUCCEED 		= 11, 		//命令接收成功
    RE_FS_CMD_HANDLE_SUCCEED 	= 12, 		//命令执行成功
} FileServCmdReply;

typedef struct
{
	unsigned long long  flag; 			//标识，固定值 FS_CMD_HEAD_FLAG
	FileServCmd 		cmd; 			//命令
	unsigned int 		cmd_index; 		//命令序号
	FileServCmdReply 	reply; 			//回应
	unsigned long long  body_length; 	//消息体字节数
} FileServCmdHead; 						//消息头结构体

typedef struct
{
	PartitionStatus		status;			//分区状态
	unsigned int		percent_used;	//分区使用百分比
} FileServPartitionInfo;

typedef struct
{
	unsigned int 		disk_id;		//本程序定义的磁盘ID
	unsigned long long 	size_in_byte;	//磁盘大小，单位：字节
	short 				is_detected;	//磁盘设备能否被探测到
	short 				is_usable;		//磁盘是否处于可用状态（是否挂载）
} FileServDiskInfo;

typedef struct
{
	FileServDiskInfo 		disk_info;							//SD卡信息
	FileServPartitionInfo 	partition_info[MMC_PARTITION_NUM];	//SD卡分区信息
} FileServMMCInfo;

typedef struct
{
	FileServDiskInfo 		disk_info;							//硬盘信息
	FileServPartitionInfo 	partition_info[HDD_PARTITION_NUM];	//硬盘分区信息
} FileServHddInfo;

typedef struct
{
	FileServMMCInfo		mmc_info;
	FileServHddInfo		hdd_info[HDD_NUM];
} FileServDiskStatus;

#endif 	/* _FILE_SYSTEM_SERVER_API_H_ */
