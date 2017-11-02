#ifndef __CONFIG_H
#define __CONFIG_H

#include "types.h"
#include "list.h"
#include "json.h"

C_CODE_BEGIN

#define JSON_CFG_HASH_SIZE	8

typedef struct json_cfg_item {
	hlist_node_t link;
	const char *field;
	int32_t (*get)(json_object *obj, void *arg);
	json_object *(*set)(const void *arg);
	int32_t (*set_cb)(const void *arg);
} json_cfg_item_t;

typedef struct json_cfg {
	hlist_head_t hash[JSON_CFG_HASH_SIZE];
}json_cfg_t;

typedef struct enum_str {
	int32_t e;
	const char *s;
}enum_str_t;


void json_cfg_init(json_cfg_t *jc, json_cfg_item_t *tab, uint_fast16_t nr);
json_cfg_item_t *json_cfg_find(json_cfg_t *jc, const char *field);
const char *enum2str(const enum_str_t *tab, int32_t nr, int32_t e);
int32_t str2enum(const enum_str_t *tab, int32_t nr, const char *s);

C_CODE_END


#endif

