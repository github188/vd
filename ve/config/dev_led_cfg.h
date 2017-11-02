#ifndef __DEV_LED_CFG_H
#define __DEV_LED_CFG_H

#include "types.h"
#include "json_cfg.h"
#include "json.h"

C_CODE_BEGIN

enum {
	LED_MODEL_DEGAO = 0,
	LED_MODEL_FLY_EQ20131,
	LED_MODEL_LISTEN_A4,
	LED_MODEL_END
};

typedef struct led_cfg{
	uint8_t model;
	uint8_t reboot_hour;
	uint8_t reboot_wday;
	uint8_t lattice;
	uint8_t height;
	uint8_t width;
	bool reboot_enable;
	char user_content[128];
} led_cfg_t;

void dev_led_cfg_init(led_cfg_t *lc);
int32_t dev_led_cfg_read_new(json_object *obj, void *arg);
json_object *dev_led_cfg_write_new(const void *arg);


C_CODE_END

#endif
