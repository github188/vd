#include "ve_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static int32_t pfm_get_resend_rotate(json_object *obj, void *arg);
static json_object *pfm_set_resend_rotate(const void *arg);

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "resend_rotate",
		.get = pfm_get_resend_rotate,
		.set = pfm_set_resend_rotate
	},
	{
		.field = "bitcom",
		.get = pfm_bitcom_cfg_read_new,
		.set = pfm_bitcom_cfg_write_new
	},
};

void pfm_cfg_init(pfm_cfg_t *pc)
{
	pc->resend_rotate = 10;
	pfm_bitcom_cfg_init(&pc->bitcom);
}

int32_t pfm_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	ve_cfg_t *vc;
	pfm_cfg_t *pc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	pc = &vc->pfm;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, pc);
		}
	}

	INFO("Read new platform cfg done!");
	INFO("pfm.resend_rotate: %d", pc->resend_rotate);

	return 0;
}

static int32_t pfm_get_resend_rotate(json_object *obj, void *arg)
{
	pfm_cfg_t *pc;

	pc = (pfm_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	pc->resend_rotate = json_object_get_int(obj);

	return 0;
}

json_object *pfm_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	ve_cfg_t *vc;
	pfm_cfg_t *pc;
	uint32_t i;

	ASSERT(arg);

	vc = (ve_cfg_t *)arg;
	pc = &vc->pfm;

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
			if ((obj = item->set(pc))) {
				json_object_object_add(root, item->field, obj);
			}
		}
	}

	return root;
}

static json_object *pfm_set_resend_rotate(const void *arg)
{
	pfm_cfg_t *pc;

	pc = (pfm_cfg_t *)arg;

	return json_object_new_int(pc->resend_rotate);
}

C_CODE_END

