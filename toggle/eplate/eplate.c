#include "eplate.h"
#include "commonfuncs.h"
#include "sys/time.h"
#include "serial_ctrl.h"
#include "logger/log.h"


static eplate_t eplate;

static int32_t eplate_rs485_set_dir(eplate_rs485_dir_t dir)
{
	system("insmod /opt/ipnc/pinmux_module.ko a=0x48140B84 v=0x80");
	system("rmmod /opt/ipnc/pinmux_module.ko");
	system("echo 114 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio114/direction");

	if (EPLATE_RS485_INPUT == dir) {
		system("echo 0 > /sys/class/gpio/gpio114/value");
	} else {
		system("echo 1 > /sys/class/gpio/gpio114/value");
	}

	return 0;
}

static int32_t eplate_rs485_init(int32_t baud)
{
	int32_t fd;

	eplate_rs485_set_dir(EPLATE_RS485_INPUT);

	fd = Open_Port(4, 'O', O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		TRACE_LOG_SYSTEM("rs485 open failed!");
		return -1;
	}

	if (0 != Set_Opt(fd, baud, 8, 'N', 1)) {
		TRACE_LOG_SYSTEM("rs485 set failed!");
		close(fd);

		return -1;
	}

	return fd;
}

static const uint8_t *eplate_find_hdr(const uint8_t *recv, size_t len)
{
	const uint8_t *p = recv;
	const uint8_t *end = recv + len;

	while ((0x0B != *p) && (++p < end));

	return (p == end) ? NULL : p;
}

static int32_t eplate_chk_frm(const uint8_t *hdr, size_t len)
{
	uint8_t cs = 0;
	const uint8_t *p, *end;
	size_t frm_len;

	frm_len = EPLATE_GET_FRM_LEN(hdr);
	TRACE_LOG_SYSTEM("eplate frame len is %d!", frm_len);
	if (len < frm_len) {
		return -1;
	}

	for (p = hdr + 1, end = hdr + 7; p < end; ++p) {
		cs += *p;
	}

	cs ^= 0xFF;
	cs += 1;
	cs ^= 0xFF;

	p = hdr + frm_len - 1;

	TRACE_LOG_SYSTEM("calculated cs is 0x%02X!", cs);
	TRACE_LOG_SYSTEM("eplate frame cs is 0x%02X!", *p);

	if (cs != *p) {
		return -1;
	}

	return 0;
}

static void print_buf(const uint8_t *buf, size_t len)
{
	int32_t i = 0;

	for (i = 0; i < len; i++) {
		TRACE_LOG_SYSTEM("0x%02X", buf[i]);
	}
}

static int32_t eplate_frm_is_heartbeat(const uint8_t *frm, size_t len)
{
	const uint8_t *hdr, *p, *end;
	uint8_t cs = 0;

	hdr = eplate_find_hdr(frm, len);
	if (!hdr) {
		return -1;
	}

	for (p = hdr + 1, end = hdr + 7; p < end; ++p) {
		cs += *p;
	}

	cs ^= 0xFF;
	cs += 1;
	cs ^= 0xFF;

	if (cs != *p) {
		return -1;
	}

	TRACE_LOG_SYSTEM("this frame is a heartbeart.");

	return 0;
}

static int32_t eplate_parse(eplate_t *ep, const uint8_t *recv, size_t len)
{
	const uint8_t *hdr;

	print_buf(recv, len);

	hdr = eplate_find_hdr(recv, len);
	if (!hdr) {
		TRACE_LOG_SYSTEM("can not find a eplate frame header.");
		return -1;
	}

	len -= (size_t)(hdr - recv);
	TRACE_LOG_SYSTEM("eplate frame len is %d!", len);
	if (eplate_chk_frm(hdr, len) < 0) {
		TRACE_LOG_SYSTEM("eplate frame check error!");
		return -1;
	}

	TRACE_LOG_SYSTEM("eplate get %d ids.", EPLATE_GET_ID_NUM(hdr));
	ep->epc_cnt += EPLATE_GET_ID_NUM(hdr);

	return 0;
}

static void *eplate_rs485_thread(void *arg)
{
	eplate_t *ep = (eplate_t *)arg;
	int32_t fd;
	int32_t ret;
	uint8_t buf[1024];
	uintptr_t wp = 0;
	fd_set rset;
	struct timeval tv;

	for(;;) {
		usleep(30000);

		fd = eplate_rs485_init(57600);
		if (fd < 0) {
			continue;
		}

		for (;;) {

			FD_ZERO(&rset);
			FD_SET(fd, &rset);
			tv.tv_sec = 0;
			tv.tv_usec = 50000;
			ret = select(fd + 1, &rset, NULL, NULL, NULL);
			if ((ret < 0) && (EINTR != ret) && (EAGAIN != ret)) {
				TRACE_LOG_SYSTEM("rs485 select failed!");
				break;
			}

			ret = read(fd, buf + wp, sizeof(buf) - wp);
			if (ret > 0) {
				TRACE_LOG_SYSTEM("rcvd a new eplate frame.");
				pthread_mutex_lock(&ep->mutex);

				wp += ret;

				if ((0 == eplate_frm_is_heartbeat(buf, wp))
					|| (0 == eplate_parse(ep, buf, wp))
					|| (wp >= sizeof(buf))) {
					wp = 0;
				}

				pthread_mutex_unlock(&ep->mutex);
			} else if ((ret < 0) && (EINTR != ret) && (EAGAIN != ret)) {
				TRACE_LOG_SYSTEM("rs485 read failed!");
				break;
			}
		}

		close(fd);

	}

	return NULL;
}

char eplate_get_status(void)
{
	int32_t ret = -1;

	pthread_mutex_lock(&eplate.mutex);

	if (eplate.epc_cnt > 0) {
		--eplate.epc_cnt;
		ret = 0;
	}

	pthread_mutex_unlock(&eplate.mutex);

	return ret;
}

int32_t eplate_init(void)
{
	int32_t ret;
	pthread_attr_t attr;

	eplate.epc_cnt = 0;

	if(0 != pthread_mutex_init(&eplate.mutex, NULL)){
		return -1;
	}

	if (0 != pthread_attr_init(&attr)){
		return -1;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	do {
		ret = pthread_create(&eplate.rs485.thread_id, &attr,
							 eplate_rs485_thread, &eplate);
		if (0 != ret) {
			DEBUG("Create eplate rs485 thread failed!");
			break;
		}
	}while (0);

	pthread_attr_destroy(&attr);

	return ret;

}
