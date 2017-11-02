#ifndef __VE_ROADGATE_H
#define __VE_ROADGATE_H

#include <pthread.h>
#include "types.h"
#include "roadgate_data.h"


C_CODE_BEGIN

int_fast32_t roadgate_init(void);
int_fast32_t roadgate_new_ctrl(rg_ctrl_t *ctrl);

C_CODE_END

#endif
