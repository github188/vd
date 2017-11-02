#ifndef __PCF8547_H
#define __PCF8547_H

#include "types.h"

C_CODE_BEGIN

#define PCF8574_IIC_FILE	"/dev/i2c-3"
#define PCF8574_IIC_ADDR		0x38

int_fast32_t pcf8574_set_bit(int_fast32_t bit, int_fast32_t value);

C_CODE_END

#endif
