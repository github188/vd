#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mq_resend.h"
#include "sqlite_util.h"
#include "commonfuncs.h"
#include "upload.h"
#include "record.h"
#include "mq_resend_db.h"
#include "logger/log.h"


C_CODE_BEGIN


typedef struct mq_resend{
	sqlite_t sqlite;
	pthread_mutex_t mutex;
	pthread_t thread;
	int32_t resv;
	int32_t backoff_ptr;
}mq_resend_t;

static void *mq_resend_pthread(void *arg);

static mq_resend_t mq_resend;
static const uint32_t backoff_time[] = {8, 16, 32, 64, 128, 256};

int32_t tabitcom_mq_resend_ram_init(const char *db_path, int32_t resv)
{
	pthread_mutex_init(&mq_resend.mutex, NULL);
	sqlite_open(&mq_resend.sqlite, db_path);
	sabitcom_mq_resend_tab_create(&mq_resend.sqlite);
	mq_resend.resv = resv;
	mq_resend.backoff_ptr = 0;

	return 0;
}

int32_t tabitcom_mq_resend_pthread_init(void)
{
	pthread_attr_t attr;

	if (0 != pthread_attr_init(&attr)) {
		ERROR("Failed to initialize thread attrs");
		return -1;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&mq_resend.thread, &attr,
					   mq_resend_pthread, &mq_resend)) {
		ERROR("Failed to create speech thread");
	}

	pthread_attr_destroy(&attr);

	return 0;
}


static void *mq_resend_pthread(void *arg)
{
	sabitcom_mq_resend_t item;
	mq_resend_t *mr = (mq_resend_t *)arg;
	int32_t id;

	for (;;) {
		sleep(backoff_time[mr->backoff_ptr]);

		pthread_mutex_lock(&mr->mutex);

		id = sabitcom_mq_resend_select(&mr->sqlite, &item);
		if (id > 0) {
			DEBUG("id: %d.", id);
			DEBUG("text: %s", item.text);

			if(0 == tabitcom_mq_send(item.text)){
				DEBUG("mq resend ok, del it.");
				sabitcom_mq_resend_del_id(&mr->sqlite, id);
				/* send successful, reduce the resend interval */
				mr->backoff_ptr = 0;
			} else {
				++mr->backoff_ptr;
				if (numberof(backoff_time) == mr->backoff_ptr) {
					mr->backoff_ptr = 0;
				}
			}
		}

		pthread_mutex_unlock(&mr->mutex);
	}

	return NULL;
}

int32_t tabitcom_mq_resend_put(const sabitcom_mq_resend_t *item)
{
	int32_t ret;

	pthread_mutex_lock(&mq_resend.mutex);

	sabitcom_mq_resend_del_redundant(&mq_resend.sqlite, mq_resend.resv);
	ret = sabitcom_mq_resend_insert(&mq_resend.sqlite, item);

	pthread_mutex_unlock(&mq_resend.mutex);

	return ret;
}


C_CODE_END
