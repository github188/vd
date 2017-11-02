#ifndef __DEV_AUDIO_8127_H
#define __DEV_AUDIO_8127_H

#include "types.h"
#include "audio_data.h"


C_CODE_BEGIN

int_fast32_t dm8127_audio_init(void);
int_fast32_t dm8127_audio_play(const audio_param_t *ap);
int_fast32_t dm8127_set_volume(int_fast32_t volume);
int_fast32_t dm8127_audio_stop(void);
int_fast32_t dm8127_audio_destory(void);


C_CODE_END

#endif
