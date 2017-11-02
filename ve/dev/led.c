#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include "types.h"
#include "list.h"
#include "logger/log.h"
#include "led.h"
#include "dev/led/led.h"
#include "led_ports.h"
#include "config/ve_cfg.h"
#include "sys/heap.h"
#include "sys/base64.h"
#include "sys/xstring.h"
#include "dev/audio/audio_data.h"
#include "ve/dev/audio.h"


C_CODE_BEGIN

#define VE_LED_DEBUG	1

typedef struct led_model {
	int32_t model;
	int32_t (*refresh)(led_screen_t *screen);
	int_fast32_t (*clear_line)(int_fast32_t idx);
	int32_t (*reboot)(void);
	int32_t (*init)(void);
	int32_t nr_line_max;
}led_model_t;

typedef struct line_info {
	int32_t delay;
	list_head_t ls;
} line_info_t;

typedef struct ve_led {
	const led_model_t *model;
	bool lock;
	line_info_t lines[LED_MAX_LINES_NUM];
	pthread_mutex_t mutex;
	pthread_t thread;
} ve_led_t;

static int32_t ve_led_init_default(void);
static int32_t ve_led_reboot_default(void);
static inline int32_t __led_get_max_line_num(ve_led_t *led);
static int_fast32_t led_disp_clear(ve_led_t *led);


static ve_led_t ve_led;
static const led_model_t model_tab[] = {
	{
		.model = LED_MODEL_FLY_EQ20131,
		.refresh = ve_fly_eq20131_refresh,
		.reboot = ve_led_reboot_default,
		.init = ve_led_init_default,
		.nr_line_max = 4
	},

	{
		.model = LED_MODEL_DEGAO,
		.refresh = ve_degao_refresh,
		.reboot = ve_led_reboot_default,
		.clear_line = ve_degao_clear_line,
		.init = ve_led_init_default,
		.nr_line_max = 1
	},

	{
		.model = LED_MODEL_LISTEN_A4,
		.refresh = listen_refresh,
		.reboot = ve_led_reboot_default,
		.init = listen_init,
		.clear_line = listen_clear_line,
		.nr_line_max = 4
	}
};



static bool ve_led_is_locked(ve_led_t *led)
{
	bool lock;

	pthread_mutex_lock(&led->mutex);
	lock = led->lock;
	pthread_mutex_unlock(&led->mutex);

	return lock;
}

static inline void __ve_led_set_lock(ve_led_t *led, bool lock)
{
	pthread_mutex_lock(&led->mutex);

	led->lock = lock;

	if (led->lock) {
		line_info_t *info = led->lines;
		line_info_t *info_end = led->lines + numberof(led->lines);
		for (; info < info_end; ++info) {
			info->delay = 0;
		}
	}

	pthread_mutex_unlock(&led->mutex);
}

void ve_led_set_lock(bool lock)
{
	return __ve_led_set_lock(&ve_led, lock);
}

/**
 * __led_add_line Add a new line to list tail
 *
 * @author chenyj (2016-09-01)
 *
 * @param led Led data struct
 * @param line Line's parameter
 *
 * @return __inline__ int_fast32_t
 */
static __inline__ int_fast32_t __led_add_lines(ve_led_t *led,
		led_line_t **lines, int_fast32_t n)
{
	int_fast32_t ret;
	int_fast32_t cnt = 0;

	ASSERT(led);
	ASSERT(lines);
	ASSERT(n > 0);

	if (ve_led_is_locked(led)) {
		for (int_fast32_t i = 0; i < n; ++i) {
			xfree(lines[i]);
		}

		return 0;
	}

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);

	if ((ret = pthread_mutex_lock(&led->mutex)) != 0) {
		CRIT("Lock mutex for led line failed");
		return -1;
	}

	for (int_fast32_t i = 0; i < n; ++i) {
		/*
		 * If check ok, add to list, otherwise free the buffer
		 */
		if (led_line_param_chk(lines[i]) == 0) {
			list_add_tail(&lines[i]->link, &led->lines[lines[i]->idx].ls);
			++cnt;
		} else {
			led_line_free(lines[i]);
			ERROR("Led line's parameter check failed, drop it");
		}
	}

	if ((ret = pthread_mutex_unlock(&led->mutex)) != 0) {
		CRIT("Unlock mutex for led line failed");
		return -1;
	}

	return cnt;
}

static void ve_led_del_line(led_line_t *line)
{
	list_del(&line->link);
	led_line_free(line);
}

int_fast32_t led_add_lines(led_line_t **lines, int_fast32_t n)
{
	return __led_add_lines(&ve_led, lines, n);
}

static int_fast32_t led_disp_a_line(ve_led_t *led, int_fast32_t idx,
									const char *s)
{
	led_screen_t screen;
	led_line_t line;

	INIT_LIST_HEAD(&screen.lines);
	line.action = LED_ACTION_DISP;
	line.idx = idx;
	strlcpy(line.text, s, sizeof(line.text));
	line.style = LED_STYLE_IMMEDIATE;
	line.mode = LED_MODE_LINE;
	line.duration = 0;
	line.color.rgb = 0xFF0000;
	line.time_per_col = 100;

	led_line_add_to_screen(&screen, &line);

	return led->model->refresh(&screen);
}

static void led_disp_init(ve_led_t *led)
{
	ve_cfg_t cfg;
	ve_cfg_get(&cfg);

	const char *first = cfg.dev.led.user_content;
	const char *second = (cfg.global.ve_mode == VE_MODE_IN) ?
		"您好" : "一路顺风";

	led_disp_clear(led);
	led_disp_a_line(led, 0, first);
	led_disp_a_line(led, 1, second);
}

static int_fast32_t led_disp_clear_a_line(ve_led_t *led, int_fast32_t idx)
{
	led_screen_t screen;
	led_line_t line;

	INIT_LIST_HEAD(&screen.lines);
	line.action = LED_ACTION_CLR;
	line.idx = idx;
	line.style = LED_STYLE_IMMEDIATE;
	line.mode = LED_MODE_LINE;
	line.duration = 0;
	line.color.rgb = 0xFF0000;
	line.time_per_col = 100;

	led_line_add_to_screen(&screen, &line);

	return led->model->refresh(&screen);
}

static int_fast32_t led_disp_clear(ve_led_t *led)
{
	int_fast32_t result = 0;
	led_screen_t screen;
	led_line_t *lines[LED_MAX_LINES_NUM] = { NULL };

	INIT_LIST_HEAD(&screen.lines);

	led_line_t **end = lines + __led_get_max_line_num(led);

	for (led_line_t **line = lines; line < end; ++line) {
		if (((*line) = (led_line_t *)malloc(sizeof(led_line_t)))) {
			(*line)->action = LED_ACTION_CLR;
			(*line)->idx = line - lines;
			(*line)->style = LED_STYLE_IMMEDIATE;
			(*line)->mode = LED_MODE_LINE;
			(*line)->duration = 0;
			(*line)->color.rgb = 0xFF0000;
			(*line)->time_per_col = 100;

			led_line_add_to_screen(&screen, (*line));
		}
	}

	result = led->model->refresh(&screen);

	for (led_line_t **line = lines; line < end; ++line) {
		if (*line) {
			free(*line);
		}
	}

	return result;
}

static led_line_t *find_curr_line(list_head_t *head)
{
	list_head_t *pos, *n;
	led_line_t *curr = NULL;

	/*
	 * Traverse all of list and only retain the last one
	 */
	list_for_each_safe(pos, n, head) {
		curr = list_entry(pos, led_line_t, link);
		if (!list_is_last(&curr->link, head)) {
			ve_led_del_line(curr);
		}
	}

	return curr;
}

static void led_refresh_daemon(ve_led_t *led)
{
	pthread_mutex_lock(&led->mutex);

	led_screen_t screen;
	INIT_LIST_HEAD(&screen.lines);

	for (uint_fast32_t i = 0; i < numberof(led->lines); ++i) {
		line_info_t *info = led->lines + i;

		if (info->delay > 0) {
			--info->delay;
		} else if (info->delay == 0) {
			led_disp_clear_a_line(led, i);
			info->delay = -1;
		}

		led_line_t *line = find_curr_line(&info->ls);
		if (line) {
			if ((line->duration > 0) || (info->delay == -1) || line->cover) {
				/* Move to screen list */
				led_line_move_to_screen(&screen, line);
				/* Refresh time */
				if (led_line_have_action(line, LED_ACTION_DISP)) {
					/*
					 * Add 2s to prevent black screen,
					 * thread run period is 10ms
					 */
					if (line->duration > 0) {
						info->delay = (line->duration + 2) * 100;
					} else {
						info->delay = -1;
					}
				}

				if (led_line_have_action(line, LED_ACTION_AUDIO)) {
					audio_param_t *ap = audio_param_alloc();
					if (ap) {
						ap->volume = line->volume;
						ap->flag = AUDIO_BUF;
						ap->len = Base64decode_len(line->audio);
						if (ap->len <= sizeof(ap->buf)) {
							Base64decode((char *)ap->buf, (char *)line->audio);
							audio_adjust_param(ap);
							audio_put(ap);
						} else {
							audio_param_free(ap);
							ERROR("Mp3 data is too large, drop it");
						}
					}
				}
			}
		}
	}

	if (!list_empty(&screen.lines)) {
		/* Show led screen lines */
		led_screen_show_lines(&screen);
		/* Refresh this screen */
		led->model->refresh(&screen);
		/* Free memory */
		led_screen_free_lines(&screen);
	}

	pthread_mutex_unlock(&led->mutex);
}

static void *ve_led_thread(void *arg)
{
	ve_led_t *led;

	ASSERT(arg);
	led = (ve_led_t *)arg;

	led_disp_init(led);

	prctl(PR_SET_NAME, "ve_led");

	for (;;) {
		led_refresh_daemon(led);
		usleep(10000);
	}

	return NULL;
}

static led_model_t *led_model_search(const led_model_t *tab, int32_t nmodel,
									 int32_t model)
{
	const led_model_t *p, *end;

	p = tab;
	end = tab + nmodel;

	for (; p < end; ++p) {
		if (p->model == model) {
			return (led_model_t *)p;
		}
	}

	return NULL;
}

static __inline__ void __led_set_mode(ve_led_t *led, const led_model_t *tab,
									  int32_t nmodel, int32_t model)
{
	led->model = led_model_search(model_tab, numberof(model_tab), model);
	if (NULL == led->model) {
		ERROR("The model of led is not support, set to degao as default!");
		led->model = &model_tab[0];
	}

	INFO("Led model changed to %d", led->model->model);

	if (led->model->init() != 0) {
		ERROR("Initialize led model failed");
	}
}

static void __led_set_model_s(ve_led_t *led, const led_model_t *tab,
							  int32_t nmodel, int32_t model)
{
	pthread_mutex_lock(&led->mutex);
	__led_set_mode(led, tab, nmodel, model);
	pthread_mutex_unlock(&led->mutex);
}

void led_set_model(int32_t model)
{
	__led_set_model_s(&ve_led, model_tab, numberof(model_tab), model);
}

static inline int32_t __led_get_max_line_num(ve_led_t *led)
{
	ASSERT(led->model->nr_line_max <= LED_MAX_LINES_NUM);
	return min(led->model->nr_line_max, LED_MAX_LINES_NUM);
}

int32_t ve_led_get_max_line_num(void)
{
	return __led_get_max_line_num(&ve_led);
}

static inline int32_t __ve_led_ram_init(ve_led_t *led)
{
	int_fast32_t ret;

	for (uint_fast32_t i = 0; i < numberof(led->lines); ++i) {
		INIT_LIST_HEAD(&led->lines[i].ls);
		led->lines[i].delay = -1;
	}

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	__led_set_mode(led, model_tab, numberof(model_tab), cfg.dev.led.model);

	led->lock = false;

	if((ret = pthread_mutex_init(&led->mutex, NULL)) != 0) {
		ERROR("Initialize mutex ")
		return -1;
	}

	return 0;
}

int32_t ve_led_ram_init(void)
{
	return __ve_led_ram_init(&ve_led);
}

static inline int32_t __ve_led_thread_init(ve_led_t *led)
{
	return pthread_create(&led->thread, NULL, ve_led_thread, led);
}

int32_t ve_led_thread_init(void)
{
	return __ve_led_thread_init(&ve_led);
}

static int32_t ve_led_reboot_default(void)
{
	return 0;
}

static int32_t ve_led_init_default(void)
{
	return 0;
}

C_CODE_END
