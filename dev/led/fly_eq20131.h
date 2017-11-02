#ifndef __FLY_LED_H
#define __FLY_LED_H


#include "types.h"
#include "dev/common/serial.h"
#include "dev/led/led.h"


C_CODE_BEGIN

#define FLY_EQ20131_DYNAMIC_SPEED		99

/**
 * enum fly_eq20131_ha - horozontal alignment
 */
typedef enum fly_eq20131_ha{
	FLY_EQ20131_HA_LEFT = 1,
	FLY_EQ20131_HA_CENTERED,
	FLY_EQ20131_HA_RIGHT
}fly_eq20131_ha_t;

/**
 * enum fly_eq20131_va - vertical alignment
 */
typedef enum fly_eq20131_va{
	FLY_EQ20131_VA_TOP = 1,
	FLY_EQ20131_VA_CENTERED,
	FLY_EQ20131_VA_BOTTOM
}fly_eq20131_va_t;

typedef enum fly_eq20131_action{
	FLY_EQ20131_ACTION_RANDOM,
	FLY_EQ20131_ACTION_IMMEDIATELY = 1,
	FLY_EQ20131_SHIFT_LEFT = 49
}fly_eq20131_action_t;

typedef struct fly_eq20131_param{
	int32_t addr;
	int32_t high;
	int32_t width;
	int32_t lattice;
}fly_eq20131_param_t;

typedef struct fly_eq20131_opt{
	int32_t (*open)(void);
	ssize_t (*send)(const uint8_t *buf, size_t size);
	ssize_t (*recv)(uint8_t *buf, size_t size);
	int32_t (*close)(void);
}fly_eq20131_opt_t;

typedef struct fly_eq20131{
	fly_eq20131_param_t param;
	fly_eq20131_opt_t opt;
}fly_eq20131_t;


int32_t fly_eq20131_init(fly_eq20131_t *led,
						 const fly_eq20131_param_t *param,
						 const fly_eq20131_opt_t *opt);
int32_t fly_eq20131_disp(fly_eq20131_t *led, led_screen_t *screen);
int32_t fly_eq20131_clear(fly_eq20131_t *led);
ssize_t fly_eq20131_serial_recv(uint8_t *buf, size_t size);
ssize_t fly_eq20131_serial_send(const uint8_t *data, size_t size);
int32_t fly_eq20131_serial_open(void);
int32_t fly_eq20131_serial_close(void);

void rs485_test(void);

C_CODE_END

#endif
