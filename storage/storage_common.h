
#ifndef _STORAGE_COMMON_H_
#define _STORAGE_COMMON_H_

#include <stdio.h>
#include <time.h>
#include <sys/types.h>


#define resume_print(msg...) \
	do{\
		printf("[resume](%s|%s|%d) ", __FILE__, __func__, __LINE__);\
		printf(msg);\
	} while (0);

#define _STORAGE_DEBUG_

#ifdef _STORAGE_DEBUG_

#define debug_print(msg...) \
	do{\
		printf("(%s|%s|%d) ", __FILE__, __func__, __LINE__);\
		printf(msg);\
	} while (0);

#define log_debug_storage(msg...) 	debug_print(msg)
#define log_state_storage(msg...) 	debug_print(msg)
#define log_warn_storage(msg...) 	debug_print(msg)
#define log_error_storage(msg...) 	debug_print(msg)

#else 	/* _STORAGE_DEBUG_ */

#include "log_interface.h"

#define debug_print(msg...)

#define log_storage(level, msg...) 	log_send(level, 0, "[存储模块]", msg)

#define log_debug_storage(msg...) 	log_storage(LOG_LEVEL_DEBUG, msg)
#define log_state_storage(msg...) 	log_storage(LOG_LEVEL_STATUS, msg)
#define log_warn_storage(msg...) 	log_storage(LOG_LEVEL_WARNING, msg)
#define log_error_storage(msg...) 	log_storage(LOG_LEVEL_FAULT, msg)

#endif 	/* _STORAGE_DEBUG_ */


#define safe_free(ptr) \
	do{\
		free(ptr);\
		ptr = NULL;\
	} while (0);

#define RETRY_TIMES 	3 		/* 重试次数 */


#ifdef  __cplusplus
extern "C"
{
#endif

void u_sleep(time_t sec, unsigned long usec);
int random_32(void);
int mkpath(const char *path, mode_t mode);
char *get_parent_path(const char *path);

#ifdef  __cplusplus
}
#endif


#endif 	/* _STORAGE_COMMON_H_ */
