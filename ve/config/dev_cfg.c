#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "ve_cfg.h"
#include "dev_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "light",
		.get = dev_light_cfg_read_new,
		.set = dev_light_cfg_write_new
	},
	{
		.field = "led",
		.get = dev_led_cfg_read_new,
		.set = dev_led_cfg_write_new
	},
	{
		.field = "audio",
		.get = dev_audio_cfg_read_new,
		.set = dev_audio_cfg_write_new
	},
};

void dev_cfg_init(dev_cfg_t *dc)
{
	dev_light_cfg_init(&dc->light);
	dev_led_cfg_init(&dc->led);
}

int32_t dev_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	ve_cfg_t *vc;
	dev_cfg_t *dc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	dc = &vc->dev;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, dc);
		}
	}

	return 0;
}

json_object *dev_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	ve_cfg_t *vc;
	dev_cfg_t *dc;
	uint32_t i;

	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	dc = &vc->dev;

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
			if ((obj = item->set(dc))) {
				json_object_object_add(root, item->field, obj);
			}
		}
	}

	return root;
}

C_CODE_END

