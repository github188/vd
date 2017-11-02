#ifndef __XSTRING_H
#define __XSTRING_H

#include "types.h"

C_CODE_BEGIN

size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

C_CODE_END


#endif
