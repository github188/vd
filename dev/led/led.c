#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "led.h"
#include "logger/log.h"
#include "sys/heap.h"

C_CODE_BEGIN

static __inline__ void led_line_param_init(led_line_t *line);

led_line_t *led_line_malloc(void)
{
	led_line_t *line = (led_line_t *)xmalloc(sizeof(led_line_t));
	if (line != NULL) {
		led_line_param_init(line);
	}

	return line;
}

void led_line_free(led_line_t *line)
{
	xfree(line);
}

static __inline__ void led_line_param_init(led_line_t *line)
{
	ASSERT(line);
	line->text[0] = '\0';
	line->audio[0] = '\0';
}

int_fast32_t led_line_param_chk(const led_line_t *line)
{
	ASSERT(line);

	if (line->idx > LED_MAX_LINES_NUM) {
		ERROR("Led line's index error, %"PRIu8, line->idx);
		return -EINVAL;
	}

	if (line->style >= LED_STYLE_END) {
		ERROR("Led line's style error, %"PRIu8, line->style);
		return -EINVAL;
	}

	if (line->volume > 100) {
		ERROR("Led line's volume error, %"PRIu8, line->volume);
		return -EINVAL;
	}

	if (line->mode >= LED_MODE_END) {
		ERROR("Led line's mode error, %"PRIu8, line->mode);
		return -EINVAL;
	}

	if ((line->action & LED_ACTION_AUDIO) && (strlen(line->audio) == 0)) {
		ERROR("Led line's audio string error");
		return -EINVAL;
	}

	if ((line->action & LED_ACTION_DISP) && (strlen(line->text) == 0)) {
		ERROR("Led line's text string error");
		return -EINVAL;
	}

	return 0;
}

bool led_line_have_action(const led_line_t *line, uint_fast32_t action)
{
	return (line->action & (uint8_t)action);
}

void led_line_add_to_screen(led_screen_t *screen, led_line_t *line)
{
	list_add_tail(&line->link, &screen->lines);
}

void led_line_move_to_screen(led_screen_t *screen, led_line_t *line)
{
	list_move_tail(&line->link, &screen->lines);
}

void led_screen_show_lines(led_screen_t *screen)
{
	(void)screen;
#if LED_DEBUG
	list_head_t *pos, *n;
	int_fast32_t cnt = 0;

	DEBUG("%-8s%-16s", "idx", "text");
	list_for_each_safe(pos, n, &screen->lines){
		led_line_t *line = list_entry(pos, led_line_t, link);
		DEBUG("%-8d%-16s", cnt++, line->text);
	}
#endif
}

void led_screen_free_lines(led_screen_t *screen)
{
	list_head_t *pos, *n;

	list_for_each_safe(pos,n,&screen->lines) {
		led_line_t *line = list_entry(pos, led_line_t, link);
		list_del(&line->link);
		xfree(line);
	}
}

/**
 * @brief Convert css rgb format to color struct, example:
 *  	  "#FF0000" -> 0xFF0000
 *
 * @author chenyj (2017-03-31)
 *
 * @param color The destination struct
 * @param str css rgb string
 *
 * @return int_fast32_t
 */
int_fast32_t rgb_str2bin(color_t *color, const char *str)
{
	ASSERT(color);
	ASSERT(str);

	if ((color == NULL) || (str == NULL)) {
		return -EINVAL;
	}

	if (sscanf(str, "#%2"SCNxFAST8"%2"SCNxFAST8"%2"SCNxFAST8,
			   &color->r, &color->g, &color->b) == 3) {
		return 0;
	}

	return -1;
}

/**
 * @brief Convert color struct to css rgb string, example:
 *  	  0xFF0000 -> "#FF0000"
 *
 * @author chenyj (2017-03-31)
 *
 * @param buf The String buffer
 * @param bufsz The size of string buffer
 * @param color The color struct
 *
 * @return ssize_t Return the length of rgb string if success,
 *  	   otherwise return an error code
 */
ssize_t rgb_bin2str(char *buf, size_t bufsz, const color_t *color)
{
	ASSERT(color);
	ASSERT(buf);

	if ((color == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (bufsz < sizeof("#FFFFFF")) {
		return -ENOBUFS;
	}

	return sprintf(buf, "#%02"PRIX8"%02"PRIX8"%02"PRIX8,
				   color->r, color->g, color->b);
}

C_CODE_END
