#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "common/serial.h"
#include "fly_eq20131.h"
#include "serial_ctrl.h"
#include "logger/log.h"

C_CODE_BEGIN

/**
 * struct region_start - the paramater of region position
 */
typedef struct region_start{
	int32_t x;
	int32_t y;
	int32_t high;
	int32_t width;
}region_start_t;

/**
 * struct fly_eq20131_region - the param of a region to disp
 *
 * @action: the display action
 * @ha: horozontal alignment
 * @va: vertical alignment
 * @duration: the disp duration
 * @speed: action's speed, zero means static display
 * @s: the pointer to disp string
 */
typedef struct region{
	int32_t id;
	fly_eq20131_action_t action;
	fly_eq20131_ha_t ha;
	fly_eq20131_va_t va;
	region_start_t start;
	int32_t duration;
	int32_t speed;
	const char *str;
}region_t;

typedef struct voice_com_hdr{
	uint8_t sof;
	uint8_t daddr;
	uint16_t len;
	uint8_t saddr;
	uint8_t cmd_type;
	uint8_t data[1];
}__attribute__((packed)) voice_com_hdr_t;

typedef struct voice_ctrl_hdr{
	uint8_t sof;
	uint16_t len;
	uint8_t cmd;
	uint8_t encode_fmt;
	uint8_t data[1];
}__attribute__((packed)) voice_ctrl_hdr_t;

typedef struct voice_com_end{
	uint8_t cs;
}__attribute__((packed)) voice_com_end_t;

#define VOICE_COM_HDR_LEN		(sizeof(voice_com_hdr_t) - 1)
#define VOICE_CTRL_HDR_LEN		(sizeof(voice_ctrl_hdr_t) - 1)
#define VOICE_COM_END_LEN		(sizeof(voice_com_end_t))


static int32_t fly_led_send(fly_eq20131_t *led, const uint8_t *data, size_t len);
static int32_t fly_voice_send(fly_eq20131_t *led,
							  const uint8_t *data, size_t len);

static void printmem(const void *mem, size_t size)
{
	char buf[128];
	char *pbuf;
	const uint8_t *p;
	size_t i, bufsz, tmp;
	ssize_t len;

	ASSERT(mem);

	p = (const uint8_t *)mem;

	while (size > 8) {
		snprintf(buf, sizeof(buf),
				 "0x%02X, 0x%02X, 0x%02X, 0x%02X, "
				 "0x%02X, 0x%02X, 0x%02X, 0x%02X",
				 p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		DEBUG("%s", buf);

		p += 8;
		size -= 8;
	}

	len = 0;
	pbuf = buf;
	bufsz = sizeof(buf);
	tmp = size - 1;
	for (i = 0; i < tmp; ++i) {
		len = snprintf(pbuf, bufsz, "0x%02X, ", p[i]);
		bufsz -= len;
		pbuf += len;
	}
	snprintf(pbuf, bufsz, "0x%02X", p[i]);
	DEBUG("%s", buf);
}

static uint8_t get_voice_485_frm_cs(const uint8_t *buf, size_t len)
{
	uint8_t cs = 0;
	const uint8_t *p, *end;

	for(p = buf, end = buf + len; p < end; ++p) {
		cs ^= *p;
	}

	return cs;
}

static ssize_t make_voice_frm(uint8_t *buf, size_t bufsz,
							  const uint8_t *cmd, size_t cmd_len)
{
	voice_com_hdr_t *hdr;
	voice_com_end_t *end;
	ssize_t len;

	/*
	 * check the frame length
	 */
	len = VOICE_COM_HDR_LEN + cmd_len + VOICE_COM_END_LEN;
	if (bufsz < len) {
		ERROR("The buffer for voice frame is not enough.");
		return -1;
	}

	DEBUG("Voice frame length total %d bytes", len);

	hdr = (voice_com_hdr_t *)buf;

	/*
	 * copy the voice content
	 */
	memcpy(hdr->data, cmd, cmd_len);

	/*
	 * make header
	 */
	hdr->sof = 0xFE;
	hdr->daddr = 0;
	hdr->len = swap_two(len - offsetof(voice_com_hdr_t, len));
	hdr->saddr = 0x99;
	hdr->cmd_type = 0x73;

	/*
	 * make the ender
	 */
	end = (voice_com_end_t *)(hdr->data + cmd_len);
	end->cs = get_voice_485_frm_cs(buf + 1, len - 2);

	return len;
}

int32_t play_voice(fly_eq20131_t *led, const char *s, int32_t volume)
{
	uint8_t frm[256] = { 0 };
	uint8_t cmd[256] = { 0 };
	ssize_t len;
	int32_t ret;
	voice_ctrl_hdr_t *hdr;

	ASSERT(led);
	ASSERT(s);

	hdr = (voice_ctrl_hdr_t *)cmd;

	len = snprintf((char *)hdr->data,
				   sizeof(cmd) - sizeof(voice_ctrl_hdr_t) + 1,
				   "[m3][v%d]%s", volume, s);
	len += VOICE_CTRL_HDR_LEN - offsetof(voice_ctrl_hdr_t, cmd);

	hdr->sof = 0xFD;
	hdr->len = swap_two(len);
	hdr->encode_fmt = 1;
	hdr->cmd = 1;

	len += VOICE_CTRL_HDR_LEN;

	len = make_voice_frm(frm, sizeof(frm), cmd, len);

	if(0 != led->opt.open()){
		ERROR("Fly open device failed!");
		return -1;
	}

	ret = fly_voice_send(led, frm, len);
	led->opt.close();

	return ret;
}

/**
 * calculate the max lines the led supported
 *
 * @author cyj (2016/2/18)
 *
 * @param high
 * @param lattice
 *
 * @return int32_t
 */
static int32_t cal_region_max_num(int32_t high, int32_t lattice)
{
	return (high / lattice);
}

static int32_t del_all_lines(fly_eq20131_t *led)
{
	char buf[32];
	ssize_t len;
	int32_t ret;

	ASSERT(led);

	len = snprintf(buf, sizeof(buf), "!#%03d%%ZD00$$", led->param.addr);

	DEBUG("The length of del all regions cmd is %d", len);
	DEBUG("The content of del all regions cmd is %s", buf);

	if(0 != led->opt.open()){
		ERROR("Fly eq20131 open failed!");
		return -1;
	}

	ret = fly_led_send(led, (uint8_t *)buf, len);

	led->opt.close();

	return ret;
}

int32_t fly_eq20131_clear(fly_eq20131_t *led)
{
	int32_t ret;

	ret = del_all_lines(led);
	if (0 != ret) {
		ERROR("Clear fly eq20131 failed!");
	}

	return ret;
}


/**
 * make_shift_left_region - make a shift left display frame
 *
 * @author cyj (2016/2/18)
 *
 * @param led - the struct of led
 * @param buf - the frame buffer
 * @param bufsz - the size of frame buffer
 * @param region - the region to add
 *
 * @return ssize_t - the length of frame
 */
static ssize_t make_shift_left_region(fly_eq20131_t *led, uint8_t *buf,
									  size_t bufsz, const region_t *region)
{
	ssize_t len;

	len = snprintf((char *)buf, bufsz,
				   "!#%03d"
				   "%%ZI%02d"
				   "%%ZC%04d%04d%04d%04d"
				   "%%ZA%02d%%ZS%02d"
				   "%%AH%1d%%AV%1d"
				   "%%C1%s"
				   "$$",
				   led->param.addr,
				   region->id,
				   region->start.x, region->start.y,
				   region->start.width, region->start.high,
				   region->action, region->speed,
				   region->ha, region->va,
				   (char *)region->str);

	return len;
}

static ssize_t make_static_region(fly_eq20131_t *led, uint8_t *buf,
								  size_t bufsz, const region_t *region)
{
	ssize_t len;

	len = snprintf((char *)buf, bufsz,
				   "!#%03d"
				   "%%ZI%02d"
				   "%%ZC%04d%04d%04d%04d"
				   "%%ZA%02d%%ZS%02d%%ZH%04d"
				   "%%AH%1d%%AV%1d"
				   "%%C1%s"
				   "$$",
				   led->param.addr,
				   region->id,
				   region->start.x, region->start.y,
				   region->start.width, region->start.high,
				   region->action, region->speed, region->duration,
				   region->ha, region->va,
				   (char *)region->str);

	return len;
}

static ssize_t add_region(fly_eq20131_t *led, const region_t *region)
{
	int32_t ret;
	uint8_t buf[512];
	ssize_t len;

	ASSERT(led);
	ASSERT(region);

	if (0 == region->speed) {
		len = make_static_region(led, buf, sizeof(buf), region);
	} else {
		len = make_shift_left_region(led, buf, sizeof(buf), region);
	}

	DEBUG("Add region, total %d bytes, %s", len, (char *)buf);

	if(0 != led->opt.open()){
		return -1;
	}

	ret = fly_led_send(led, buf, len);

	led->opt.close();

	return ret;
}

static uint8_t *find_ask_frm_hdr(const uint8_t *frm, size_t size)
{
	const uint8_t *p, *end;

	/*
	 * find the header of ask fream
	 */
	p = frm;
	end = frm + size -1;
	for (; p < end; ++p) {
		if (('#' == *p) && ('#' == *(p + 1))) {
			return (uint8_t *)p;
		}
	}

	return NULL;
}

static uint8_t *find_ask_frm_end(const uint8_t *frm, size_t size)
{
	const uint8_t *p, *end;

	/*
	 * find the header of ask fream
	 */
	p = frm;
	end = frm + size;
	for (; p < end; ++p) {
		if ('$' == *p) {
			return (uint8_t *)p;
		}
	}

	return NULL;

}

static uint8_t *find_ask_frm(const uint8_t *frm, size_t *size)
{
	uint8_t *start, *end;
	size_t len;

	len = *size;
	start = find_ask_frm_hdr(frm, len);
	if (!start) {
		return NULL;
	}

	len -= (size_t)(start - frm + 1);
	end = find_ask_frm_end(start, len);
	if (NULL == end) {
		return NULL;
	}

	*size = end - start + 1;
	return start;
}

static int32_t fly_led_chk_ask(fly_eq20131_t *led,
							   const uint8_t *frm, size_t size)
{
	const char ok_ask[] = "##FOK$";
	uint8_t *ask;
	size_t len;

	len = size;
	ask = find_ask_frm(frm, &len);
	if (NULL == ask) {
		return -1;
	}

	if(0 != memcmp(ask, ok_ask, min(strlen(ok_ask), len))){
		return -1;
	}

	return 0;
}

static const uint8_t *voice_find_ask_hdr(const uint8_t *frm, size_t len)
{
	const uint8_t *p, *end;
	voice_com_hdr_t hdr;

	hdr.sof = 0xFE;
	hdr.daddr = 0x99;

	p = frm;
	end = frm + len - 1;
	for (; p < end; ++p) {
		if ((*p == hdr.sof) && (*(p + 1) == hdr.daddr)) {
			return p;
		}
	}

	return NULL;
}

static const uint8_t *voice_find_ask_frm(const uint8_t *frm, size_t len)
{
	const voice_com_hdr_t *hdr;
	const voice_com_end_t *end;
	size_t l;
	size_t data_len;
	uint8_t cs;

	if (!(hdr = (const voice_com_hdr_t *)voice_find_ask_hdr(frm, len))) {
		ERROR("Find hdr from fly voice failed!");
		return NULL;
	}

	l = swap_two(hdr->len);
	data_len = sizeof(hdr->len) + sizeof(hdr->saddr)
			   + sizeof(hdr->cmd_type) + sizeof(end->cs);
	data_len = l - data_len;

	end = (const voice_com_end_t *)(hdr->data + data_len);
	cs = get_voice_485_frm_cs(&hdr->daddr, l);
	if (cs == end->cs) {
		return (uint8_t *)hdr;
	}

	return NULL;
}

static int32_t voice_chk_ask(fly_eq20131_t *led,
							 const uint8_t *frm, size_t len)
{
	const voice_com_hdr_t *hdr;

	hdr = (const voice_com_hdr_t *)voice_find_ask_frm(frm, len);
	return hdr ? 0 : -1;
}

int32_t fly_eq20131_init(fly_eq20131_t *led,
						 const fly_eq20131_param_t *param,
						 const fly_eq20131_opt_t *opt)
{
	ASSERT(led);
	ASSERT(param);
	ASSERT(opt);
	ASSERT(param->lattice > 0);
	ASSERT(param->high > 0);
	ASSERT(param->width > 0);
	ASSERT(opt->send);
	ASSERT(opt->recv);
	ASSERT(opt->close);
	ASSERT(opt->open);

	memcpy(&led->opt, opt, sizeof(fly_eq20131_opt_t));
	memcpy(&led->param, param, sizeof(fly_eq20131_param_t));

	return 0;
}

#if 0
static int32_t fly_eq20131_disp_a_line(fly_eq20131_t *led, const char *str)
{
	int32_t max_len;
	region_t region;

	max_len = led->param.width / led->param.lattice * 2;
	max_len *= led->param.high / led->param.lattice;
	region.start.x = 0;
	region.start.y = 0;
	region.start.high =  led->param.high;
	region.start.width = led->param.width;
	region.id = 1;
	region.va = FLY_EQ20131_VA_CENTERED;
	region.str = str;
	region.duration = 0;

	if (strlen(region.str) > max_len) {
		region.speed = FLY_EQ20131_DYNAMIC_SPEED;
		region.action = FLY_EQ20131_SHIFT_LEFT;
		region.ha = FLY_EQ20131_HA_LEFT;
	} else {
		region.speed = 0;
		region.action = FLY_EQ20131_ACTION_IMMEDIATELY;
		region.ha = FLY_EQ20131_HA_LEFT;
	}

	return add_region(led, &region);
}
#endif

/**
 * fly led display a string
 *
 * @author cyj (2016/2/1)
 *
 * @param led
 * @param s - a character string, must end of '\0'
 *
 * @return int32_t
 */
int32_t fly_eq20131_disp(fly_eq20131_t *led, led_screen_t *screen)
{
	led_line_t *line;
	int32_t nline, ncol, tmp, i;
	region_t region;
	region_start_t start;
	int32_t vertical_edge;

	ASSERT(screen);
	ASSERT(led);

	/* Delete all lines */
	if(0 != del_all_lines(led)){
		ERROR("Fly eq20131 del all regions failed!");
		return -1;
	}

	/*
	 * Calculate the lines and cloumns number
	 */
	tmp = cal_region_max_num(led->param.high, led->param.lattice);
	//nline = min(screen->nline, tmp);
	ncol = (led->param.width / led->param.lattice) * 2;

	/* Get the pointer of first line */
	//line = screen->lines;

#if 0
	/* When only one line, separate process */
	if ((1 == nline) && (strlen(line->text) > ncol)) {
		if(0 != fly_eq20131_disp_a_line(led, line->text)){
			ERROR("Send to fly eq20131 failed!", line->text);
			return -1;
		}
		return play_voice(led, line->text, screen->volume);
	}
#endif

	/*
	 * Calculate the high and width for a region
	 */
	start.high = led->param.lattice;
	start.width = led->param.width;
	start.x = 0;
	nline = 0;
	vertical_edge = (led->param.high - led->param.lattice * nline) / 2;

	for (i = 0; i < nline; ++i) {
		DEBUG("Region[%d]: %s", i + 1, line->text);

		start.y = vertical_edge + led->param.lattice * i;
		memcpy(&region.start, &start, sizeof(start));
		region.id = i + 1;
		region.va = FLY_EQ20131_VA_CENTERED;
		region.str = line->text;
		region.duration = 0;

		if (strlen(region.str) > ncol) {
			region.speed = FLY_EQ20131_DYNAMIC_SPEED;
			region.action = FLY_EQ20131_SHIFT_LEFT;
			region.ha = FLY_EQ20131_HA_LEFT;
		} else {
			region.speed = 0;
			region.action = FLY_EQ20131_ACTION_IMMEDIATELY;
			region.ha = FLY_EQ20131_HA_CENTERED;
		}

		if(0 != add_region(led, &region)){
			ERROR("Send to fly eq20131 led failed!", line->text);
			return -1;
		}

		//if ((line->play_audio) && (screen->volume > 0)) {
		//	if (0 != play_voice(led, line->text, screen->volume)) {
		//		ERROR("Send to fly eq20131 voice failed!", line->text);
		//	}
		//}

		++line;
	}

	return 0;
}

static int32_t fly_led_send(fly_eq20131_t *led,
							const uint8_t *data, size_t len)
{
	/* retry times, should >= 1 */
	int32_t retry = 3;
	ssize_t l;
	uint8_t buf[256];

	do {
		led->opt.send(data, len);
		usleep(150000);

		l = led->opt.recv(buf, sizeof(buf));
		if (l > 0) {
			DEBUG("Rcvd %d bytes, %s", l, buf);
			printmem(buf, l);
			if (0 == fly_led_chk_ask(led, buf, l)) {
				return 0;
			}
		}
	}while (--retry);

	return -1;
}

static int32_t fly_voice_send(fly_eq20131_t *led,
							  const uint8_t *data, size_t len)
{
	int32_t retry = 3;
	uint8_t buf[128];
	ssize_t l;

	do {
		led->opt.send(data, len);
		usleep(150000);

		l = led->opt.recv(buf, sizeof(buf));
		if (l > 0) {
			DEBUG("Rcvd %d bytes, %s", l, buf);
			printmem(buf, l);
			if (0 == voice_chk_ask(led, buf, l)) {
				return 0;
			}
		}
	}while (--retry);

	return -1;
}


static serial_t serial = { 0 };

int32_t fly_eq20131_serial_open(void)
{
	serial_cfg_t cfg = {
		.name = "/dev/ttyO4",
		.baud = 9600,
		.datasz = 8,
		.paity = SERIAL_PAITY_NONE,
		.stop_bit = 1
	};

	return serial_open(&serial, &cfg);
}

int32_t fly_eq20131_serial_close(void)
{
	return serial_close(&serial);
}

ssize_t fly_eq20131_serial_send(const uint8_t *data, size_t size)
{
	return serial_send(&serial, data, size);
}

ssize_t fly_eq20131_serial_recv(uint8_t *buf, size_t size)
{
	return serial_recv_timeout(&serial, buf, size, 500);
}

void rs485_test(void)
{

	int32_t fd;
	char buf[128];
	ssize_t len;

	fd = Open_Port(4, 'O', O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		ALERT("open cvd port failed!");
		return;
	}

	if (Set_Opt(fd, 9600, 8, 'N', 1) < 0) {
		ALERT("Set cvd port option failed !");
		close(fd);
		return;
	}

	for (;;) {
		len = snprintf(buf, sizeof(buf), "1234567890");
		write(fd, buf, len);
		DEBUG("Send %s", buf);
		usleep(150000);
		memset(buf, 0, sizeof(buf));
		len = read(fd, (uint8_t *)buf, sizeof(buf));
		if (len > 0) {
			DEBUG("RS485 rcvd %d bytes", len);
			printmem(buf, len);
		}
		sleep(1);
	}
}


C_CODE_END
