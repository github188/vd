#include <string.h>
#include "types.h"
#include "json.h"
#include "json_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN


#define xfree(ptr)	do {free((ptr));}while(0);
#define xmalloc(size)	({malloc((size));})

static uint32_t cal_str_hash_val(const char *s)
{
	uint32_t value = 0;

	while (*s && s){
		value = value * 37 + (uint32_t)(*s++);
	}

	return value;
}

static intptr_t cal_parser_field_hash_pos(const char *s)
{
	return cal_str_hash_val(s) % JSON_CFG_HASH_SIZE;
}

static void json_cfg_parser_add_item(json_cfg_t *jc, json_cfg_item_t *item)
{
	intptr_t addr;

	addr = cal_parser_field_hash_pos(item->field);
	hlist_add_head(&item->link, &jc->hash[addr]);
}

void json_cfg_init(json_cfg_t *jc, json_cfg_item_t *tab, uint_fast16_t nr)
{
	uint_fast16_t i;

	for (i = 0; i < numberof(jc->hash); ++i) {
		INIT_HLIST_HEAD(jc->hash + i);
	}

	for (i = 0; i < nr; ++i) {
		json_cfg_parser_add_item(jc, tab + i);
	}
}

json_cfg_item_t *json_cfg_find(json_cfg_t *jc, const char *field)
{
	intptr_t addr;
	json_cfg_item_t *item;
	hlist_node_t *pos, *n;

	addr = cal_parser_field_hash_pos(field);

	hlist_for_each_safe(pos, n, &jc->hash[addr]) {
		item = hlist_entry(pos, json_cfg_item_t, link);

		if (strlen(item->field) == strlen(field)){
			if (0 == strcmp(item->field, field)) {
				return item;
			}
		}
	}

	return NULL;
}

const char *enum2str(const enum_str_t *tab, int32_t nr, int32_t e)
{
	const enum_str_t *p, *end;

	for (p = tab, end = p + nr; p < end; ++p) {
		if (p->e == e) {
			return p->s;
		}
	}

	return NULL;
}

int32_t str2enum(const enum_str_t *tab, int32_t nr, const char *s)
{
	const enum_str_t *p, *end;

	for (p = tab, end = p + nr; p < end; ++p) {
		if (strlen(p->s) == strlen(s)) {
			if (0 == strcmp(p->s, s)) {
				return p->e;
			}
		}
	}

	return -1;
}

C_CODE_END
