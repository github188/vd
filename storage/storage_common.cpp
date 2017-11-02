#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "storage_common.h"


/*******************************************************************************
 * 函数名: u_sleep
 * 功  能: 延时指定时长
 * 参  数: sec，秒；usec，微秒
*******************************************************************************/
void u_sleep(time_t sec, unsigned long usec)
{
	struct timespec interval, remainder;

	if (usec >= 1000000)
	{
		interval.tv_sec 	= sec + (usec / 1000000);
		interval.tv_nsec 	= ((usec % 1000000) * 1000);
	}
	else
	{
		interval.tv_sec 	= sec;
		interval.tv_nsec 	= (usec * 1000);
	}

	int i;
	for (i=0; i<RETRY_TIMES; i++)
	{
		if (nanosleep(&interval, &remainder) < 0)
		{
			log_debug_storage("nanosleep %ld %ld error : %m !\n",
			                  interval.tv_sec, interval.tv_nsec);
			interval.tv_sec 	= remainder.tv_sec;
			interval.tv_nsec 	= remainder.tv_nsec;
		}
		else
			break;
	}
}


/*******************************************************************************
 * 函数名: random_32
 * 功  能: 生成一个整型随机数
 * 返回值: random_num，随机数
*******************************************************************************/
int random_32(void)
{
	int random_num = 0;

	int fd_urandom = open("/dev/urandom", O_RDONLY);
	if (fd_urandom < 0)
	{
		log_warn_storage("Open /dev/urandom failed !\n");

		char statebuf[256];
		struct random_data randomData;

		memset(&randomData, 0, sizeof(randomData));

		initstate_r(0, statebuf, sizeof(statebuf), &randomData);
		srandom_r(time(NULL), &randomData);
		random_r(&randomData, &random_num);

		return random_num;
	}

	read(fd_urandom, &random_num, sizeof(random_num));

	close(fd_urandom);

	return random_num;
}


/*******************************************************************************
 * 函数名: mkpath
 * 功  能: 创建一个路径
 * 参  数: path，路径；mode，权限
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int mkpath(const char *path, mode_t mode)
{
	if (path == NULL)
	{
		log_warn_storage("%s path can not be NULL !\n", __func__);
		return -1;
	}

	char *tmp_path = strdup(path);
	if (tmp_path == NULL)
	{
		log_warn_storage("Duplicate string %s failed !\n", path);
		return -1;
	}

	char *s = tmp_path;
	char c;
	struct stat st;

	for (;;)
	{
		c = '\0';

		while (*s)
		{
			if (*s == '/')
			{
				do
				{
					++s;
				}
				while (*s == '/');

				c = *s;
				*s = '\0';

				break;
			}
			++s;
		}

		if (access(tmp_path, F_OK) == 0)
		{
			if ( (stat(tmp_path, &st) < 0) || (!S_ISDIR(st.st_mode)) )
			{
				log_warn_storage("%s is not a directory !\n", tmp_path);
				break;
			}
		}
		else
		{
			if (mkdir(tmp_path, mode) < 0)
			{
				log_warn_storage("mkdir %s : %m !\n", tmp_path);
				break;
			}
		}

		if (!c)
		{
			safe_free(tmp_path);
			return 0;
		}

		*s = c;
	}

	safe_free(tmp_path);

	return -1;
}


/*******************************************************************************
 * 函数名: get_parent_path
 * 功  能: 获取上级路径
 * 参  数: path，路径
 * 返回值: 成功，返回上级路径指针；失败，返回NULL
 * 注  意: 使用完毕后，需要将返回结果释放！
*******************************************************************************/
char *get_parent_path(const char *path)
{
	if (path == NULL)
	{
		log_warn_storage("%s path can not be NULL !\n", __func__);
		return NULL;
	}

	size_t path_len = strlen(path);

	char *parent_path = (char *)malloc(path_len);
	if (parent_path == NULL)
	{
		log_error_storage("Duplicate string %s failed !\n", path);
		return NULL;
	}

	memcpy(parent_path, path, path_len-1); 	/* 最后一个字符无需判断 */

	int i = path_len - 2;

	while (i >= 0)
	{
		if (parent_path[i] == '/')
		{
			parent_path[i+1] = '\0';
			return parent_path;
		}

		i--;
	}

	safe_free(parent_path);

	return NULL;
}
