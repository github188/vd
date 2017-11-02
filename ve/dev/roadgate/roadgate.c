#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include "sys/time_util.h"
#include "config/ve_cfg.h"
#include "roadgate.h"
#include "roadgate_data.h"
#include "roadgate_ports.h"
#include "types.h"
#include "coil/coil.h"
#include "video/video.h"
#include "mix/mix.h"
#include "logger/log.h"

C_CODE_BEGIN

static rg_t rg = {
	.once = PTHREAD_ONCE_INIT
};

static int_fast32_t roadgate_once(rg_t *rg);

/**
 * Road gate control daemon
 *
 * @author chenyj (4/26/2017)
 *
 * @param rg Road gate parameter
 *
 * @return int_fast32_t If success return 0, otherwise return -1
 */
static int_fast32_t roadgate_daemon(rg_t *rg)
{
	int_fast32_t result;

	static const rg_sm_fp rg_sm_tab[DETECT_MODE_END] = {
		[DETECT_MODE_COIL] = coil_mode,
		[DETECT_MODE_VIDEO] = video_mode,
		[DETECT_MODE_MIX] = mix_mode,
	};

	pthread_mutex_lock(&rg->mutex);

	do {
		result = -1;

		ve_cfg_t cfg;
		ve_cfg_get(&cfg);

		if (cfg.global.detect_mode >= DETECT_MODE_END) {
			CRIT("An error detecte mode for @ %s", __func__);
			break;
		}

		if (rg_sm_tab[cfg.global.detect_mode]) {
			rg_sm_tab[cfg.global.detect_mode](rg);
		}

		result = 0;
	} while (0);

	pthread_mutex_unlock(&rg->mutex);

	return result;
}

static void *roadgate_thread(void *arg)
{
	rg_t *rg = (rg_t *)arg;
	ASSERT(rg);

	roadgate_once(rg);
	rg_dev_init();

	for (;;) {
		roadgate_daemon(rg);
		usleep(1 * 1000);
	}

	return NULL;
}

static int_fast32_t roadgate_ram_init(rg_t *rg)
{
	int_fast32_t ret;

	if((ret = pthread_mutex_init(&rg->mutex, NULL)) != 0){
		CRIT("Initialize mutex for roadgate failed: %s", strerror(ret));
		return -1;
	}

	INIT_LIST_HEAD(&rg->ctrl_ls);
	rg->status = STATUS_IDLE;

	return 0;
}

static void roadgate_once_routine(void)
{
	roadgate_ram_init(&rg);
}

static int_fast32_t roadgate_once(rg_t *rg)
{
	int_fast32_t ret;

	if((ret = pthread_once(&rg->once, roadgate_once_routine)) != 0){
		CRIT("Pthread once for roadgate failed: %s", strerror(ret));
		return -1;
	}

	return 0;
}

static int_fast32_t __roadgate_init(rg_t *rg)
{
	int_fast32_t ret;

	roadgate_once(rg);

	if((ret = pthread_create(&rg->tid, NULL, roadgate_thread, rg)) != 0){
		CRIT("Create thread for roadgate failed: %s", strerror(ret));
		return -1;
	}

	return 0;
}

int_fast32_t roadgate_init(void)
{
	return __roadgate_init(&rg);
}

static int_fast32_t __roadgate_new_ctrl(rg_t *rg, rg_ctrl_t *ctrl)
{
	ASSERT(rg);
	ASSERT(ctrl);

	roadgate_once(rg);

	pthread_mutex_lock(&rg->mutex);

	INFO("New road gate ctrl @ %s, controller: %s, type: %s, "
		 "action: %s, timeout: %lu(ms)",
		 ustime_msec(),
		 rg_get_ctrl_ctrler_text(ctrl->ctrler),
		 rg_get_ctrl_type_text(ctrl->type),
		 rg_get_ctrl_action_text(ctrl->action),
		 ctrl->timeout);

	list_add_tail(&ctrl->link, &rg->ctrl_ls);

	pthread_mutex_unlock(&rg->mutex);

	return 0;
}

int_fast32_t roadgate_new_ctrl(rg_ctrl_t *ctrl)
{
	return __roadgate_new_ctrl(&rg, ctrl);
}

C_CODE_END
