/**
 * ve/dev/light.h
 *
 */
#ifndef __LIGHT_H
#define __LIGHT_H

#include "types.h"
#include "list.h"
#include "sys/xtimer.h"
#include "sys/time_util.h"


C_CODE_BEGIN

#define LIGHT_INFO_ENTRY_NUM	4

/**
 * If you want add a new light, add a item in this enum and call
 * light_regst to register it
 */
enum {
	LIGHT_ID_GREEN = 0,
	LIGHT_ID_RED,
	LIGHT_ID_BLUE,
	LIGHT_ID_WHITE,
	LIGHT_ID_END
};

typedef struct light light_t;

typedef struct light_info {
	uint_fast8_t value;
	uint_fast8_t intv;
} light_info_t;

typedef struct light_ctrl {
	/* Light's control information */
	light_info_t info[LIGHT_ID_END];	
	/* 
	 * Control mask, if you want control a light,
	 *  examples: mask |= (1 << light_id)
	 */
	uint_fast16_t mask;
} light_ctrl_t;

typedef struct light_opt {
	int_fast8_t (*set)(light_t *light, uint_fast8_t value);
} light_opt_t;

struct light {
	list_head_t link;
	uint_fast8_t id;
	light_opt_t opt;
	light_info_t info;
	struct timespec prev_tm;
	uint_fast8_t prev_val;
};

/**
 * @brief Initialize ram for light, the function must be invoke
 *  	  before dsp init
 *
 * @author cyj (2016/6/14)
 *
 * @param void
 */
void light_ram_init(void);

/**
 * @brief Put a light control information to
 *  	  light module
 *
 * @author cyj (2016/6/14)
 *
 * @param ctrl - The light control information
 *
 * @return int_fast8_t - If success return 0, otherwise occur
 *  	   some error
 */
int_fast8_t ve_light_ctrl_put(const light_ctrl_t *ctrl);

/**
 * @brief Register a light, the function can add
 * your hardware options into light manager
 *
 * @author cyj (2016/6/14)
 *
 * @param id - The light's id
 * @param opt - The options of light
 *
 * @return int_fast8_t - If success return 0, otherwise occur
 *  	   some error
 */
int_fast8_t light_regst(uint_fast8_t id, const light_opt_t *opt);

/**
 * @brief Initialize the light's thread
 *
 * @author cyj (2016/6/14)
 *
 * @param void
 *
 * @return int32_t - If success return 0, otherwise occur
 *  	   some error
 */
int32_t light_thread_init(void);

/**
 * @brief Set the light status for white light
 *
 * @author cyj (2016/6/14)
 *
 * @param on - true: open the white light, false: close the
 *  		 white light
 */
void ve_cm_white_light_set(bool on);

C_CODE_END

#endif
