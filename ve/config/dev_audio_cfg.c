#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include "ve_cfg.h"
#include "global_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"
#include "ve/dev/audio.h"

C_CODE_BEGIN

static int32_t dev_audio_get_model(json_object *obj, void *arg);
static int32_t dev_audio_get_volume(json_object *obj, void *arg);
static json_object *dev_audio_set_volume(const void *arg);
static json_object *dev_audio_set_model(const void *arg);
static int32_t dev_audio_set_model_handle(const void *arg);
static int32_t dev_audio_set_volume_handle(const void *arg);

static const enum_str_t audio_model_tab[] = {
	{ AUDIO_MODEL_OFF, "off" },
	{ AUDIO_MODEL_LO, "local" },
};

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "model",
		.get = dev_audio_get_model,
		.set = dev_audio_set_model,
		.set_cb = dev_audio_set_model_handle
	},
	{
		.field = "volume",
		.get = dev_audio_get_volume,
		.set = dev_audio_set_volume,
		.set_cb = dev_audio_set_volume_handle
	}
};

void dev_audio_cfg_init(audio_cfg_t *ac)
{
	ac->model = AUDIO_MODEL_OFF;
	ac->volume = 80;
}

int32_t dev_audio_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	dev_cfg_t *dc;
	audio_cfg_t *ac;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	dc = (dev_cfg_t *)arg;
	ac = &dc->audio;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, ac);
		}
	}

	INFO("Read new device audio cfg done!");
	INFO("audio.model: %d", ac->model);
	INFO("audio.volume: %d", ac->volume);

	return 0;
}

static int32_t dev_audio_get_model(json_object *obj, void *arg)
{
	audio_cfg_t *ac = (audio_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	ac->model = str2enum(audio_model_tab, numberof(audio_model_tab),
						 json_object_get_string(obj));
	if (ac->model >= AUDIO_MODEL_END) {
		ac->model = AUDIO_MODEL_OFF;
		ERROR("Got an error audio model, set to off as default");
	}

	return 0;
}

static int32_t dev_audio_get_volume(json_object *obj, void *arg)
{
	audio_cfg_t *ac = (audio_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	int_fast32_t tmp = json_object_get_int(obj);
	if ((tmp >= 0) && (tmp <= 100)) {
		ac->volume = tmp;
	} else {
		ac->volume = 100;
		ERROR("Get an error audio max volume, set to 100 as default");
	}

	return 0;
}

json_object *dev_audio_cfg_write_new(const void *arg)
{
	ASSERT(arg);

	dev_cfg_t *dc = (dev_cfg_t *)arg;
	audio_cfg_t *ac = &dc->audio;

	json_object *root = json_object_new_object();
	if (NULL == root) {
		CRIT("Alloc memory for json object failed!");
		return NULL;
	}

	json_cfg_t json_cfg;
	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	for (uint_fast32_t i = 0; i < numberof(cfg_entry); ++i) {
		json_cfg_item_t *item = json_cfg_find(&json_cfg, cfg_entry[i].field);
		if (item) {
			ASSERT(item->set);
			json_object *obj;
			if ((obj = item->set(ac))) {
				json_object_object_add(root, item->field, obj);
			}

			if (item->set_cb) {
				if(0 != item->set_cb(ac)){
					ERROR("Do audio set callback failed, %s", item->field);
				}
			}
		}
	}

	return root;
}


static json_object *dev_audio_set_volume(const void *arg)
{
	audio_cfg_t *ac = (audio_cfg_t *)arg;

	if (ac->volume > 100) {
		ac->volume = 100;
		ERROR("Set an error audio volume, set to 100 as default");
	}

	return json_object_new_int(ac->volume);
}

static json_object *dev_audio_set_model(const void *arg)
{
	ASSERT(arg);

	audio_cfg_t *ac = (audio_cfg_t *)arg;

	const char *s = enum2str(audio_model_tab, numberof(audio_model_tab),
							 ac->model);
	if (NULL == s) {
		s = "off";
		ERROR("Set an error audio model, set to off as default");
	}

	return json_object_new_string(s);
}

static int32_t dev_audio_set_volume_handle(const void *arg)
{
	audio_cfg_t *ac = (audio_cfg_t *)arg;

	audio_dev_set_volume(ac->volume);

	return 0;
}

static int32_t dev_audio_set_model_handle(const void *arg)
{
	audio_cfg_t *ac = (audio_cfg_t *)arg;

	audio_dev_change(ac->model);

	return 0;
}

C_CODE_END
