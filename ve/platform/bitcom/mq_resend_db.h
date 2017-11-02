#ifndef __SABITCOM_MQ_RESEND_H
#define __SABITCOM_MQ_RESEND_H

#include "types.h"
#include "sqlite_util.h"

C_CODE_BEGIN


#define SABITCOM_MQ_RESEND_TAB		"mq_resend"
#define SABITCOM_MQ_TEXT_SIZE		2048

typedef struct sabitcom_mq_resend{
	char text[SABITCOM_MQ_TEXT_SIZE];
}sabitcom_mq_resend_t;


int32_t sabitcom_mq_resend_tab_create(sqlite_t *sqlite);
int32_t sabitcom_mq_resend_insert(sqlite_t *sqlite,
								  const sabitcom_mq_resend_t *item);
int32_t sabitcom_mq_resend_select(sqlite_t *sqlite,
								  sabitcom_mq_resend_t *item);
int32_t sabitcom_mq_resend_get_cnt(sqlite_t *sqlite);
int32_t sabitcom_mq_resend_del_redundant(sqlite_t *sqlite, int32_t nr_max);
int32_t sabitcom_mq_resend_del_id(sqlite_t *sqlite, int32_t id);

C_CODE_END

#endif
