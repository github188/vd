#include "detect.h"
#include "ve/dev/light.h"
#include "ve/dev/roadgate/dsp_msg.h"
#include "config/cam_param.h"
#include "config/hide_param.h"
#include "ve/dev/roadgate/roadgate_data.h"
#include "ve/dev/roadgate/roadgate_ports.h"
#include "ve/dev/roadgate/roadgate.h"
#include "ve/config/ve_cfg.h"
#include "sys/time_util.h"
#include "logger/log.h"
#include "types.h"
#include "ve/event.h"
#include "sys/xstring.h"

C_CODE_BEGIN

typedef struct coil_detect {
	pthread_t dc_tid;
	pthread_t sc_tid;
} coil_detect_t;

static coil_detect_t coil_detect;

static void detect_coil_daemon(void)
{
	static uint8_t prev_status = COIL_DETECT_NONE;
	int_fast32_t curr_status = rg_read_in_coil();

	if (curr_status != prev_status) {
		DEBUG("Detect coil status chaned, %d -> %d", 
			  prev_status, curr_status);

		event_item_t *ei = event_item_alloc();
		if (ei) {
			strlcpy(ei->ident, "detect coil", sizeof(ei->ident));
			snprintf(ei->desc, sizeof(ei->desc), "%s -> %s",
					 get_coil_text(prev_status), get_coil_text(curr_status));
			event_put(ei);
		}

		prev_status = curr_status;

		habitcom_t param;
		habitcom_get(&param);

		if (curr_status == COIL_DETECT_VEHICLE) {
			if (param.run_mode == 2) {
				/* Turn on the light */
				ve_cm_white_light_set(true);

				ve_cfg_t cfg;
				ve_cfg_get(&cfg);
				if (cfg.global.rg_mode == ROAD_GATE_CTRL_FRONT) {
					rg_ctrl_t *rg_ctrl = rg_new_ctrl_alloc();
					if (rg_ctrl) {
						rg_ctrl->action = RG_ACTION_UP;
						rg_ctrl->ctrler = RG_CTRLER_LO;
						rg_ctrl->timeout = 1000;
						rg_ctrl->type = RG_CTRL_AUTO;

						roadgate_new_ctrl(rg_ctrl);
					}
				}

				detect_coil_sendto_dsp(DSP_DETECT_COIL_INTO);
			} else if (6 == param.run_mode) {
				ytbrk_rg_status_sendto_dsp(DSP_RG_UP);
			}
		} else {
			if (param.run_mode == 2) {
				/* Turn on the light */
				ve_cm_white_light_set(false);
				detect_coil_sendto_dsp(DSP_DETECT_COIL_LEAVE);
			} else if (6 == param.run_mode) {
				ytbrk_rg_status_sendto_dsp(DSP_RG_DN);
			}
		}
	}
}

static void safe_coil_daemon(void)
{
	static uint8_t prev_status = COIL_DETECT_NONE;
	int_fast32_t curr_status = rg_read_safe_coil();

	if (prev_status != curr_status) {
		DEBUG("Safe coil status chaned, %d -> %d", 
			  prev_status, curr_status);

		event_item_t *ei = event_item_alloc();
		if (ei) {
			strlcpy(ei->ident, "safe coil", sizeof(ei->ident));
			snprintf(ei->desc, sizeof(ei->desc), "%s -> %s",
					 get_coil_text(prev_status), get_coil_text(curr_status));
			event_put(ei);
		}

		prev_status = curr_status;
		if (curr_status == COIL_DETECT_VEHICLE) {
			safe_coil_sendto_dsp(DSP_SAFE_COIL_INTO);
		} else {
			safe_coil_sendto_dsp(DSP_SAFE_COIL_LEAVE); 
		}
	}
}

static void *detect_coil_thread(void *arg)
{
	for (;;) {
		ve_cfg_t cfg;
		ve_cfg_get(&cfg);

		if (cfg.global.detect_mode == DETECT_MODE_COIL) {
			detect_coil_daemon();
			usleep(1000);
		} else {
			sleep(10);
		}
	}

	return NULL;
}

static void *safe_coil_thread(void *arg)
{
	for (;;) {
		ve_cfg_t cfg;
		ve_cfg_get(&cfg);

		if (cfg.global.detect_mode == DETECT_MODE_COIL) {
			safe_coil_daemon();
			usleep(1000);
		} else {
			sleep(10);
		}
	}

	return NULL;
}

static void coil_detect_once_routine(void)
{
	pthread_create(&coil_detect.dc_tid, NULL, detect_coil_thread, NULL);
	pthread_create(&coil_detect.sc_tid, NULL, safe_coil_thread, NULL);
}

void coil_detect_once(void)
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;

	pthread_once(&once, coil_detect_once_routine);
}

C_CODE_END
