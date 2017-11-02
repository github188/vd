#ifndef __TIME_STAT_H
#define __TIME_STAT_H

#include "types.h"

C_CODE_BEGIN

enum {
	VE_TS_DETECT = 0,
	VE_TS_DSP_REC,
	VE_TS_DSP_MSG,
	VE_TS_DSP_RG,
	VE_TS_SERV_RG,
	VE_TS_SOSR,	/* Start of send record */
	VE_TS_EOSR,	/* End of send record */
	VE_TS_RG_UP,	/* Road gate up */
	VE_TS_END
};


void time_stat_update(uint_fast8_t pos);
void time_stat_show_after_rg_up(void);
void time_stat_show_after_send(void);

C_CODE_END

#endif
