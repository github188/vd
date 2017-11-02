#ifndef __EPLATE_H
#define __EPLATE_H

#include "types.h"
#include "pthread.h"

C_CODE_BEGIN

#define SZ_RS485_SEND_BUF_MAX		1024
#define SZ_EPLATE_ID		12
#define NR_EPLATE_MAX		8

#define EPLATE_GET_FRM_LEN(hdr)		\
	(sizeof(eplate_hdr_t) + ((eplate_hdr_t *)(hdr))->nr * SZ_EPLATE_ID + 1)

#define EPLATE_GET_ID_NUM(hdr)	(((eplate_hdr_t *)(hdr))->nr)


typedef enum{
	EPLATE_RS485_INPUT,
	EPLATE_RS485_OUTPUT
}eplate_rs485_dir_t;

typedef struct eplate_hdr{
	uint8_t header;
	uint8_t mac[6];
	uint8_t nr;
}__attribute__((aligned(1))) eplate_hdr_t;

typedef struct eplate_rs485{
	pthread_t thread_id;
	pthread_mutex_t send_mutex;
	uint8_t send_buf[SZ_RS485_SEND_BUF_MAX];
	size_t send_len;
}eplate_rs485_t;

typedef struct eplate{
	pthread_mutex_t mutex;
	eplate_rs485_t rs485;
	int32_t epc_cnt;
}eplate_t;


char eplate_get_status(void);
int32_t eplate_init(void);

C_CODE_END

#endif
