#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "list.h"
#include "sys/pool.h"
#include "profile.h"
#include "logger/log.h"
#include "config/hide_param.h"
#include "config/ve_cfg.h"
#include "config/cam_param.h"
#include "dev/light.h"
#include "dev/light_ports.h"


C_CODE_BEGIN

#define LIGHT_OFF	0
#define LIGHT_ON		1

enum {
	WL_STATUS_OFF = 0,
	WL_STATUS_ON
};

typedef struct vm_wlight {
	uint8_t action;
} vm_white_t;

typedef struct cm_wlight_t {
	int8_t action;
} cm_white_t;


typedef struct white {
	pthread_mutex_t mutex;
	uint8_t status;
	uint8_t prev_status;
	union {
		vm_white_t vm;
		cm_white_t cm;
	};
} white_t;

typedef struct light_mng {
	list_head_t idle_ls;
	list_head_t steady_ls;
	list_head_t flash_ls;
	pthread_t tid;
	pool_t light_po;
	pthread_mutex_t mutex;
	light_t light_entry[LIGHT_ID_END];
	white_t white;

} light_mng_t;

static void white_light_daemon(light_mng_t *lm);

static light_mng_t light_mng;

/* 
 * This table is use to convert light id to string
 */
static const char *light_id_name[] = {
	[LIGHT_ID_GREEN] = "green",
	[LIGHT_ID_RED] = "red",
	[LIGHT_ID_BLUE] = "blue",
	[LIGHT_ID_WHITE] = "white"
};

static const char *light_status_text[] = {
	[WL_STATUS_ON] = "on",
	[WL_STATUS_OFF] = "off"
};

static const char *get_light_status_text(uint_fast32_t status)
{
	if (status < numberof(light_status_text)) {
		return light_status_text[status];
	}

	return NULL;
}

static __inline__ void __light_mng_init(light_mng_t *lm)
{
	ASSERT(lm);

	/* Initialize idle list */
	INIT_LIST_HEAD(&lm->idle_ls);
	/* Initialize steady list */
	INIT_LIST_HEAD(&lm->steady_ls);
	/* Initialize flash list */
	INIT_LIST_HEAD(&lm->flash_ls);
	/* Create pool for lights */
	create_pool(&lm->light_po, lm->light_entry, numberof(lm->light_entry),
				sizeof(lm->light_entry[0]), offsetof(light_t, link));
	pthread_mutex_init(&lm->mutex, NULL);
	pthread_mutex_init(&lm->white.mutex, NULL);
	lm->white.status = WL_STATUS_OFF;
}

/**
 * @brief Find a light id from list
 * 
 * @author cyj (2016/7/18)
 * 
 * @param ls 
 * @param id 
 * 
 * @return light_t* - The light's pointer
 */
static light_t *light_find_from_list(list_head_t *ls, uint_fast8_t id)
{
	list_head_t *pos, *n;
	light_t *light;

	list_for_each_safe(pos, n, ls) {
		light = list_entry(pos, light_t, link);
		if (light->id == id) {
			return light;
		}
	}

	return NULL;
}

static __inline__ light_t *__light_find(light_mng_t *lm, uint_fast8_t id)
{
	light_t *light;

	if ((light = light_find_from_list(&lm->idle_ls, id))) {
		return light;
	}

	if ((light = light_find_from_list(&lm->steady_ls, id))) {
		return light;
	}

	if ((light = light_find_from_list(&lm->flash_ls, id))) {
		return light;
	}

	return NULL;
}

static __inline__ int_fast8_t __light_regst(light_mng_t *lm, uint_fast8_t id,
											const light_opt_t *opt)
{
	light_t *_new;

	ASSERT(lm);
	ASSERT(opt);
	ASSERT(opt->set);

	pthread_mutex_lock(&lm->mutex);

	if (NULL != __light_find(lm, id)) {
		ERROR("The light is already registed");
		return -1;
	}

	_new = (light_t *)get_from_pool(&lm->light_po, offsetof(light_t, link));
	if (NULL == _new) {
		CRIT("Alloc memory from pool for light %d failed!", id);
		return -1;
	}

	_new->id = id;
	memcpy(&_new->opt, opt, sizeof(*opt));
	list_add_tail(&_new->link, &lm->idle_ls);

	pthread_mutex_unlock(&lm->mutex);

	return 0;
}

static int_fast8_t light_ctrl_put_id(light_mng_t *lm, uint_fast8_t id,
									 const light_info_t *info)
{
	light_t *light;

	light = __light_find(lm, id);
	if (NULL == light) {
		CRIT("Failed to find light %d", id);
		return -1;
	}

	DEBUG("Find the light: %s", light_id_name[light->id]);

	/* Copy the information */
	memcpy(&light->info, info, sizeof (*info));

	/*
	 * If the interval is zero, add to steady list, otherwise add to
	 * flash list
	 */
	if (0 == info->intv) {
		list_move_tail(&light->link, &lm->steady_ls);
		DEBUG("Light move to steady list");
	} else {
		if (-1 == clock_gettime(CLOCK_MONOTONIC, &light->prev_tm)) {
			light->prev_tm.tv_sec = 0;
			light->prev_tm.tv_nsec = 0;
		}
		light->prev_val = 0;
		list_move_tail(&light->link, &lm->flash_ls);
		DEBUG("Light move to flash list");
	}

	return 0;
}

static void light_ctrl_show(const light_ctrl_t *ctrl)
{
	uint_fast8_t i;

	INFO("========== light control ===========");
	INFO("%-16s%-16s%-16s", "name", "value", "intv");
	INFO("------------------------------------");
	for (i = 0; i < LIGHT_ID_END; ++i) {
		if (getbit(ctrl->mask, i)) {
			INFO("%-16s%-16d%-16d",
				 light_id_name[i], ctrl->info[i].value, ctrl->info[i].intv);
		}
	}
	INFO("====================================");
}

static __inline__ int_fast8_t __light_ctrl_put(light_mng_t *lm,
											   const light_ctrl_t *ctrl)
{
	uint_fast8_t i;
	int_fast8_t ret = 0;

	light_ctrl_show(ctrl);

	pthread_mutex_lock(&lm->mutex);

	for (i = 0; i < LIGHT_ID_END; ++i) {
		if (getbit(ctrl->mask, i)) {
			/*
			 * Have a new control msg for green light
			 */
			if (0 != light_ctrl_put_id(lm, i, ctrl->info + i)) {
				ret = -1;
				break;
			}
		}
	}

	pthread_mutex_unlock(&lm->mutex);

	return ret;
}

static __inline__ int_fast8_t light_ctrl_daemon(light_mng_t *lm)
{
	light_t *light;
	list_head_t *pos, *n;
	struct timespec curr_tm;
	int64_t tm_diff;
	uint_fast8_t value;

	pthread_mutex_lock(&lm->mutex);

	list_for_each_safe(pos, n, &lm->steady_ls) {
		light = list_entry(pos, light_t, link);
		light->opt.set(light, light->info.value);

		list_move_tail(&light->link, &lm->idle_ls);
	}

	if (-1 == clock_gettime(CLOCK_MONOTONIC, &curr_tm)) {
		curr_tm.tv_sec = 0;
		curr_tm.tv_nsec = 0;
	}

	list_for_each_safe(pos, n, &lm->flash_ls) {
		light = list_entry(pos, light_t, link);

		tm_diff = timespec_diff_msec(&curr_tm, &light->prev_tm);
		if ((tm_diff >= (light->info.intv * 100)) || (tm_diff <= 0)) {
			value = (0 == light->prev_val) ? light->info.value : 0;
			light->opt.set(light, value);
			light->prev_val = value;

			light->prev_tm.tv_sec = curr_tm.tv_sec;
			light->prev_tm.tv_nsec = curr_tm.tv_nsec;
		}
	}

	pthread_mutex_unlock(&lm->mutex);

	return 0;
}

static void *light_thread(void *arg)
{
	light_mng_t *lm;

	ASSERT(arg);

	lm = (light_mng_t *)arg;

	for (;;) {
		light_ctrl_daemon(lm);
		white_light_daemon(lm);

		usleep(1000);
	}

	return NULL;
}

static void white_light_set(bool on)
{
	light_ctrl_t ctrl;

	memset(&ctrl, 0, sizeof(ctrl));

	ctrl.info[LIGHT_ID_WHITE].value = on ? 255 : 0;
	ctrl.info[LIGHT_ID_WHITE].intv = 0;
	setbit(ctrl.mask, LIGHT_ID_WHITE);

	__light_ctrl_put(&light_mng, &ctrl);
}

/**
 * Coil mode set light ctrl
 *
 * @author chenyj (2016/9/24)
 *
 * @param lm
 * @param on
 */
static void __ve_cm_white_light_set(light_mng_t *lm, bool on)
{
	pthread_mutex_lock(&lm->white.mutex);

	habitcom_t hide_param;
	habitcom_get(&hide_param);

	if (hide_param.run_mode == 2) {
		lm->white.cm.action = on ? LIGHT_ON : LIGHT_OFF;
	}

	pthread_mutex_unlock(&lm->white.mutex);
}

/**
 * ve_cm_white_light_set - ve coil mode white light set
 *
 * @author cyj (2016/6/13)
 *
 * @param on
 */
void ve_cm_white_light_set(bool on)
{
	__ve_cm_white_light_set(&light_mng, on);
}

static int_fast32_t white_light_always(light_mng_t *lm)
{
	int_fast32_t status = lm->white.status;

	switch (lm->white.status) {
		case WL_STATUS_OFF:
			white_light_set(true);
			status = WL_STATUS_ON;
			break;

		default:
			break;
	}

	return status;
}

static int_fast32_t white_light_night(light_mng_t *lm)
{
	int_fast32_t status = lm->white.status;
	bool daytime = isdaytime();

	switch (lm->white.status) {
		case WL_STATUS_ON:
		{
			ve_cfg_t cfg;
			ve_cfg_get(&cfg);

			static uint8_t prev_bright = 0;
			if (prev_bright != cfg.dev.light.white_bright) {
				INFO("White light brightness changed, %d => %d",
					 prev_bright, cfg.dev.light.white_bright);
				prev_bright = cfg.dev.light.white_bright;
				white_light_set(true);
			}

			if (daytime) {
				white_light_set(false);
				status = WL_STATUS_OFF;
			}

			break;
		}

		case WL_STATUS_OFF:
			if (!daytime) {
				white_light_set(true);
				status = WL_STATUS_ON;
			}
			break;

		default:
			break;
	}

	return status;
}

static int_fast32_t white_light_night_coil(light_mng_t *lm)
{
	int_fast32_t status = lm->white.status;

	switch (lm->white.status) {
		case WL_STATUS_ON:
			if (lm->white.cm.action == LIGHT_OFF) {
				lm->white.cm.action = -1;
				white_light_set(false);
				status = WL_STATUS_OFF;
			}
			break;

		case WL_STATUS_OFF:
			if (lm->white.cm.action == LIGHT_ON) {
				lm->white.cm.action = -1;
				if (!isdaytime()) {
					white_light_set(true);
					status = WL_STATUS_ON;
				}
			}
			break;

		default:
			break;
	}

	return status;
}


static void white_light_update_status(light_mng_t *lm, uint_fast32_t next)
{
	if (next != lm->white.status) {
		lm->white.prev_status = lm->white.status;
		lm->white.status = next;
		INFO("White light status changed: %s -> %s",
			 get_light_status_text(lm->white.prev_status),
			 get_light_status_text(lm->white.status));
	}
}


static void cm_white_light_daemon(light_mng_t *lm)
{
	static int_fast32_t (*wlight_ctrl_handle_tab[])(light_mng_t *lm) = {
		[WLIGHT_MODE_NIGHT_ON] = white_light_night,
		[WLIGHT_MODE_ALWAYS_ON] = white_light_always,
		[WLIGHT_MODE_COIL_TOGGLE] = white_light_night_coil,
	};

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	light_cfg_t *lc = &cfg.dev.light;

	if (lc->white_mode < WLIGHT_MODE_END) {
		if (wlight_ctrl_handle_tab[lc->white_mode]) {
			int_fast32_t next = wlight_ctrl_handle_tab[lc->white_mode](lm);
			white_light_update_status(lm, next);
		}
	}
}

static void mm_white_light_daemon(light_mng_t *lm)
{
	static int_fast32_t (*wlight_ctrl_handle_tab[])(light_mng_t *lm) = {
		[WLIGHT_MODE_NIGHT_ON] = white_light_night,
		[WLIGHT_MODE_ALWAYS_ON] = white_light_always,
		[WLIGHT_MODE_COIL_TOGGLE] = white_light_night_coil,
	};

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	light_cfg_t *lc = &cfg.dev.light;

	if (lc->white_mode < WLIGHT_MODE_END) {
		if (wlight_ctrl_handle_tab[lc->white_mode]) {
			int_fast32_t next = wlight_ctrl_handle_tab[lc->white_mode](lm);
			white_light_update_status(lm, next);
		}
	}
}

static void vm_white_light_daemon(light_mng_t *lm)
{
	static int_fast32_t (*wlight_ctrl_handle_tab[])(light_mng_t *lm) = {
		[WLIGHT_MODE_NIGHT_ON] = white_light_night,
		[WLIGHT_MODE_ALWAYS_ON] = white_light_always,
	};

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	light_cfg_t *lc = &cfg.dev.light;

	if (lc->white_mode < WLIGHT_MODE_END) {
		if (wlight_ctrl_handle_tab[lc->white_mode]) {
			int_fast32_t next = wlight_ctrl_handle_tab[lc->white_mode](lm);
			white_light_update_status(lm, next);
		}
	}
}

static void white_light_daemon(light_mng_t *lm)
{
	static void (*wlight_daemon_tab[DETECT_MODE_END])(light_mng_t *lm) = {
		[DETECT_MODE_VIDEO] = vm_white_light_daemon,
		[DETECT_MODE_COIL] = cm_white_light_daemon,
		[DETECT_MODE_MIX] = mm_white_light_daemon,
	};

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	global_cfg_t *gc = &cfg.global;

	pthread_mutex_lock(&lm->white.mutex);

	if (gc->detect_mode < DETECT_MODE_END) {
		if (wlight_daemon_tab[gc->detect_mode]) {
			wlight_daemon_tab[gc->detect_mode](lm);
		}
	}

	pthread_mutex_unlock(&lm->white.mutex);
}

int_fast8_t ve_light_ctrl_put(const light_ctrl_t *ctrl)
{
	return __light_ctrl_put(&light_mng, ctrl);
}

int_fast8_t light_regst(uint_fast8_t id, const light_opt_t *opt)
{
	return __light_regst(&light_mng, id, opt);
}

void light_ram_init(void)
{
	__light_mng_init(&light_mng);
}

int32_t light_thread_init(void)
{
	return pthread_create(&light_mng.tid, NULL, light_thread, &light_mng);
}

C_CODE_END
