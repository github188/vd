#ifndef __HEAP_H
#define __HEAP_H

#include "types.h"

C_CODE_BEGIN

void *xmalloc(size_t size);
void *xrealloc(void *addr, size_t newsize);
void xfree(void *ptr);

C_CODE_END

#endif
