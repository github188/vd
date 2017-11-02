#ifndef __AUDIO_DATA_H
#define __AUDIO_DATA_H

#include <limits.h>
#include "types.h"

C_CODE_BEGIN

#define AUDIO_FILE	0x01
#define AUDIO_TEXT		0x02
#define AUDIO_BUF		0x04

#define AUDIO_BUF_SIZE		(512 * 1024)

typedef struct audio_param {
	uint8_t volume;
	uint8_t flag;
	size_t len;
	union {
		uint8_t buf[AUDIO_BUF_SIZE];
		char file[PATH_MAX + 1];
	};
} audio_param_t;


audio_param_t *audio_param_alloc(void);
void audio_param_free(audio_param_t *ptr);

C_CODE_END

#endif
