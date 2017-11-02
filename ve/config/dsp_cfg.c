#include "ve_cfg.h"
#include "dsp_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static int32_t dsp_get_white_list_match(json_object *obj, void *arg);
static json_object *dsp_set_white_list_match(const void *arg);
static int32_t dsp_get_non_motor_mode(json_object *obj, void *arg);
static json_object *dsp_set_non_motor_mode(const void *arg);
static int32_t dsp_get_capture_mode(json_object *obj, void *arg);
static json_object *dsp_set_capture_mode(const void *arg);

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "white_list_match",
		.get = dsp_get_white_list_match,
		.set = dsp_set_white_list_match
	},

	/*
	 * Non-motor vehicle reocrd mode
	 */
	{
		.field = "non_motor_mode",
		.get = dsp_get_non_motor_mode,
		.set = dsp_set_non_motor_mode
	},

	/*
	 * Capture mode when coil mode is enable
	 */
	{
		.field = "coil_capture_mode",
		.get = dsp_get_capture_mode,
		.set = dsp_set_capture_mode
	}
};

static const enum_str_t non_motor_mode_tab[] = {
	{ DSP_NON_MOTOR_AS_DFT, "default" },
	{ DSP_NON_MOTOR_AS_UNLICENSED, "unlicensed" },
	{ DSP_NON_MOTOR_AS_NON_MOTOR, "non-motor" }
};


static const enum_str_t capture_mode_tab[] = {
	{ DSP_CAPTURE_DFT, "default" },
	{ DSP_CAPTURE_REALTIME, "realtime" }
};

void dsp_cfg_init(dsp_cfg_t *dc)
{
	dc->white_list_match = 1;
	dc->capture_mode = DSP_CAPTURE_DFT;
	dc->non_motor_mode = DSP_NON_MOTOR_AS_DFT;
}

int32_t dsp_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	ve_cfg_t *vc;
	dsp_cfg_t *dc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	dc = &vc->dsp;

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

	INFO("Read new dsp cfg done!");
	INFO("dsp.white_list_match: %d", dc->white_list_match);
	INFO("dsp.non_motor_mode: %d", dc->non_motor_mode);
	INFO("dsp.capture_mode: %d", dc->capture_mode);

	return 0;
}

json_object *dsp_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	ve_cfg_t *vc;
	dsp_cfg_t *dc;
	uint32_t i;

	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	dc = &vc->dsp;

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

static int32_t dsp_get_white_list_match(json_object *obj, void *arg)
{
	dsp_cfg_t *dc;

	dc = (dsp_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	dc->white_list_match = json_object_get_int(obj);

	return 0;
}

static json_object *dsp_set_white_list_match(const void *arg)
{
	dsp_cfg_t *dc;

	ASSERT(arg);
	dc = (dsp_cfg_t *)arg;

	return json_object_new_int(dc->white_list_match);
}

static int32_t dsp_get_non_motor_mode(json_object *obj, void *arg)
{
	dsp_cfg_t *dc = (dsp_cfg_t *)arg;

	if (json_object_get_type(obj) != json_type_string) {
		return -1;
	}

	dc->non_motor_mode = str2enum(non_motor_mode_tab,
								  numberof(non_motor_mode_tab),
								  json_object_get_string(obj));
	if (dc->non_motor_mode >= DSP_NON_MOTOR_MODE_END) {
		const enum_str_t *dft = &non_motor_mode_tab[0];
		dc->non_motor_mode = dft->e;
		ERROR("Got an error non-motor mode, set to %s as default", dft->s);
	}

	return 0;
}

static json_object *dsp_set_non_motor_mode(const void *arg)
{
	ASSERT(arg);

	dsp_cfg_t *dc = (dsp_cfg_t *)arg;

	const char *s = enum2str(non_motor_mode_tab, numberof(non_motor_mode_tab),
							 dc->non_motor_mode);
	if (s == NULL) {
		s = non_motor_mode_tab[0].s;
		ERROR("Set an error dsp capture mode, set to %s as default", s);
	}

	return json_object_new_string(s);
}

static int32_t dsp_get_capture_mode(json_object *obj, void *arg)
{
	dsp_cfg_t *dc = (dsp_cfg_t *)arg;

	if (json_object_get_type(obj) != json_type_string) {
		return -1;
	}

	dc->capture_mode = str2enum(capture_mode_tab,
								numberof(capture_mode_tab),
								json_object_get_string(obj));
	if (dc->capture_mode >= DSP_CAPTURE_END) {
		const enum_str_t *dft = &capture_mode_tab[0];
		dc->capture_mode = dft->e;
		ERROR("Got an error non-motor mode, set to %s as default", dft->s);
	}

	return 0;
}

static json_object *dsp_set_capture_mode(const void *arg)
{
	ASSERT(arg);

	dsp_cfg_t *dc = (dsp_cfg_t *)arg;

	const char *s = enum2str(capture_mode_tab, numberof(capture_mode_tab),
							 dc->capture_mode);
	if (s == NULL) {
		s = capture_mode_tab[0].s;
		ERROR("Set an error dsp capture mode, set to %s as default", s);
	}

	return json_object_new_string(s);
}

C_CODE_END
