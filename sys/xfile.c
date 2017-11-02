#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "types.h"

C_CODE_BEGIN

ssize_t get_file_size(const char *path)
{
	if (path == NULL) {
		return -EINVAL;
	}

	struct stat _stat;

	if (stat(path, &_stat) < 0) {
		return -EPERM;
	}

	return (ssize_t)_stat.st_size;
}

C_CODE_END
