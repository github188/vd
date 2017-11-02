#ifndef __PCM_H
#define __PCM_H

#include <alsa/asoundlib.h>
#include "types.h"

C_CODE_BEGIN

#ifndef BITS2BYTES
#define BITS2BYTES(bits)	((bits) >> 3)
#endif

#ifndef BYTES2FRAMES
#define BYTES2FRAMES(bytes, bpf)	(((bytes) << 3) / bpf)
#endif

typedef struct pcm_param {
	uint32_t rate;
	uint32_t fmt;
	uint32_t nr_chn;
} pcm_param_t;

typedef struct pcm_info {
	uint32_t chunk_size;
	uint32_t chunk_bytes;
	uint32_t bits_per_frame;
	snd_pcm_format_t fmt;
	uint32_t nchn;
} pcm_info_t;

int_fast32_t pcm_open(snd_pcm_t **pcm, const char *name);
int_fast32_t pcm_set_param(snd_pcm_t *pcm, pcm_info_t *info,
						   const pcm_param_t *param);
int_fast32_t pcm_close(snd_pcm_t *pcm);
int_fast32_t mixer_set_volume(int_fast32_t volume);
int_fast32_t pcm_writei(snd_pcm_t *pcm, uint8_t *frm, uint_fast32_t nfrm,
						const pcm_info_t *pcm_info);

C_CODE_END


#endif
