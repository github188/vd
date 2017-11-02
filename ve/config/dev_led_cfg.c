#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sys/xstring.h"
#include "ve_cfg.h"
#include "global_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"
#include "dev/led.h"

C_CODE_BEGIN

static int32_t dev_led_get_model(json_object *obj, void *arg);
static int32_t dev_led_get_reboot_enable(json_object *obj, void *arg);
static int32_t dev_led_get_reboot_hour(json_object *obj, void *arg);
static int32_t dev_led_get_reboot_wday(json_object *obj, void *arg);
static int32_t dev_led_get_user_content(json_object *obj, void *arg);
static int32_t dev_led_get_lattice(json_object *obj, void *arg);
static int32_t dev_led_get_width(json_object *obj, void *arg);
static int32_t dev_led_get_height(json_object *obj, void *arg);

static json_object *dev_led_set_reboot_hour(const void *arg);
static json_object *dev_led_set_reboot_wday(const void *arg);
static json_object *dev_led_set_reboot_enable(const void *arg);
static json_object *dev_led_set_model(const void *arg);
static json_object *dev_led_set_user_content(const void *arg);
static int32_t dev_led_set_mode_handle(const void *arg);
static json_object *dev_led_set_height(const void *arg);
static json_object *dev_led_set_lattice(const void *arg);
static json_object *dev_led_set_width(const void *arg);

static const enum_str_t led_model_tab[] = {
	{ LED_MODEL_DEGAO, "degao" },
	{ LED_MODEL_FLY_EQ20131, "fly_eq20131" },
	{ LED_MODEL_LISTEN_A4, "listen_a4" }
};

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "model",
		.get = dev_led_get_model,
		.set = dev_led_set_model,
		.set_cb = dev_led_set_mode_handle
	},
	{
		.field = "reboot_enable",
		.get = dev_led_get_reboot_enable,
		.set = dev_led_set_reboot_enable
	},
	{
		.field = "reboot_wday",
		.get = dev_led_get_reboot_wday,
		.set = dev_led_set_reboot_wday
	},
	{
		.field = "reboot_hour",
		.get = dev_led_get_reboot_hour,
		.set = dev_led_set_reboot_hour
	},
	{
		.field = "user_content",
		.set = dev_led_set_user_content,
		.get = dev_led_get_user_content,
	},
	{
		.field = "lattice",
		.set = dev_led_set_lattice,
		.get = dev_led_get_lattice,
	},
	{
		.field = "width",
		.set = dev_led_set_width,
		.get = dev_led_get_width,
	},
	{
		.field = "height",
		.set = dev_led_set_height,
		.get = dev_led_get_height,
	},
};

void dev_led_cfg_init(led_cfg_t *lc)
{
	lc->model = LED_MODEL_DEGAO;
	lc->reboot_hour = 2;
	lc->reboot_wday = 6;
	lc->reboot_enable = false;
	lc->lattice = 16;
	lc->width = 64;
	lc->height = 32;
	strlcpy(lc->user_content, "特易停", sizeof(lc->user_content));
}

int32_t dev_led_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	dev_cfg_t *dc;
	led_cfg_t *lc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	dc = (dev_cfg_t *)arg;
	lc = &dc->led;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, lc);
		}
	}

	INFO("Read new device led cfg done!");
	INFO("led.model: %d", lc->model);
	INFO("led.reboot_enable: %d", lc->reboot_enable);
	INFO("led.reboot_hour: %d", lc->reboot_hour);
	INFO("led.reboot_wday: %d", lc->reboot_wday);
	INFO("led.user_content: %s", lc->user_content);
	INFO("led.lattice: %d", lc->lattice);
	INFO("led.width: %d", lc->width);
	INFO("led.height: %d", lc->height);

	return 0;
}

static int32_t dev_led_get_model(json_object *obj, void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	lc->model = str2enum(led_model_tab, numberof(led_model_tab),
						 json_object_get_string(obj));
	if (lc->model >= LED_MODEL_END) {
		lc->model = LED_MODEL_DEGAO;
		ERROR("Got an error led model, set to degao as default");
	}

	return 0;
}

static int32_t dev_led_get_reboot_enable(json_object *obj, void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (json_type_boolean != json_object_get_type(obj)) {
		return -1;
	}

	lc->reboot_enable = json_object_get_boolean(obj);

	return 0;
}

static int32_t dev_led_get_reboot_hour(json_object *obj, void *arg)
{
	led_cfg_t *lc;
	int32_t tmp;

	lc = (led_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	tmp = json_object_get_int(obj);
	if ((tmp >= 0) && (tmp <= 23)) {
		lc->reboot_hour = tmp;
	} else {
		lc->reboot_hour = 2;
		ERROR("Get an error led reboot hour, set to 2 as default");
	}

	return 0;
}

static int32_t dev_led_get_reboot_wday(json_object *obj, void *arg)
{
	led_cfg_t *lc;
	int32_t tmp;

	lc = (led_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	tmp = json_object_get_int(obj);
	if ((tmp >= 0) && (tmp <= 6)) {
		lc->reboot_wday = tmp;
	} else {
		lc->reboot_wday = 5;
		ERROR("Get an error led reboot wday, set to 6 as default");
	}

	return 0;
}

static int32_t dev_led_get_lattice(json_object *obj, void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	lc->lattice = json_object_get_int(obj);

	return 0;
}

static int32_t dev_led_get_width(json_object *obj, void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	lc->width = json_object_get_int(obj);

	return 0;
}

static int32_t dev_led_get_height(json_object *obj, void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	lc->height = json_object_get_int(obj);

	return 0;
}

static int32_t dev_led_get_user_content(json_object *obj, void *arg)
{
	led_cfg_t *lc;
	const char *s;

	lc = (led_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	s = json_object_get_string(obj);
	if (NULL == s) {
		s = "特易停";
		ERROR("Get an error first line content, set to %s as default", s);
	}

	snprintf(lc->user_content, sizeof(lc->user_content), "%s", s);

	return 0;
}

json_object *dev_led_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	dev_cfg_t *dc;
	led_cfg_t *lc;
	uint32_t i;

	ASSERT(arg);

	dc = (dev_cfg_t *)arg;
	lc = &dc->led;

	root = json_object_new_object();
	if (NULL == root) {
		CRIT("Alloc memory for json object failed!");
		return NULL;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	for (i = 0; i < numberof(cfg_entry); ++i) {
		item = json_cfg_find(&json_cfg, cfg_entry[i].field);
		if (item) {
			ASSERT(item->set);
			if ((obj = item->set(lc))) {
				json_object_object_add(root, item->field, obj);
			}

			if (item->set_cb) {
				if(0 != item->set_cb(lc)){
					ERROR("Do led set callback failed, %s", item->field);
				}
			}
		}
	}

	return root;
}

static json_object *dev_led_set_reboot_hour(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (lc->reboot_hour > 23) {
		lc->reboot_hour = 2;
		ERROR("Set an error led reboot hour, set to 2 as default");
	}

	return json_object_new_int(lc->reboot_hour);
}

static json_object *dev_led_set_reboot_wday(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	if (lc->reboot_wday > 6) {
		lc->reboot_wday = 6;
		ERROR("Set an error led reboot wday, set to 6 as default");
	}

	return json_object_new_int(lc->reboot_wday);
}

static json_object *dev_led_set_reboot_enable(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	return json_object_new_boolean(lc->reboot_enable);
}

static json_object *dev_led_set_model(const void *arg)
{
	led_cfg_t *lc;
	const char *s;

	lc = (led_cfg_t *)arg;

	s = enum2str(led_model_tab, numberof(led_model_tab), lc->model);
	if (NULL == s) {
		s = "degao";
		ERROR("Set an error led model, set to degao as default");
	}

	return json_object_new_string(s);
}

static json_object *dev_led_set_user_content(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	return json_object_new_string(lc->user_content);
}

static int32_t dev_led_set_mode_handle(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	led_set_model(lc->model);
	return 0;
}

static json_object *dev_led_set_height(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	return json_object_new_int(lc->height);
}

static json_object *dev_led_set_width(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	return json_object_new_int(lc->width);
}

static json_object *dev_led_set_lattice(const void *arg)
{
	led_cfg_t *lc;

	lc = (led_cfg_t *)arg;

	return json_object_new_int(lc->lattice);
}

C_CODE_END
