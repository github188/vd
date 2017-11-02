#ifndef __RG_RECORD_H
#define __RG_RECORD_H

#include "types.h"
#include "upload.h"
#include "videoAnalysisLink_parm.h"
#include "mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"
#include "mcfw/interfaces/ti_media_std.h"
#include "mcfw/interfaces/link_api/systemLink_m3vpss.h"
#include "traffic_records_process.h"

C_CODE_BEGIN

int32_t process_rg_mq(MSGDATACMEM *msg);
int process_rg_records_motor_vehicle(const SystemVpss_SimcopJpegInfo *info);
int32_t tabitcom_mq_send(const char *text);
int32_t tabitcom_ftp_send(const EP_PicInfo *picinfo);
void rg_record_ram_init(void);
int32_t tabitcom_mq_send_ram_init(void);
int32_t tabitcom_mq_send_pthread_init(void);

C_CODE_END

#endif


