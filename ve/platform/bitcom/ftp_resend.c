#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ftp_resend_db.h"
#include "sqlite_util.h"
#include "commonfuncs.h"
#include "record.h"
#include "ftp_resend_db.h"
#include "logger/log.h"


C_CODE_BEGIN


typedef struct ftp_resend{
	sqlite_t sqlite;
	pthread_mutex_t mutex;
	pthread_t thread;
	char pic_dir[256];
	int32_t resv;
	int32_t backoff_ptr;
}ftp_resend_t;

static void *ftp_resend_pthread(void *arg);

static ftp_resend_t ftp_resend;
static const uint32_t backoff_time[] = {8, 16, 32, 64, 128, 256};

int32_t tabitcom_ftp_resend_ram_init(const char *db_path, const char *pic_dir,
									 int32_t resv)
{
	pthread_mutex_init(&ftp_resend.mutex, NULL);
	sqlite_open(&ftp_resend.sqlite, db_path);
	strncpy(ftp_resend.pic_dir, pic_dir, sizeof(ftp_resend.pic_dir));
	sabitcom_ftp_resend_tab_create(&ftp_resend.sqlite);
	ftp_resend.resv = resv;
	ftp_resend.backoff_ptr = 0;

	return 0;
}

int32_t tabitcom_ftp_resend_pthread_init(void)
{
	pthread_attr_t attr;
	int ret;

	if (0 != pthread_attr_init(&attr)) {
		CRIT("Failed to initialize thread attrs\n");
		return -1;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&ftp_resend.thread, &attr,
						 ftp_resend_pthread, &ftp_resend);
	if (0 != ret) {
		CRIT("Failed to create speech thread\n");
	}

	pthread_attr_destroy(&attr);

	return ret;
}


static void *ftp_resend_pthread(void *arg)
{
	int32_t id;
	int32_t i, j;
	char *p, *p_end;
	char path[512];
	uint8_t buf[SIZE_JPEG_BUFFER];
	FILE *file;
	sabitcom_ftp_resend_t item;
	EP_PicInfo picinfo;
	ftp_resend_t *fr = (ftp_resend_t *)arg;

	for (;;) {
		sleep(backoff_time[fr->backoff_ptr]);

		pthread_mutex_lock(&fr->mutex);

		id = sabitcom_ftp_resend_select(&fr->sqlite, &item);
		if(id > 0){
			DEBUG("id: %d.", id);
			DEBUG("name: %s", item.name);
			DEBUG("path: %s", item.path);
			DEBUG("size: %d", item.size);

			snprintf(path, sizeof(path), "%s/%s", fr->pic_dir, item.name);
			file = fopen(path, "rb");
			if (file) {
				DEBUG("open %s success.", path);
				picinfo.size = fread(buf, sizeof(uint8_t), sizeof(buf), file);
				fclose(file);
			} else {
				sabitcom_ftp_resend_del_id(&fr->sqlite, id);
			}

			if (picinfo.size > 0) {
				picinfo.buf = buf;
				strcpy(picinfo.name, item.name);

				/* 路径格式转换 */
				i = 0;
				j = 0;
				p = item.path;
				p_end = item.path + strlen(item.path);
				while (p < p_end) {
					picinfo.path[i][j++] = *p++;
					if ('/' == *p) {
						picinfo.path[i][j++] = 0;
						j = 0;
						++i;
					}
				}
				picinfo.path[i][j] = 0;

				if (0 == tabitcom_ftp_send(&picinfo)) {
					sabitcom_ftp_resend_del_id(&fr->sqlite, id);
					remove(path);
					fr->backoff_ptr = 0;
				} else {
					++fr->backoff_ptr;
					if (numberof(backoff_time) == fr->backoff_ptr) {
						fr->backoff_ptr = 0;
					}
				}
			} else {
				/* 图片打开失败直接删除数据库 */
				sabitcom_ftp_resend_del_id(&fr->sqlite, id);
			}

		}

		pthread_mutex_unlock(&fr->mutex);
	}

	return NULL;
}

int32_t tabitcom_ftp_resend_put(const sabitcom_ftp_resend_t *item)
{
	int32_t ret, i;
	sabitcom_ftp_name_t del_name[10];
	char buf[512];
	FILE *file;

	pthread_mutex_lock(&ftp_resend.mutex);

	ret = sabitcom_ftp_resend_del_redundant(&ftp_resend.sqlite,
											ftp_resend.resv,
											del_name,
											numberof(del_name));
	for (i = 0; i < ret; ++i) {
		snprintf(buf, sizeof(buf), "%s/%s", ftp_resend.pic_dir, del_name[i]);
		TRACE_LOG_SYSTEM("remove %s.", buf);
		remove(buf);
	}

	ret = sabitcom_ftp_resend_insert(&ftp_resend.sqlite, item);
	if (SQLITE_OK == ret) {
		snprintf(buf, sizeof(buf), "%s/%s", ftp_resend.pic_dir, item->name);
		file = fopen(buf, "wb");
		if (file) {
			fwrite(item->pic, sizeof(uint8_t), item->size, file);
			fclose(file);
			TRACE_LOG_SYSTEM("write %s success.", buf);
		}
	}

	pthread_mutex_unlock(&ftp_resend.mutex);

	return ret;
}


C_CODE_END

