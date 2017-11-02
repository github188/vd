#ifndef __DEV_AUDIO_CFG_H
#define __DEV_AUDIO_CFG_H

#include "types.h"
#include "json_cfg.h"
#include "json.h"

C_CODE_BEGIN

enum {
	AUDIO_MODEL_OFF = 0,
	AUDIO_MODEL_LO,
	AUDIO_MODEL_END
};

typedef struct audio_cfg{
	uint8_t model;
	uint8_t volume;
} audio_cfg_t;

void dev_audio_cfg_init(audio_cfg_t *ac);
int32_t dev_audio_cfg_read_new(json_object *obj, void *arg);
json_object *dev_audio_cfg_write_new(const void *arg);


C_CODE_END

#endif
