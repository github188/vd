#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include "types.h"
#include "logger/log.h"
#include "file_msg_drv.h"
#include "sys/time_util.h"
#include "dev/light.h"
#include "config/ve_cfg.h"
#include "config/cam_param.h"
#include "modules/ve/pwm.h"
#include "dev/pcf8574.h"
#include "dev/gpio.h"


C_CODE_BEGIN

#define BLUE_LIGHT_BIT		7
#define WHITE_LIGHT_BIT		5
#define WHITE_LIGHT_PWM_FILE		"/dev/pwm"
#define GREEN_LIGHT_GPIO		"gpio100"
#define LIGHT_ENABLE_GPIO		"gpio99"
#define RED_LIGHT_GPIO		"gpio73"






static int_fast8_t light_enable_set(bool enable)
{
	return gpio_set(LIGHT_ENABLE_GPIO,  enable ? 1 : 0);
}

static int_fast8_t white_light_set(light_t *light, uint_fast8_t value)
{
	(void)light;

	int fd = open(WHITE_LIGHT_PWM_FILE, 0);
	if (fd < 0) {
		ERROR("Open pwm device failed");
		return -1;
	}

	if (0 == value) {
		INFO("Close white light");
		/* First white light */
		ioctl(fd, PWM_IOCTL_SET_DUTY_CYCLE, 0);
		/* Second white light */
		pcf8574_set_bit(WHITE_LIGHT_BIT, 1);
	} else {
		ve_cfg_t cfg;
		ve_cfg_get(&cfg);
		uint_fast8_t rate = cfg.dev.light.white_bright;
		if (rate > 100) {
			rate = 100;
		}
		/* First white light */
		ioctl(fd, PWM_IOCTL_SET_DUTY_CYCLE, rate);
		/* Second white light */
		pcf8574_set_bit(WHITE_LIGHT_BIT, 0);
		DEBUG("Open white light @ %s, rate: %d", ustime_msec(), rate);
	}

	close(fd);

	return 0;
}

static int_fast8_t green_light_set(light_t *light, uint_fast8_t value)
{
	int_fast8_t ret;

	light_enable_set(false);
	ret = gpio_set(GREEN_LIGHT_GPIO, (0 == value) ? 0 : 1);
	light_enable_set(true);

	return ret;
}

static int_fast8_t blue_light_set(light_t *light, uint_fast8_t value)
{

	light_enable_set(false);
	pcf8574_set_bit(BLUE_LIGHT_BIT, value);
	light_enable_set(true);

	return 0;
}

static int_fast8_t red_light_set(light_t *light, uint_fast8_t value)
{
	int_fast8_t ret;

	light_enable_set(false);
	ret = gpio_set(RED_LIGHT_GPIO, (0 == value) ? 0 : 1);
	light_enable_set(true);

	return 0;
}

void board_light_init(void)
{
	light_opt_t opt;

	opt.set = red_light_set;
	light_regst(LIGHT_ID_RED, &opt);
	opt.set = blue_light_set;
	light_regst(LIGHT_ID_BLUE, &opt);
	opt.set = white_light_set;
	light_regst(LIGHT_ID_WHITE, &opt);
	opt.set = green_light_set;
	light_regst(LIGHT_ID_GREEN, &opt);
}

C_CODE_END


