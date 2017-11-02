#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "time_util.h"


C_CODE_BEGIN

void ustime(ustime_t *dt)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	dt->usec = tv.tv_usec;
	dt->sec = tv.tv_sec;
}

const char *ustime_sec(void)
{
	static char buf[32];
	ustime_t ut;
	struct tm *tm;

	ustime(&ut);
	tm = localtime(&ut.sec);

	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	return buf;
}

const char *ustime_sec_s(char *buf, size_t bufsz)
{
	ustime_t ut;
	struct tm tm;

	ustime(&ut);
	localtime_r(&ut.sec, &tm);

	snprintf(buf, bufsz, "%04d-%02d-%02d %02d:%02d:%02d",
			 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			 tm.tm_hour, tm.tm_min, tm.tm_sec);

	return buf;
}

char *ustime_msec(void)
{
	static char buf[32];
	ustime_t ut;
	struct tm *tm, _tm;

	ustime(&ut);
	tm = localtime_r(&ut.sec, &_tm);

	snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, ut.usec / 1000);

	return buf;
}

char *ustime_usec(void)
{
	static char buf[32];
	ustime_t ut;
	struct tm *tm, _tm;

	ustime(&ut);
	tm = localtime_r(&ut.sec, &_tm);

	snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06d",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, ut.usec);

	return buf;
}

int64_t ustime_diff_sec(const ustime_t *a, const ustime_t *b)
{
	return (int64_t)a->sec - (int64_t)b->sec;
}

int64_t ustime_diff_usec(const ustime_t *a, const ustime_t *b)
{
	int64_t aus, bus;

	aus = (int64_t)a->sec * 1000000 + (int64_t)a->usec;
	bus = (int64_t)b->sec * 1000000 + (int64_t)b->usec;
	return aus - bus;
}

int64_t ustime_diff_msec(const ustime_t *a, const ustime_t *b)
{
	int64_t ams, bms;

	ams = (int64_t)a->sec * 1000 + (int64_t)a->usec / 1000;
	bms = (int64_t)b->sec * 1000 + (int64_t)b->usec / 1000;
	return ams - bms;
}

void ustimecpy(ustime_t *dest, const ustime_t *src)
{
	memcpy(dest, src, sizeof(ustime_t));
}

ssize_t xtime2msec(char *buf, size_t size, const ustime_t *t)
{
	struct tm *tm;
	struct tm _tm;

	tm = localtime_r(&t->sec, &_tm);

	return snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
					tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec, t->usec / 1000);
}

ssize_t timeval2str(char *buf, size_t size, const struct timeval *tv)
{
	struct tm *tm, _tm;

	tm = localtime_r(&tv->tv_sec, &_tm);

	return snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
					tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec, tv->tv_usec / 1000);
}

xtm_t *xlocaltime(const ustime_t *t, xtm_t *result)
{
	time_t sec;

	sec = t->sec;
	return localtime_r(&sec, result);
}

xtime_t xmktime(xtm_t *tm)
{
	return mktime(tm);
}

void timespec_sub(struct timespec *t, const struct timespec *a,
				  const struct timespec *b)
{
	t->tv_sec = a->tv_sec - b->tv_sec;
	t->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (t->tv_nsec < 0) {
		--t->tv_sec;
		t->tv_nsec -= 1000000000;
	}
}

int64_t timespec_diff_msec(const struct timespec *a, const struct timespec *b)
{
	struct timespec t;

	timespec_sub(&t, a, b);

	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int64_t timespec_diff_sec(const struct timespec *a, const struct timespec *b)
{
	struct timespec t;

	timespec_sub(&t, a, b);

	return t.tv_sec;
}

bool timeval_is_overtime(const struct timeval *prev, int_fast32_t timeout)
{
	struct timeval curr, diff;

	gettimeofday(&curr, NULL);
	timersub(&curr, prev, &diff);

	if (diff.tv_sec < 0) {
		return true;
	}

	int_fast32_t diff_ms = diff.tv_sec * 1000;
	if (diff_ms > timeout) {
		return true;
	}

	diff_ms += diff.tv_usec / 1000;

	if (diff_ms >= timeout) {
		return true;
	}

	return false;
}

C_CODE_END
