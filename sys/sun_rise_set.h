#ifndef __SUN_RISE_SET_H
#define __SUN_RISE_SET_H

#include <time.h>
#include "types.h"


C_CODE_BEGIN

typedef struct tm sun_tm_t;

int_fast32_t sun_rise_set_time(sun_tm_t *rise, sun_tm_t *set,
							   double lon, double lat);



C_CODE_END



#endif
