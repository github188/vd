#include "ve/dev/roadgate/roadgate_data.h"
#include "ve/dev/roadgate/roadgate_ports.h"
#include "config/ve_cfg.h"
#include "sys/time_util.h"
#include "logger/log.h"
#include "types.h"
#include "mix.h"



C_CODE_BEGIN


int_fast32_t mix_mode(rg_t *rg)
{
	static const rg_sm_fp mm_sm_tab[VE_MODE_END][ROAD_GATE_CTRL_END] = {
		[VE_MODE_IN][ROAD_GATE_CTRL_FRONT] = mix_front_entrance,
		[VE_MODE_IN][ROAD_GATE_CTRL_FRONT_RECORD] = mix_front_record_entrance,
		[VE_MODE_IN][ROAD_GATE_CTRL_BACKGROUND] = mix_center_entrance,
		[VE_MODE_IN][ROAD_GATE_CTRL_MIX] = mix_mix_entrance,

		[VE_MODE_OUT][ROAD_GATE_CTRL_FRONT] = mix_front_export,
		[VE_MODE_OUT][ROAD_GATE_CTRL_FRONT_RECORD] = mix_front_record_export,
		[VE_MODE_OUT][ROAD_GATE_CTRL_BACKGROUND] = mix_center_export,
		[VE_MODE_OUT][ROAD_GATE_CTRL_MIX] = mix_mix_export
	};

	mix_detect_once();

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);

	if (cfg.global.ve_mode >= VE_MODE_END) {
		ERROR("An error ve mode, %d", cfg.global.ve_mode);
		return -1;
	}

	if (cfg.global.rg_mode >= ROAD_GATE_CTRL_END) {
		ERROR("An error rg mode, %d", cfg.global.rg_mode);
		return -1;
	}

	if (mm_sm_tab[cfg.global.ve_mode][cfg.global.rg_mode]) {
		return mm_sm_tab[cfg.global.ve_mode][cfg.global.rg_mode](rg);
	}

	return -1;

}


C_CODE_END
