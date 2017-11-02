#ifndef __DSP_CFG_H
#define __DSP_CFG_H

#include "types.h"
#include "json.h"

C_CODE_BEGIN

enum {
	DSP_NON_MOTOR_AS_DFT = 0,
	DSP_NON_MOTOR_AS_UNLICENSED,
	DSP_NON_MOTOR_AS_NON_MOTOR,
	DSP_NON_MOTOR_MODE_END
};

enum {
	DSP_CAPTURE_DFT = 0,
	DSP_CAPTURE_REALTIME,
	DSP_CAPTURE_END
};

typedef struct ve_dsp_cfg {
	uint8_t white_list_match;
	uint8_t capture_mode;
	uint8_t non_motor_mode;
} dsp_cfg_t;

void dsp_cfg_init(dsp_cfg_t *dc);
int32_t dsp_cfg_read_new(json_object *obj, void *arg);
json_object *dsp_cfg_write_new(const void *arg);


C_CODE_END

#endif
