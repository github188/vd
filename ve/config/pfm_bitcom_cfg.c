#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "ve_cfg.h"
#include "json_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

static int32_t pfm_bitcom_get_enable(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_id(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_serv_ip(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_ftp_enable(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_mq_enable(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_up_port(json_object *obj, void *arg);
static int32_t pfm_bitcom_get_down_port(json_object *obj, void *arg);
static json_object *pfm_bitcom_set_id(const void *arg);
static json_object *pfm_bitcom_set_enable(const void *arg);
static json_object *pfm_bitcom_set_ftp_enable(const void *arg);
static json_object *pfm_bitcom_set_mq_enable(const void *arg);
static json_object *pfm_bitcom_set_down_port(const void *arg);
static json_object *pfm_bitcom_set_up_port(const void *arg);
static json_object *pfm_bitcom_set_serv_ip(const void *arg);

static json_cfg_item_t cfg_entry[] = {
	{
		.field = "id",
		.get = pfm_bitcom_get_id,
		.set = pfm_bitcom_set_id
	},
	{
		.field = "enable",
		.get = pfm_bitcom_get_enable,
		.set = pfm_bitcom_set_enable
	},
	{
		.field = "ftp_enable",
		.get = pfm_bitcom_get_ftp_enable,
		.set = pfm_bitcom_set_ftp_enable
	},
	{
		.field = "mq_enable",
		.get = pfm_bitcom_get_mq_enable,
		.set = pfm_bitcom_set_mq_enable
	},
	{
		.field = "serv_ip",
		.get = pfm_bitcom_get_serv_ip,
		.set = pfm_bitcom_set_serv_ip
	},
	{
		.field = "up_port",
		.get = pfm_bitcom_get_up_port,
		.set = pfm_bitcom_set_up_port
	},
	{
		.field = "down_port",
		.get = pfm_bitcom_get_down_port,
		.set = pfm_bitcom_set_down_port
	},

};

void pfm_bitcom_cfg_init(pfm_bitcom_cfg_t *bc)
{
	bc->enable = false;
	bc->ftp_enable = false;
	bc->mq_enable = false;
	bc->http_long = false;
	inet_pton(AF_INET, "192.168.9.70", &bc->serv_ip);
	snprintf(bc->id, sizeof(bc->id), "3700009099");
	bc->up_port = 8088;
	bc->down_port = 2015;
}

int32_t pfm_bitcom_cfg_read_new(json_object *obj, void *arg)
{
	json_cfg_t json_cfg;
	pfm_cfg_t *pc;
	pfm_bitcom_cfg_t *bc;
	json_cfg_item_t *item;

	ASSERT(obj);
	ASSERT(arg);

	pc = (pfm_cfg_t *)arg;
	bc = &pc->bitcom;

	if (json_type_object != json_object_get_type(obj)) {
		return -1;
	}

	json_cfg_init(&json_cfg, cfg_entry, numberof(cfg_entry));

	json_object_object_foreach(obj, key, val) {
		item = json_cfg_find(&json_cfg, key);
		if (item) {
			ASSERT(item->get);
			item->get(val, bc);
		}
	}

	INFO("Read new bitcom platform cfg done!");
	INFO("bitcom.id: %s", bc->id);
	INFO("bitcom.enable: %d", bc->enable);
	INFO("bitcom.ftp_enable: %d", bc->ftp_enable);
	INFO("bitcom.serv_ip: %s", inet_ntoa(bc->serv_ip));
	INFO("bitcom.up_port: %d", bc->up_port);
	INFO("bitcom.down_port: %d", bc->down_port);

	return 0;
}

static int32_t pfm_bitcom_get_enable(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_boolean != json_object_get_type(obj)) {
		return -1;
	}

	bc->enable = json_object_get_boolean(obj);

	return 0;
}

static int32_t pfm_bitcom_get_ftp_enable(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_boolean != json_object_get_type(obj)) {
		return -1;
	}

	bc->ftp_enable = json_object_get_boolean(obj);

	return 0;
}

static int32_t pfm_bitcom_get_mq_enable(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_object_get_type(obj) != json_type_boolean) {
		return -1;
	}

	bc->mq_enable = json_object_get_boolean(obj);

	return 0;
}

static int32_t pfm_bitcom_get_up_port(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;
	int32_t tmp;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	tmp = json_object_get_int(obj);
	if ((tmp > 0) && (tmp <= 65535)) {
		bc->up_port = tmp;
	} else {
		bc->up_port = 8088;
		ERROR("Get an error bitcom up port, set to 8088 as default");
	}

	return 0;
}

static int32_t pfm_bitcom_get_down_port(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;
	int32_t tmp;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_int != json_object_get_type(obj)) {
		return -1;
	}

	tmp = json_object_get_int(obj);
	if ((tmp > 0) && (tmp <= 65535)) {
		bc->down_port = tmp;
	} else {
		bc->down_port = 2015;
		ERROR("Get an error bitcom up port, set to 2015 as default");
	}

	return 0;
}

static int32_t pfm_bitcom_get_serv_ip(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	if(1 != inet_pton(AF_INET, json_object_get_string(obj), &bc->serv_ip)) {
		inet_pton(AF_INET, "192.168.9.70", &bc->serv_ip);
		ERROR("Get an error bitcom server ip, "
			  "set to \"192.168.9.70\" as default");
	}

	return 0;
}

static int32_t pfm_bitcom_get_id(json_object *obj, void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	if (json_type_string != json_object_get_type(obj)) {
		return -1;
	}

	snprintf(bc->id, sizeof(bc->id), "%s", json_object_get_string(obj));

	return 0;
}


json_object *pfm_bitcom_cfg_write_new(const void *arg)
{
	json_cfg_item_t *item;
	json_cfg_t json_cfg;
	json_object *root, *obj;
	pfm_cfg_t *pc;
	pfm_bitcom_cfg_t *bc;
	uint32_t i;

	ASSERT(arg);

	pc = (pfm_cfg_t *)arg;
	bc = &pc->bitcom;

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
			if ((obj = item->set(bc))) {
				json_object_object_add(root, item->field, obj);
			}
		}
	}

	return root;
}

static json_object *pfm_bitcom_set_id(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_string(bc->id);
}

static json_object *pfm_bitcom_set_enable(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_boolean(bc->enable);
}

static json_object *pfm_bitcom_set_serv_ip(const void *arg)
{
	pfm_bitcom_cfg_t *bc;
	char buf[128];

	bc = (pfm_bitcom_cfg_t *)arg;

	if (NULL == inet_ntop(AF_INET, &bc->serv_ip, buf, INET_ADDRSTRLEN)) {
		snprintf(buf, sizeof(buf), "192.168.9.70");
		ERROR("Set an error server ip, set to \"192.168.9.70\" as default");
	}
	return json_object_new_string(buf);
}

static json_object *pfm_bitcom_set_up_port(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_int(bc->up_port);
}

static json_object *pfm_bitcom_set_down_port(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_int(bc->down_port);
}

static json_object *pfm_bitcom_set_ftp_enable(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_boolean(bc->ftp_enable);
}

static json_object *pfm_bitcom_set_mq_enable(const void *arg)
{
	pfm_bitcom_cfg_t *bc;

	bc = (pfm_bitcom_cfg_t *)arg;

	return json_object_new_boolean(bc->mq_enable);
}

C_CODE_END


