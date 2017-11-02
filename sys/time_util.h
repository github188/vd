#ifndef __TIME_UTIL_H
#define __TIME_UTIL_H

#include <time.h>
#include "types.h"

C_CODE_BEGIN

typedef struct tm xtm_t;
typedef time_t xtime_t;

typedef struct ustime{
	time_t sec;
	int32_t usec;
}ustime_t;

void ustime(ustime_t *ut);
const char *ustime_sec(void);
const char *ustime_sec_s(char *buf, size_t bufsz);
char *ustime_msec(void);
char *ustime_usec(void);
int64_t ustime_diff_sec(const ustime_t *a, const ustime_t *b);
int64_t ustime_diff_msec(const ustime_t *a, const ustime_t *b);
int64_t ustime_diff_usec(const ustime_t *a, const ustime_t *b);
void ustimecpy(ustime_t *dest, const ustime_t *src);
ssize_t xtime2msec(char *buf, size_t size, const ustime_t *t);
ssize_t timeval2str(char *buf, size_t size, const struct timeval *tv);
xtm_t *xlocaltime(const ustime_t *t, xtm_t *result);
xtime_t xmktime(xtm_t *tm);
void timespec_sub(struct timespec *t, const struct timespec *a,
				  const struct timespec *b);
int64_t timespec_diff_sec(const struct timespec *a, const struct timespec *b);
int64_t timespec_diff_msec(const struct timespec *a, const struct timespec *b);
bool timeval_is_overtime(const struct timeval *prev, int_fast32_t timeout);

C_CODE_END


#endif
