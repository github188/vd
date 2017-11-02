#ifndef __DEV_GPIO_H
#define __DEV_GPIO_H

#include "types.h"

C_CODE_BEGIN

int_fast32_t gpio_set(const char *gpio, int32_t val);
int_fast32_t gpio_get(const char *gpio);
int_fast32_t gpio_get_filter(const char *gpio, int_fast32_t times,
							 int_fast32_t intv);


C_CODE_END


#endif
