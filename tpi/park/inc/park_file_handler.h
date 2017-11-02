#ifndef __PARK_FILE_HANDLER__
#define __PARK_FILE_HANDLER__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"

/**
 * Name:        create_multi_dir
 * Description: the same as mkdir -p
 */
int create_multi_dir(const char* path);
int park_save_picture(const char *picname, unsigned char *picbuff, int size);
int park_read_picture(const char *picname, unsigned char *picbuff, int size);
int park_delete_picture(const char *picname);

int park_save_light_state(void);
#if 0
int park_read_light_state(void);
#endif
int park_light_state_exist();
int park_delete_light_state();
#ifdef __cplusplus
}
#endif
#endif
