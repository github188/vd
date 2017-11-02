#include <sys/file.h>
#include "types.h"
#include "serial_ctrl.h"
#include "sys/time_util.h"
#include "logger/log.h"
#include "dev/common/serial.h"

C_CODE_BEGIN

static int32_t serial_set_baud(struct termios *tio, int32_t baud)
{
	typedef struct baud_item{
		int32_t a;
		int32_t b;
	}baud_item_t;

	const baud_item_t baud_tab[] = {
		{0, B0},
		{50, B50},
		{75, B75},
		{110, B110},
		{134, B134},
		{150, B150},
		{200, B200},
		{300, B300},
		{600, B600},
		{1200, B1200},
		{2400, B2400},
		{4800, B4800},
		{9600, B9600},
		{19200, B19200},
		{38400, B38400},
		{57600, B57600},
		{115200, B115200},
		{230400, B230400}
	};

	const baud_item_t *p = baud_tab;
	const baud_item_t *end = baud_tab + numberof(baud_tab);

	ASSERT(tio);

	while ((p->a != baud) && (p++ < end));
	if (p == end) {
		cfsetispeed(tio, B9600);
		cfsetospeed(tio, B9600);
		return -1;
	}

	cfsetispeed(tio, p->b);
	cfsetospeed(tio, p->b);
	return 0;
}

static int32_t serial_set_data_size(struct termios *tio, int32_t datasz)
{
	typedef struct datasz_item{
		int32_t a;
		int32_t b;
	}datasz_item_t;

	const datasz_item_t datasz_tab[] = {
		{5, CS5},
		{6, CS6},
		{7, CS7},
		{8, CS8}
	};

	const datasz_item_t *p = datasz_tab;
	const datasz_item_t *end = datasz_tab + numberof(datasz_tab);

	ASSERT(tio);

	tio->c_cflag &= ~CSIZE;

	while ((p->a != datasz) && (p++ < end));
	if (p == end) {
		tio->c_cflag |= CS8;
		return -1;
	}

	tio->c_cflag |= p->b;
	return 0;
}

static int32_t serial_set_paity(struct termios *tio, serial_paity_t paity)
{
	int32_t ret = 0;

	ASSERT(tio);

	switch (paity) {
		case SERIAL_PAITY_NONE:
			tio->c_cflag &= ~PARENB;
			break;
		case SERIAL_PAITY_EVEN:
			tio->c_iflag |= (INPCK | ISTRIP);
			tio->c_cflag |= PARENB;
			tio->c_cflag &= ~PARODD;
			break;
		case SERIAL_PAITY_ODD:
			tio->c_cflag |= PARENB;
			tio->c_cflag |= PARODD;
			tio->c_iflag |= (INPCK | ISTRIP);
			break;
		case SERIAL_PAITY_MARK:
			break;
		case SERIAL_PAITY_SPACE:
			break;
		default:
			tio->c_cflag &= ~PARENB;
			ret = -1;
			break;
	}

	return ret;
}

static int32_t serial_set_stop_bit(struct termios *tio, int32_t stop_bit)
{
	int32_t ret = 0;

	ASSERT(tio);

	switch (stop_bit) {
		case 1:
			tio->c_cflag &= ~CSTOPB;
			break;
		case 2:
			tio->c_cflag |= CSTOPB;
			break;
		default:
			tio->c_cflag &= ~CSTOPB;
			ret = -1;
			break;
	}

	return ret;
}

static int32_t serial_config(serial_t *serial, const serial_cfg_t *cfg)
{
	struct termios opt = {0};

	ASSERT(cfg);
	ASSERT(serial);
	ASSERT(serial->fd);

	opt.c_cflag |= (CLOCAL | CREAD);
	opt.c_cc[VTIME] = 2;
	opt.c_cc[VMIN] = 0;

	serial_set_baud(&opt, cfg->baud);
	serial_set_data_size(&opt, cfg->datasz);
	serial_set_paity(&opt, cfg->paity);
	serial_set_stop_bit(&opt, cfg->stop_bit);

	tcflush(serial->fd, TCIFLUSH);
	if (0 != tcsetattr(serial->fd, TCSANOW, &opt)) {
		CRIT("Set attr for %s failed!", cfg->name);
		return -1;
	}

	return 0;
}

int32_t serial_open(serial_t *serial, const serial_cfg_t *cfg)
{
	ASSERT(cfg);
	ASSERT(serial);

	serial->fd = open(cfg->name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == serial->fd) {
		ERROR("Open %s failed, error: %d", cfg->name, errno);
		return -1;
	}

	if (flock(serial->fd, LOCK_EX) == -1) {
		int _errno = errno;
		ERROR("Lock file %s failed:%s", cfg->name, strerror(_errno));
		close(serial->fd);
		return -1;
	}

	if (fcntl(serial->fd, F_SETFL, 0) == -1) {
		ERROR("File ctrl %s failed, error: %d", cfg->name, errno);
	}

	if (!isatty(STDIN_FILENO)) {
		ERROR("Standard input is not a termined device");
	}

	if (0 != serial_config(serial, cfg)) {
		ERROR("Config serial %s failed!", cfg->name);
		flock(serial->fd, LOCK_UN);
		close(serial->fd);
		return -1;
	}

	return 0;
}

ssize_t serial_send(serial_t *serial, const uint8_t *data, size_t size)
{
	ASSERT(serial);
	ASSERT(data);
	ASSERT(serial->fd > 0);
	return write(serial->fd, data, size);
}

ssize_t serial_recv(serial_t *serial, uint8_t *buf, size_t size)
{
	ASSERT(serial);
	ASSERT(buf);
	ASSERT(serial->fd > 0);
	return read(serial->fd, buf, size);
}

ssize_t serial_recv_timeout(serial_t *serial, uint8_t *buf, size_t size, 
							int32_t timeout)
{
	struct timeval tv;
	fd_set rfds; 

	ASSERT(serial);
	ASSERT(serial->fd > 0);
	ASSERT(buf);

	FD_ZERO(&rfds);
	FD_SET(serial->fd, &rfds);
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (select(serial->fd + 1, &rfds, NULL, NULL, &tv) > 0) {
		if (FD_ISSET(serial->fd, &rfds)) {
			return read(serial->fd, buf, size);
		}
	}

	return -1;
}

int32_t serial_close(serial_t *serial)
{
	int32_t ret;

	ASSERT(serial);
	ASSERT(serial->fd > 0);

	flock(serial->fd, LOCK_UN);
	ret = close(serial->fd);
	serial->fd = -1;
	return ret;
}

C_CODE_END
