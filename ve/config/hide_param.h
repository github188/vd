#ifndef __HIDE_PARAM_H
#define __HIDE_PARAM_H

#include "types.h"

C_CODE_BEGIN

enum {
	VE_DETECT_MODE_VIDEO,
	VE_DETECT_MODE_COIL
};

typedef struct habitcom{
	int32_t rg_timeout;
	int32_t run_mode;
}habitcom_t;


void habitcom_init(void);
void habitcom_read_new(void);
void habitcom_get(habitcom_t *param);


C_CODE_END


#endif
