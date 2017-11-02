#ifndef __SABITCOM_FTP_RESEND_H
#define __SABITCOM_FTP_RESEND_H


#include "types.h"
#include "upload.h"
#include "sqlite_util.h"
#include "mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h"


C_CODE_BEGIN

#define SABITCOM_FTP_RESEND_TAB		"ftp_resend"
#define SABITCOM_FTP_PIC_SIZE		SIZE_JPEG_BUFFER
#define SABITCOM_FTP_PATH_SIZE		(FTP_PATH_DEPTH * FTP_PATH_LEN)
#define SABITCOM_FTP_NAME_SIZE		(NAME_MAX_LEN)


typedef struct sabitcom_ftp_resend{
	uint8_t pic[SABITCOM_FTP_PIC_SIZE];
	char name[SABITCOM_FTP_NAME_SIZE];
	char path[SABITCOM_FTP_PATH_SIZE];
	ssize_t size;
}sabitcom_ftp_resend_t;

typedef char sabitcom_ftp_name_t[NAME_MAX_LEN];


int32_t sabitcom_ftp_resend_tab_create(sqlite_t *sqlite);
int32_t sabitcom_ftp_resend_insert(sqlite_t *sqlite,
								   const sabitcom_ftp_resend_t *item);
int32_t sabitcom_ftp_resend_select(sqlite_t *sqlite,
								   sabitcom_ftp_resend_t *item);
int32_t sabitcom_ftp_resend_get_cnt(sqlite_t *sqlite);
int32_t sabitcom_ftp_resend_del_id(sqlite_t *sqlite, int32_t id);
int32_t sabitcom_ftp_resend_del_redundant(sqlite_t *sqlite, int32_t nr_resv,
										  sabitcom_ftp_name_t *name,
										  int32_t nr_name_max);


C_CODE_END

#endif
