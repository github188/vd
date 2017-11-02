#ifndef __SABITCOM_HTTP_RESEND_H
#define __SABITCOM_HTTP_RESEND_H

#include <limits.h>
#include "types.h"
#include "sqlite_util.h"
#include "mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h"

C_CODE_BEGIN


#define SABITCOM_HTTP_RESEND_TAB		"http_resend"
#define SABITCOM_HTTP_JSON_SIZE		2048
#define SABITCOM_HTTP_PIC_NAME_SIZE		(PATH_MAX + 1)
#define SABITCOM_HTTP_PIC_SIZE		SIZE_JPEG_BUFFER

typedef struct sabitcom_http_resend{
	char json[SABITCOM_HTTP_JSON_SIZE];
	char name[SABITCOM_HTTP_PIC_NAME_SIZE];
	uint8_t pic[SABITCOM_HTTP_PIC_SIZE];
	size_t size;
}sabitcom_http_resend_t;

typedef char sabitcom_http_name_t[SABITCOM_HTTP_PIC_NAME_SIZE];

int32_t sabitcom_http_resend_tab_create(sqlite_t *sqlite);
int32_t sabitcom_http_resend_insert(sqlite_t *sqlite,
									const sabitcom_http_resend_t *item);
int32_t sabitcom_http_resend_select(sqlite_t *sqlite,
									sabitcom_http_resend_t *item);
int32_t sabitcom_http_resend_del_id(sqlite_t *sqlite, int32_t id);
int32_t sabitcom_http_resend_get_cnt(sqlite_t *sqlite);
int32_t sabitcom_http_resend_del_redundant(sqlite_t *sqlite, int32_t nr_resv,
										  sabitcom_http_name_t *name,
										  int32_t nr_name_max);



C_CODE_END

#endif
