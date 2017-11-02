#ifndef __POOL_H
#define __POOL_H

#include "types.h"
#include "list.h"

C_CODE_BEGIN


/**
 * struct pool - 内存池
 *
 * @free: 空闲链表
 * @nr_free: 空闲元素个数
 * @nr_used: 使用的元素个数
 */
typedef struct pool {
	list_head_t free;
	int32_t nr_free;
	int32_t nr_used;
	int32_t nr_total;
}pool_t;


void create_pool(pool_t *po, void *buf, int32_t nr, size_t sz, off_t offset);
void *get_from_pool(pool_t *po, int32_t offset);
void put_into_pool(pool_t *po, void *ent, int32_t offset);

C_CODE_END

#endif

