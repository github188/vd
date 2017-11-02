#ifndef __MIX_H
#define __MIX_H

#include "types.h"

C_CODE_BEGIN

void mix_detect_once(void);
int_fast32_t mix_front_entrance(rg_t *rg);
int_fast32_t mix_front_export(rg_t *rg);
int_fast32_t mix_front_record_entrance(rg_t *rg);
int_fast32_t mix_front_record_export(rg_t *rg);
int_fast32_t mix_center_entrance(rg_t *rg);
int_fast32_t mix_center_export(rg_t *rg);
int_fast32_t mix_mix_entrance(rg_t *rg);
int_fast32_t mix_mix_export(rg_t *rg);
int_fast32_t mix_mode(rg_t *rg);

C_CODE_END

#endif


