#ifndef __PFM_BITCOM_CFG_H
#define __PFM_BITCOM_CFG_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "types.h"
#include "json.h"

C_CODE_BEGIN

typedef struct pfm_bitcom_cfg{
	bool enable;
	bool ftp_enable;
	bool mq_enable;
	bool http_long;
	struct in_addr serv_ip;
	uint16_t up_port;
	uint16_t down_port;
	char id[32];
} pfm_bitcom_cfg_t;

int32_t pfm_bitcom_cfg_read_new(json_object *obj, void *arg);
json_object *pfm_bitcom_cfg_write_new(const void *arg);
void pfm_bitcom_cfg_init(pfm_bitcom_cfg_t *bc);

C_CODE_END

#endif
