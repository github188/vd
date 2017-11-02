#ifndef __TABITCOM_HTTP_RESEND_H
#define __TABITCOM_HTTP_RESEND_H

#include "types.h"
#include "http_resend_db.h"

C_CODE_BEGIN

typedef struct tabitcom_http_info{
	char dev_id[32];
	char serv_ip[32];
	uint16_t port;
	char usr_agent[32];
}tabitcom_http_info_t;

int32_t tabitcom_http_resend_ram_init(const char *db_path,
									  const char *pic_dir,
									  int32_t resv);
int32_t tabitcom_http_resend_pthread_init(void);
int32_t tabitcom_http_get_resend_info(sabitcom_http_resend_t *item,
									  const char *http,
									  const uint8_t *pic,
									  size_t sz_pic);
int32_t tabitcom_http_resend_put(const sabitcom_http_resend_t *item);
int32_t tabitcom_http_resend_set_switch(bool on);

C_CODE_END

#endif
