
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

#define HDD_NUM				2	//Ӳ������
#define HDD_PARTITION_NUM	4	//Ӳ�̷�������
#define MMC_PARTITION_NUM 	2	//SD����������

#define DISK_DETECTED 		1 	//���̴���
#define DISK_UNDETECTED 	0 	//���̲�����
#define DISK_USABLE 		1 	//���̿���
#define DISK_UNUSABLE 		0 	//���̲�����

typedef enum
{
    PARTITION_DAMAGED = -1,		//�����𻵣��޸�ʧ��
    PARTITION_OK,				//��������
    PARTITION_ABNORMAL,			//�����쳣
    PARTITION_RESCRUE,			//�������ڱ������޸�
    PARTITION_FORMAT,			//�������ڱ���ʽ��
    PARTITION_CLEAR,			//�������ڱ�����
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

#define FS_CMD_HEAD_FLAG 	0x426974636f6d00ULL 	//�ַ���"Bitcom"

typedef enum
{
    FS_CMD_GET_STATUS, 		//��ȡ״̬����
    FS_CMD_LIST_DIR, 		//�г�Ŀ¼����
    FS_CMD_DOWNLOAD_FILE, 	//�����ļ�����
    FS_CMD_MOUNT_HDD, 		//����Ӳ������
} FileServCmd;

typedef enum
{
    RE_FS_CMD_CONN_ERROR 		= -8, 		//������λ��Serverʧ��
    RE_FS_CMD_MALLOC_ERROR 		= -7, 		//�ڴ�����ʧ��
    RE_FS_CMD_MAX_CONNECT 		= -6, 		//�ﵽ���������
    RE_FS_CMD_HEAD_ERROR 		= -5, 		//����ͷ����
    RE_FS_CMD_HANG_IGNORE 		= -4, 		//������ֹ����
    RE_FS_CMD_HANDLE_ERROR 		= -3, 		//����ִ�г���
    RE_FS_CMD_NOT_SUPPORT 		= -2, 		//�����֧��
    RE_FS_CMD_DATA_ERROR 		= -1, 		//�������ݴ���

    RE_FS_CMD_CONN_SUCCEED 		= 10, 		//���Ӵ����ɹ�
    RE_FS_CMD_RECV_SUCCEED 		= 11, 		//������ճɹ�
    RE_FS_CMD_HANDLE_SUCCEED 	= 12, 		//����ִ�гɹ�
} FileServCmdReply;

typedef struct
{
	unsigned long long  flag; 			//��ʶ���̶�ֵ FS_CMD_HEAD_FLAG
	FileServCmd 		cmd; 			//����
	unsigned int 		cmd_index; 		//�������
	FileServCmdReply 	reply; 			//��Ӧ
	unsigned long long  body_length; 	//��Ϣ���ֽ���
} FileServCmdHead; 						//��Ϣͷ�ṹ��

typedef struct
{
	PartitionStatus		status;			//����״̬
	unsigned int		percent_used;	//����ʹ�ðٷֱ�
} FileServPartitionInfo;

typedef struct
{
	unsigned int 		disk_id;		//��������Ĵ���ID
	unsigned long long 	size_in_byte;	//���̴�С����λ���ֽ�
	short 				is_detected;	//�����豸�ܷ�̽�⵽
	short 				is_usable;		//�����Ƿ��ڿ���״̬���Ƿ���أ�
} FileServDiskInfo;

typedef struct
{
	FileServDiskInfo 		disk_info;							//SD����Ϣ
	FileServPartitionInfo 	partition_info[MMC_PARTITION_NUM];	//SD��������Ϣ
} FileServMMCInfo;

typedef struct
{
	FileServDiskInfo 		disk_info;							//Ӳ����Ϣ
	FileServPartitionInfo 	partition_info[HDD_PARTITION_NUM];	//Ӳ�̷�����Ϣ
} FileServHddInfo;

typedef struct
{
	FileServMMCInfo		mmc_info;
	FileServHddInfo		hdd_info[HDD_NUM];
} FileServDiskStatus;

#endif 	/* _FILE_SYSTEM_SERVER_API_H_ */
