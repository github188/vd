#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "time_stat.h"
#include "sys/time_util.h"
#include "logger/log.h"

C_CODE_BEGIN

typedef struct time_stat {
	ustime_t tab[VE_TS_END];
} time_stat_t;

static time_stat_t time_stat;

static __inline__ void __time_stat_update(time_stat_t *stat, uint_fast8_t pos)
{
	ASSERT(pos < VE_TS_END);

	if (VE_TS_DETECT == pos) {
		memset(stat->tab, 0, sizeof(stat->tab));
	}

	ustime(&stat->tab[pos]);
}

static __inline__ void __time_stat_show_after_send(time_stat_t *stat)
{
	char buf[32];
	ustime_t *ts;

	ts = stat->tab;

	xtime2msec(buf, sizeof(buf), ts + VE_TS_DETECT);
	INFO("[detect]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_DSP_MSG);
	INFO("[dsp_msg]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_DSP_REC);
	INFO("[dsp_rec]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_SOSR);
	INFO("[sosr]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_EOSR);
	INFO("[eosr]: %s", buf);

	INFO("[dsp_msg - detect]: %lld",
		 ustime_diff_msec(ts + VE_TS_DSP_MSG, ts + VE_TS_DETECT));
	INFO("[dsp_rec - detect]: %lld",
		 ustime_diff_msec(ts + VE_TS_DSP_REC, ts + VE_TS_DETECT));
	INFO("[dsp_rec - dsp_msg]: %lld",
		 ustime_diff_msec(ts + VE_TS_DSP_REC, ts + VE_TS_DSP_MSG));
	INFO("[sosr - detect]: %lld",
		 ustime_diff_msec(ts + VE_TS_SOSR, ts + VE_TS_DETECT));
	INFO("[eosr - detect]: %d",
		 ustime_diff_msec(ts + VE_TS_EOSR, ts + VE_TS_DETECT));
	INFO("[sosr - dsp_msg]: %lld",
		 ustime_diff_msec(ts + VE_TS_SOSR, ts + VE_TS_DSP_MSG));
	INFO("[eosr - dsp_msg]: %d",
		 ustime_diff_msec(ts + VE_TS_EOSR, ts + VE_TS_DSP_MSG));
	INFO("[sosr - dsp_rec]: %lld",
		 ustime_diff_msec(ts + VE_TS_SOSR, ts + VE_TS_DSP_REC));
	INFO("[eosr - dsp_rec]: %d",
		 ustime_diff_msec(ts + VE_TS_EOSR, ts + VE_TS_DSP_REC));
	INFO("[eosr - sosr]: %d",
		 ustime_diff_msec(ts + VE_TS_EOSR, ts + VE_TS_SOSR));
}


static __inline__ void __time_stat_show_after_rg_up(time_stat_t *stat)
{
	char buf[32];
	ustime_t *ts;

	ts = stat->tab;

	xtime2msec(buf, sizeof(buf), ts + VE_TS_DETECT);
	INFO("[detect]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_DSP_RG);
	INFO("[dsp_rg]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_SERV_RG);
	INFO("[serv_rg]: %s", buf);
	xtime2msec(buf, sizeof(buf), ts + VE_TS_RG_UP);
	INFO("[rg_up]: %s", buf);

	INFO("[dsp_rg - detect]: %lld",
		 ustime_diff_msec(ts + VE_TS_DSP_RG, ts + VE_TS_DETECT));
	INFO("[serv_rg - detect]: %lld",
		 ustime_diff_msec(ts + VE_TS_SERV_RG, ts + VE_TS_DETECT));
	INFO("[rg_up - detect]: %d",
		 ustime_diff_msec(ts + VE_TS_RG_UP, ts + VE_TS_DETECT));
}

void time_stat_update(uint_fast8_t pos)
{
	__time_stat_update(&time_stat, pos);
}

void time_stat_show_after_rg_up(void)
{
	__time_stat_show_after_rg_up(&time_stat);
}

void time_stat_show_after_send(void)
{
	__time_stat_show_after_send(&time_stat);
}


C_CODE_END
