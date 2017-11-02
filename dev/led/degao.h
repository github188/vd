#ifndef __TABITCOM_CVD_H
#define __TABITCOM_CVD_H

#include "types.h"
#include "sys/time_util.h"

C_CODE_BEGIN

#define CVD_STX		0x7F
#define CVD_NOT_MOVE		0xFF

typedef enum cvd_disp_mode{
	CVD_DISP_STATIC,
	CVD_DISP_DYNAMIC
}cvd_disp_mode_t;

typedef enum cvd_cmd{
	CVD_GET_STATE = 0x40,
	CVD_REFRESH_STATIC,
	CVD_SET_PARAM,
	CVD_SET_MOVE = 0x44,
	CVD_REFRESH_DISP,
	CVD_SET_PARAM1,
	CVD_SET_RIGHT,
	CVD_SET_UP,
	CVD_SET_FLUSH,
	CVD_REFRESH = 0x50,
	CVD_SET_DOWN = 0x51,
	CVD_SET_MODE = 0x56,
	CVD_GET_VERSION = 0xEE
}cvd_cmd_t;

typedef struct cvd_hdr {
	uint8_t stx;
	uint8_t addr;
	uint8_t len;
	uint8_t cmd;
}__attribute__((aligned(1))) cvd_hdr_t;

typedef struct cvd_cs {
	uint8_t xorx;
	uint8_t sum;
}__attribute__((aligned(1))) cvd_cs_t;

typedef struct cvd_param{
	uint16_t width;
	uint16_t high;
	uint8_t color;
	uint8_t mode;
	uint8_t speed;
}cvd_param_t;

typedef struct cvd {
	struct timespec prev_time;
	uint8_t addr;
	cvd_param_t param;
	int32_t sz_max;
	ssize_t (*putnbyte)(const uint8_t *data, size_t len);
}cvd_t;

extern void cvd_init(cvd_t *cvd, uint8_t addr, 
					 ssize_t (*putnbyte)(const uint8_t *data, size_t len));
void cvd_disp(cvd_t *cvd, const char *str, uint8_t speed);
void cvd_clear(cvd_t *cvd);
void cvd_reset(cvd_t *cvd);
extern ssize_t cvd_putnbyte_default(const uint8_t *data, size_t len);


C_CODE_END


#endif
