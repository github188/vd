#include <string.h>
#include "types.h"

C_CODE_BEGIN

size_t strlcpy(char *dst, const char *src, size_t size)
{
	char *d = dst;
	const char *s = src;
	size_t n = size;

	if (s == 0 || d == 0) {
		return 0;
	}

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0) {
				break;
			}
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (size != 0) {
			*d = '\0';		/* NUL-terminate dst */
		}
		while (*s++);
	}

	return (s - src - 1);		/* count does not include NUL */
}

size_t strlcat(char *dst, const char *src, size_t size)
{
	char *d = dst;
	const char *s = src;
	size_t n = size;
	size_t dlen;

	if (s == 0 || d == 0) {
		return 0;
	}

	while (n-- != 0 && *d != '\0') {
		d++;
	}
	dlen = d - dst;
	n = size - dlen;

	if (n == 0) {
		return (dlen + strlen(s));
	}
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return (dlen + (s - src));
}

C_CODE_END
