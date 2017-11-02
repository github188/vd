#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "types.h"
#include "logger/log.h"
#include "cam_param.h"
#include "sys/sun_rise_set.h"
#include "sys/prov_lon_lat.h"
#include "sys/xtimer.h"
#include "sys/time_util.h"
#include "sys/prov_city.h"
#include "ve_cfg.h"
#include "storage/data_process.h"

C_CODE_BEGIN

typedef struct cam_param{
	int32_t daynight;
	time_t sunrise;
	time_t sunset;
	xtimer_t sun_rise_set_tmr;
	pthread_mutex_t mutex;
}cam_param_t;

static cam_param_t cam_param;


void daynight_set(int32_t daynight)
{
	INFO("Daynight changed: %d", daynight);
	pthread_mutex_lock(&cam_param.mutex);
	cam_param.daynight = daynight;
	pthread_mutex_unlock(&cam_param.mutex);
}

int32_t daynight_get(int32_t daynight)
{
	int32_t ret;
	pthread_mutex_lock(&cam_param.mutex);
	ret = cam_param.daynight;
	pthread_mutex_unlock(&cam_param.mutex);

	return ret;
}

bool isdaytime(void)
{
	bool daytime;
	time_t curr;
	cam_param_t *param;

	pthread_mutex_lock(&cam_param.mutex);

	param = &cam_param;
	daytime = true;

	if (NIGHTTIME == cam_param.daynight) {
		/*
		 * The sensor consider that now is night time
		 */
		daytime = false;
	} else {
		time(&curr);
		if (curr <= param->sunrise) {
			/*
			 * 00:00 - sunrise
			 */
			daytime = false;
		} else if(curr >= param->sunset) {
			/*
			 * sunset - 00:00
			 */
			daytime = false;
		}
	}

	pthread_mutex_unlock(&cam_param.mutex);

	return daytime;
}

static int32_t get_daynight_file(const char *file)
{
	FILE *f;
	char buf[8];
	int32_t ret = -1;

	if ((f = fopen(file, "r"))) {
		if (fgets(buf, sizeof(buf), f)) {
			ret = (0 == atoi(buf)) ? DAYTIME : NIGHTTIME;
		}

		fclose(f);
	}

	return ret;
}

static void sun_rise_set_refresh(cam_param_t *param)
{
	ve_cfg_t cfg;
	lon_lat_t lon, lat;
	city_code_t *cc;
	const char *prov;
	struct tm sunrise, sunset;

	/* Get the province */
	ve_cfg_get(&cfg);
	cc = prov_city_find(cfg.global.prov, cfg.global.city);
	prov = (NULL == cc) ? "³" : cc->prov_code;

	/* Get the longitude and latitude of the default province */
	if(0 != local_lon_lat(prov, &lon, &lat)){
		ERROR("Failed to get lon and lat");
		lat = PROV_SHANDONG_LAT;
		lon = PROV_SHANDONG_LON;
	}

	/* Get the sun rise and set time */
	sun_rise_set_time(&sunrise, &sunset, lon, lat);
	param->sunrise = mktime(&sunrise);
	param->sunset = mktime(&sunset);

	INFO("Current province is %s", prov);
	INFO("Current longitude is %.2f", lon);
	INFO("Current latitude is %.2f", lat);
	INFO("Sun rise at %s", asctime(&sunrise));
	INFO("Sun set at %s", asctime(&sunset));

}

static void cal_sun_rise_set_tmr_cb(union sigval sig)
{
	cam_param_t *param = (cam_param_t *)sig.sival_ptr;

	pthread_mutex_lock(&param->mutex);
	sun_rise_set_refresh(param);
	pthread_mutex_unlock(&param->mutex);
}

int32_t cam_param_ram_init(void)
{
	int32_t ret;

	ret = pthread_mutex_init(&cam_param.mutex, NULL);
	if (0 != ret) {
		ERROR("cam_param.mutex create failed!");
		return ret;
	}

	ret = get_daynight_file(DAYNIGHT_INFO_FILE);
	if (-1 != ret) {
		INFO("Read daynight value from file: %d", ret);
		daynight_set(ret);
	}

	return 0;
}


int32_t get_direction(void)
{
	return EP_DIRECTION;
}

int32_t cam_param_thread_init(void)
{
	xtimer_create(&cam_param.sun_rise_set_tmr, 
				  cal_sun_rise_set_tmr_cb, &cam_param);
	xtimer_settime(&cam_param.sun_rise_set_tmr, 1000, 60 * 1000);

	return 0;
}

C_CODE_END
