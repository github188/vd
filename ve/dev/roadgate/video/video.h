#ifndef __video_H
#define __video_H

#include "types.h"

C_CODE_BEGIN

int_fast32_t video_front_entrance(rg_t *rg);
int_fast32_t video_front_export(rg_t *rg);
int_fast32_t video_front_record_entrance(rg_t *rg);
int_fast32_t video_front_record_export(rg_t *rg);
int_fast32_t video_center_entrance(rg_t *rg);
int_fast32_t video_center_export(rg_t *rg);
int_fast32_t video_mix_entrance(rg_t *rg);
int_fast32_t video_mix_export(rg_t *rg);
int_fast32_t video_mode(rg_t *rg);

C_CODE_END

#endif


