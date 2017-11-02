#ifndef __AUDIO_MP3_H
#define __AUDIO_MP3_H

#include "types.h"
#include "audio_data.h"

C_CODE_BEGIN

#define MP3_PALY_BY_MPLAY	1
#define MPLAY		"/usr/local/bin/mplay"

int_fast32_t mp3_player_init(void);
int_fast32_t mp3_set_volume(int_fast32_t volume);
int_fast32_t mp3_play_put(const audio_param_t *ap);
int_fast32_t mp3_player_destroy(void);

C_CODE_END

#endif
