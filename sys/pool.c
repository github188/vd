#include <pthread.h>
#include "assert.h"
#include "types.h"
#include "list.h"
#include "pool.h"

C_CODE_BEGIN


/**
 *
 *
 * @author cyj (2016/3/2)
 *
 * @param po
 * @param buf
 * @param nr
 * @param sz
 * @param offset
 */
void create_pool(pool_t * po, void *buf, int32_t nr, size_t sz, off_t offset)
{
	uint8_t *pi;
	list_head_t *link;

	po->nr_free = nr;
	po->nr_total = nr;
	po->nr_used = 0;
	INIT_LIST_HEAD(&po->free);

	pi = (uint8_t *)buf;
	while (nr--) {
		link = (list_head_t *)(pi + offset);
		list_add_tail(link, &po->free);
		pi += sz;
	}
}

/**
 *
 *
 * @author cyj (2016/3/2)
 *
 * @param po
 * @param offset
 *
 * @return void*
 */
void *get_from_pool(pool_t *po, int32_t offset)
{
	list_head_t *link;

	if (list_empty(&po->free)) {
		return NULL;
	}

	link = po->free.next;
	list_del(link);
	--po->nr_free;
	++po->nr_used;

	return ((uint8_t *)link - offset);
}

/**
 *
 *
 * @author cyj (2016/3/2)
 *
 * @param po
 * @param ent
 * @param offset
 */
void put_into_pool(pool_t *po, void *ent, int32_t offset)
{
	list_head_t *link;

	link = (list_head_t *)((uint8_t *)ent + offset);
	list_add_tail(link, &po->free);
	--po->nr_used;
	++po->nr_free;
}



C_CODE_END
