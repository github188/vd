/**
 * @file parking_lock_cram_protocal.h
 * @brief this file include the parking lock protocal analyze for parking lock
 *        from CrAM IoT Technology Co.,Ltd
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-05-18
 */

#ifndef __PARK_LOCK_CRAM_PROTOCAL_H__
#define __PARK_LOCK_CRAM_PROTOCAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* define lock commands */
#define CRAM_LOCK_COMMAND_UP_DOWN_CTL (0x01)
#define CRAM_LOCK_COMMAND_STATUS      (0x04)
#define CRAM_LOCK_COMMAND_BEEP        (0x0A)
#define CRAM_LOCK_COMMAND_REFRESH     (0xF0)
#define CRAM_LOCK_COMMAND_GETLIST     (0xF1)

/* define lock up/down command data */
#define CRAM_LOCK_COMMAND_CTL_DATA_UP   (0x01)
#define CRAM_LOCK_COMMAND_CTL_DATA_DOWN (0x02)

#define CRAM_LOCK_COMMAND_STATUS_DATA     (0x01)
#define CRAM_LOCK_COMMAND_STATUS_RESP_OK  (0x00)
#define CRAM_LOCK_COMMAND_STATUS_RESP_ERR (0x01)

/* reset source */
#define CRAM_LOCK_COMMAND_RST_SRC_AUTO       (0x00)
#define CRAM_LOCK_COMMAND_RST_SRC_REMOTE     (0x01)
#define CRAM_LOCK_COMMAND_RST_SRC_BLUETOOTH  (0x02)
#define CRAM_LOCK_COMMAND_RST_SRC_ULTRASOUND (0x03)

#define CRAM_LOCK_COMMAND_ALARM   (0x00)
#define CRAM_LOCK_COMMAND_NOALARM (0x01)

#define CRAM_LOCK_COMMAND_STATUS_LOCK     (0x01)
#define CRAM_LOCK_COMMAND_STATUS_UNLOCK   (0x02)
#define CRAM_LOCK_COMMAND_STATUS_ABNORMAL (0x03)

typedef struct _cram_lock_cmd
{
    char ver;
    char mac[17];
    char code[8];
    char type[8];
    char payload[];
}cram_lock_cmd;

typedef struct _cram_lock_cmd cram_lock_cmd_req;
typedef struct _cram_lock_cmd cram_lock_cmd_resp;

int park_lock_cram_construct_json(const cram_lock_cmd_req *cmd,
                                  char* result, size_t result_len);
int park_lock_cram_decode_data(const char* buff, const size_t len);

#ifdef __cplusplus
}
#endif
#endif
