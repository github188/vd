#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "logger/log.h"
#include "commonfuncs.h"
#include "sys/heap.h"

C_CODE_BEGIN


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
int32_t vip_info_save_to_file(const char *file, const void *buf, size_t len)
{
	FILE *f;
	size_t l;

	f = fopen(file, "w");
	if (NULL == f) {
		return -1;
	}

	l = fwrite(buf, 1, len, f);
	fclose(f);
	fflush(f);

	return (l == len) ? 0 : -2;
}

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
ssize_t vip_info_read_from_file(const char *file, void *buf, size_t bufsz)
{
	FILE *f;
	size_t l;

	f = fopen(file, "r");
	if (NULL == f) {
		ERROR("Open vip file failed, %s", file);
		return -1;
	}

	l = fread(buf, 1, bufsz, f);
	fclose(f);

	return (ssize_t)l;
}

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
int32_t vip_info_send_to_dsp(const void *data, size_t len)
{
	char *gbkbuf;
	ssize_t gbk_len;
	int32_t ret;

	gbk_len = len * 3;
	gbkbuf = (char *)xmalloc(gbk_len);
	if (NULL == gbkbuf) {
		CRIT("Failed to alloc memory for vip info");
		return -1;
	}

	gbk_len = convert_enc_s("UTF-8", "GBK", (char *)data, len,
							gbkbuf, gbk_len);
	if (-1 == gbk_len) {
		ERROR("Failed to convert code for vip json string");
		return -1;
	}

	ret = send_dsp_msg(gbkbuf, gbk_len, 11);
	xfree(gbkbuf);

	return ret;
}

C_CODE_END
