#ifndef __PFM_CFG_H
#define __PFM_CFG_H

#include "types.h"
#include "json.h"
#include "pfm_bitcom_cfg.h"

C_CODE_BEGIN

typedef struct pfm_cfg {
	uint8_t resend_rotate;
	pfm_bitcom_cfg_t bitcom;
} pfm_cfg_t;

int32_t pfm_cfg_read_new(json_object *obj, void *arg);
json_object *pfm_cfg_write_new(const void *arg);
void pfm_cfg_init(pfm_cfg_t *pc);

C_CODE_END

#endif
