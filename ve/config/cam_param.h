#ifndef __CAM_PARAM_H
#define __CAM_PARAM_H

#include "types.h"

C_CODE_BEGIN

#define DAYNIGHT_INFO_FILE	"/tmp/fillinlight_smart_control.info"

enum{
	NIGHTTIME = 0,
	DAYTIME
};

enum {
	DIR_IN = 9,
	DIR_OUT = 10,
};

void daynight_set(int32_t daynight);
int32_t daynight_get(int32_t daynight);
bool isdaytime(void);
int32_t cam_param_ram_init(void);
int32_t cam_param_thread_init(void);
int32_t get_direction(void);

C_CODE_END


#endif
