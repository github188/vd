#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "ve_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static int32_t global_get_prov(json_object *obj, void *arg);
static int32_t global_get_city(json_object *obj, void *arg);
static int32_t global_get_detect_mode(json_object *obj, void *arg);
static int32_t global_get_road_gate_mode(json_object *obj, void *arg);
static int32_t global_get_ve_mode(json_object *obj, void *arg);
static json_object *global_set_prov(const void *arg);
static json_object *global_set_city(const void *arg);
static json_object *global_set_ve_mode(const void *arg);
static json_object *global_set_detect_mode(const void *arg);
static json_object *global_set_rg_mode(const void *arg);

static const enum_str_t detect_mode_tab[] = {
	{ DETECT_MODE_VIDEO, "video" },
	{ DETECT_MODE_COIL, "coil" },
	{ DETECT_MODE_MIX, "mix" }
};

static const enum_str_t ve_mode_tab[] = {
	{ VE_MODE_IN, "in" },
	{ VE_MODE_OUT, "out" }
};

static const enum_str_t rg_mode_tab[] = {
	{ ROAD_GATE_CTRL_FRONT, "front" },
	{ ROAD_GATE_CTRL_FRONT_RECORD, "front_record" },
	{ ROAD_GATE_CTRL_BACKGROUND, "background" },
	{ ROAD_GATE_CTRL_MIX, "mix" },
};

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "province",
		.get = global_get_prov,
		.set = global_set_prov
	},
	{
		.field = "city",
		.get = global_get_city,
		.set = global_set_city
	},
	{
		.field = "detect_mode",
		.get = global_get_detect_mode,
		.set = global_set_detect_mode
	},
	{
		.field = "ve_mode",
		.get = global_get_ve_mode,
		.set = global_set_ve_mode
	},
	{
		.field = "road_gate_mode",
		.get = global_get_road_gate_mode,
		.set = global_set_rg_mode
	}
};

void global_cfg_init(global_cfg_t *gc)
{
	snprintf(gc->prov, sizeof(gc->prov), "É½¶«");
	snprintf(gc->city, sizeof(gc->city), "Çàµº");
	gc->detect_mode = DETECT_MODE_COIL;
	gc->ve_mode = VE_MODE_IN;
	gc->rg_mode = ROAD_GATE_CTRL_FRONT;
}

int32_t global_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	ve_cfg_t *vc;
	global_cfg_t *gc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	gc = &vc->global;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, gc);
		}
	}

	INFO("Read new global cfg done!");
	INFO("global.prov: %s", gc->prov);
	INFO("global.city: %s", gc->city);
	INFO("global.detect_mode: %d", gc->detect_mode);
	INFO("global.ve_mode: %d", gc->ve_mode);
	INFO("global.rg_mode: %d", gc->rg_mode);

	return 0;
}


static int32_t global_get_prov(json_object *obj, void *arg)
{
	global_cfg_t *gc;

	gc = (global_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	snprintf(gc->prov, sizeof(gc->prov), "%s", json_object_get_string(obj));

	return 0;
}

static int32_t global_get_city(json_object *obj, void *arg)
{
	global_cfg_t *gc;

	gc = (global_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	snprintf(gc->city, sizeof(gc->city), "%s", json_object_get_string(obj));

	return 0;
}

static int32_t global_get_detect_mode(json_object *obj, void *arg)
{
	global_cfg_t *gc;

	gc = (global_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	gc->detect_mode = str2enum(detect_mode_tab, numberof(detect_mode_tab),
							   json_object_get_string(obj));
	if (gc->detect_mode >= DETECT_MODE_END) {
		gc->detect_mode = DETECT_MODE_COIL;
		ERROR("Got an error detect mode, set to coil as default");
	}

	return 0;
}

static int32_t global_get_ve_mode(json_object *obj, void *arg)
{
	global_cfg_t *gc;

	gc = (global_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	gc->ve_mode = str2enum(ve_mode_tab, numberof(ve_mode_tab),
						   json_object_get_string(obj));
	if (gc->ve_mode >= VE_MODE_END) {
		gc->ve_mode = VE_MODE_IN;
		ERROR("Got an error ve mode, set to in as default");
	}

	return 0;
}

static int32_t global_get_road_gate_mode(json_object *obj, void *arg)
{
	global_cfg_t *gc;

	gc = (global_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	gc->rg_mode = str2enum(rg_mode_tab, numberof(rg_mode_tab),
						   json_object_get_string(obj));
	if (gc->rg_mode >= ROAD_GATE_CTRL_END) {
		gc->rg_mode = ROAD_GATE_CTRL_FRONT;
		ERROR("Got an error road gate ctrl mode, set to front as default");
	}

	return 0;
}


json_object *global_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	ve_cfg_t *vc;
	global_cfg_t *gc;
	uint32_t i;

	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	gc = &vc->global;

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
			if ((obj = item->set(gc))) {
				json_object_object_add(root, item->field, obj);
			}
		}
	}

	return root;
}

static json_object *global_set_prov(const void *arg)
{
	global_cfg_t *gc;

	ASSERT(arg);
	gc = (global_cfg_t *)arg;

	return json_object_new_string(gc->prov);
}

static json_object *global_set_city(const void *arg)
{
	global_cfg_t *gc;

	ASSERT(arg);
	gc = (global_cfg_t *)arg;

	return json_object_new_string(gc->city);
}

static json_object *global_set_detect_mode(const void *arg)
{
	ASSERT(arg);

	global_cfg_t *gc = (global_cfg_t *)arg;

	const char *s = enum2str(detect_mode_tab, numberof(detect_mode_tab),
							 gc->detect_mode);
	if (s == NULL) {
		const enum_str_t *dft = &detect_mode_tab[0];
		s = dft->s;
		ERROR("Got an error detect mode, set to %s as default", dft->s);
	}

	return json_object_new_string(s);
}

static json_object *global_set_ve_mode(const void *arg)
{
	global_cfg_t *gc;
	const char *s;

	ASSERT(arg);
	gc = (global_cfg_t *)arg;

	s = enum2str(ve_mode_tab, numberof(ve_mode_tab), gc->ve_mode);
	if (NULL == s) {
		s = "in";
		ERROR("Set an error ve mode, set to in as default");
	}

	return json_object_new_string(s);
}

static json_object *global_set_rg_mode(const void *arg)
{
	global_cfg_t *gc;
	const char *s;

	ASSERT(arg);
	gc = (global_cfg_t *)arg;

	s = enum2str(rg_mode_tab, numberof(rg_mode_tab), gc->rg_mode);
	if (NULL == s) {
		s = "front";
		ERROR("Set an error road gate ctrl mode, set to front as default");
	}

	return json_object_new_string(s);
}

C_CODE_END
