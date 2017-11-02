#include <unistd.h>
#include "logger/log.h"
#include "types.h"
#include "dev/pcf8574.h"
#include "dev/gpio.h"
#include "ve/event.h"
#include "sys/xstring.h"

C_CODE_BEGIN

#define VE_RG_DN_WHEN_START	0

#define PCF8574_I2C_BUS		3
#define PCF8574_I2C_ADDR		0x38
#define RG_STOP_BIT		0
#define RG_DN_BIT		1
#define RG_UP_BIT		2
#define SAFE_COIL_GPIO		"gpio24"
#define DETECT_COIL_GPIO		"gpio72"

static const char *coil_text_tab[2] = {
	[0] = "detected",
	[1] = "undetected"
};

const char *get_coil_text(int_fast32_t value)
{
	if (value < numberof(coil_text_tab)) {
		return coil_text_tab[value];
	}

	return NULL;
}

void rg_down(void)
{
	event_item_t *ei = event_item_alloc();
	if (ei) {
		strlcpy(ei->ident, "gate", sizeof(ei->ident));
		strlcpy(ei->desc, "down", sizeof(ei->desc));
		event_put(ei);
	}

	pcf8574_set_bit(RG_DN_BIT, 0);
	usleep(100 * 1000);
	pcf8574_set_bit(RG_DN_BIT, 1);
}

void rg_set_up_signal(void)
{
	event_item_t *ei = event_item_alloc();
	if (ei) {
		strlcpy(ei->ident, "gate", sizeof(ei->ident));
		strlcpy(ei->desc, "up", sizeof(ei->desc));
		event_put(ei);
	}

	pcf8574_set_bit(RG_UP_BIT, 0);
	DEBUG("Roadgate set up signal.");
}

void rg_clr_up_signal(void)
{
	pcf8574_set_bit(RG_UP_BIT, 1);
	DEBUG("Roadgate clr up signal.");
}

void rg_stop(void)
{
	event_item_t *ei = event_item_alloc();
	if (ei) {
		strlcpy(ei->ident, "gate", sizeof(ei->ident));
		strlcpy(ei->desc, "stop", sizeof(ei->desc));
		event_put(ei);
	}

	pcf8574_set_bit(RG_STOP_BIT, 0);
	usleep(100 * 1000);
	pcf8574_set_bit(RG_STOP_BIT, 1);
	DEBUG("Roadgate stop.");
}

int_fast32_t rg_read_safe_coil(void)
{
	return gpio_get_filter(SAFE_COIL_GPIO, 5, 1);
}

int_fast32_t rg_read_in_coil(void)
{
	return gpio_get_filter(DETECT_COIL_GPIO, 5, 1);
}

void rg_down_s(void)
{
	if (1 == rg_read_safe_coil()) {
		INFO("Now is safe, put down the roadgate.");
		rg_clr_up_signal();
		rg_down();
	}
}

void rg_dev_init(void)
{
	pcf8574_set_bit(RG_DN_BIT, 1);
	pcf8574_set_bit(RG_UP_BIT, 1);
	pcf8574_set_bit(RG_STOP_BIT, 1);
}

C_CODE_END

