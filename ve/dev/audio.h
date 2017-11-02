#ifndef __VE_DEV_AUDIO_H
#define __VE_DEV_AUDIO_H


#include "types.h"
#include "dev/audio/audio_data.h"

C_CODE_BEGIN

int_fast32_t ve_audio_ram_init(void);
void audio_adjust_param(audio_param_t *ap);
int_fast32_t audio_put(const audio_param_t *ap);
int_fast32_t audio_dev_change(uint_fast8_t model);
int_fast32_t audio_dev_set_volume(uint_fast8_t volume);
int_fast32_t audio_stop(void);

C_CODE_END


#endif
