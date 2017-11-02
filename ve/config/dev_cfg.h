#ifndef __DEV_CFG_H
#define __DEV_CFG_H

#include "types.h"
#include "json_cfg.h"
#include "dev_light_cfg.h"
#include "dev_led_cfg.h"
#include "dev_audio_cfg.h"

C_CODE_BEGIN


typedef struct dev_cfg {
	light_cfg_t light;
	led_cfg_t led;
	audio_cfg_t audio;
} dev_cfg_t;

void dev_cfg_init(dev_cfg_t *dc);
int32_t dev_cfg_read_new(json_object *obj, void *arg);
json_object *dev_cfg_write_new(const void *arg);

C_CODE_END

#endif
