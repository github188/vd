#ifndef __PROFILE_H
#define __PROFILE_H

#include <stdio.h>
#include "types.h"

C_CODE_BEGIN

typedef struct profile {
	FILE *f;
}profile_t;


extern int32_t profile_open(profile_t *file, const char *path, 
							const char *mode);
extern int32_t profile_close(profile_t *file);
extern ssize_t profile_rd_val(profile_t *file, const char *key, char *val);


C_CODE_END

#endif
