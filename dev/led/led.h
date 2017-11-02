#ifndef __LED_H
#define __LED_H

#include <endian.h>
#include "types.h"
#include "list.h"

C_CODE_BEGIN

#define LED_DEBUG	1

#define LED_TEXT_MAX_LEN	(1024)
#define LED_AUDIO_MAX_LEN		(1024 * 512)
#define LED_MAX_LINES_NUM		16

enum {
	LED_STYLE_IMMEDIATE = 0,
	LED_STYLE_SHIFT_LEFT,
	LED_STYLE_END
};

enum {
	LED_COLOR_RED = 1,
	LED_COLOR_END
};

#define LED_ACTION_CLR	0x01
#define LED_ACTION_DISP		0x02
#define LED_ACTION_AUDIO		0x04

enum {
	LED_MODE_LINE = 0,
	LED_MODE_PAGE,
	LED_MODE_END
};

typedef union color {
	uint32_t rgb;

	struct {

#if __BYTE_ORDER == __LITTLE_ENDIAN
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t rsvd;
#else
		uint8_t rsvd;
		uint8_t r;
		uint8_t g;
		uint8_t b;
#endif
	};
} color_t;

/**
 * struct led_line Led line's data
 *
 * @idx Index
 * @mode Page display or line display
 * @action Led action, clear or display
 * @style Display style
 * @duration Led display duration
 * @rgb Led display color rgb value
 * @volume Audio play volume
 * @time_per_col Run a column elapsed time
 * @text Led display text
 * @audio Audio data encode by base64
 */
typedef struct led_line {
	union {
		list_head_t link;
		hlist_node_t hlink;
	};

	uint8_t idx;
	uint8_t mode;
	uint8_t action;
	uint8_t style;
	uint8_t duration;
	color_t color;
	uint8_t volume;
	uint16_t time_per_col;
	bool cover;
	char text[LED_TEXT_MAX_LEN];
	char audio[LED_AUDIO_MAX_LEN];
} led_line_t;

/**
 * struct led_screen - The screen data
 *
 * @mode Display mode, line or page mode, when in page mode
 *  	 there will be only one line
 * @volume Audio volume
 * @nlines Number of lines
 * @lines Lines data
 */
typedef struct led_screen {
	list_head_t lines;
} led_screen_t;


int_fast32_t led_line_param_chk(const led_line_t *line);
led_line_t *led_line_malloc(void);
void led_line_free(led_line_t *line);
bool led_line_have_action(const led_line_t *line, uint_fast32_t action);
void led_line_add_to_screen(led_screen_t *screen, led_line_t *line);
void led_line_move_to_screen(led_screen_t *screen, led_line_t *line);
void led_screen_free_lines(led_screen_t *screen);
void led_screen_show_lines(led_screen_t *screen);
void led_clear_line_init(led_line_t *line, int_fast32_t idx);

int_fast32_t rgb_str2bin(color_t *color, const char *str);
ssize_t rgb_bin2str(char *buf, size_t bufsz, const color_t *color);

C_CODE_END

#endif
