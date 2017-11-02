
#ifndef _PARTITION_FUNC_H_
#define _PARTITION_FUNC_H_

#include <linux/types.h>

#include "disk_mng.h"

/*
 * Structure of the super block
 */
struct ext4_super_block
{
	/*00*/
	__le32	s_inodes_count;			/* Inodes count */
	__le32	s_blocks_count_lo;		/* Blocks count */
	__le32	s_r_blocks_count_lo;	/* Reserved blocks count */
	__le32	s_free_blocks_count_lo;	/* Free blocks count */
	/*10*/
	__le32	s_free_inodes_count;	/* Free inodes count */
	__le32	s_first_data_block;		/* First Data Block */
	__le32	s_log_block_size;		/* Block size */
	__le32	s_obso_log_frag_size;	/* Obsoleted fragment size */
	/*20*/
	__le32	s_blocks_per_group;		/* # Blocks per group */
	__le32	s_obso_frags_per_group;	/* Obsoleted fragments per group */
	__le32	s_inodes_per_group;		/* # Inodes per group */
	__le32	s_mtime;				/* Mount time */
	/*30*/
	__le32	s_wtime;				/* Write time */
	__le16	s_mnt_count;			/* Mount count */
	__le16	s_max_mnt_count;		/* Maximal mount count */
	__le16	s_magic;				/* Magic signature */
	__le16	s_state;				/* File system state */
	__le16	s_errors;				/* Behaviour when detecting errors */
	__le16	s_minor_rev_level;		/* minor revision level */
	/*40*/
	__le32	s_lastcheck;			/* time of last check */
	__le32	s_checkinterval;		/* max. time between checks */
	__le32	s_creator_os;			/* OS */
	__le32	s_rev_level;			/* Revision level */
	/*50*/
	__le16	s_def_resuid;			/* Default uid for reserved blocks */
	__le16	s_def_resgid;			/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT4_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__le32	s_first_ino;			/* First non-reserved inode */
	__le16  s_inode_size;			/* size of inode structure */
	__le16	s_block_group_nr;		/* block group # of this superblock */
	__le32	s_feature_compat;		/* compatible feature set */
	/*60*/
	__le32	s_feature_incompat;		/* incompatible feature set */
	__le32	s_feature_ro_compat;	/* readonly-compatible feature set */
	/*68*/
	__u8	s_uuid[16];				/* 128-bit uuid for volume */
	/*78*/
	char	s_volume_name[16];		/* volume name */
	/*88*/
	char	s_last_mounted[64];		/* directory where last mounted */
	/*C8*/
	__le32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;		/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__le16	s_reserved_gdt_blocks;	/* Per group desc for online growth */
	/*
	 * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	/*D0*/
	__u8	s_journal_uuid[16];		/* uuid of journal superblock */
	/*E0*/
	__le32	s_journal_inum;			/* inode number of journal file */
	__le32	s_journal_dev;			/* device number of journal file */
	__le32	s_last_orphan;			/* start of list of inodes to delete */
	__le32	s_hash_seed[4];			/* HTREE hash seed */
	__u8	s_def_hash_version;		/* Default hash version to use */
	__u8	s_jnl_backup_type;
	__le16  s_desc_size;			/* size of group descriptor */
	/*100*/
	__le32	s_default_mount_opts;
	__le32	s_first_meta_bg;		/* First metablock block group */
	__le32	s_mkfs_time;			/* When the filesystem was created */
	__le32	s_jnl_blocks[17];		/* Backup of the journal inode */
	/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
	/*150*/
	__le32	s_blocks_count_hi;		/* Blocks count */
	__le32	s_r_blocks_count_hi;	/* Reserved blocks count */
	__le32	s_free_blocks_count_hi;	/* Free blocks count */
	__le16	s_min_extra_isize;		/* All inodes have at least # bytes */
	__le16	s_want_extra_isize; 	/* New inodes should reserve # bytes */
	__le32	s_flags;				/* Miscellaneous flags */
	__le16  s_raid_stride;			/* RAID stride */
	__le16  s_mmp_interval;         /* # seconds to wait in MMP checking */
	__le64  s_mmp_block;            /* Block for multi-mount protection */
	__le32  s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
	__u8	s_log_groups_per_flex;  /* FLEX_BG group size */
	__u8	s_reserved_char_pad;
	__le16  s_reserved_pad;
	__le64	s_kbytes_written;		/* nr of lifetime kilobytes written */
	__le32	s_snapshot_inum;		/* Inode number of active snapshot */
	__le32	s_snapshot_id;			/* sequential ID of active snapshot */
	__le64	s_snapshot_r_blocks_count; /* reserved blocks for active
										  snapshot's future use */
	__le32	s_snapshot_list;		/* inode number of the head of the
									   on-disk snapshot list */
	__le32	s_error_count;			/* number of fs errors */
	__le32	s_first_error_time;		/* first time an error happened */
	__le32	s_first_error_ino;		/* inode involved in first error */
	__le64	s_first_error_block;	/* block involved of first error */
	__u8	s_first_error_func[32];	/* function where the error happened */
	__le32	s_first_error_line;		/* line number where error happened */
	__le32	s_last_error_time;		/* most recent time of an error */
	__le32	s_last_error_ino;		/* inode involved in last error */
	__le32	s_last_error_line;		/* line number where error happened */
	__le64	s_last_error_block;		/* block involved of last error */
	__u8	s_last_error_func[32];	/* function where the error happened */
	__u8	s_mount_opts[64];
	__le32	s_reserved[112];        /* Padding to the end of the block */
};

/*******************************************************************************
 * Ext4 ����������Ϣ�ṹ��
 * ���ں��еĽṹ����ȣ�ȥ���˺�32�ֽڵ�64λ����֧�ֲ���
*******************************************************************************/
struct ext4_group_desc
{
	__le32	bg_block_bitmap_lo;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap_lo;		/* Inodes bitmap block */
	__le32	bg_inode_table_lo;		/* Inodes table block */
	__le16	bg_free_blocks_count_lo;/* Free blocks count */
	__le16	bg_free_inodes_count_lo;/* Free inodes count */
	__le16	bg_used_dirs_count_lo;	/* Directories count */
	__le16	bg_flags;				/* EXT4_BG_flags (INODE_UNINIT, etc) */
	__u32	bg_reserved[2];			/* Likely block/inode bitmap checksum */
	__le16  bg_itable_unused_lo;	/* Unused inodes count */
	__le16  bg_checksum;			/* crc16(sb_uuid+group+desc) */
};


#define FILE_SYSTEM_TYPE	"ext4" 				//�ļ�ϵͳ��ʽ
#define MKFS				"/sbin/mkfs.ext4"	//Ext4 ��ʽ������
#define TUNE2FS 			"/sbin/tune2fs" 	//tune2fs ����
#define RM 					"/bin/rm" 			//rm ����
#define SB_COPY_OPTION		"sb=131072" 		//ʹ���׸������鸱������ѡ��
#define SB_COPY_POSITION 	134217728 			//�׸������鸱��λ�� 32768��4K

#define MMC_MOUNT_POINT 	"/media/hi_mmc" 	//SD������·��
#define HDD_MOUNT_POINT 	"/media/hi_sda" 	//Ӳ�̹���·��
#define HDD_MNT_DIFF_POS 	12 					//����/media/hi_sda��a��λ��

#define SUPER_BLOCK_SIZE 	(sizeof(struct ext4_super_block))
#define GRP_DESC_SIZE 		(sizeof(struct ext4_group_desc))
#define FS_BLOCK_SIZE 		4096		//������������ļ�ϵͳ���С
#define PADDING_SIZE 		1024 		//����0����С

#define PARTITION_NAME_LEN	64			//�����豸���Ƴ���
#define MOUNT_POINT_LEN		24			//��������·������

#define PARTITION_WR_TEST_TEXT \
	"This file was created by ep, used for writing and reading test.\n"


int partition_name_cal(int disk_num, int partition_num, char *partition_name);
int partition_format(int disk_num, int partition_num);
PartitionInfo *which_partition(int disk_num, int partition_num);
int partition_umount(int disk_num, int partition_num);
char *partition_mount_point_find(int disk_num, int partition_num);
int mount_point_check(int disk_num, int partition_num);
int mount_point_set(int disk_num, int partition_num, char *mount_point);
int partition_mount(int disk_num, int partition_num, const void *option);
int partition_status_init(int disk_num);
int partition_status_check(int disk_num, int partition_num);
int is_data_to_upload(const char *path_name);
int partition_sb_backup(int disk_num, int partition_num);
int partition_sb_readbak(int disk_num, int partition_num, void *buf);
__u16 ext2fs_group_desc_csum(struct ext4_super_block *super_block,
                             int group, struct ext4_group_desc *group_desc);
int partition_sb_restore(int disk_num, int partition_num);
int partition_sb_copy(int disk_num, int partition_num);
int partition_clear_journal(int disk_num, int partition_num);
int partition_add_journal(int disk_num, int partition_num);
int partition_wr_test(int disk_num, int partition_num);
int partition_write_test(const char *test_file_path);
int partition_read_test(const char *test_file_path);
int partition_rescure_superblock(int disk_num, int partition_num);
int partition_rescure_journal(int disk_num, int partition_num);
int partition_rescure(int disk_num, int partition_num);
int partition_switch(void);
int partition_clear_trash(int disk_num, int partition_num);
int partition_clear_all(int disk_num, int partition_num);
int remove_recursively(const char *remove_path);
int partition_mark_using(int disk_num, int partition_num);

#endif 	/* _PARTITION_FUNC_H_ */