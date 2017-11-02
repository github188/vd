#ifndef __COIL_H
#define __COIL_H

#include "types.h"

C_CODE_BEGIN

void coil_detect_once(void);
int_fast32_t coil_front_entrance(rg_t *rg);
int_fast32_t coil_front_export(rg_t *rg);
int_fast32_t coil_front_record_entrance(rg_t *rg);
int_fast32_t coil_front_record_export(rg_t *rg);
int_fast32_t coil_center_entrance(rg_t *rg);
int_fast32_t coil_center_export(rg_t *rg);
int_fast32_t coil_mix_entrance(rg_t *rg);
int_fast32_t coil_mix_export(rg_t *rg);
int_fast32_t coil_mode(rg_t *rg);

C_CODE_END

#endif


