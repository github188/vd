#ifndef __JSON_FORMAT_H
#define __JSON_FORMAT_H

#include "types.h"

C_CODE_BEGIN

ssize_t json_format(char *dest, size_t destsz, const char *src, size_t indent);
ssize_t json_format_stream(const char *src, size_t indent, FILE *f);

C_CODE_END

#endif