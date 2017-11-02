#include <string.h>
#include "types.h"
#include "list.h"
#include "roadgate_data.h"
#include "logger/log.h"
#include "sys/time_util.h"

C_CODE_BEGIN

static const char *ctrl_type_text[RG_CTRL_END] = {
	[RG_CTRL_AUTO] = "auto",
	[RG_CTRL_MANUAL] = "manual",
	[RG_CTRL_OTHER] = "other"
};

static const char *ctrl_ctrler_text[RG_CTRLER_END] = {
	[RG_CTRLER_LO] = "local",
	[RG_CTRLER_CENTER] = "center"
};

static const char *ctrl_action_text[RG_ACTION_END] = {
	[RG_ACTION_UP] = "up",
	[RG_ACTION_DN] = "down",
	[RG_ACTION_STOP] = "stop",
	[RG_ACTION_NC_SET] = "normal close set",
	[RG_ACTION_NC_CLR] = "normal close clear",
	[RG_ACTION_NO_SET] = "normal open set",
};

static const char *status_text[STATUS_END] = {
	[STATUS_IDLE] = "idle",
	[STATUS_CENTER_OPEN] = "center open",
	[STATUS_CENTER_CLOSE] = "center close",
	[STATUS_CENTER_STOP] = "center stop",
	[STATUS_CENTER_NC] = "center normal close",
	[STATUS_CENTER_NC_LO_WAIT] = "center normal close and wait cancel",
	[STATUS_CENTER_NC_CENTER_OPEN] = "center normal close and manual open",
	[STATUS_CENTER_NC_CENTER_CLOSE] = "center normal close and manual close",
	[STATUS_CENTER_NC_CENTER_STOP] = "center normal close and manual stop",
	[STATUS_CENTER_NO] = "center normal open",
	[STATUS_LO_OPEN] = "local open",
};

const char *rg_get_status_text(int_fast32_t type)
{
	if (type < numberof(status_text)) {
		return status_text[type];
	}

	return NULL;
}

/**
 *
 *
 * @author chenyj (9/8/2016)
 *
 * @param type
 *
 * @return const char*
 */
const char *rg_get_ctrl_type_text(int_fast32_t type)
{
	if (type < numberof(ctrl_type_text)) {
		return ctrl_type_text[type];
	}

	return NULL;
}

/**
 *
 *
 * @author chenyj (9/8/2016)
 *
 * @param ctrler
 *
 * @return const char*
 */
const char *rg_get_ctrl_ctrler_text(int_fast32_t ctrler)
{
	if (ctrler < numberof(ctrl_ctrler_text)) {
		return ctrl_ctrler_text[ctrler];
	}

	return NULL;
}

/**
 *
 *
 * @author chenyj (9/8/2016)
 *
 * @param action
 *
 * @return const char*
 */
const char *rg_get_ctrl_action_text(int_fast32_t action)
{
	if (action < numberof(ctrl_action_text)) {
		return ctrl_action_text[action];
	}

	return NULL;
}

/**
 * Check if the road gate control parameter is ok
 *
 * @author chenyj (9/7/2016)
 *
 * @param ctrl
 *
 * @return bool
 */
bool rg_ctrl_is_ok(const rg_ctrl_t *ctrl)
{
	if (ctrl->ctrler >= RG_CTRLER_END) {
		return false;
	}

	if (ctrl->ctrler == RG_CTRLER_LO) {
		return (((ctrl->action ^ RG_ACTION_UP) 
				 | (ctrl->type ^ RG_CTRL_AUTO)) == 0);
	}

	if (ctrl->ctrler == RG_CTRLER_CENTER) {
		return ((ctrl->action < RG_ACTION_END) && (ctrl->type < RG_CTRL_END));
	}

	return false;
}

/**
 * Get a new control parameter
 *
 * @author chenyj (9/7/2016)
 *
 * @param rg
 *
 * @return rg_ctrl_t*
 */
rg_ctrl_t *rg_get_new_ctrl(rg_t *rg)
{
	if (!list_empty(&rg->ctrl_ls)) {
		return list_entry(rg->ctrl_ls.next, rg_ctrl_t, link);
	}
	
	return NULL;
}

rg_ctrl_t *rg_new_ctrl_alloc(void)
{
	return (rg_ctrl_t *)malloc(sizeof(rg_ctrl_t));
}

void rg_del_new_ctrl(rg_t *rg, rg_ctrl_t *ctrl)
{
	list_del(&ctrl->link);
	free(ctrl);
}

void rg_update_curr_ctrl(rg_t *rg, const rg_ctrl_t *ctrl)
{
	memcpy(&rg->ctrl, ctrl, sizeof(rg->ctrl));
}

int_fast32_t rg_get_prev_status(rg_t *rg)
{
	return rg->prev_status;
}

int_fast32_t rg_get_curr_status(rg_t *rg)
{
	return rg->status;
}

static void show_status_change(rg_t *rg)
{
	INFO("Road gate status changed @ %s: '%s' -> '%s'",
		 ustime_msec(),
		 rg_get_status_text(rg->prev_status),
		 rg_get_status_text(rg->status));
}

void rg_refresh_last_time(rg_t *rg)
{
	/* Refresh last status changed time */
	gettimeofday(&rg->last_time, NULL);
}

void rg_refresh_status(rg_t *rg, int_fast32_t next_status)
{
	if (rg->status != next_status) {
		rg->prev_status = rg->status;
		rg->status = next_status;
		/* Print status */
		show_status_change(rg); 
	}
}

C_CODE_END
