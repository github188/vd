#ifndef __DEV_LIGHT_CFG_H
#define __DEV_LIGHT_CFG_H

#include "types.h"
#include "json_cfg.h"
#include "json.h"

C_CODE_BEGIN

enum {
	WLIGHT_MODE_NIGHT_ON = 0,
	WLIGHT_MODE_COIL_TOGGLE,
	WLIGHT_MODE_ALWAYS_ON,
	WLIGHT_MODE_END
};

typedef struct light_cfg {
	uint8_t white_bright;
	uint8_t white_mode;
}light_cfg_t;

int32_t dev_light_cfg_read_new(json_object *obj, void *arg);
void dev_light_cfg_init(light_cfg_t *lc);
json_object *dev_light_cfg_write_new(const void *arg);


C_CODE_END

#endif
