#ifndef __VE_CONFIG_H
#define __VE_CONFIG_H

#include <pthread.h>
#include "types.h"
#include "global_cfg.h"
#include "dev_cfg.h"
#include "dsp_cfg.h"
#include "pfm_cfg.h"

C_CODE_BEGIN

#define VE_CFG_FILE_PATH	"/mnt/nand/ve.conf"

typedef struct ve_cfg {
	pfm_cfg_t pfm;
	dsp_cfg_t dsp;
	dev_cfg_t dev;
	global_cfg_t global;
} ve_cfg_t;

/**
 * @brief Load ve configures 
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_load(void);

/**
 * @brief Dump ve configures 
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_dump(void);

/**
 * @brief Send ve configure to dsp core
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_send_to_dsp(void);

/**
 * @brief Initialize memory for ve configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param void 
 * 
 * @return int32_t Return 0 on success, otherwise return -1
 */
int32_t ve_cfg_ram_init(void);

/**
 * @brief Set new configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param cfg New configure 
 */
void ve_cfg_set(const ve_cfg_t *cfg);

/**
 * @brief Get new configure
 * 
 * @author cyj (2016/7/20)
 * 
 * @param cfg 
 */
void ve_cfg_get(ve_cfg_t *cfg);

C_CODE_END


#endif
