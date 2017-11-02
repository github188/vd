#ifndef __EVENT_H
#define __EVENT_H

#include "types.h"
#include "list.h"

C_CODE_BEGIN


#define EVENT_IDENT_LEN	16
#define EVENT_DESC_LEN		512
#define EVENT_FILE_MAX_SIZE		(1024 * 100)

typedef struct event_item {
	list_head_t link;
	struct timeval tv;
	char ident[EVENT_IDENT_LEN];
	char desc[EVENT_DESC_LEN];
} event_item_t;

event_item_t *event_item_alloc(void);
void event_item_free(event_item_t *item);
int_fast32_t event_put(event_item_t *item);
int_fast32_t event_init(void);

C_CODE_END

#endif
