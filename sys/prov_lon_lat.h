#ifndef __PROV_LON_LAT_H
#define __PROV_LON_LAT_H

#include "types.h"

C_CODE_BEGIN

#define PROVINCE_CAP_NAME_LEN	8
#define PROV_SHANDONG_LON		117.0
#define PROV_SHANDONG_LAT		36.67

typedef float lon_lat_t;

/**
 * struct city_lon_lat - the struct of longitude and latitude
 *
 * @name: the city name
 * @lat: latitude
 * @lon: longitude
 */
typedef struct prov_lon_lat {
	char name[PROVINCE_CAP_NAME_LEN];
	lon_lat_t lon;
	lon_lat_t lat;
}prov_lon_lat_t;

int32_t local_lon_lat(const char *prov, lon_lat_t *lon, lon_lat_t *lat);

C_CODE_END


#endif
