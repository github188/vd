#ifndef _BITCOM_PRO_H_
#define _BITCOM_PRO_H_

#include <errno.h>
#include "traffic_records_process.h"
#include "ve/dev/led.h"

/* The version of down tunnel protocol */
#define DN_MAJOR_VERSION	1
#define DN_MINOR_VERSION		0

#define PRO_ENCODE	(1)
#define PRO_DECODE	(2)

#define PAB_HTTP_PRO	(1)
#define PAB_TCP_PRO		(2)

#define JSON_LENS	(1024 * 500)


enum {
	HTTP_CODE_201 = 201,
	HTTP_CODE_END
};

#define HTTP_CODE_201_STR	"HTTP/1.1 201 created"

enum {
	VE_DTS_TYPE_CRTL = 1,
	VE_DTS_TYPE_QUERY,
	VE_DTS_TYPE_DOWNLOAD,
	VE_DTS_TYPE_ASK = 100
};

enum {
	VE_DTS_RG_UP = 1,
	VE_DTS_RG_DN,
	VE_DTS_RG_STOP,
	VE_DTS_RG_NO,
	VE_DTS_RG_NC,
	VE_DTS_RG_NC_CLR,
	VE_DTS_RG_END
};

enum {
	VE_DTS_RG_AUTO = 1,
	VE_DTS_RG_MANUAL,
	VE_DTS_RG_OTHRER,
	VE_DTS_RG_TYPE_END
};

typedef enum ve_bitcom_fault{
	VE_BITCOM_FAULT_NETWORK = 2,
	VE_BITCOM_FAULT_RESTART = 15
}ve_bitcom_fault_t;

typedef struct
{
	DB_TrafficRecord	*db_traffic_record;
	EP_PicInfo	*pic_info;
	char http_type;
}str_bitcom_http_pro;

typedef struct
{
	int valid_flag;
	int barriergate;
	int bgkeeptime;
	int bgmode;
}str_pab_roadgate;

typedef struct
{
	uint8_t valid_flag;
	uint8_t nlines;
	led_line_t *lines[LED_MAX_LINES_NUM];
}str_pab_led;

typedef struct
{
	int valid_flag;
	char content[128];
	int volume;
}str_pab_voice;

typedef struct
{
	int valid_flag;
	int mode;
	int reg;
	int picnum;
}str_pab_capture;

typedef struct pab_lock{
	bool valid;
	int32_t action;
}pab_lock_t;

typedef struct vip_info {
	bool valid;
	char *data;		/* Vip data, a string of json */
	size_t data_len;		/* The data length */
}vip_info_t;

typedef struct
{
	char hdr[32];
	char daddr[32];
	int32_t type;

	str_pab_roadgate roadgate;
	str_pab_led led;
	str_pab_voice voice;
	str_pab_capture capture;
	pab_lock_t lock;
	vip_info_t vip;
	struct {
		int32_t major;
		int32_t minor;
	}ver;
}str_bitcom_pro;


typedef struct http_client{
	pthread_mutex_t mutex;
	int32_t sockfd;
	char ip[32];
	uint16_t port;
	int32_t retry;
	bool tcplong;
	int32_t recv_timeout;
	int32_t send_timeout;
}http_client_t;

typedef struct dts_head {
	char sof[32];
	char daddr[32];
	int32_t type;
	uint32_t cs;
	size_t data_len;
	size_t head_len;
}dts_head_t;


int alleyway_bitcom_protocol(char *ac_buff, int ai_lens, void *aos_bitcom_pro, char ac_protype, char ac_prostyle);
void http_client_init(http_client_t *hc, const char *ip, uint16_t port,
					  bool tcplong, int32_t retry, int32_t recv_timeout);
int32_t http_send_recv(http_client_t *hc,
					   const uint8_t *send_buf, size_t send_len,
					   uint8_t *recv_buf, size_t sz_recv_buf);
void http_client_set_server_param(http_client_t *hc,
								  const char *ip, uint16_t port);

uint32_t get_sum_chk(const uint8_t *data, size_t len);
int32_t dts_get_frm_info(const uint8_t *hdr, size_t len, dts_head_t *info);
const uint8_t *dts_find_head(const uint8_t *data, size_t len);





#endif

