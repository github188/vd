#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "platform.h"
#include "types.h"
#include "list.h"
#include "sys/pool.h"
#include "logger/log.h"

C_CODE_BEGIN

/* The number of record pool */
#define PFM_REC_POOL_ENTRY_NUM	10

/* The platform number */
#define PFM_NUM_MAX	VE_PFM_ID_MAX
#define PFM_MASK_GRP_NUM		(PFM_NUM_MAX / 8 + 1)

typedef struct pfm_rec{
	list_head_t link;
	uint8_t mask[PFM_MASK_GRP_NUM];
	ve_pfm_rec_t info;
}pfm_rec_t;

typedef struct pfm_item{
	list_head_t link;
	int32_t id;
	pthread_t tid;
	int32_t (*init)(void);
	int32_t (*rec_recv)(const ve_pfm_rec_t *rec);
}pfm_item_t;

/**
 * struct pfm - The struct of platform
 *
 * @mask: The ready platform mask
 * @ready: The ready platform list
 * @suspend: The suspend platform list
 * @rec_po: The pool of records
 * @rec_buf: The buffer of records
 */
typedef struct pfm{
	pthread_mutex_t mutex;
	list_head_t ready;
	list_head_t suspend;
	pool_t rec_po;
	uint8_t mask[PFM_MASK_GRP_NUM];
	uint8_t readymask[PFM_MASK_GRP_NUM];
	pfm_rec_t rec_buf[PFM_REC_POOL_ENTRY_NUM];
#if VE_PFM_MEM_DBG
	list_head_t po_used;
	pthread_t po_stat_tid;
#endif
}pfm_t;

static pfm_t platform;

static __inline__ void *xmalloc(size_t size)
{
	return malloc(size);
}

static __inline__ void xfree(void *ptr)
{
	free(ptr);
}

static bool pfm_bit_is_set(uint8_t *mask, int32_t ngrp, int32_t id)
{
	int32_t grp, bit;

	grp = id >> 3;
	bit = id & 0x07;

	ASSERT(grp < ngrp);

	return getbit(mask[grp], bit) ? true : false;
}

static void pfm_set_bit(uint8_t *mask, int32_t ngrp, int32_t id)
{
	int32_t grp, bit;

	grp = id >> 3;
	bit = id & 0x07;

	ASSERT(grp < ngrp);

	setbit(mask[grp], bit);
}

static void pfm_clr_bit(uint8_t *mask, int32_t ngrp, int32_t id)
{
	int32_t grp, bit;

	grp = id >> 3;
	bit = id & 0x07;

	ASSERT(grp < ngrp);

	clrbit(mask[grp], bit);
}

#if VE_PFM_MEM_DBG
static void pfm_print_mask(const uint8_t *mask, int32_t ngrp)
{
	int32_t i;

	for (i = 0; i < ngrp; ++i) {
		DEBUG("mask[%d]: 0x%02X", i, mask[i]);
	}
}
#endif

static __inline__ void pfm_regst(pfm_t *pfm, pfm_item_t *item)
{
	/* If the id already exist, report the error */
	ASSERT(pfm_bit_is_set(pfm->mask, numberof(pfm->mask), item->id));
	ASSERT(item->rec_recv);
	ASSERT(item->init);

	list_add_tail(&item->link, &pfm->suspend);
	pfm_set_bit(pfm->mask, numberof(pfm->mask), item->id);
}

#if VE_PFM_MEM_DBG
static __inline__ void *pfm_po_stat_thread(void *arg)
{
	pfm_rec_t *rec;
	list_head_t *pos, *n;
	pfm_t *pfm = (pfm_t *)arg;

	ASSERT(pfm);

	for (;;) {
		pthread_mutex_lock(&pfm->mutex);
		list_for_each_safe(pos, n, &pfm->po_used){
			rec = list_entry(pos, pfm_rec_t, link);
			pfm_print_mask(rec->mask, numberof(rec->mask));
		}
		pthread_mutex_unlock(&pfm->mutex);

		sleep(60);
	}

	return NULL;
}
#endif

static __inline__ int32_t pfm_init(pfm_t *pfm)
{
	pthread_attr_t attr;
	list_head_t *pos, *n;
	int32_t ret;
	pfm_item_t *item;

	INIT_LIST_HEAD(&pfm->ready);
	INIT_LIST_HEAD(&pfm->suspend);

#if VE_PFM_MEM_DBG
	INIT_LIST_HEAD(&pfm->po_used);
#endif

	create_pool(&pfm->rec_po, pfm->rec_buf,
				numberof(pfm->rec_buf), sizeof(pfm_rec_t),
				offsetof(pfm_rec_t, link));

	if(0 != (ret = pthread_mutex_init(&pfm->mutex, NULL))){
		ERROR("Failed to initialize thread mutex");
		return ret;
	}

#if VE_PFM_MEM_DBG
	ret = pthread_create(&pfm->po_stat_tid, &attr,
						 pfm_po_stat_thread, pfm);
	if (0 != ret) {
		ERROR("Failed to create pthread");
		return ret;
	}
#endif

	list_for_each_safe(pos, n, &pfm->suspend) {
		item = list_entry(pos, pfm_item_t, link);
		ret = item->init();
		if (0 != ret) {
			ERROR("Failed to create pthread");
		}
	}

	return 0;
}

static pfm_item_t *pfm_find(list_head_t *head, int32_t id)
{
	list_head_t *pos, *n;
	pfm_item_t *item;

	list_for_each_safe(pos, n, head){
		item = list_entry(pos, pfm_item_t, link);
		if (item->id == id) {
			return item;
		}
	}

	return NULL;
}

static __inline__ int32_t pfm_start(pfm_t *pfm, int32_t id)
{
	pfm_item_t *item;
	int32_t ret = 0;

	pthread_mutex_lock(&pfm->mutex);

	do {
		if (NULL == (item = pfm_find(&pfm->suspend, id))) {
			ret = -1;
			break;
		}

		list_move_tail(&item->link, &pfm->ready);
		pfm_set_bit(pfm->readymask, numberof(pfm->readymask), id);
	}while (0);

	pthread_mutex_unlock(&pfm->mutex);

	return ret;
}

static __inline__ int32_t pfm_stop(pfm_t *pfm, int32_t id)
{
	pfm_item_t *item;
	int32_t ret = 0;

	pthread_mutex_lock(&pfm->mutex);

	do {
		if (NULL == (item = pfm_find(&pfm->ready, id))) {
			ret = -1;
			break;
		}

		list_move_tail(&item->link, &pfm->suspend);
		pfm_clr_bit(pfm->readymask, numberof(pfm->readymask), id);
	}while (0);

	pthread_mutex_unlock(&pfm->mutex);

	return ret;
}

static __inline__ int32_t pfm_record_recv(pfm_t *pfm, const ve_pfm_rec_t *rec)
{
	pfm_rec_t *_new;
	list_head_t *pos, *n;
	pfm_item_t *item;
	int32_t ret = 0;

	pthread_mutex_lock(&pfm->mutex);

	do {
		_new = get_from_pool(&pfm->rec_po, offsetof(pfm_rec_t, link));
		if (NULL == _new) {
			ERROR("Failed to get memory from ve platform record pool");
			ret = -1;
			break;
		}

		memcpy(&_new->info, rec, sizeof(ve_pfm_rec_t));
		memcpy(_new->mask, pfm->readymask, numberof(_new->mask));

#if VE_PFM_MEM_DBG
		list_add_tail(&_new->link, &pfm->po_used);
#endif

		list_for_each_safe(pos, n, &pfm->ready){
			item = list_entry(pos, pfm_item_t, link);
			if (pfm_bit_is_set(_new->mask, numberof(_new->mask), item->id)) {
				item->rec_recv(&_new->info);
			}
		}

	}while (0);

	pthread_mutex_unlock(&pfm->mutex);

	return ret;
}

static __inline__ int32_t pfm_record_del(pfm_t *pfm, int32_t id,
										 ve_pfm_rec_t *info)
{
	pfm_rec_t *rec;

	ASSERT(info);

	pthread_mutex_lock(&pfm->mutex);

	/* Get the information of record */
	rec = (pfm_rec_t *)((uint8_t *)info - offsetof(pfm_rec_t, info));

	/* Don't allow duplicate deletion */
	ASSERT(!pfm_bit_is_set(rec->mask, numberof(rec->mask), id));

	/* Delete it */
	pfm_clr_bit(rec->mask, numberof(rec->mask), id);

	/*
	 * Mask of zero means that can free the memory
	 */
	if (0 == rec->mask) {

#if VE_PFM_MEM_DBG
		list_del(&rec->link);
#endif
		put_into_pool(&pfm->rec_po, rec, offsetof(pfm_rec_t, link));
	}

	pthread_mutex_unlock(&pfm->mutex);

	return 0;
}

int32_t ve_pfm_regst(int32_t id,
					 int32_t (*init)(void),
					 int32_t (*rec_recv)(const ve_pfm_rec_t *rec))
{
	pfm_item_t *item;

	if(NULL == (item = xmalloc(sizeof(pfm_item_t)))){
		ERROR("Malloc failed!");
		return -1;
	}

	item->id = id;
	item->init = init;
	item->rec_recv = rec_recv;

	pfm_regst(&platform, item);

	return 0;
}

int32_t ve_pfm_init(void)
{
	return pfm_init(&platform);
}

int32_t ve_pfm_record_recv(const ve_pfm_rec_t *rec)
{
	return pfm_record_recv(&platform, rec);
}

int32_t ve_pfm_record_del(int32_t id, ve_pfm_rec_t *rec)
{
	return pfm_record_del(&platform, id, rec);
}

int32_t ve_pfm_start(int32_t id)
{
	return pfm_start(&platform, id);
}

int32_t ve_pfm_stop(int32_t id)
{
	return pfm_stop(&platform, id);
}

C_CODE_END
