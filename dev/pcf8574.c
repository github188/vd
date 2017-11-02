#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <inttypes.h>
#include <sys/file.h>
#include "pcf8574.h"
#include "logger/log.h"

C_CODE_BEGIN

int_fast32_t pcf8574_set_bit(int_fast32_t bit, int_fast32_t value)
{
	int_fast32_t ret;

	int fd = open(PCF8574_IIC_FILE, O_RDWR);
	if (fd == -1) {
		int _errno = errno;
		ERROR("Open %s failed:%s", PCF8574_IIC_FILE, strerror(_errno));
		return -1;
	}

	if (flock(fd, LOCK_EX) == -1) {
		int _errno = errno;
		ERROR("Lock file %s failed:%s", PCF8574_IIC_FILE, strerror(_errno));
		close(fd);
		return -1;
	}

	ioctl(fd, I2C_TIMEOUT, 1);
	ioctl(fd, I2C_RETRIES, 3);

	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg msg;
	uint8_t regval;

	data.nmsgs = 1;
	data.msgs = &msg;
	data.msgs[0].len = 1;
	data.msgs[0].buf = &regval;
	data.msgs[0].addr = PCF8574_IIC_ADDR;

	/*
	 * First read old value
	 */
	data.msgs[0].flags = I2C_M_RD;
	ret = ioctl(fd, I2C_RDWR, &data);
	if (ret == -1) {
		int _errno = errno;
		ERROR("ioctl I2C_RDWR @%s:%d:%s",
			  __FILE__, __LINE__, strerror(_errno));
		flock(fd, LOCK_UN);
		close(fd);
		return -1;
	}

	DEBUG("pcf8574 old value: 0x%"PRIx8, regval);

	/*
	 * Set new value for special bit
	 */
	if (value == 1) {
		setbit(regval, bit);
	} else {
		clrbit(regval, bit);
	}

	DEBUG("pcf8574 new value: 0x%"PRIx8, regval);

	/*
	 * Set New value to pcf8574
	 */
	data.msgs[0].flags = 0;
	ret = ioctl(fd, I2C_RDWR, &data);
	if (ret == -1) {
		int _errno = errno;
		ERROR("ioctl I2C_RDWR @%s:%d:%s",
			  __FILE__, __LINE__, strerror(_errno));
		flock(fd, LOCK_UN);
		close(fd);
		return -1;
	}

	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

C_CODE_END

