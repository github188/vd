#ifndef __SERIAL_H
#define __SERIAL_H

#include "types.h"

C_CODE_BEGIN

typedef enum serial_paity{
	SERIAL_PAITY_NONE,
	SERIAL_PAITY_ODD,
	SERIAL_PAITY_EVEN,
	SERIAL_PAITY_MARK,
	SERIAL_PAITY_SPACE
}serial_paity_t;

typedef struct serial_cfg{
	const char *name;
	int32_t baud;
	serial_paity_t paity;
	int32_t datasz;
	int32_t stop_bit;
}serial_cfg_t;

typedef struct serial{
	int32_t fd;
}serial_t;

typedef ssize_t (*serial_send_fp)(const uint8_t *data, size_t size);
typedef ssize_t (*serial_recv_fp)(uint8_t *buf, size_t size);

int32_t serial_open(serial_t *serial, const serial_cfg_t *cfg);
ssize_t serial_send(serial_t *serial, const uint8_t *data, size_t size);
ssize_t serial_recv(serial_t *serial, uint8_t *buf, size_t size);
ssize_t serial_recv_timeout(serial_t *serial, uint8_t *buf, size_t size, 
							int32_t timeout);
int32_t serial_close(serial_t *serial);

C_CODE_END

#endif
