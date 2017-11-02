
#ifndef DSP_CONFIG_H

#define DSP_CONFIG_H

//#include "config_epcs.h"
#include "mcfw/src_bios6/links_c6xdsp/videoAnalysis/config_vdcs.h"


#define DSP_PARAM_FILE "dsp_config.xml"
#define DSP_PARAM_FILE_PATH "/config/dsp_config.xml"

#define DSP_PD_PARAM_FILE "illpk_config.xml"
#define DSP_PD_PARAM_FILE_PATH "/config/illpk_config.xml"

#define HIDE_PARAM_FILE "hide_param"
#define HIDE_PARAM_FILE_PATH "/config/hide_param"
#define WHITE_BLACK_LIST_FILE_PATH "/config/whiteBlackList"

#define send_config_to_dsp() 		send_dsp_cfg(1, 0)
#define send_hidden_param_to_dsp() 	send_dsp_cfg(0, 1)

extern VDConfigData *p_dsp_cfg;


#ifdef  __cplusplus
extern "C"
{
#endif

VDConfigData *get_dsp_cfg_pointer(void);
char *get_hidden_param_pointer(void);
void *get_dsp_result_cmem_pointer(void);
unsigned long get_dsp_result_addr_phy(void);
int dsp_init(void);
int send_dsp_cfg(char cfg_changed, char hidden_param_changed);
int send_arm_cfg();
int send_dsp_msg(void * buf, int size, int msg_type);

#ifdef  __cplusplus
}
#endif


#endif
