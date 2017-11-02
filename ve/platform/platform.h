#ifndef __PLATFORM_H
#define __PLATFORM_H

#include "types.h"
#include "list.h"
#include "mcfw/interfaces/link_api/jpeg_enc_info.h"

C_CODE_BEGIN

#define VE_PFM_MEM_DBG	1

/*
 * Ve platform id
 */
enum{
	VE_PFM_ID_BITCOM = 0,
	VE_PFM_ID_MAX
};

/*
 * Ve platform record type
 */
enum {
	VE_PFM_REC_MSG = 0,
	VE_PFM_REC_PIC,
	VE_PFM_REC_MSG_PIC
};

typedef struct ve_pfm_rec{
	int32_t type;
	SystemVpss_SimcopJpegInfo *info;
}ve_pfm_rec_t;

int32_t ve_pfm_regst(int32_t id,
					 int32_t (*init)(void),
					 int32_t (*rec_recv)(const ve_pfm_rec_t *rec));
int32_t ve_pfm_init(void);
int32_t ve_pfm_record_recv(const ve_pfm_rec_t *rec);
int32_t ve_pfm_record_del(int32_t id, ve_pfm_rec_t *rec);
int32_t ve_pfm_start(int32_t id);
int32_t ve_pfm_stop(int32_t id);


C_CODE_END

#endif
