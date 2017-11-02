#include <stdlib.h>
#include "types.h"

C_CODE_BEGIN

void *xmalloc(size_t size)
{
	return malloc(size);
}

void xfree(void *ptr)
{
	free(ptr);
}

void *xrealloc(void *addr, size_t newsize)
{
	return realloc(addr, newsize);
}

C_CODE_END
