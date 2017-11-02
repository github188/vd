#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "led_ports.h"
#include "dev/led/fly_eq20131.h"
#include "dev/led/degao.h"
#include "dev/led/listen.h"
#include "config/ve_cfg.h"
#include "logger/log.h"

C_CODE_BEGIN

/**
 * Refresh a new screen for fly eq20131
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int_fast32_t ve_fly_eq20131_refresh(led_screen_t *screen)
{
	fly_eq20131_t led;
	fly_eq20131_param_t param;
	fly_eq20131_opt_t opt;

	param.addr = 1;
	param.high = 64;
	param.width = 64;
	param.lattice = 16;
	opt.open = fly_eq20131_serial_open;
	opt.close = fly_eq20131_serial_close;
	opt.send = fly_eq20131_serial_send;
	opt.recv = fly_eq20131_serial_recv;

	fly_eq20131_init(&led, &param, &opt);
	return fly_eq20131_disp(&led, screen);
}

/**
 * Clear screen for fly eq20131
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_fly_eq20131_clear(void)
{
	fly_eq20131_t led;
	fly_eq20131_param_t param;
	fly_eq20131_opt_t opt;

	param.addr = 1;
	param.high = 64;
	param.width = 64;
	param.lattice = 16;
	opt.open = fly_eq20131_serial_open;
	opt.close = fly_eq20131_serial_close;
	opt.send = fly_eq20131_serial_send;
	opt.recv = fly_eq20131_serial_recv;

	fly_eq20131_init(&led, &param, &opt);
	return fly_eq20131_clear(&led);
}

static cvd_t degao;
static bool degao_init = true;

/**
 * Refresh a new screen for degao
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int_fast32_t ve_degao_refresh(led_screen_t *screen)
{
	if (degao_init) {
		cvd_init(&degao, 1, cvd_putnbyte_default);
		degao_init = false;
	}

	if (!list_empty(&screen->lines)) {
		led_line_t *line = list_entry(screen->lines.next, led_line_t, link);
		cvd_disp(&degao, line->text, 50);
	}

	return 0;
}

/**
 * Clear screen for degao
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_degao_clear_line(int_fast32_t idx)
{
	(void)idx;

	if (degao_init) {
		cvd_init(&degao, 1, cvd_putnbyte_default);
		degao_init = false;
	}
	cvd_clear(&degao);
	return 0;
}

/**
 * Refresh a new screen for listen a4
 * 
 * @author cyj (2016/7/20)
 * 
 * @param screen New screen's data 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t listen_refresh(led_screen_t *screen)
{
	/*
	 * Get configures
	 */
	ve_cfg_t ve_cfg;
	ve_cfg_get(&ve_cfg);

	/*
	 * Check arguments
	 */
	if ((ve_cfg.dev.led.height % LISTEN_A4_LATTICE_UNIT) != 0) {
		CRIT("The height of listen a4 is error, "
			 "current is %"PRIu8" and it should be %"PRIu8"*n",
			 ve_cfg.dev.led.height, LISTEN_A4_LATTICE_UNIT);
		return -EINVAL;
	}

	if ((ve_cfg.dev.led.width % LISTEN_A4_LATTICE_UNIT) != 0) {
		CRIT("The width of listen a4 is error, "
			 "current is %"PRIu8" and it should be %"PRIu8"*n",
			 ve_cfg.dev.led.width, LISTEN_A4_LATTICE_UNIT);
		return -EINVAL;
	}

	if ((ve_cfg.dev.led.lattice % LISTEN_A4_LATTICE_UNIT) != 0) {
		CRIT("The lattice of listen a4 is error, "
			 "current is %"PRIu8" and it should be %"PRIu8"*n",
			 ve_cfg.dev.led.lattice, LISTEN_A4_LATTICE_UNIT);
		return -EINVAL;
	}

	listen_led_t led;
	listen_led_cfg_t cfg;
	listen_led_opt_t opt;

	cfg.addr = 1;
	cfg.high = min(ve_cfg.dev.led.height, (uint8_t)LISTEN_A4_HIGH_MAX);
	cfg.width = min(ve_cfg.dev.led.width, (uint8_t)LISTEN_A4_WIDTH_MAX);
	cfg.lattice = min(ve_cfg.dev.led.lattice, (uint8_t)LISTEN_A4_LATTICE_MAX);
	opt.open = listen_open;
	opt.close = listen_close;
	opt.send = listen_send;
	opt.recv = listen_recv;

	listen_led_init(&led, &cfg, &opt);

	return listen_led_refresh(&led, screen);
}

int_fast32_t listen_clear_line(int_fast32_t idx)
{
	listen_led_t led;
	listen_led_cfg_t cfg;
	listen_led_opt_t opt;

	cfg.addr = 1;
	cfg.high = 32;
	cfg.width = 64;
	cfg.lattice = 16;
	opt.open = listen_open;
	opt.close = listen_close;
	opt.send = listen_send;
	opt.recv = listen_recv;

	listen_led_init(&led, &cfg, &opt);

	led_screen_t screen;
	led_line_t line;

	INIT_LIST_HEAD(&screen.lines);
	line.action = LED_ACTION_CLR;
	line.idx = idx;
	line.style = LED_STYLE_IMMEDIATE;
	line.mode = LED_MODE_LINE;
	line.duration = 0;
	line.color.rgb = 0xFF0000;
	line.time_per_col = 100;

	led_line_add_to_screen(&screen, &line);

	return listen_led_refresh(&led, &screen);
}

/**
 * Initialize listen a4 card
 *
 * @author cyj (2016/7/21)
 *
 * @param void
 *
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t listen_init(void)
{
#if 0
	listen_led_t led;
	listen_led_cfg_t cfg;
	listen_led_opt_t opt;

	cfg.addr = 1;
	cfg.high = 32;
	cfg.width = 64;
	cfg.lattice = 16;
	opt.open = listen_open;
	opt.close = listen_close;
	opt.send = listen_send;
	opt.recv = listen_recv;

	listen_led_init(&led, &cfg, &opt);

	return listen_led_update_time(&led);
#else
	return 0;
#endif
}

C_CODE_END
