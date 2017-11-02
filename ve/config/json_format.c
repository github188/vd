#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "logger/log.h"

C_CODE_BEGIN

static size_t json_add_space(char *s, size_t size, size_t nr_space)
{
	char *_s;

	_s = s;

	while ((nr_space--) && (size--)) {
		*_s++ = ' ';
	}

	return (size_t)(_s - s);
}

static size_t json_add_space_stream(size_t nr_space, FILE *f)
{
	size_t len = nr_space;

	while (len--) {
		if (' ' != fputc(' ', f)) {
			return -1;
		}
	}

	return nr_space;
}

ssize_t json_format(char *dest, size_t destsz, const char *src, size_t indent)
{
	const char *s;
	char *d, *d_end;
	size_t indent_len = 0;

	s = src;
	d = dest;
	d_end = d + destsz - 1;

	ASSERT(s);
	ASSERT(d);

	if ('{' != *s) {
		return -1;
	}

	while ((d < d_end) && ('\0' != *s)) {
		*d++ = *s;

		if ('{' == *s) {
			*d++ = '\n';
			indent_len += indent;
			d += json_add_space(d, (size_t)(d_end - d), indent_len);
		} else if(',' == *s) {
			*d++ = '\n';
			d += json_add_space(d, (size_t)(d_end - d), indent_len);
		} else if('}' == *(s + 1)) {
			*d++ = '\n';
			indent_len -= indent;
			d += json_add_space(d, (size_t)(d_end - d), indent_len);
		}

		++s;
	}

	*d = '\0';

	return ('\0' != *s) ? -1 : (ssize_t)(d - dest);
}


ssize_t json_format_stream(const char *src, size_t indent, FILE *f)
{
	const char *s;
	size_t indent_len = 0;
	ssize_t len = 0;

	s = src;

	ASSERT(s);

	if ('{' != *s) {
		return -1;
	}

	while ('\0' != *s) {
		if (((uint8_t)*s) != fputc(*s, f)) {
			return -1;
		}
		++len;

		if ('{' == *s) {
			indent_len += indent;
		}  else if('}' == *(s + 1)) {
			indent_len -= indent;
		}

		if (('{' == *s) || (',' == *s) || ('}' == *(s + 1))) {
			if ('\n' != fputc('\n', f)) {
				return -1;
			}
			++len;

			if (indent_len != json_add_space_stream(indent_len, f)) {
				return -1;
			}
			len += indent_len;
		}

		++s;
	}

	return len;
}


C_CODE_END
