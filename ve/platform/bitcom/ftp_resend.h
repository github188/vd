#ifndef __TABITCOM_FTP_RESEND_H
#define __TABITCOM_FTP_RESEND_H


#include "types.h"
#include "ftp_resend_db.h"


C_CODE_BEGIN

int32_t tabitcom_ftp_resend_ram_init(const char *db_path, const char *pic_dir,
									 int32_t resv);
int32_t tabitcom_ftp_resend_pthread_init(void);
int32_t tabitcom_ftp_resend_put(const sabitcom_ftp_resend_t *item);

C_CODE_END

#endif
