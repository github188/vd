
#ifndef _DISK_MNG_H_
#define _DISK_MNG_H_

#include <linux/types.h>


#define DISK_DETECT_TIME	120			//̽������豸��ʱ��������λ����
#define DISK_CHECK_TIME		600			//������״̬��ʱ��������λ����


#ifdef 	_STORAGE_DEBUG_

#define PARTITION_FULL_PERCENT	85		//����ʹ���ʴﵽ85%ʱ����Ϊ��������

#else 	/* _STORAGE_DEBUG_ */

#include "arm_config.h"

extern ARM_config g_arm_config;

#define PARTITION_FULL_PERCENT 	\
	(100 - \
	 ((((g_arm_config.basic_param.data_save.disk_wri_data.remain_disk) < 10) ||\
	   ((g_arm_config.basic_param.data_save.disk_wri_data.remain_disk) > 50)) ?\
	  15 : (g_arm_config.basic_param.data_save.disk_wri_data.remain_disk)))

#endif 	/* _STORAGE_DEBUG_ */

#define PARTITION_EMPTY_PERCENT	10		//����ʹ����С��10%ʱ����Ϊ����Ϊ��


#define HDD_NUM				2			//Ӳ������
#define MBR_PARTITION_NUM 	4			//MBR֧�ֵĴ��̷�������
#define HDD_PARTITION_NUM	4			//Ӳ�̷�������
#define MMC_PARTITION_NUM	2			//SD����������
#define DISK_NAME_LEN		48			//�����豸���Ƴ���

#define PATH_MAX_LEN 		4096 		//����������������·��
#define NAME_MAX_LEN 		256 		//�ļ����Ƶ���󳤶�

#define DISK_INFO_DIR 		"/var/disk_info" 	//�洢������Ϣ���ļ���

#define DISK_RECORD_PATH 	"record" 		//�����䣬ͼ�����Ƶ��¼��������
#define DISK_TRASH_PATH 	"trash" 		//�����䣬��ʱɾ���ļ�¼��������
#define DISK_DB_PATH 		"DB" 			//���ݿ�Ŀ¼���������ݿ��������


typedef struct
{
	__u32 	margic_num; 		//ħ���������жϴ����Ƿ񾭹���������
	__u32 	id;					//��������Ĵ���ID���������ֲ�ͬ����
	__u8 	last_used;			//�ϴ��õ��Ǹô�������Ϊ1
	__u8 	last_partition;		//�ϴ�ʹ�õķ�����
	__u8 	damaged_partition; 	//�Ѿ����˵ķ�����
} DiskFlag;

typedef struct
{
	__u8 	status;							//�����״̬
	__u8 	chs_first_head;					//CHS��ʼ��ͷ��
	__u16 	chs_first_sector_cylinder;		//CHS��ʼ�����ź������
	__u8 	type;							//��������
	__u8 	chs_last_head;					//CHS������ͷ��
	__u16 	chs_last_sector_cylinder;		//CHS���������ź������
	__u32 	lba_first_sector;				//LBA��ʼ������
	__u32 	sectors_num;					//����������
} PartitionEntry;

typedef struct
{
	PartitionEntry partition_entry[MBR_PARTITION_NUM];
} PartitionTable;

typedef enum
{
    PARTITION_DAMAGED = -1,		//�����𻵣��޸�ʧ��
    PARTITION_OK,				//��������
    PARTITION_ABNORMAL,			//�����쳣
    PARTITION_RESCRUE,			//�������ڱ������޸�
    PARTITION_FORMAT,			//�������ڱ���ʽ��
    PARTITION_CLEAR,			//�������ڱ�����
} PartitionStatus;

typedef struct
{
	PartitionStatus		status;				//����״̬
	unsigned short		data_to_upload;		//�����Ƿ������ݵȴ�����
	unsigned short		percent_used;		//����ʹ����
	unsigned int 		block_group_num;	//������������
} PartitionInfo;


#define DISK_DETECTED 		1 		//̽�⵽����
#define DISK_UNDETECTED 	0 		//δ̽�⵽����
#define DISK_SIZE_SUITABLE 	0 		//���������ʺ�
#define DISK_SIZE_SMALL 	1 		//��������̫С
#define DISK_SIZE_LARGE 	2 		//��������̫��
#define DISK_USABLE 		1 		//���̴��ڿ���״̬
#define DISK_UNUSABLE 		0 		//���̴��ڲ�����״̬
#define DISK_FORMATTING 	1 		//�������ڱ���ʽ��
#define DISK_NOTFORMATTING 	0 		//����û�����ڱ���ʽ��


typedef struct
{
	DiskFlag disk_flag; 				//���̱�ʶ��Ϣ
	char dev_name[DISK_NAME_LEN]; 		//���̶�Ӧ���豸���ƣ���/dev/sda
	unsigned long long size_in_byte;	//���̴�С����λ���ֽ�
	short is_detected;					//�����豸�ܷ�̽�⵽
	short is_size_suitable; 			//���̴�С�Ƿ��ڱ�����֧�ַ�Χ��
	short is_usable;					//�����Ƿ��ڿ���״̬
	short is_format;					//�����Ƿ����ڸ�ʽ��
} DiskInfo;

typedef struct
{
	DiskInfo 		disk_info;							//SD����Ϣ
	PartitionInfo 	partition_info[MMC_PARTITION_NUM];	//SD����������Ϣ
} MMCInfo;

typedef struct
{
	DiskInfo 		disk_info;							//Ӳ����Ϣ
	PartitionInfo 	partition_info[HDD_PARTITION_NUM];	//Ӳ�̷�������Ϣ
} HDDInfo;

enum
{
    DISK_CMD_FDISK,
    DISK_CMD_FORMAT,
    DISK_CMD_CLEAR,
    DISK_CMD_RESCURE,
}; 							//���̹����ڲ�����

typedef struct
{
	int 	cmd; 			//ָ��
	int 	disk_num; 		//���̺�
	int 	partition_num; 	//������
} DiskCmd; 					//���̹����ڲ�����ṹ��


extern HDDInfo 	disk_hdd[HDD_NUM];
extern MMCInfo 	disk_mmc;

#define DISK_IS_MMC 	-1 	/* SD���ı�� */

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


