#ifndef __GLOBAL_CFG_H
#define __GLOBAL_CFG_H

#include "types.h"
#include "json.h"

C_CODE_BEGIN

enum {
	DETECT_MODE_VIDEO = 0,
	DETECT_MODE_COIL,
	DETECT_MODE_MIX,
	DETECT_MODE_END
};

enum {
	VE_MODE_IN = 0,
	VE_MODE_OUT,
	VE_MODE_END
};

enum {
	ROAD_GATE_CTRL_FRONT = 0,
	ROAD_GATE_CTRL_FRONT_RECORD,
	ROAD_GATE_CTRL_BACKGROUND,
	ROAD_GATE_CTRL_MIX,
	ROAD_GATE_CTRL_END
};

typedef struct ve_global_cfg {
	char prov[32];
	char city[32];
	uint8_t run_mode;
	uint8_t ve_mode;
	uint8_t detect_mode;
	uint8_t rg_mode;
} global_cfg_t;

void global_cfg_init(global_cfg_t *gc);
int32_t global_cfg_read_new(json_object *obj, void *arg);
json_object *global_cfg_write_new(const void *arg);


C_CODE_END

#endif
