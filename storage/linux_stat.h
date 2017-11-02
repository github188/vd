
#ifndef _LINUX_STAT_H_
#define _LINUX_STAT_H_

/* Encoding of the file mode.  */

#define	__S_IFMT	0170000	/* These bits determine file type.  */


/* File types.  */

#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */


/* Protection bits.  */

#define	__S_ISUID	04000	/* Set user ID on execution.  */
#define	__S_ISGID	02000	/* Set group ID on execution.  */
#define	__S_ISVTX	01000	/* Save swapped text after use (sticky).  */
#define	__S_IREAD	0400	/* Read by owner.  */
#define	__S_IWRITE	0200	/* Write by owner.  */
#define	__S_IEXEC	0100	/* Execute by owner.  */


/* Test macros for file types.	*/

#define	__S_ISTYPE(mode, mask)	(((mode) & __S_IFMT) == (mask))

#define	S_ISDIR(mode)	 __S_ISTYPE((mode), __S_IFDIR)
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), __S_IFCHR)
#define	S_ISBLK(mode)	 __S_ISTYPE((mode), __S_IFBLK)
#define	S_ISREG(mode)	 __S_ISTYPE((mode), __S_IFREG)
#define S_ISFIFO(mode)	 __S_ISTYPE((mode), __S_IFIFO)
#define S_ISLNK(mode)	 __S_ISTYPE((mode), __S_IFLNK)
#define S_ISSOCK(mode)   __S_ISTYPE((mode), __S_IFSOCK)


/* 仿照 Linux 系统 struct stat （启用LFS）编写，以用于 Windows 系统 */
struct linux_stat
{
	unsigned long long 	dev;		/* ID of device containing file */
	unsigned short int 	__pad1;
	unsigned long 		__st_ino; 	/* 32bit file serial number. */
	unsigned int 		mode;		/* protection */
	long 				nlink;		/* number of hard links */
	unsigned int 		uid;		/* user ID of owner */
	unsigned int 		gid;		/* group ID of owner */
	unsigned long long 	rdev;		/* device ID (if special file) */
	unsigned short int 	__pad2;
	long long 			size;		/* total size, in bytes */
	long 				blksize;	/* blocksize for file system I/O */
	long long 			blocks;		/* number of 512B blocks allocated */
	long 				atime;		/* Time of last access.  */
	unsigned long int 	atimensec;	/* Nscecs of last access.  */
	long 				mtime;		/* Time of last modification.  */
	unsigned long int 	mtimensec;	/* Nsecs of last modification.  */
	long 				ctime;		/* Time of last status change.  */
	unsigned long int 	ctimensec;	/* Nsecs of last status change.  */
	unsigned long long 	inode;		/* inode number */
};

#endif 	/* _LINUX_STAT_H_ */
