#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include "types.h"
#include "logger/log.h"
#include "led.h"
#include "listen.h"
#include "sys/xstring.h"

C_CODE_BEGIN

#define A4_LED_LINES_NUM	4

#define UPDATE_OPT_CODE	0xDA00
#define PARAM_CFG_CODE		0xC100

/*
 * Ask code
 */
#define ASK_EBUSY	0x00000000
#define ASK_SUCCESS		0x00000001
#define ASK_ETOOLONG	0x000000A1
#define ASK_EINVAL		0x000000A2
#define ASK_ESOF		0x000000A6
#define ASK_EOPT		0x00000084
#define ASK_EFSN		0x00000082
#define ASK_EHWBUSY		0x0000DAA3
#define ASK_EINOPT		0x0000DAAB
#define ASK_EFW		0x0000DAB1
#define ASK_EPARAM		0x0000DAB8
#define ASK_ESHOWRANGE		0x0000DABE
#define ASK_ESHOWSIZE		0x0000DAB5
#define ASK_ESHOWNO		0x0000DABE
#define ASK_ECFGSMALL		0x0000C1B1
#define ASK_ECFGNOCHNG		0x0000C1BF
#define ASK_EINSIZE		0x0000C1B9
#define ASK_ESIZERANGE		0x0000C1BA
#define ASK_ECOLOR		0x0000C1BB
#define ASK_OEDA		0x0000C1C5
#define ASK_ETIME		0x0000C1BD

#define STYLE_IMMEDIATE	1
#define STYLE_SHIFT_LEFT		97
#define STYLE_SHIFT_RIGHT		96
#define STYLE_SHIFT_UP		94
#define STYLE_SHIFT_DN		95

#define TIME_TO_A4_SPEED(ms)	((ms) > (5 * 64) ? 64 : (ms) / 5)

typedef struct req_hdr{
	uint8_t sof[4];
	uint8_t addr;
	uint8_t flag;
	uint16_t opt;
	uint16_t rsvd;
	uint32_t sn;
	uint32_t len;
	uint16_t frm_len;
	uint8_t data[1];
} __attribute__((packed)) req_hdr_t;

typedef struct req_tail{
	uint8_t eof[4];
} __attribute__((packed)) req_tail_t;

typedef struct ask_hdr{
	uint8_t sof[4];
	uint8_t addr;
	uint8_t flag;
	uint16_t opt;
	uint16_t rsvd;
	uint32_t sn;
	uint32_t len;
	uint16_t frm_len;
	uint8_t data[1];
} __attribute__((packed)) ask_hdr_t;

typedef struct ask_tail{
	uint8_t eof[4];
} __attribute__((packed)) ask_tail_t;

typedef struct ask_err{
	uint32_t errno;
} __attribute__((packed)) ask_err_t;


typedef struct disp_data {
	uint8_t model[16];
	uint16_t width;
	uint16_t high;
	uint8_t color;
	uint8_t nr_show;
	uint8_t rsvd[8];
	uint8_t data[1];
} __attribute__((packed)) disp_data_t;

typedef struct show_data {
	uint8_t id;
	uint32_t len;
	uint8_t nr_region;
	uint16_t play_time;
	uint8_t loop_times;
	uint8_t rsvd[16];
	uint8_t data[1];
} __attribute__((packed)) show_data_t;

typedef struct region_data {
	struct {
		uint8_t fob :1;
		uint8_t :7;
	} type;
	uint32_t len;
	uint8_t type1;
	uint16_t x;
	uint16_t y;
	uint16_t xx;
	uint16_t yy;
	union {
		struct {
			uint8_t red :1;
			uint8_t green :1;
			uint8_t blue :1;
			uint8_t :5;
			uint8_t rsvd[2];
		} other;
		struct {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		} full;
	} color;
	uint8_t in_style;
	uint8_t speed;
	uint16_t duration;
	uint8_t lattice;
	uint32_t text_len;
	uint8_t text[1];
} __attribute__((packed)) region_data_t;

typedef struct region_pos {
	uint8_t x;
	uint8_t y;
	uint8_t width;
	uint8_t high;
} region_pos_t;

typedef struct param_cfg_mask {
	uint8_t base :1;
	uint8_t :3;
	uint8_t datetime :1;
	uint8_t sw :1;
	uint8_t :2;
	uint8_t rsvd[7];
} __attribute__((packed)) param_cfg_mask_t;

typedef struct param_cfg_datetime {
	uint16_t year;
	uint8_t mon;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} __attribute__((packed)) param_cfg_datetime_t;

typedef struct ask_text_item {
	uint32_t code;
	const char *text;
} ask_text_item_t;

static const uint8_t line_y_tab[A4_LED_LINES_NUM] = { 0, 16, 32, 48 };

static const ask_text_item_t ask_text[] = {
	{ ASK_EBUSY, "Communication is busing" },
	{ ASK_SUCCESS, "Success" },
	{ ASK_ETOOLONG, "The length of real frame is lager than told to A4" },
	{ ASK_EINVAL, "Invalid value" },
	{ ASK_ESOF, "Start of frame is error" },
	{ ASK_EOPT, "An error option code" },
	{ ASK_EFSN, "An error frame serial number" },
	{ ASK_EHWBUSY, "Hardware is busing while refresh display" },
	{ ASK_EINOPT, "An invalid option code " },
	{ ASK_EFW, "The length of firmware is too small" },
	{ ASK_EPARAM, "Parameter is not matched with A4" },
	{ ASK_ESHOWRANGE, "Show number is out of range or is zero" },
	{ ASK_ESHOWSIZE, "Show file is too large" },
	{ ASK_ESHOWNO, "Show number is not supported" },
	{ ASK_ECFGSMALL, "Configuration is too small" },
	{ ASK_ECFGNOCHNG, "Configuration is not changed" },
	{ ASK_EINSIZE, "An illegal screen size, shouled be 8n * 2n" },
	{ ASK_ESIZERANGE, "Screen is out of range" },
	{ ASK_ECOLOR, "The color is not supported" },
	{ ASK_OEDA, "The oeda parameter is not defined" },
	{ ASK_ETIME, "The set time is illegal" }
};


static void printmem(const void *mem, size_t size)
{
	(void)mem;
	(void)size;

#if LISTEN_LED_DEBUG

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

#endif
}


static const char *get_ask_code_text(int_fast32_t code)
{
	const ask_text_item_t *p = ask_text;
	const ask_text_item_t *end = ask_text + numberof(ask_text);

	for (; p < end; ++p) {
		if ((p->code == code)) {
			return p->text;
		}
	}

	return "Unknown ask code";
}

static ssize_t make_region(listen_led_t *led, uint8_t *buf, size_t bufsz,
						   const led_line_t *line, const region_pos_t *pos)
{
	region_data_t *region;

	ASSERT(buf);
	ASSERT(led);
	ASSERT(line);

	ASSERT(led->cfg.lattice == 16);

	region = (region_data_t *)buf;

	region->text_len = strlen(line->text);
	region->len = sizeof(*region) - 1 + region->text_len;
	if (region->len > bufsz) {
		return -1;
	}

	region->type.fob = 1;
	region->type1 = 0x0E;
	region->color.other.red = 1;
	region->color.other.green = 0;
	region->color.other.blue = 0;
	region->lattice = led->cfg.lattice;
	region->speed = TIME_TO_A4_SPEED(line->time_per_col);
	region->duration = 65533;
	memcpy(region->text, line->text, region->text_len);

	switch (line->style) {
		case LED_STYLE_IMMEDIATE:
			region->in_style = STYLE_IMMEDIATE;
			region->x = pos->x;
			region->y = pos->y;
			region->xx = pos->x + pos->width - 1;
			region->yy = pos->y + pos->high - 1;
			break;

		default:
			/* Use shift left as default */
			region->in_style = STYLE_SHIFT_LEFT;
			region->x = 0;
			region->y = pos->y;
			region->xx = led->cfg.width - 1;
			region->yy = pos->y + pos->high - 1;
			break;
	}

	DEBUG("Region, x: %d, y: %d, xx: %d, yy: %d",
		  region->x, region->y, region->xx, region->yy);
	return region->len;
}

static ssize_t make_show(listen_led_t *led, uint8_t *buf, size_t bufsz,
						 const led_screen_t *screen)
{
	ASSERT(buf);
	ASSERT(screen);
	ASSERT(led);

	show_data_t *show = (show_data_t *)buf;

	int_fast32_t nr_row = led->cfg.high / led->cfg.lattice;
	int_fast32_t c_width = led->cfg.lattice / 2;
	int_fast32_t nr_col = led->cfg.width / c_width;

	/* Adjust to max support lines */
	if (nr_row > A4_LED_LINES_NUM) {
		nr_row = A4_LED_LINES_NUM;
	}

	show->id = 0;
	show->nr_region = nr_row;
	show->play_time = 100;
	show->loop_times = 11;
	memset(show->rsvd, 0, sizeof(show->rsvd));
	show->len = sizeof(*show) - 1;

	typedef struct line_buf {
		led_line_t line;
		region_pos_t pos;
	}line_buf_t;

	static line_buf_t lines_buf[A4_LED_LINES_NUM];
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	/* Lock mutex */
	pthread_mutex_lock(&mutex);

	list_head_t *pos, *n;
	list_for_each_safe(pos, n, &screen->lines) {

		led_line_t *line = list_entry(pos, led_line_t, link);
		if (line->idx < nr_row) {

			/* Get pointer of previous line */
			line_buf_t *prev_line = &lines_buf[line->idx];

			/*
			 * Line's information
			 */
			if (line->action & LED_ACTION_CLR) {
				strlcpy(line->text, " ", sizeof(line->text));
			}

			memcpy(prev_line, line, sizeof(*prev_line));

			/*
			 * Start point position
			 */
			prev_line->pos.high = led->cfg.lattice;
			prev_line->pos.y = line_y_tab[line->idx];
			size_t len = strlen(line->text);
			if (len > nr_col) {
				prev_line->pos.width = led->cfg.width;
				prev_line->pos.x = 0;
				prev_line->line.style = LED_STYLE_SHIFT_LEFT;
			} else {
				prev_line->pos.x = (nr_col - len) / 2 * c_width;
				prev_line->pos.width = len * c_width;
			}
		}
	}

	uint8_t *p = show->data;
	size_t remain = bufsz - sizeof(*show) + 1;

	for (uint_fast32_t i = 0; i < nr_row; ++i) {

		line_buf_t *line_buf = &lines_buf[i];

		size_t len = make_region(led, p, remain, &line_buf->line,
								 &line_buf->pos);
		if (len <= 0) {
			return -1;
		}

		remain -= len;
		p += len;
		show->len += len;
	}

	/* Unlock mutex */
	pthread_mutex_unlock(&mutex);

	DEBUG("The length of show is %d", show->len);

	return show->len;
}

static ssize_t make_disp(listen_led_t *led, uint8_t *buf, size_t bufsz,
						 const led_screen_t *screen)
{
	disp_data_t *disp;
	size_t remain;
	ssize_t len;

	ASSERT(led);
	ASSERT(buf);

	disp = (disp_data_t *)buf;

	memcpy(disp->model, led->cfg.model, sizeof(disp->model));
	disp->high = led->cfg.high;
	disp->width = led->cfg.width;
	disp->color = 0x01;
	/* Only need 1 show */
	disp->nr_show = 1;

	DEBUG("Width: %d, high: %d", disp->width, disp->high);

	remain = bufsz - sizeof(*disp) + 1;
	len = make_show(led, disp->data, remain, screen);
	if (len <= 0) {
		return -1;
	}

	DEBUG("The length of disp is %d",(sizeof(*disp) - 1 + len));

	return (sizeof(*disp) - 1 + len);
}

static ssize_t make_req_frm(listen_led_t *led, uint8_t *buf, size_t bufsz,
							const led_screen_t *screen)
{
	req_hdr_t *req;
	req_tail_t *tail;
	size_t remain;
	ssize_t len;

	ASSERT(buf);
	ASSERT(led);
	ASSERT(screen);

	req = (req_hdr_t *)buf;

	remain = bufsz - (sizeof(*req) - 1) - sizeof(*tail);

	len = make_disp(led, req->data, remain, screen);
	if (len <= 0) {
		return -1;
	}

	req->sof[0] = 0x55;
	req->sof[1] = 0xAA;
	req->sof[2] = 0x00;
	req->sof[3] = 0x00;
	req->addr = led->cfg.addr;
	req->rsvd = 0;
	req->flag = 0x01;
	req->opt = UPDATE_OPT_CODE;
	req->sn = 0;
	req->frm_len = len;
	req->len = len;

	tail = (req_tail_t *)(req->data + len);

	tail->eof[0] = 0x00;
	tail->eof[1] = 0x00;
	tail->eof[2] = 0x0D;
	tail->eof[3] = 0x0A;

	DEBUG("The length of req is %d", req->len);

	return (sizeof(*req) - 1 + len + sizeof(*tail));
}

static ssize_t make_update_time(listen_led_t *led, uint8_t *buf, size_t bufsz)
{
	param_cfg_mask_t *mask;
	param_cfg_datetime_t *datetime;

	(void)led;

	ASSERT(buf);
	ASSERT(led);
	ASSERT(bufsz >= (sizeof(*mask) + sizeof(*datetime)));

	mask = (param_cfg_mask_t *)buf;
	datetime = (param_cfg_datetime_t *)(buf + sizeof(param_cfg_mask_t));

	/*
	 * Set mask
	 */
	memset(mask, 0, sizeof(*mask));
	mask->datetime = 1;

	/*
	 * Set date time
	 */
	time_t rawtime;
	struct tm tm;

	time(&rawtime);
	localtime_r(&rawtime, &tm);

	datetime->year = tm.tm_year + 1900;
	datetime->mon = tm.tm_mon + 1;
	datetime->day = tm.tm_mday;
	datetime->hour = tm.tm_hour;
	datetime->min = tm.tm_min;
	datetime->sec = tm.tm_sec;

	return (sizeof(*mask) + sizeof(*datetime));
}

static ssize_t make_update_time_frm(listen_led_t *led, uint8_t *buf,
									size_t bufsz)
{
	req_hdr_t *req;
	req_tail_t *tail;
	size_t remain;
	ssize_t len;

	ASSERT(buf);
	ASSERT(led);

	req = (req_hdr_t *)buf;

	remain = bufsz - (sizeof(*req) - 1) - sizeof(*tail);

	len = make_update_time(led, req->data, remain);
	if (len <= 0) {
		return -1;
	}

	req->sof[0] = 0x55;
	req->sof[1] = 0xAA;
	req->sof[2] = 0x00;
	req->sof[3] = 0x00;
	req->addr = led->cfg.addr;
	req->rsvd = 0;
	req->flag = 0x01;
	req->opt = PARAM_CFG_CODE;
	req->sn = 0;
	req->frm_len = len;
	req->len = len;

	tail = (req_tail_t *)(req->data + len);

	tail->eof[0] = 0x00;
	tail->eof[1] = 0x00;
	tail->eof[2] = 0x0D;
	tail->eof[3] = 0x0A;

	DEBUG("The length of update time is %d", req->len);

	return (sizeof(*req) - 1 + len + sizeof(*tail));
}


static const uint8_t *ask_find_hdr(listen_led_t *led, const uint8_t *buf,
								   size_t len, uint32_t flag, uint32_t opt)
{
	const uint8_t *p, *end;
	ask_hdr_t *ask;

	ASSERT(buf);
	ASSERT(led);

	for (p = buf, end = p + len; p < end; ++p) {
		ask = (ask_hdr_t *)p;

		if ((0x55 == ask->sof[0]) && (0xAA == ask->sof[1])
			&& (0x00 == ask->sof[2]) && (0x00 == ask->sof[3])
			&& (ask->flag == flag) && (ask->opt == opt)) {
			return p;
		}
	}

	return NULL;
}

static bool ask_tail_is_pass(ask_tail_t *tail)
{
	return ((0x00 == tail->eof[0]) && (0x00 == tail->eof[1])
			&& (0x0D == tail->eof[2]) && (0x0A == tail->eof[3]));
}

static int32_t parse_ask_frm(listen_led_t *led, const uint8_t *data,
							 size_t len, uint32_t opt_code)
{
	ask_hdr_t *ask;
	ask_err_t *err;

	ASSERT(data);

	ask = (ask_hdr_t *)ask_find_hdr(led, data, len, 0, opt_code);
	if (NULL == ask) {
		ERROR("Find ask frame failed");
		return -1;
	}

	if (ask->frm_len != ask->len) {
		ERROR("Not support multi packets");
		return -1;
	}

	if (ask->frm_len < sizeof(*err)) {
		ERROR("The length of ask frame is not support");
		return -1;
	}

	if (!ask_tail_is_pass((ask_tail_t *)(ask->data + ask->frm_len))) {
		ERROR("Ask tail is error");
		return -1;
	}

	err = (ask_err_t *)ask->data;
	if (err->errno != 1) {
		ERROR("Refresh A4 failed, ask code 0x%"PRIX32":%s",
			  err->errno, get_ask_code_text(err->errno));
		return -1;
	}

	return 0;
}

static void print_info(const uint8_t *buf)
{
#if LISTEN_LED_DEBUG
	req_hdr_t *req;
	disp_data_t *disp;
	show_data_t *show;
	region_data_t *region;

	req = (req_hdr_t *)buf;
	disp = (disp_data_t *)req->data;
	show = (show_data_t *)disp->data;
	region = (region_data_t *)show->data;

	DEBUG("Width: %d, high: %d", disp->width, disp->high);
	DEBUG("Color: %d", disp->color);

	DEBUG("Show id: %d", show->id);
	DEBUG("Show play time: %d", show->play_time);
	DEBUG("Show loop times: %d", show->loop_times);
	DEBUG("Show region count: %d", show->nr_region);

	for (int32_t i = 0; i < show->nr_region; ++i) {
		DEBUG("region[%d] type: %d", i, region->type);
		DEBUG("region[%d] len: %d", i, region->len);
		DEBUG("region[%d] type1: %d", i, region->type1);
		DEBUG("region[%d] x: %d, y: %d", i, region->x, region->y);
		DEBUG("region[%d] xx: %d, yy: %d", i, region->xx, region->yy);
		DEBUG("region[%d] color: %d", i, region->color);
		DEBUG("region[%d] speed: %d", i, region->speed);
		DEBUG("region[%d] duration: %d", i, region->duration);
		DEBUG("region[%d] size: %d", i, region->lattice);
		DEBUG("region[%d] text len: %d", i, region->text_len);
		DEBUG("region[%d] text: %s", i, region->text);

		region = (region_data_t *)((uint8_t *)region + region->len);
	}
#endif
}

int32_t listen_open(listen_led_t *led)
{
	serial_cfg_t cfg = {
		.name = "/dev/ttyO4",
		.baud = 9600,
		.datasz = 8,
		.paity = SERIAL_PAITY_NONE,
		.stop_bit = 1
	};

	return serial_open(&led->serial, &cfg);
}

int32_t listen_close(listen_led_t *led)
{
	return serial_close(&led->serial);
}

int32_t listen_recv(listen_led_t *led, uint8_t *buf, size_t size)
{
	return serial_recv(&led->serial, buf, size);
}

int32_t listen_send(listen_led_t *led, const uint8_t *buf, size_t len)
{
	return serial_send(&led->serial, buf, len);
}

int32_t listen_led_refresh(listen_led_t *led, const led_screen_t *screen)
{
	if(0 != led->opt.open(led)){
		return -1;
	}

	uint8_t buf[2048];
	int32_t ret = -1;

	ssize_t len = make_req_frm(led, buf, sizeof(buf), screen);
	if (len > 0) {
		print_info((const uint8_t *)buf);
		led->opt.send(led, buf, len);
		usleep(450 * 1000);
		len = led->opt.recv(led, buf, sizeof(buf));
		if (len > 0) {
			DEBUG("Rcvd %d bytes from listen A4", len);
			printmem(buf, len);
			ret = parse_ask_frm(led, buf, len, UPDATE_OPT_CODE);
		}
	}

	led->opt.close(led);

	return ret;
}

/**
 * Update date time for listen a4 card
 *
 * @author cyj (2016/7/21)
 *
 * @param led
 *
 * @return int32_t
 */
int32_t listen_led_update_time(listen_led_t *led)
{
	uint8_t buf[2048];
	ssize_t len;
	int32_t ret;

	if(0 != led->opt.open(led)){
		return -1;
	}

	ret = -1;
	len = make_update_time_frm(led, buf, sizeof(buf));
	if (len > 0) {
		led->opt.send(led, buf, len);
		usleep(250 * 1000);
		len = led->opt.recv(led, buf, sizeof(buf));
		if (len > 0) {
			DEBUG("Rcvd %d bytes from listen a4", len);
			ret = parse_ask_frm(led, buf, len, PARAM_CFG_CODE);
		}
	}

	led->opt.close(led);

	return ret;
}

int32_t listen_led_clear(listen_led_t *led)
{
	led_screen_t screen;
	led_line_t lines[A4_LED_LINES_NUM];

	INIT_LIST_HEAD(&screen.lines);
	memset(lines, 0, sizeof(lines));

	for (int_fast32_t i = 0; i < numberof(lines); ++i) {
		lines[i].idx = i;
		strlcpy(lines[i].text, " ", sizeof(lines[i].text));
		lines[i].style = LED_STYLE_IMMEDIATE;
		led_line_add_to_screen(&screen, &lines[i]);
	}

	return listen_led_refresh(led, &screen);
}

void listen_led_init(listen_led_t *led, const listen_led_cfg_t *cfg,
					 const listen_led_opt_t *opt)
{
	ASSERT(opt);
	ASSERT(cfg);

	ASSERT(opt->open);
	ASSERT(opt->close);
	ASSERT(opt->recv);
	ASSERT(opt->send);
	memcpy(&led->cfg, cfg, sizeof(led->cfg));
	memcpy(&led->opt, opt, sizeof(led->opt));
}

#if 0
void listen_led_test(void)
{
	int32_t i = 0;
	listen_led_t led;
	listen_led_cfg_t cfg;
	listen_led_opt_t opt;
	led_line_t lines[] = {
		{
			.text = "特易停",
			.lattice = 16,
			.in_style = LED_STYLE_IMMEDIATE
		},
		{
			.text = "鲁B12345",
			.lattice = 16,
			.in_style = LED_STYLE_IMMEDIATE
		}
	};

	led_screen_t screen = {
		.lines = lines,
		.nline = numberof(lines)
	};

	cfg.addr = 1;
	cfg.high = 32;
	cfg.width = 64;
	cfg.lattice = 16;
	opt.open = listen_open;
	opt.close = listen_close;
	opt.send = listen_send;
	opt.recv = listen_recv;

	listen_led_init(&led, &cfg, &opt);

	for(;;) {
		snprintf(lines[0].text, sizeof(lines[0].text), "%s", "特易停");
		snprintf(lines[1].text, sizeof(lines[1].text), "鲁B%05d", i % 100000);
		listen_led_refresh(&led, &screen);
		++i;
		usleep(500 * 1000);
	}

}
#endif

C_CODE_END
