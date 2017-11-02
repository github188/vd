#include "degao.h"
#include "stdio.h"
#include "stdlib.h"
#include "serial_ctrl.h"
#include "string.h"
#include "commonfuncs.h"
#include "sys/time_util.h"
#include "ctype.h"
#include "logger/log.h"

C_CODE_BEGIN

static uint8_t cvd_xor(const uint8_t *data, size_t len);
static uint8_t cvd_sum(const uint8_t *data, size_t len);
static ssize_t cvd_esc(uint8_t *dest, size_t sz_dest,
					   const uint8_t *src, size_t src_len);

ssize_t cvd_putnbyte_default(const uint8_t *data, size_t len)
{
	int32_t fd;

	fd = Open_Port(4, 'O', O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		ALERT("open cvd port failed!");
		return -1;
	}

	if(Set_Opt(fd, 19200, 8, 'N', 1) < 0){
		ALERT("Set cvd port option failed !");
		close(fd);
		return -2;
	}

	write(fd, data, len);
	close(fd);

	INFO("send data to cvd success at %s.", ustime_msec());

	return 0;
}

#if 0
static void print_buf(const uint8_t *buf, size_t len)
{
	const uint8_t *p = buf;
	const uint8_t *end = buf + len;

	INFO("echo buffer, the length is %d.", len);

	for (; p < end; ++p) {
		INFO("0x%02X", *p);
	}
}
#endif

static void cvd_send(struct cvd *cvd, cvd_cmd_t cmd, 
					 const uint8_t *data, size_t len)
{
	uint8_t buf[512], tx_buf[512];
	ssize_t tx_len;
	uint8_t *var;
	cvd_hdr_t *hdr;
	cvd_cs_t *cs;
	struct timespec curr;
	int64_t diff;
	const int64_t diff_time_max = 1250;

	hdr = (cvd_hdr_t *)buf;
	var = buf + sizeof(cvd_hdr_t);

	if (len > (sizeof(buf) - sizeof(cvd_hdr_t))) {
		return;
	}

	memcpy(var, data, len);
	hdr->stx = CVD_STX;
	hdr->addr = 0x80 + cvd->addr;
	hdr->len = len + sizeof(hdr->cmd);
	hdr->cmd = cmd;

	cs = (cvd_cs_t *)(buf + sizeof(cvd_hdr_t)+ len);
	cs->xorx = cvd_xor(buf + sizeof(hdr->stx),
					   sizeof(cvd_hdr_t)+ len - sizeof(hdr->stx));
	cs->sum = cvd_sum(buf + sizeof(hdr->stx),
					  sizeof(cvd_hdr_t)+ len - sizeof(hdr->stx));
	tx_len = cvd_esc(tx_buf, sizeof(tx_buf),
					 buf, sizeof(cvd_hdr_t)+ len + sizeof(cvd_cs_t));

	if (tx_len > 0) {
		if(0 == clock_gettime(CLOCK_MONOTONIC, &curr)) {
			diff = timespec_diff_msec(&curr, &cvd->prev_time);
			cvd->prev_time.tv_sec = curr.tv_sec;
			cvd->prev_time.tv_nsec = curr.tv_nsec;
		} else {
			diff = 0;
		}

		if (diff < diff_time_max) {
			INFO("Degao led delay %d ms to display", diff_time_max - diff);
			usleep((diff_time_max - diff) * 1000);
		}

		cvd->putnbyte(tx_buf, tx_len);
	}
}

void cvd_reset(cvd_t *cvd)
{
	uint8_t cmd[] = { 0x7F, 0x81, 0x04, 0x56, 0x01, 0x05, 0xFF, 0x28, 0xE0 };
	cvd->putnbyte(cmd, sizeof(cmd));
}

void cvd_static_disp(cvd_t *cvd, const char *str)
{
	size_t len, front_space_len, tail_space_len;
	char *c, *end;
	char tmp[32] = {0};
	int32_t cnt;
	int32_t fill_adjust_tab[5] = {0, 1, 0, 1, 0};

	INFO("the disp str is %s.", str);

	/* 计算字符串长度 */
	len = strlen(str);

	/* 居中显示时后面要补空格的个数, sz_max = 8(LED最多显示8个字符) */
	tail_space_len = (cvd->sz_max - len) / 2;

	/*
	 * 补的空格不足四个, 可能造成被截断,需要处理后四个字节
	 * 统计后四个字节中非填充空格的非ascii字符个数
	 */
	end = (char *)str + len;
	/* 从四字节中非填充的字符开始遍历 */
	c = end - (4 - tail_space_len);
	cnt = 0;
	for (; c < end; ++c) {
		if (!isascii(*c)) {
			++cnt;
		}
	}

	INFO("the count of gb2312 char is %d.", cnt);
	INFO("the tail should add %d spaces.", fill_adjust_tab[cnt]);

	/* 根据后四字节的非ascii码个数调整后面的空格数 */
	tail_space_len += fill_adjust_tab[cnt];

	/* 计算前面的空格数 */
	front_space_len = cvd->sz_max - len - tail_space_len;
	/* 共16字节, 静态显示需要填充前面8个字节为空格 */
	front_space_len += 8;

	INFO("the len of front space is %d.", front_space_len);
	INFO("the len of tail space is %d.", tail_space_len);

	c = tmp;
	end = tmp + front_space_len;
	for (; c < end; ++c) {
		*c = ' ';
	}

	strcpy(c, str);
	c += len;

	end = c + tail_space_len;
	for (; c < end; ++c) {
		*c = ' ';
	}

	INFO("the str send to cvd is %s.", tmp);

	cvd_send(cvd, CVD_REFRESH_STATIC, (const uint8_t *)tmp,
			 front_space_len + len + tail_space_len);
}

static void cvd_dynamic_disp(cvd_t *cvd, const char *str, uint8_t speed)
{
	struct {
		uint8_t speed;
		uint8_t buf[1024];
	} __attribute__((aligned(1))) dynamic;

	
	if ((strlen(str) + 1) > sizeof(dynamic.buf)) {
		ERROR("cvd disp buf is overflow!");
		return;
	}

	dynamic.speed = speed;
	strcpy((char *)dynamic.buf, str);
	cvd_send(cvd, CVD_SET_MOVE, (const uint8_t *)&dynamic,
			  sizeof(dynamic.speed) + strlen(str));
}

void cvd_disp(cvd_t *cvd, const char *str, uint8_t speed)
{
	cvd_dynamic_disp(cvd, str, speed);
}

void cvd_clear(cvd_t *cvd)
{
	char clr[32];
	
	memset(clr, 0x20, cvd->sz_max);
	clr[cvd->sz_max] = 0;
	cvd_static_disp(cvd, clr);
}


static uint8_t cvd_xor(const uint8_t *data, size_t len)
{
	const uint8_t *p = data;
	const uint8_t *end = data + len;
	uint8_t cs = 0;

	for ( ; p < end; ++p) {
		cs ^= *p;
	}

	return cs;
}

static uint8_t cvd_sum(const uint8_t *data, size_t len)
{
	const uint8_t *p = data;
	const uint8_t *end = data + len;
	uint8_t cs = 0;

	for ( ; p < end; ++p) {
		cs += *p;
	}

	return cs;
}

static ssize_t cvd_esc(uint8_t *dest, size_t sz_dest,
					   const uint8_t *src, size_t src_len)
{
	const uint8_t *s = src;
	const uint8_t *src_end = src + src_len;
	uint8_t *d = dest;
	uint8_t *dest_end = dest + sz_dest;

	/* 跳过起始符 */
	*d++ = *s++;
	while ((s < src_end) && (d < dest_end)) {
		if (CVD_STX == *s) {
			*d++ = CVD_STX;
		}
		*d++ = *s++;
	}

	return (d == dest_end) ? -1 : d - dest;
}

void cvd_init(cvd_t *cvd, uint8_t addr, 
			  ssize_t (*putnbyte)(const uint8_t *data, size_t len))
{
	cvd->addr = addr;
	cvd->sz_max = 8;
	cvd->putnbyte = putnbyte;

	if(-1 == clock_gettime(CLOCK_MONOTONIC, &cvd->prev_time)){
		cvd->prev_time.tv_sec = 0;
		cvd->prev_time.tv_nsec = 0;
	}
}

C_CODE_END
