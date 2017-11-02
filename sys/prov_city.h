#ifndef __PROV_CITY_H
#define __PROV_CITY_H

#include "types.h"
#include "list.h"

C_CODE_BEGIN

#define PROV_CITY_HASH_TAB_SIZE	128

typedef struct city_code{
	const char *prov_name;
	const char *city_name;
	const char *prov_code;
	const char *city_code;
	hlist_node_t link;
} city_code_t;

int_fast8_t prov_city_init(void);
city_code_t *prov_city_find(const char *prov, const char *city);
void prov_city_show(void);

C_CODE_END

#endif
