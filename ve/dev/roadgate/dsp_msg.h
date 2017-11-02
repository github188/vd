#ifndef __RG_DSP_MSG_H
#define __RG_DSP_MSG_H

#include "types.h"

C_CODE_BEGIN

enum {
	DSP_DETECT_COIL_INTO = 1,
	DSP_DETECT_COIL_LEAVE,
};

enum {
	DSP_SAFE_COIL_INTO = 1,
	DSP_SAFE_COIL_LEAVE,
};

enum {
	DSP_RG_DN = 1,
	DSP_RG_UP
};

void rg_status_sendto_dsp(int_fast32_t status);
void detect_coil_sendto_dsp(int_fast32_t status);
void safe_coil_sendto_dsp(int_fast32_t status);
void ytbrk_rg_status_sendto_dsp(int_fast32_t status);

C_CODE_END


#endif
