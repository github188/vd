#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "ve_cfg.h"
#include "global_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static int32_t dev_light_get_white_bright(json_object *obj, void *arg);
static json_object *dev_light_set_white_bright(const void *arg);
static int32_t dev_light_get_white_mode(json_object *obj, void *arg);
static json_object *dev_light_set_white_mode(const void *arg);


static json_cfg_item_t cfg_entry[] = {
	{
		.field = "white_bright",
		.get = dev_light_get_white_bright,
		.set = dev_light_set_white_bright
	},
	{
		.field = "white_mode",
		.get = dev_light_get_white_mode,
		.set = dev_light_set_white_mode
	}
};

static const enum_str_t white_mode_tab[] = {
	{ WLIGHT_MODE_NIGHT_ON, "night_on" },
	{ WLIGHT_MODE_COIL_TOGGLE, "coil_toggle" },
	{ WLIGHT_MODE_ALWAYS_ON, "always_on" }
};

void dev_light_cfg_init(light_cfg_t *lc)
{
	lc->white_bright = 50;
	lc->white_mode = WLIGHT_MODE_NIGHT_ON;
}

int32_t dev_light_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	dev_cfg_t *dc;
	light_cfg_t *lc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	dc = (dev_cfg_t *)arg;
	lc = &dc->light;

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

	INFO("Read new device light cfg done!");
	INFO("light.white_bright: %d", lc->white_bright);
	INFO("light.white_mode: %d", lc->white_mode);

	return 0;
}

static int32_t dev_light_get_white_bright(json_object *obj, void *arg)
{
	light_cfg_t *lc;
	int32_t tmp;

	lc = (light_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	tmp = json_object_get_int(obj);
	if ((tmp >= 0) && (tmp <= 100)) {
		lc->white_bright = tmp;
	} else {
		lc->white_bright = 100;
		ERROR("Get an error white light brightness, set to 100 as default");
	}

	return 0;
}

static int32_t dev_light_get_white_mode(json_object *obj, void *arg)
{
	light_cfg_t *lc;

	lc = (light_cfg_t *)arg;

	if (json_object_get_type(obj) != json_type_string) {
		return -1;
	}

	lc->white_mode = str2enum(white_mode_tab, numberof(white_mode_tab),
							  json_object_get_string(obj));
	if (lc->white_mode >= WLIGHT_MODE_END) {
		const enum_str_t *dft = &white_mode_tab[0];
		lc->white_mode = dft->e;
		ERROR("Get an error white light mode, set to %d as default", dft->e);
	}

	return 0;
}

json_object *dev_light_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	dev_cfg_t *dc;
	light_cfg_t *lc;
	uint32_t i;

	ASSERT(arg);

	dc = (dev_cfg_t *)arg;
	lc = &dc->light;

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
		}
	}

	return root;
}

static json_object *dev_light_set_white_bright(const void *arg)
{
	light_cfg_t *lc;

	lc = (light_cfg_t *)arg;

	if(lc->white_bright > 100) {
		lc->white_bright = 100;
		ERROR("Set an error white light brightness, set to 100 as default");
	}

	return json_object_new_int(lc->white_bright);
}

static json_object *dev_light_set_white_mode(const void *arg)
{
	light_cfg_t *lc;

	lc = (light_cfg_t *)arg;

	const char *s = enum2str(white_mode_tab, numberof(white_mode_tab),
							 lc->white_mode);
	if (s == NULL) {
		const enum_str_t *dft = &white_mode_tab[0];
		s = dft->s;
		ERROR("Set an error white light mode, set to %s as default", dft->s);
	}

	return json_object_new_string(s);
}

C_CODE_END

