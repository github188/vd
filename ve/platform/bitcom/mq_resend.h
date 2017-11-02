#ifndef __TABITCOM_MQ_RESEND_H
#define __TABITCOM_MQ_RESEND_H


#include "types.h"
#include "mq_resend_db.h"


C_CODE_BEGIN


int32_t tabitcom_mq_resend_ram_init(const char *db_path, int32_t resv);
int32_t tabitcom_mq_resend_pthread_init(void);
int32_t tabitcom_mq_resend_put(const sabitcom_mq_resend_t *item);



C_CODE_END


#endif
