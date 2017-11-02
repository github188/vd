#ifndef __TCP_UTIL_H
#define __TCP_UTIL_H

#include "types.h"

C_CODE_BEGIN

int32_t tcp_connect_timeout(const char *ip, uint16_t port, int32_t timeout);


C_CODE_END

#endif
