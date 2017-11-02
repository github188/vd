#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "list.h"
#include "logger/log.h"
#include "sys/config.h"
#include "sys/time_util.h"
#include "sys/xfile.h"
#include "event.h"

C_CODE_BEGIN

typedef struct event{
	list_head_t ls;
	pthread_t tid;
	pthread_mutex_t mutex;
	sem_t sem;
} event_t;

static int_fast32_t event_once(void);

static event_t event;

static __inline__ int_fast32_t __event_item_init(event_item_t *item)
{
	gettimeofday(&item->tv, NULL);
	return 0;
}

event_item_t *event_item_alloc(void)
{
	event_item_t *item = (event_item_t *)malloc(sizeof(event_item_t));
	if (item == NULL) {
		CRIT("Alloc memory for event item failed");
		return NULL;
	}

	__event_item_init(item);

	return item;
}

void event_item_free(event_item_t *item)
{
	free(item);
}

static __inline__ int_fast32_t __event_put(event_t *ent, event_item_t *item)
{
	event_once();

	pthread_mutex_lock(&ent->mutex);

	list_add_tail(&item->link, &ent->ls);

	pthread_mutex_unlock(&ent->mutex);

	sem_post(&ent->sem);

	return 0;
}


int_fast32_t event_put(event_item_t *item)
{
	return __event_put(&event, item);
}

static __inline__ void file_rotate(void)
{
	const char *curr = EVENT_FILE;

	if (get_file_size(curr) < (EVENT_FILE_MAX_SIZE / 2)) {
		return;
	}

	const char *old = EVENT_FILE".1";

	if (access(old, F_OK) == 0) {
		remove(old);
	}

	rename(curr, old);
}

static __inline__ int_fast32_t __event_daemon(event_t *ent)
{
	int_fast32_t result;

	ASSERT(ent);

	file_rotate();

	sem_wait(&ent->sem);

	pthread_mutex_lock(&ent->mutex);

	do {
		result = 0;

		if (list_empty(&ent->ls)) {
			break;
		}

		FILE *f = fopen(EVENT_FILE, "a+");
		if (f == NULL) {
			int _errno = errno;
			ERROR("Open event file failed:%s", strerror(_errno));
			result = -EPERM;
			break;
		}

		list_head_t *pos, *n;
		list_for_each_safe(pos, n, &ent->ls) {
			event_item_t *item = list_entry(pos, event_item_t, link);

			char dt[64];
			timeval2str(dt, sizeof(dt), &item->tv);

			char buf[1024];
			ssize_t l = snprintf(buf, sizeof(buf), "|%-24s|%-32s|%-64s|\n",
								 dt, item->ident, item->desc);
			l = min(l, (ssize_t)(sizeof(buf) - 1));
			fwrite(buf, sizeof(char), l, f);

			memset(buf, '-', l - 1);
			buf[l - 1] = '\n';
			buf[l] = 0;
			fwrite(buf, sizeof(char), l, f);

			list_del(&item->link);
			event_item_free(item);
		}

		fclose(f);

	} while (0);

	pthread_mutex_unlock(&ent->mutex);

	return result;
}

static void *event_thread(void *arg)
{
	event_once();

	for (;;) {
		__event_daemon((event_t *)arg);
		pthread_testcancel();
	}

	return NULL;
}

static int_fast32_t __event_init(event_t *ent)
{
	int_fast32_t ret;

	ASSERT(ent);

	ret = pthread_create(&ent->tid, NULL, event_thread, ent);
	if (ret != 0) {
		CRIT("Create thread for event failed:%s", strerror(ret));
		return -EPERM;
	}

	return 0;
}

static __inline__ int_fast32_t __event_ram_init(event_t *ent)
{
	int_fast32_t ret;

	if((ret = pthread_mutex_init(&ent->mutex, NULL)) != 0){
		CRIT("Initialize mutex for event failed: %s", strerror(ret));
		return -1;
	}

	if((ret = sem_init(&ent->sem, 0, 0)) != 0) {
		CRIT("Initialize semphore for event failed: %s", strerror(ret));
		return -1;
	}

	INIT_LIST_HEAD(&ent->ls);

	return 0;
}

static void event_ram_init(void)
{
	if (__event_ram_init(&event) != 0) {
		exit(1);
	}
}

static int_fast32_t event_once(void)
{
	int_fast32_t ret;
	static pthread_once_t once = PTHREAD_ONCE_INIT;

	if(0 != (ret = pthread_once(&once, event_ram_init))){
		CRIT("pthread_once failed @ (%s:%d): %s",
			  __FILE__, __LINE__, strerror(ret));
		return -1;
	}

	return 0;
}

int_fast32_t event_init(void)
{
	return __event_init(&event);
}

C_CODE_END
