#ifndef __LISTEN_H
#define __LISTEN_H

#include "dev/common/serial.h"

C_CODE_BEGIN

#define LISTEN_LED_DEBUG	0

#define LISTEN_A4_HIGH_MAX	64
#define LISTEN_A4_WIDTH_MAX		64
#define LISTEN_A4_LATTICE_MAX		32
#define LISTEN_A4_LATTICE_UNIT		8

#if ((LISTEN_A4_LATTICE_MAX % LISTEN_A4_LATTICE_UNIT) != 0)
	#error "Please check lattice of listen a4 card"
#endif

#if ((LISTEN_A4_WIDTH_MAX % LISTEN_A4_LATTICE_UNIT) != 0)
	#error "Please check width of listen a4 card"
#endif

#if ((LISTEN_A4_HIGH_MAX % LISTEN_A4_LATTICE_UNIT) != 0)
	#error "Please check higth of listen a4 card"
#endif

typedef struct listen_led listen_led_t;

typedef struct listen_led_cfg {
	uint8_t model[16];
	uint8_t addr;
	uint8_t high;
	uint8_t width;
	uint8_t lattice;
} listen_led_cfg_t;

typedef struct listen_led_opt {
	int32_t (*open)(listen_led_t *led);
	ssize_t (*send)(listen_led_t *led, const uint8_t *buf, size_t size);
	ssize_t (*recv)(listen_led_t *led, uint8_t * buf, size_t size);
	int32_t (*close)(listen_led_t *led);
} listen_led_opt_t;

struct listen_led {
	listen_led_cfg_t cfg;
	listen_led_opt_t opt;
	serial_t serial;
};

int32_t listen_led_refresh(listen_led_t *led, const led_screen_t *screen);
int32_t listen_led_clear(listen_led_t *led);
void listen_led_init(listen_led_t *led, const listen_led_cfg_t *cfg,
					 const listen_led_opt_t *opt);
int32_t listen_open(listen_led_t *led);
int32_t listen_close(listen_led_t *led);
int32_t listen_send(listen_led_t *led, const uint8_t *buf, size_t len);
int32_t listen_recv(listen_led_t *led, uint8_t *buf, size_t size);


/**
 * Update date time for listen a4 card
 *
 * @author cyj (2016/7/21)
 *
 * @param led
 *
 * @return int32_t
 */
int32_t listen_led_update_time(listen_led_t *led);

C_CODE_END


#endif
