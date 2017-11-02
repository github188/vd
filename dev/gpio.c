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
#include "gpio.h"
#include "logger/log.h"

C_CODE_BEGIN

int_fast32_t gpio_set(const char *gpio, int32_t val)
{
	ASSERT((0 == val) || (1 == val));

	char buf[64];
	snprintf(buf, sizeof(buf), "/sys/class/gpio/%s/value", gpio);

	int fd = open(buf, O_WRONLY);
	if (fd < 0) {
		int _errno = errno;
		ERROR("Open %s failed:%s", gpio, strerror(_errno));
		return -1;
	}

	char c = (char)(val + 0x30);
	write(fd, &c, sizeof(c));
	close(fd);
	usleep(10000);

	return 0;
}

int_fast32_t gpio_get(const char *gpio)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "/sys/class/gpio/%s/value", gpio);

	int fd = open(buf, O_RDONLY);
	if(fd < 0){
		ALERT("%s open failed", gpio);
		return -1;
	}

	char c = 0x2F;
	read(fd, &c, sizeof(c));
	close(fd);

	return c - 0x30;
}

int_fast32_t gpio_get_filter(const char *gpio, int_fast32_t times,
							 int_fast32_t intv)
{
	if (times < 4) {
		times = 4;
	}

	if ((times & 0x01) == 0) {
		times -= 1;
	}

	int_fast32_t high_cnt = 0;

	for (int_fast32_t i = 0; i < times; ++i) {
		if (gpio_get(gpio) == 1) {
			high_cnt += 1;
		}

		usleep(intv * 1000);
	}

	int_fast32_t low_cnt = times - high_cnt;

	return (high_cnt > low_cnt) ? 1 : 0; 
}

C_CODE_END
