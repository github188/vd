#ifndef __TYPES_H
#define __TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __linux__
#include <sys/types.h>
#else
typedef long off_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
#define C_CODE_BEGIN	extern "C" {
#define C_CODE_END		}
#else
#define C_CODE_BEGIN
#define C_CODE_END
#endif

#ifdef __GUNC__
#define __inline__	inline
#endif

#ifndef numberof
#define numberof(a) (sizeof((a)) / sizeof((a)[0]))
#endif

#ifndef setbit
#define setbit(n, b)		do{(n) |= (1 << (b));}while(0)
#endif

#ifndef clrbit
#define clrbit(n, b)		do{(n) &= ~(1 << (b));}while(0)
#endif

#ifndef getbit
#define getbit(n, b)		((n) & (1 << (b)))
#endif

/*
 * __weak__
 */
#ifndef weak
#define weak	__attribute__((weak))
#endif

#ifndef min
#define min(x, y)	({	\
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x < _y ? _x : _y;})
#endif

#ifndef max
#define max(x, y)	({	\
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);	\
	_x > _y ? _x : _y;})
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

#ifdef split_write_two
#define split_write_two(le, buf, n)		do{	\
	if((le)) {	\
		(buf)[0] = (n) & 0xFF;	\
		(buf)[1] = ((n) & 0x0000FF00) >> 8;	\
	} else {	\
		(buf)[0] = ((n) & 0xFF000000) >> 24;	\
		(buf)[1] = ((n) & 0x00FF0000) >> 16;	\
	}	\
}while(0)
#endif

#ifdef split_read_two
#define split_read_two(le, buf)		({	\
	(le) ?	\
		((buf)[0] & 0x00FF | ((buf)[1] << 8) & 0xFF00)	\
		:	\
		(((buf)[0] << 8) & 0xFF00 | (buf)[1] & 0x00FF);	\
})
#endif

#ifdef split_write_four
#define split_write_four(le, buf, n)		do{	\
	if((le)) {	\
		(buf)[0] = (n) & 0xFF;	\
		(buf)[1] = ((n) & 0x0000FF00) >> 8;	\
		(buf)[2] = ((n) & 0x00FF0000) >> 16;	\
		(buf)[3] = ((n) & 0xFF000000) >> 24;	\
	} else {	\
		(buf)[0] = ((n) & 0xFF000000) >> 24;	\
		(buf)[1] = ((n) & 0x00FF0000) >> 16;	\
		(buf)[2] = ((n) & 0x0000FF00) >> 8;	\
		(buf)[3] = (n) & 0xFF;	\
	}	\
}while(0)
#endif

#ifdef split_read_four
#define split_read_four(le, buf)		({	\
	(le) ?	\
		(buf)[0] & 0x000000FF | ((buf)[1] << 8) & 0x0000FF00 |	\
		((buf)[2] << 16) & 0x00FF0000 | ((buf)[3] << 24) & 0xFF000000	\
		:	\
		((buf)[0] << 24) & 0xFF000000 | ((buf)[1] << 16) & 0x00FF0000 |	\
		((buf)[2] << 8) & 0x0000FF00 | (buf)[3] & 0x000000FF |	\
})
#endif

#ifndef swap_two
#define swap_two(n)		({	\
	(uint16_t)((((n) >> 8) & 0x00FF) | (((n) << 8) & 0xFF00));	\
})
#endif

#ifndef swap_four
#define swap_four(n)		({	\
	(uint32_t)((((n) >> 24) & 0x000000FF) | (((n) >> 8) & 0x0000FF00) |	\
			   (((n) << 8) & 0x00FF0000) | (((n) << 24) & 0xFF000000));	\
})
#endif

#ifdef __cplusplus
}
#endif

#endif	/* end of _TYPES_H */
