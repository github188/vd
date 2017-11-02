#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "logger/log.h"
#include "park_file_handler.h"

/**
 * Name:        create_multi_dir
 * Description: the same as mkdir -p
 */
int create_multi_dir(const char *path)
{
	if (NULL == path) {
		ERROR("NULL == path");
		return -1;
	}
	int i, len;

	len = strlen(path);
	char dir_path[len+1];
	dir_path[len] = '\0';

	strncpy(dir_path, path, len);

	for (i=0; i<len; i++)
	{
		if (dir_path[i] == '/' && i > 0)
		{
			dir_path[i]='\0';
			if (access(dir_path, F_OK) < 0)
			{
				if (mkdir(dir_path, 0755) < 0)
				{
					return -1;
				}
			}
			dir_path[i]='/';
		}
	}

	return 0;
}

//save picture into files
int park_save_picture(const char *picname, unsigned char *picbuff, int size)
{
	if ((NULL == picname) || (NULL == picbuff)) {
		ERROR("(NULL == picname) || (NULL == picbuff)");
		return -1;
	}

	if (size <= 0) {
		ERROR("Picture size = %d for %s", size, picname);
		return -1;
	}

	int file_fd;
	int ret = 0;
	char pic_path[128] = {0};

    if (picname[0] == '/') {
        /* give the picname with absolute path */
        strcpy(pic_path, picname);
    } else {
        sprintf(pic_path, "/home/records/park/system/picture/%s", picname);
    }
    DEBUG("pic_path = %s", pic_path);
	file_fd = open(pic_path, O_CREAT | O_RDWR);
	if (file_fd < 0) {
		ERROR("Open file %s failed, errno = %d", pic_path, errno);
		return -1;
	}

	if ((ret = write(file_fd, picbuff, size)) != size) {
		ERROR("write error errno = %d", errno);
		return -1;
	}

	close(file_fd);

	return ret;
}

//read picture
int park_read_picture(const char *picname, unsigned char *picbuff, int size)
{
	if ((NULL == picname) || (NULL == picbuff)) {
		ERROR("(NULL == picname) || (NULL == picbuff)");
		return -1;
	}

	int file_fd;
	int ret = 0;
	char pic_path[128] = {0};

    if (picname[0] == '/') {
        /* give the picname with absolute path */
        strcpy(pic_path, picname);
    } else {
        sprintf(pic_path, "/home/records/park/system/picture/%s", picname);
    }

	file_fd = open(pic_path, O_RDWR);
	if (file_fd < 0) {
		ERROR("Open file %s failed, errno = %d", pic_path, errno);
		return -1;
	}

	if (size > 0) {
        if ((ret = read(file_fd, picbuff, size)) < 0) {
            ERROR("Read %s failed. errno = %d", picname, errno);
            return -1;
        }
    } else if (size == 0) { /* read to EOF */
        unsigned char *p = picbuff;
        int read_size = 0;
        while((read_size = read(file_fd, p, 1024)) > 0) {
            p += 1024;
            ret += read_size;
        }
        ret += read_size;
    } else {
		ERROR("Picture size = %d for %s", size, picname);
		return -1;
    }


	close(file_fd);

	return ret;
}

int park_delete_picture(const char* picname)
{
    if (NULL == picname) {
        ERROR("NULL is picname");
        return -1;
    }

    if (picname[0] == '/') {
        return unlink(picname);
    } else {
        char pic_path[128] = {0};
        sprintf(pic_path, "/home/records/park/system/%s", picname);
        return unlink(pic_path);
    }
}

int park_save_light_state(void)
{
	int file_fd;
	int ret = 0;
	char pic_path[128] = {0};

	sprintf(pic_path, "/home/records/park/system/%s", "light.txt");
	file_fd = open(pic_path, O_CREAT | O_RDWR);
	if (file_fd < 0) {
		ERROR("Open file %s failed, errno = %d", pic_path, errno);
		return -1;
	}

    const char *buff = "light_controlled by platform.";
	if ((ret = write(file_fd, buff, strlen(buff))) != (int)strlen(buff)) {
		ERROR("write error errno = %d", errno);
		return -1;
	}

	close(file_fd);

	return ret;
}

#if 0 /* useless now, only check the file exist should be ok */
int park_read_light_state(Lighting_park* light)
{
	int file_fd;
	int ret = 0;
	char pic_path[128] = {0};

	sprintf(pic_path, "/home/records/park/system/%s", "light.txt");
	file_fd = open(pic_path, O_RDWR);
	if (file_fd < 0) {
		ERROR("Open file %s failed, errno = %d", pic_path, errno);
		return -1;
	}

	if ((ret = read(file_fd, light, sizeof(Lighting_park))) <= 0) {
		ERROR("Read light state failed. errno = %d", errno);
		return -1;
	}

	close(file_fd);

	return ret;
}
#endif

int park_light_state_exist()
{
	char pic_path[128] = {0};
	sprintf(pic_path, "/home/records/park/system/%s", "light.txt");
    return !access(pic_path, F_OK);
}

int park_delete_light_state()
{
	char pic_path[128] = {0};
	sprintf(pic_path, "/home/records/park/system/%s", "light.txt");
    return unlink(pic_path);
}
