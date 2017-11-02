#ifndef _TABITCOM_PTHREAD_H_
#define _TABITCOM_PTHREAD_H_

#include "types.h"
#include "proto.h"

C_CODE_BEGIN

#define TABITCOM_HEARTBEAT		(1)
#define TABITCOM_STATUS		(2)
#define TABITCOM_RECORD		(3)
#define TABITCOM_CONTROL		(4)

#define HTTP_LONG	0
#define HTTP_SHORT		1
//#define HTTPLINK_TYPE		0	/* http连接方式, 0 - 长连接, 1 - 短连接 */
#define HTTPLONG_DOUBLE_TEST		0

typedef struct {
	int red;
	int green;
	int blue;
	int all;
}str_IndicatoLightState;

typedef struct 
{
	int FaultState;                 //设备故障状态
	int SenseCoilState;             //线圈状态
	int FlashlLightState;           //补光灯状态
	str_IndicatoLightState IndicatoLightState;         //指示灯状态
	
}str_tabitcom_status;


extern str_tabitcom_status gstr_tabitcom_status;


int tabitcom_control(str_bitcom_pro *as_bitcom_pro);
int tabitcom_httpshortlink_socket(char *ac_data, int ai_data_lens);
int tabitcom_status(int ai_sockfd, char *ac_data, int ai_data_lens, int ai_type);
int tabitcom_record(int ai_sockfd, char *ac_data, int ai_data_lens, int ai_type);
int tabitcom_heartbeate(int ai_sockfd, char *ac_data, int ai_data_lens, int ai_type);
int32_t ve_pfm_bitcom_init(void);
int32_t tcp_connect_timeout(const char *ip, uint16_t port, int32_t timeout);
ssize_t tabitcom_http_record(const uint8_t *data, size_t len);

int_fast32_t vip_info_init(void);

C_CODE_END

#endif
 
