#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hide_param.h"
#include "profile.h"
#include "commonfuncs.h"
#include "logger/log.h"


typedef struct hide_param{
	pthread_mutex_t mutex;
	habitcom_t data;
}hide_param_t;


static hide_param_t hide_param;

static int32_t read_rg_timeout(void)
{
	int32_t val_len = -1;
	char val[128] = {0};
	profile_t profile;
	static int32_t timeout = 10;

	if (0 == profile_open(&profile, "/config/hide_param", "r")) {
		val_len = profile_rd_val(&profile, "rg_timeout", val);
		profile_close(&profile);
	}
	if (val_len > 0) {
		timeout = atoi(val);
		if (timeout <= 0) {
			timeout = 10;
		}
	}

	INFO("roadgate except timeout: %ds.", timeout);

	return timeout;
}

static int32_t read_run_mode(void)
{
	int32_t val_len = -1;
	char val[128] = {0};
	profile_t profile;
	int32_t run_mode = 2;

	if (0 == profile_open(&profile, "/config/hide_param", "r")) {
		val_len = profile_rd_val(&profile, "vm_mode", val);
		profile_close(&profile);
	}

	if (val_len > 0) {
		run_mode = atoi(val);
		if (run_mode <= 0) {
			run_mode = 2;
		}
	}

	INFO("run mode: %d.", run_mode);

	return run_mode;
}

void habitcom_init(void)
{
	pthread_mutex_init(&hide_param.mutex, NULL);
	habitcom_read_new();
}

void habitcom_read_new(void)
{
	pthread_mutex_lock(&hide_param.mutex);

	hide_param.data.rg_timeout = read_rg_timeout();
	hide_param.data.run_mode = read_run_mode();

	pthread_mutex_unlock(&hide_param.mutex);
}


void habitcom_get(habitcom_t *param)
{
	pthread_mutex_lock(&hide_param.mutex);

	memcpy(param, &hide_param.data, sizeof(habitcom_t));

	pthread_mutex_unlock(&hide_param.mutex);
}
