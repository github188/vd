#ifndef __VE_CFG_VIP_H
#define __VE_CFG_VIP_H

#include "types.h"

C_CODE_BEGIN

#define VE_CFG_VIP_FILE	"/mnt/nand/vip.json"
/* The size of cmem is 1MB, so remain 3KB for information */
#define VE_CFG_VIP_MAX_LEN		(1021 * 1024)

/**
 * @brief Save vip data to file 
 *  
 * @param file - the vip file path
 * @param buf - the vip data 
 * @param len - the length of data 
 *   
 * @return int32_t return 0 on success, otherwise occur some 
 *  	   error
 */
int32_t vip_info_save_to_file(const char *file, const void *buf, size_t len);

/**
 * @brief Read vip data from file
 * 
 * @author cyj (2016/7/20)
 * 
 * @param file - the vip file path
 * @param buf - the buffer where to save vip data
 * @param bufsz - the length of data 
 * 
 * @return size_t return a non-negative integer indicating the 
 *  	   number of bytes actually read. Otherwise, the
 *  	   functions shall return -1
 */
ssize_t vip_info_read_from_file(const char *file, void *buf, size_t bufsz);

/**
 * @brief Send vip data to dsp core
 * 
 * @author cyj (2016/7/20)
 * 
 * @param data 
 * @param len 
 * 
 * @return int32_t return zero on sucess, otherwise return -1
 */
int32_t vip_info_send_to_dsp(const void *data, size_t len);


C_CODE_END


#endif
