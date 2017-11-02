#ifndef __ROADGATE_DATA_H
#define __ROADGATE_DATA_H

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include "types.h"
#include "list.h"

C_CODE_BEGIN

#define COIL_DETECT_NONE	1
#define COIL_DETECT_VEHICLE		0

enum {
	RG_CTRLER_LO = 0,
	RG_CTRLER_CENTER,
	RG_CTRLER_END
};

enum {
	RG_CTRL_AUTO = 0,
	RG_CTRL_MANUAL,
	RG_CTRL_OTHER,
	RG_CTRL_END
};

enum {
	RG_ACTION_DN = 0,
	RG_ACTION_UP,
	RG_ACTION_STOP,
	RG_ACTION_NC_SET,
	RG_ACTION_NC_CLR,
	RG_ACTION_NO_SET,
	RG_ACTION_END
};

/**
 * enum Road gate's status for status machine
 */
enum {
	STATUS_IDLE = 0,		/* Idle */
	STATUS_CENTER_OPEN,		/* Open by center */
	STATUS_CENTER_CLOSE,		/* Close by central */
	STATUS_CENTER_STOP,		/* Stop by central */
	STATUS_CENTER_NC,		/* Normal by central */
	STATUS_CENTER_NC_LO_WAIT,
	STATUS_CENTER_NC_CENTER_OPEN,
	STATUS_CENTER_NC_CENTER_CLOSE,
	STATUS_CENTER_NC_CENTER_STOP,
	STATUS_CENTER_NO,
	STATUS_LO_OPEN,
	STATUS_END
};

typedef struct ctrl_handle {
	void (*handle)(void);
	int_fast32_t status;
} ctrl_handle_t;

/**
 * struct rg_ctrl - road gate control
 *
 * @ctrler Controller
 * @type Auto, manual, or others
 * @action Center action
 * @timeout Timeout value, unit: ms
 *
 */
typedef struct rg_ctrl {
	list_head_t link;
	uint8_t ctrler;
	uint8_t type;
	uint8_t action;
	uint32_t timeout;
} rg_ctrl_t;

/**
 * struct rg Road gate data struct
 */
typedef struct rg {
	uint8_t status;
	uint8_t prev_status;
	rg_ctrl_t ctrl;
	list_head_t ctrl_ls;
	pthread_mutex_t mutex;
	pthread_t tid;
	pthread_once_t once;
	struct timeval last_time;
}rg_t;

/**
 * Road gate status machine function pointer
 */
typedef int_fast32_t (*rg_sm_fp)(rg_t *rg);

const char *rg_get_status_text(int_fast32_t type);
const char *rg_get_ctrl_type_text(int_fast32_t type);
const char *rg_get_ctrl_ctrler_text(int_fast32_t ctrler);
const char *rg_get_ctrl_action_text(int_fast32_t action);
bool rg_ctrl_is_ok(const rg_ctrl_t *ctrl);
rg_ctrl_t *rg_get_new_ctrl(rg_t *rg);
rg_ctrl_t *rg_new_ctrl_alloc(void);
void rg_update_curr_ctrl(rg_t *rg, const rg_ctrl_t *ctrl);
void rg_del_new_ctrl(rg_t *rg, rg_ctrl_t *ctrl);
int_fast32_t rg_get_prev_status(rg_t *rg);
int_fast32_t rg_get_curr_status(rg_t *rg);
void rg_refresh_last_time(rg_t *rg);
void rg_refresh_status(rg_t *rg, int_fast32_t next_status);

C_CODE_END

#endif
