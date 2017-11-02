#ifndef __ROADGATE_PORTS_H
#define __ROADGATE_PORTS_H

#include "types.h"

C_CODE_BEGIN

void rg_down(void);
void rg_set_up_signal(void);
void rg_clr_up_signal(void);
void rg_stop(void);
int_fast32_t rg_read_safe_coil(void);
int_fast32_t rg_read_in_coil(void);
void rg_down_s(void);
void rg_dev_init(void);
const char *get_coil_text(int_fast32_t value);

C_CODE_END

#endif
