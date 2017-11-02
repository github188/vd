#include "ve/dev/roadgate/roadgate_data.h"
#include "ve/dev/roadgate/roadgate_ports.h"
#include "ve/dev/roadgate/dsp_msg.h"
#include "sys/time_util.h"
#include "logger/log.h"
#include "types.h"

C_CODE_BEGIN

static void center_center_set_up_signal(void)
{
	rg_set_up_signal();
	rg_status_sendto_dsp(DSP_RG_UP);
}

static void rg_do_dummy(void)
{
	return;
}

/**
 * Coil && Front && Entrance idle status machine
 *
 * @author chenyj (9/7/2016)
 *
 * @param rg
 *
 * @return int_fast32_t
 */
static int_fast32_t center_idle(rg_t *rg)
{
	int_fast32_t status = rg_get_curr_status(rg); 
	rg_ctrl_t *ctrl = rg_get_new_ctrl(rg);

	if (ctrl == NULL) {
		return status;
	}

	bool changed = false;
	bool resume = true;

	if (ctrl->ctrler == RG_CTRLER_CENTER) {

		static const ctrl_handle_t tab[RG_CTRL_END][RG_ACTION_END] = {
			[RG_CTRL_MANUAL][RG_ACTION_UP] =
				{ rg_set_up_signal, STATUS_CENTER_OPEN },
			[RG_CTRL_MANUAL][RG_ACTION_DN] =
				{ rg_down_s, STATUS_CENTER_CLOSE },
			[RG_CTRL_MANUAL][RG_ACTION_STOP] =
				{ rg_stop, STATUS_CENTER_STOP },
			[RG_CTRL_MANUAL][RG_ACTION_NC_SET] =
				{ rg_clr_up_signal, STATUS_CENTER_NC },
			[RG_CTRL_MANUAL][RG_ACTION_NO_SET] =
				{ rg_set_up_signal, STATUS_CENTER_NO },

			[RG_CTRL_AUTO][RG_ACTION_UP] = 
				{ center_center_set_up_signal, STATUS_CENTER_OPEN },

			[RG_CTRL_OTHER][RG_ACTION_UP] = 
				{ center_center_set_up_signal, STATUS_CENTER_OPEN },
		};

		if ((ctrl->action < RG_ACTION_END) && (ctrl->type < RG_CTRL_END)) {
			if (tab[ctrl->type][ctrl->action].handle) {
				tab[ctrl->type][ctrl->action].handle();
				status = tab[ctrl->type][ctrl->action].status;
				changed = true;
			}
		}

		/* Some actions that not need resume */
		if ((ctrl->type == RG_CTRL_MANUAL)
			&& (ctrl->action == RG_ACTION_NC_CLR)
			&& ((rg_get_prev_status(rg) == STATUS_CENTER_NC)
				|| (rg_get_prev_status(rg) == STATUS_CENTER_NO))) {
			resume = false;
		}
	}

	if (changed) {
		rg_refresh_last_time(rg);
		rg_update_curr_ctrl(rg, ctrl);
	} else {
		if (resume) {
			status = rg_get_prev_status(rg);
		}
	}

	rg_del_new_ctrl(rg, ctrl);

	return status;
}

static int_fast32_t center_center_open(rg_t *rg)
{
	int_fast32_t status = rg_get_curr_status(rg);

	/*
	 * If there is not had new control, then check the timeout, if is
	 * overtimed clear up signal and change to idle status;
	 * otherwise do the new control
	 */
	if (timeval_is_overtime(&rg->last_time, rg->ctrl.timeout)) {
		rg_clr_up_signal();
		status = STATUS_IDLE;
	}

	return status;
}

static int_fast32_t center_center_close(rg_t *rg)
{
	return STATUS_IDLE;
}

static int_fast32_t center_center_stop(rg_t *rg)
{
	return STATUS_IDLE;
}

static int_fast32_t center_center_nc(rg_t *rg)
{
	int_fast32_t status = rg_get_curr_status(rg); 
	rg_ctrl_t *ctrl = rg_get_new_ctrl(rg);

	/*
	 * If there is not had new control, then check the timeout, if is
	 * overtimed clear up signal and change to idle status;
	 * otherwise do the new control
	 */
	if (ctrl == NULL) {
		return status;
	}

	bool changed = false;

	if (ctrl->ctrler == RG_CTRLER_CENTER) {

		static const ctrl_handle_t tab[RG_CTRL_END][RG_ACTION_END] = {
			[RG_CTRL_MANUAL][RG_ACTION_NC_CLR] =
				{ rg_do_dummy, STATUS_IDLE },
			[RG_CTRL_MANUAL][RG_ACTION_UP] =
				{ rg_set_up_signal, STATUS_CENTER_NC_CENTER_OPEN },
			[RG_CTRL_MANUAL][RG_ACTION_DN] =
				{ rg_down_s, STATUS_CENTER_NC_CENTER_CLOSE },
			[RG_CTRL_MANUAL][RG_ACTION_STOP] =
				{ rg_clr_up_signal, STATUS_CENTER_NC_CENTER_STOP },

			[RG_CTRL_AUTO][RG_ACTION_UP] = 
				{ center_center_set_up_signal, STATUS_CENTER_OPEN },

			[RG_CTRL_OTHER][RG_ACTION_UP] = 
				{ center_center_set_up_signal, STATUS_CENTER_OPEN },
		};

		if ((ctrl->action < RG_ACTION_END) && (ctrl->type < RG_CTRL_END)) {
			if (tab[ctrl->type][ctrl->action].handle) {
				tab[ctrl->type][ctrl->action].handle();
				status = tab[ctrl->type][ctrl->action].status;
				changed = true;
			}
		}
	}

	if (changed) {
		rg_refresh_last_time(rg);
		rg_update_curr_ctrl(rg, ctrl);
	} else {
		status = rg_get_prev_status(rg);
	}

	rg_del_new_ctrl(rg, ctrl);

	return status;
}

static int_fast32_t center_center_nc_center_open(rg_t *rg)
{
	int_fast32_t status = rg->status;

	if (rg_get_new_ctrl(rg) == NULL) {
		if (timeval_is_overtime(&rg->last_time, rg->ctrl.timeout)) {
			rg_clr_up_signal();
			status = STATUS_CENTER_NC;
		}
		return status;
	}

	status = STATUS_CENTER_NC;

	return status;
}

static int_fast32_t center_center_nc_center_close(rg_t *rg)
{
	return STATUS_CENTER_NC;
}

static int_fast32_t center_center_nc_center_stop(rg_t *rg)
{
	return STATUS_CENTER_NC;
}

static int_fast32_t center_center_no(rg_t *rg)
{
	int_fast32_t status = rg->status;
	rg_ctrl_t *ctrl = rg_get_new_ctrl(rg);

	/*
	 * If there is not had new control, then check the timeout, if is
	 * overtimed clear up signal and change to idle status;
	 * otherwise do the new control
	 */
	if (ctrl == NULL) {
		return status;
	}

	bool changed = false;

	if (ctrl->ctrler == RG_CTRLER_CENTER) {

		static const ctrl_handle_t tab[RG_CTRL_END][RG_ACTION_END] = {
			[RG_CTRL_MANUAL][RG_ACTION_DN] =
				{ rg_down_s, STATUS_IDLE }
		};

		if ((ctrl->action < RG_ACTION_END) && (ctrl->type < RG_CTRL_END)) {
			if (tab[ctrl->type][ctrl->action].handle) {
				tab[ctrl->type][ctrl->action].handle();
				status = tab[ctrl->type][ctrl->action].status;
				changed = true;
			}
		}
	}

	if (changed) {
		rg_update_curr_ctrl(rg, ctrl);
		rg_refresh_last_time(rg);
	}

	rg_del_new_ctrl(rg, ctrl);

	return status;
}


int_fast32_t coil_center_export(rg_t *rg)
{
	static const rg_sm_fp status_handle[STATUS_END] = {
		[STATUS_IDLE] = center_idle,
		[STATUS_CENTER_OPEN] = center_center_open,
		[STATUS_CENTER_CLOSE] = center_center_close,
		[STATUS_CENTER_STOP] = center_center_stop,
		[STATUS_CENTER_NC] = center_center_nc,
		[STATUS_CENTER_NC_CENTER_OPEN] = center_center_nc_center_open,
		[STATUS_CENTER_NC_CENTER_CLOSE] = center_center_nc_center_close,
		[STATUS_CENTER_NC_CENTER_STOP] = center_center_nc_center_stop,
		[STATUS_CENTER_NO] = center_center_no
	};

	if (rg->status >= STATUS_END) {
		CRIT("An error road gate status for @ %s", __func__);
		return -1;
	}

	int_fast32_t next_status = status_handle[rg_get_curr_status(rg)](rg); 
	rg_refresh_status(rg, next_status);

	return 0;
}

C_CODE_END
