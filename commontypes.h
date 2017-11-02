/*
 * commontypes.h
 *
 *  Created on: 2013-4-8
 *      Author: shanhongwei
 */

#ifndef COMMONTYPES_H_
#define COMMONTYPES_H_


#include <stdint.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef char S8;
typedef short S16;
typedef int S32;
typedef long long S64;


#ifndef WORD_BYTE

#define WORD_BYTE

typedef short SHORT;
typedef int BOOL;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef long LONG;
typedef float FLOAT;
typedef double DOUBLE;
typedef long TIME; /* time value */

#endif


#endif /* COMMONTYPES_H_ */
