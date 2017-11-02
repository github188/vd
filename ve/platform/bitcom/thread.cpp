#include <stdio.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include "sys/config.h"
#include "ctrl.h"
#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "profile.h"
#include "logger/log.h"
#include "sys/time_util.h"
#include "sys/heap.h"
#include "sys/tcp_util.h"
#include "dev/led/fly_eq20131.h"
#include "dev/led/degao.h"
#include "ve/config/hide_param.h"
#include "ve/config/vip.h"
#include "record.h"
#include "ve/platform/bitcom/mq_resend.h"
#include "ve/platform/bitcom/ftp_resend.h"
#include "ve/platform/bitcom/http_resend.h"
#include "ve/dev/roadgate/roadgate_data.h"
#include "ve/dev/roadgate/roadgate.h"
#include "ve/config/hide_param.h"
#include "logger/log.h"
#include "sys/tcp_util.h"
#include "dev/led/fly_eq20131.h"
#include "config/ve_cfg.h"
#include "time_stat.h"
#include "record.h"
#include "thread.h"
#include "proto.h"
#include "sys/xstring.h"

C_CODE_BEGIN

typedef struct resend_conf{
	char dir[512];
	int32_t nr_record;
}resend_conf_t;

typedef struct dts_send_item{
	list_head_t link;
	void *buf;
	size_t len;
} dts_send_item_t;

typedef struct dts{
	struct timespec rcvd_time;
	list_head_t send_ls;
	pthread_mutex_t mutex;
}dts_t;

typedef enum{
	DEV_LOCK = 1,
	DEV_UNLOCK,
}lock_act_t;

typedef struct lock{
	lock_act_t action;
}lock_t;

static ssize_t send_s(int32_t sockfd, const char *data, size_t len);
#if HTTPLONG_DOUBLE_TEST
static ssize_t send_test_s(int32_t sockfd, const char *data, size_t len);
#endif

static void *heartbeat_thread(void * arg);

extern PLATFORM_SET msgbuf_paltform;

int gi_tabitcom_httplink_sockfd = 0;

#if HTTPLONG_DOUBLE_TEST
int32_t httplong_test_sockfd;
#endif

str_tabitcom_status gstr_tabitcom_status;
str_bitcom_pro gstr_bitcom_pro;
static bool ghttplong_rcvd = false;
static int32_t HTTPLINK_TYPE = 0;
static dts_t dts;
static lock_t lock;


#if 0
static bool dev_is_locked(void)
{
	return (DEV_LOCK == lock.action);
}
#endif

static bool record_is_locked(void)
{
#if 0
	if (dev_is_locked()) {
		return true;
	}
#endif

	return false;
}

static void dts_rcvd_time_refresh(struct timespec *t)
{
	ASSERT(NULL != t);
	if(-1 == clock_gettime(CLOCK_MONOTONIC, t)){
		t->tv_sec = 0;
		t->tv_nsec = 0;
		ERROR("Get clock time failed, set to zero");
	}
}

static int_fast32_t dts_send_put(dts_t *dts, const void *data, size_t len)
{
	if ((data == NULL) || (len == 0) || (dts == NULL)) {
		return -EINVAL;
	}

	dts_send_item_t *item;
	if ((item = (dts_send_item_t *)xmalloc(sizeof(*item))) == NULL) {
		return -ENOBUFS;
	}

	if ((item->buf = xmalloc(len)) == NULL) {
		xfree(item);
		return -ENOBUFS;
	}

	DEBUG("Dts will send %"PRIdPTR" bytes:", len);
	DEBUG("%s", (const char *)data);

	memcpy(item->buf, data, len);
	item->len = len;
	pthread_mutex_lock(&dts->mutex);
	list_add_tail(&item->link, &dts->send_ls);
	pthread_mutex_unlock(&dts->mutex);

	return 0;
}

static dts_send_item_t *dts_send_get_head(dts_t *dts)
{
	dts_send_item_t *item = NULL;

	pthread_mutex_lock(&dts->mutex);
	if (!list_empty(&dts->send_ls)) {
		item = list_entry(dts->send_ls.next, dts_send_item_t, link);
	}
	pthread_mutex_unlock(&dts->mutex);

	return item;
}

static void dts_send_del(dts_t *dts, dts_send_item_t *item)
{
	pthread_mutex_lock(&dts->mutex);
	list_del(&item->link);
	pthread_mutex_unlock(&dts->mutex);

	xfree(item->buf);
	xfree(item);
}

int tabitcom_control(str_bitcom_pro *proto)
{
	size_t len = 0;
	char buf[1024];
	int32_t ret;

	/* Received data refresh heart beate time */
	dts_rcvd_time_refresh(&dts.rcvd_time);

	/*
	 * Control command
	 */
	if (VE_DTS_TYPE_CRTL == proto->type) {

		if (proto->roadgate.valid_flag == 1) {
			memcpy(&gstr_bitcom_pro.roadgate, &proto->roadgate,
				   sizeof(str_pab_roadgate));
		}

		if ((proto->led.valid_flag == 1) && (proto->led.nlines > 0)) {
			led_add_lines(proto->led.lines, proto->led.nlines);
		}

		if (true == proto->lock.valid) {
			lock.action = (lock_act_t)proto->lock.action;
			if (1 == proto->lock.action) {
				ve_led_set_lock(true);
			} else {
				ve_led_set_lock(false);
			}
		}

		len = alleyway_bitcom_protocol(buf, sizeof(buf), proto, 4, PRO_ENCODE);
		if (len > 0) {
			dts_send_put(&dts, buf, len);
		}
	}

	/*
	 * Query command
	 */
	if (VE_DTS_TYPE_QUERY == proto->type) {
		len = alleyway_bitcom_protocol(buf, sizeof(buf), proto, 4, PRO_ENCODE);
		if (len > 0) {
			dts_send_put(&dts,buf,len);
		}
	}

	/*
	 * Download command
	 */
	if(VE_DTS_TYPE_DOWNLOAD == proto->type) {
		if (true == proto->vip.valid) {
			/*
			 * If the data is too long, drop it
			 */
			if (proto->vip.data_len < VE_CFG_VIP_MAX_LEN) {
				/* Save to vip file */
				ret = vip_info_save_to_file(VE_CFG_VIP_FILE,
											proto->vip.data,
											proto->vip.data_len);
				if (0 == ret) {
					INFO("Vip data save success.");

					len = alleyway_bitcom_protocol(buf, sizeof(buf),
												   proto, 4, PRO_ENCODE);
					if (len > 0) {
						dts_send_put(&dts, buf, len);
					}

					ret = vip_info_send_to_dsp((uint8_t *)proto->vip.data,
											   proto->vip.data_len);
					if (0 != ret) {
						ERROR("Vip data send to dsp failed");
					}
				}
			}

			/* Free the buffer */
			xfree(proto->vip.data);
		}
	}

	return 0;
	
}

int tabitcom_status(int ai_sockfd, char *ac_data, int ai_data_lens, int ai_type)
{
	ssize_t ret = 0;

	if(record_is_locked()){
		INFO("record is locked, do not send status");
		return 0;
	}

	if(((ai_sockfd==0)&&(ai_type!=1)) || (ac_data==NULL) || (ai_data_lens==0))
	{
		ERROR("tabitcom_status Input is error!");

		return -1;
	}

	INFO("Status:");
	INFO("%s", ac_data);

	if(ai_type == 0)
	{
#if HTTPLONG_DOUBLE_TEST
		if (0 != httplong_test_sockfd) {
			send_test_s(httplong_test_sockfd, ac_data, ai_data_lens);
		} else {
			ERROR("httplong_test_sockfd is error!");
		}
#endif
		ret = send_s(ai_sockfd, ac_data, ai_data_lens);
	}
	else if(ai_type == 1)
	{
		ret = tabitcom_httpshortlink_socket(ac_data, ai_data_lens);
	}

	return 0;
}

#if HTTPLONG_DOUBLE_TEST
static ssize_t send_test_s(int32_t sockfd, const char *data, size_t len)
{
	ssize_t left = len;
	ssize_t ret;
	const char *p = data;

	TRACE_LOG_PLATFROM_INTERFACE("send_test_s will send %d bytes at %s.", 
								 len, ustime_msec());

	while (left > 0) {
		ret = send(sockfd, p, left, 0);
		if (ret < 0) {
			TRACE_LOG_PLATFROM_INTERFACE("send_test_s error: return %d at %s", 
										 ret, ustime_msec());
			return -1;
		}
		left -= ret;
		p += ret;
	}

	TRACE_LOG_PLATFROM_INTERFACE("send_test_s has sent %d bytes at %s.", 
								 (ssize_t)(p - data), ustime_msec());

	return (ssize_t)(p - data);
}
#endif

static ssize_t send_s(int32_t sockfd, const char *data, size_t len)
{
	ssize_t left = len;
	ssize_t ret;
	const char *p = data;
	int32_t i;

	while (left > 0) {
		ret = send(sockfd, p, left, 0);
		if (ret < 0) {
			ERROR("send failed at %s", ustime_msec());
			return -1;
		}
		left -= ret;
		p += ret;
	}

	/* 
	 * 发完之后等回应, 若超时之后则认为当前socket已经断开
	 */
	ghttplong_rcvd = false;
	i = 0;
	while ((!ghttplong_rcvd) && (i < 200)) {
		++i;
		usleep(10000);
	}

	INFO("Sent %d bytes @ %s.", (ssize_t)(p - data), ustime_msec());

	return (ssize_t)(p - data);
}

int tabitcom_record(int sfd, char *data, int len, int type)
{
	int ret = -1;

	if (record_is_locked()) {
		INFO("Record report is locked, do not send to server.");
		return 0;
	}

	if ((data == NULL) || (len <= 0)) {
		ERROR("The data will be sent is invalied");
		return -EINVAL;
	}

	INFO("Report start at %s:", ustime_msec());
	DEBUG("%s", data);

	if (type == 0) {
		/* http long  link */
		if (sfd == 0) {
			ERROR("http long link do not exist!");
			return -1;
		}

#if HTTPLONG_DOUBLE_TEST
		if (0 != httplong_test_sockfd) {
			send_test_s(httplong_test_sockfd, data, len);
		} else {
			ERROR("httplong_test_sockfd is error!");
		}
#endif

		if(send_s(sfd, data, len) >= len){
			ret = 0;
		}
	} else if (type == 1) {
		ret = tabitcom_httpshortlink_socket(data, len);
	}

	INFO("Report stop at %s", ustime_msec());

	return ret;
}

ssize_t tabitcom_http_record(const uint8_t *data, size_t len)
{
	return tabitcom_record(gi_tabitcom_httplink_sockfd,
						   (char *)data, len, HTTPLINK_TYPE);
}

int tabitcom_heartbeate(int ai_sockfd, char *ac_data, int ai_data_lens, int ai_type)
{
	ssize_t ret = 0;

	if(((ai_sockfd==0)&&(ai_type!=1)) || (ac_data==NULL) || (ai_data_lens==0))
	{
		ERROR("tabitcom_heartbeate Input is error!");

		return -1;
	}

	INFO("Heart beate:");
	INFO(" %s", ac_data);
	
	if(ai_type == 0)
	{
#if HTTPLONG_DOUBLE_TEST
		if (0 != httplong_test_sockfd) {
			send_test_s(httplong_test_sockfd, ac_data, ai_data_lens);
		} else {
			ERROR("httplong_test_sockfd is error!");
		}
#endif
		ret = send_s(ai_sockfd, ac_data, ai_data_lens);
	}
	else if(ai_type == 1)
	{
		ret = tabitcom_httpshortlink_socket(ac_data, ai_data_lens);
	}

	return 0;
}

static void *chk_status_thread(void * arg)
{
	static str_tabitcom_status prev_status;
	str_alleyway_msg msg;
	str_bitcom_http_pro proto;
	ssize_t len;

	memset(&msg, 0, sizeof(msg));
	memset(&proto, 0, sizeof(proto));

	prev_status.FaultState = 0;
	prev_status.SenseCoilState = 0;
	prev_status.FlashlLightState = 0;
	prev_status.IndicatoLightState.all = 0;

	while(1)
	{
		usleep(1000);
		
		//check the bitcom alleyway device status
		if ((prev_status.FaultState != gstr_tabitcom_status.FaultState) ||
			(prev_status.SenseCoilState != gstr_tabitcom_status.SenseCoilState) ||
			(prev_status.FlashlLightState != gstr_tabitcom_status.FlashlLightState) ||
			(prev_status.IndicatoLightState.all != gstr_tabitcom_status.IndicatoLightState.all)) {

			prev_status.FaultState = gstr_tabitcom_status.FaultState;
			prev_status.SenseCoilState = gstr_tabitcom_status.SenseCoilState;
			prev_status.FlashlLightState = gstr_tabitcom_status.FlashlLightState;
			prev_status.IndicatoLightState.all = gstr_tabitcom_status.IndicatoLightState.all;

			msg.bitcom.data_lens = 1024;
			msg.bitcom.data = (char *)xmalloc(msg.bitcom.data_lens);
			if (NULL != msg.bitcom.data) {
				len = alleyway_bitcom_protocol(msg.bitcom.data,
											   msg.bitcom.data_lens,
											   &proto,
											   2, PRO_ENCODE);
				msg.bitcom.data_lens = len;
				vmq_sendto_tabitcom(msg, TABITCOM_STATUS);
			}
		}

		if (1 == gstr_bitcom_pro.roadgate.valid_flag) {
			gstr_bitcom_pro.roadgate.valid_flag = 0;

			INFO("Rcvd road gate ctrl cmd from server @ %s", ustime_msec());

			static const uint8_t action_tab[VE_DTS_RG_END] = {
				0,
				RG_ACTION_UP,
				RG_ACTION_DN,
				RG_ACTION_STOP,
				RG_ACTION_NO_SET,
				RG_ACTION_NC_SET,
				RG_ACTION_NC_CLR
			};

			static const uint8_t type_tab[VE_DTS_RG_TYPE_END] = {
				0,
				RG_CTRL_AUTO,
				RG_CTRL_MANUAL,
				RG_CTRL_OTHER
			};

			uint_fast32_t action = gstr_bitcom_pro.roadgate.barriergate;
			uint_fast32_t mode = gstr_bitcom_pro.roadgate.bgmode;
			uint_fast32_t timeout = gstr_bitcom_pro.roadgate.bgkeeptime * 1000;

			if (((action != 0) && (action < numberof(action_tab)))
				&& ((mode != 0) && (mode < numberof(type_tab)))) {

				rg_ctrl_t *ctrl = rg_new_ctrl_alloc(); 
				
				if (ctrl) {
					ctrl->ctrler = RG_CTRLER_CENTER;
					ctrl->action = action_tab[action];
					ctrl->type = type_tab[mode];
					ctrl->timeout = timeout;

					roadgate_new_ctrl(ctrl);
				}
			}
		}
	}

	return NULL;
}


int tabitcom_httpshortlink_socket(char *data, int len)
{
	ssize_t l;
	int_fast32_t result;

	BITCOM_MSG *pfm_cfg = &msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg;

	const char *ip = (const char *)pfm_cfg->ServerIp;
	uint16_t port = pfm_cfg->MsgPort;

	int sfd = tcp_connect_timeout(ip, port, 1000);
	if (sfd == -1) {
		return -1;
	}

	l = send(sfd, data, len, 0);
	if (l < len) {
		int _errno = (l == -1) ? errno : 0;
		CRIT("Send to %s:%d failed, total sent %d bytes, %s",
			  ip, port, l, strerror(_errno));
		close(sfd);
		return -1;
	}

	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(sfd, &rset);
	tv.tv_sec = 1;
	tv.tv_usec = 500000;
	result = -1;
	
	int ret = select(sfd + 1, &rset, NULL, NULL, &tv);
	if (ret > 0) {
		if (FD_ISSET(sfd, &rset)) {
			char buf[1024];

			l = recv(sfd, buf, sizeof(buf), 0);
			if (l > 0) {
				DEBUG("Received %d bytes from %s:%d, %s",
					  l, ip, port, buf);
				if (strstr(buf, HTTP_CODE_201_STR)) {
					result = 0;
				}
			} else if(l == 0) {
				CRIT("Host %s:%d is closed", ip, port);
			} else {
				int _errno = errno;
				CRIT("Receive from %s:%d failed, %s",
					  ip, port, strerror(_errno));
			}
		} else {
			CRIT("Select %s:%d success, but rset is not be setted!",
				  ip, port);
		}
	} else if (ret == 0) {
		CRIT("Select to %s:%d timeout", ip, port);
	} else {
		int _errno = errno;
		CRIT("Select to %s:%d failed, %s", ip, port, strerror(_errno));
	}

	close(sfd);
	return result;
}

static void *lhttp_thread(void * arg)
{
	int sockfd;
	fd_set rset;
	char lc_tmp[1024] = {0};
	struct timeval tv;
	struct sockaddr_in server_addr;
	str_alleyway_msg lstr_alleyway_msg;
	ssize_t len = 0;

	memset(&lstr_alleyway_msg, 0, sizeof(lstr_alleyway_msg));

	while(1)
	{
		usleep(10000);

		if( (sockfd = socket(AF_INET, SOCK_STREAM,0)) == -1 )
		{
			continue;
		}

		bzero(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);
		server_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp);
		
		if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
		{
			close(sockfd);
				
			continue;
		}
		
		gi_tabitcom_httplink_sockfd = sockfd;

		INFO("Tabitcom http long socket  is builed!");
		
		while(1)
		{
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset); 
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			if (select(sockfd + 1, &rset, NULL, NULL, &tv) < 0) {
				break;
			}

			len = recv(sockfd, lc_tmp, sizeof(lc_tmp) - 1, 0);
			if(len > 0){
				ghttplong_rcvd = true;
			}else if((len != EAGAIN) && (len != EWOULDBLOCK) && (len != EINTR)){
				break;
			}
			
		}

		INFO("Tabitcom http long socket  is closed!");
		gi_tabitcom_httplink_sockfd = 0;
		close(sockfd);
		             
	}	
	
	return NULL;
}


#if HTTPLONG_DOUBLE_TEST
static void *lhttp_test_thread(void * arg)
{
	int sockfd;
	fd_set rset;
	char lc_tmp[1024] = {0};
	struct timeval tv;
	struct sockaddr_in server_addr;
	str_alleyway_msg lstr_alleyway_msg;
	ssize_t len = 0;

	memset(&lstr_alleyway_msg, 0, sizeof(lstr_alleyway_msg));

	while(1)
	{
		usleep(10000);

		if( (sockfd = socket(AF_INET, SOCK_STREAM,0)) == -1 )
		{
			continue;
		}

		bzero(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(5566);
		server_addr.sin_addr.s_addr = inet_addr("192.168.1.197");
		
		if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
		{
			close(sockfd);
				
			continue;
		}
		
		httplong_test_sockfd = sockfd;

		INFO("http long test socket  is builed!");
		
		while(1)
		{
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset); 
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			if (select(sockfd + 1, &rset, NULL, NULL, &tv) < 0) {
				break;
			}

			len = recv(sockfd, lc_tmp, sizeof(lc_tmp) - 1, 0);
			if(len > 0){
				;
			}else if((len != EAGAIN) && (len != EWOULDBLOCK) && (len != EINTR)){
				break;
			}
			
		}

		INFO("http long test socket  is closed!");
		httplong_test_sockfd = 0;
		close(sockfd);
		             
	}	
	
	return NULL;
}

#endif





static int32_t dts_recv(const uint8_t *data, size_t len)
{
	static uint8_t *wp;
	static str_alleyway_msg msg;
	static dts_head_t info;
	static size_t remain;
	const uint8_t *hdr;
	const uint8_t *frm;
	uint32_t cs;
	size_t l;
	size_t frm_len;

	frm = data;
	frm_len = len;

	DEBUG("Dts received %d bytes", len);

	hdr = dts_find_head(data, len);
	if (NULL != hdr) {

		DEBUG("Find a head from dts frame");

		len -= (size_t)(hdr - data);

		if (0 == dts_get_frm_info(hdr, len, &info)) {
			/*
			 * Receiving, drop the old frame
			 */
			if (NULL != msg.bitcom.data) {
				DEBUG("Dts drop the old frame");
				xfree(msg.bitcom.data);
			}

			DEBUG("Dts sof: %s", info.sof);
			DEBUG("Dts daddr: %s", info.daddr);
			DEBUG("Dts type: %d", info.type);
			DEBUG("Dts cs: %d(0x%08X)", info.cs, info.cs);
			DEBUG("Dts head_len: %d", info.head_len);
			DEBUG("Dts data_len: %d", info.data_len);

			msg.bitcom.data_lens = info.data_len + info.head_len;
			msg.bitcom.data = (char *)xmalloc(msg.bitcom.data_lens);
			if (NULL == msg.bitcom.data) {
				CRIT("Failed to alloc memory for dts frame");
				return -1;
			}

			remain = msg.bitcom.data_lens;
			wp = (uint8_t *)msg.bitcom.data;
			frm = hdr;
			frm_len = len;
		}
	}

	if (msg.bitcom.data) {
		l = min(len, remain);
		memcpy(wp, frm, l);
		remain -= l;
		wp += l;
		if (0 == remain) {
			frm = (uint8_t *)msg.bitcom.data + info.head_len;
			cs = get_sum_chk(frm, info.data_len);

			DEBUG("len: %d", info.data_len);
			DEBUG("cs: %d", cs);
			DEBUG("Dts data: ");
			DEBUG("%s", msg.bitcom.data);

			if (cs == info.cs) {
				/* Sent to process thread */
				INFO("Dts frame check ok, send to user");
				vmq_sendto_tabitcom(msg, TABITCOM_CONTROL);
				msg.bitcom.data = NULL;
				return 0;
			} else {
				INFO("Dts frame check error, drop it");
				xfree(msg.bitcom.data);
				msg.bitcom.data = NULL;
			}
		}
	}

	return -1;
}

static void dts_ram_init(dts_t *dts)
{
	INIT_LIST_HEAD(&dts->send_ls);
	pthread_mutex_init(&dts->mutex, NULL);
}

static void *ltcp_thread(void *arg)
{
	int32_t ret;

	while (1) {
		sleep(1);

		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1) {
			CRIT("Get socket failed!");
			continue;
		}

		const char *ip = msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp;
		uint16_t port = 2015;

		struct sockaddr_in serv_addr;
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = inet_addr(ip);
		ret = connect(sockfd, (struct sockaddr *)(&serv_addr),
					  sizeof(struct sockaddr));
		if (ret == -1) {
			close(sockfd);
			CRIT("Connect to %s:%d failed:%m", ip, port);
			continue;
		}

		INFO("Connected to %s:%d", ip, port);

		dts_rcvd_time_refresh(&dts.rcvd_time);

		while (1) {
			fd_set rset;
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);

			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1000;

			ret = select(sockfd + 1, &rset, NULL, NULL, &tv);
			if (ret == -1) {
				int _errno = errno;
				if ((_errno != EAGAIN) && (_errno != EWOULDBLOCK)
					&& (_errno != EINTR)) {
					CRIT("Select to %s:%d failed:%m", ip, port);
					break;
				}
			} else if (ret > 0) {
				if (FD_ISSET(sockfd, &rset)) {
					uint8_t buf[16 * 1024];
					ssize_t recv_size = recv(sockfd, buf, sizeof(buf), 0);
					if (recv_size > 0) {
						dts_recv((uint8_t *)buf, recv_size);
					} else if (recv_size == 0) {
						CRIT("Host %s:%d is closed", ip, port);
						break;
					} else {
						int _errno = errno;
						if ((_errno != EAGAIN) && (_errno != EWOULDBLOCK)
							&& (_errno != EINTR)) {
							CRIT("Receive from %s:%d failed:%m", ip, port);
							break;
						}
					}
				}
			}

			dts_send_item *send_item = dts_send_get_head(&dts);
			if (send_item != NULL) {
				ssize_t l = send(sockfd, send_item->buf, send_item->len, 0);
				if (l < (ssize_t)send_item->len) {
					CRIT("Dts send to %s:%d failed:%m", ip, port);
				}
				dts_send_del(&dts, send_item);
			}

			struct timespec curr_time;
			if(0 == clock_gettime(CLOCK_MONOTONIC, &curr_time)){
				if (timespec_diff_sec(&curr_time, &dts.rcvd_time) > 10) {
					CRIT("Lost heart beat with %s:%d", ip, port);
					break;
				}
			} else {
				CRIT("Get clock time failed, close %s:%d", ip, port);
				break;
			}

		}

		CRIT("Disconnect with %s:%d", ip, port);
		close(sockfd);
	}

	return NULL;
}

/**
 * http_read_mode - read http mode from profile,
 * 					zero is long, 1 is short
 */
static int32_t http_read_mode(void)
{
	ve_cfg_t cfg;
	int32_t mode;

	ve_cfg_get(&cfg);

	mode = cfg.pfm.bitcom.http_long ? HTTP_LONG : HTTP_SHORT;

	INFO("Http link is %d", mode);

	return mode;
}

static void set_http_mode(void)
{
	HTTPLINK_TYPE = http_read_mode();
}

static void mkdirs(const char *path)
{
	char cmd[1024];

	sprintf(cmd, "mkdir -p %s", path);
	system(cmd);
	INFO("%s", cmd);
}

static void resend_get_conf(resend_conf_t *conf)
{
	char buf[256];
	ve_cfg_t cfg;

	ve_cfg_get(&cfg);

	/* 默认放在内存中, 重传10条记录 */
	strlcpy(conf->dir, RUN_DIR"/"LOST_DIR, sizeof(conf->dir));
	conf->nr_record = cfg.pfm.resend_rotate;

	if (0 == get_cur_partition_path(buf)) {
		/* 检测到sd卡 */
		snprintf(conf->dir, sizeof(conf->dir), "%s/"LOST_DIR, buf);
	}

	INFO("Resend dir: %s, ", conf->dir);
	INFO("Record num: %d.", conf->nr_record);
}

static void resend_mkdir(resend_conf_t *conf, char *db_path, char *pic_dir)
{
	/* Output the database path */
	sprintf(db_path, "%s/"LOST_DB, conf->dir);
	/* Output the picture path */
	sprintf(pic_dir, "%s/pic", conf->dir);
	/* Create lost directory*/
	mkdirs(conf->dir);
	/* Create pic directory */
	mkdirs(pic_dir);
}

void tabitcom_pthread_ram_init(void)
{
	resend_conf_t resend_conf;
	char db_path[PATH_MAX + 1];
	char pic_dir[PATH_MAX + 1];


	rg_record_ram_init();
	tabitcom_mq_send_ram_init();

	/* 读重传配置 */
	resend_get_conf(&resend_conf);
	/* 根据配置创建目录, 并获取数据库和图片的路径 */
	resend_mkdir(&resend_conf, db_path, pic_dir);
	tabitcom_mq_resend_ram_init(db_path, resend_conf.nr_record);
	tabitcom_ftp_resend_ram_init(db_path, pic_dir, resend_conf.nr_record);
	tabitcom_http_resend_ram_init(db_path, pic_dir, resend_conf.nr_record);

	lock.action = DEV_UNLOCK;
	dts_ram_init(&dts);
}

int_fast32_t vip_info_init(void)
{
	char *buf;
	ssize_t len;

	len = VE_CFG_VIP_MAX_LEN;
	buf = (char *)xmalloc(len);
	if (!buf) {
		return -1;
	}

	len = vip_info_read_from_file(VE_CFG_VIP_FILE, buf, len);
	if (len > 0) {
		vip_info_send_to_dsp(buf, len);
	}

	xfree(buf);

	return 0;
}

static void *pfm_thread(void *arg)
{
	ssize_t ret;
	int32_t _errno;
	str_alleyway_msg msg;
	str_bitcom_pro proto;
	ssize_t len;
	sabitcom_http_resend_t item;

	memset(&msg, 0, sizeof(msg));
	memset(&proto, 0, sizeof(proto));

	prctl(PR_SET_NAME, "bitcom_pfm");
	INFO("Bitcom platform tid: %d", pthread_self());

	while (1) {
		len = msgrcv(MSG_ALLEYWAY_ID, &msg, sizeof(msg) - sizeof(long),
					 MSG_ALLEYWAY_BITCOM_TYPE, 0);
		if (len < (ssize_t)(sizeof(msg) - sizeof(long))) {
			_errno = errno;
			ERROR("VE bitcom recv from msg queue failed: %d", _errno);
			continue;
		}

		switch (msg.bitcom.func) {
			case TABITCOM_HEARTBEAT:
				tabitcom_heartbeate(gi_tabitcom_httplink_sockfd,
									msg.bitcom.data,
									msg.bitcom.data_lens,
									HTTPLINK_TYPE);
				xfree(msg.bitcom.data);
				break;

			case TABITCOM_RECORD: {
					time_stat_update(VE_TS_SOSR);

					int_fast32_t err_cnt = 0;
					do {
						ret = tabitcom_record(gi_tabitcom_httplink_sockfd,
											  (char *)msg.bitcom.http.frm,
											  msg.bitcom.http.frm_len,
											  HTTPLINK_TYPE);
					} while ((ret < 0) && (err_cnt++ < 2));

					time_stat_update(VE_TS_EOSR);
					time_stat_show_after_send();

					if (ret < 0) {
						ERROR("Record send to server failed");

						if (msg.bitcom.http.pic) {
							/*
							 * Retransmit only the complete record
							 */
							ret = tabitcom_http_get_resend_info(&item,
									(char *)msg.bitcom.http.frm,
									msg.bitcom.http.pic,
									msg.bitcom.http.pic_len);
							if (ret == 0) {
								tabitcom_http_resend_put(&item);
							}
						}
					}

					xfree(msg.bitcom.http.frm);
					if (msg.bitcom.http.pic) {
						/* Pic is not exist when only have a http message */
						xfree(msg.bitcom.http.pic);
					}
				}

				break;

			case TABITCOM_STATUS:
				tabitcom_status(gi_tabitcom_httplink_sockfd,
								msg.bitcom.data,
								msg.bitcom.data_lens,
								HTTPLINK_TYPE);
				xfree(msg.bitcom.data);
				break;

			case TABITCOM_CONTROL:
				memset(&proto, 0, sizeof(proto));
				alleyway_bitcom_protocol(msg.bitcom.data,
										 msg.bitcom.data_lens,
										 (void *)&proto,
										 PAB_TCP_PRO, PRO_DECODE);
				xfree(msg.bitcom.data);

				INFO("recv down tunnel ctrl cmd at %s", ustime_msec());

				INFO("roadgate.valid_flag=%d", proto.roadgate.valid_flag);
				INFO("roadgate.barriergate=%d", proto.roadgate.barriergate);
				INFO("roadgate.bgkeeptime=%d", proto.roadgate.bgkeeptime);
				INFO("roadgate.bgmode=%d", proto.roadgate.bgmode);

				INFO("led.valid_flag=%d", proto.led.valid_flag);
				INFO("led.nlines=%d", proto.led.nlines);

				INFO("lock.valid=%d", proto.lock.valid);
				INFO("lock.action=%d", proto.lock.action);

				tabitcom_control(&proto);

				break;
		}

	}

	return NULL;
}


static void *heartbeat_thread(void * arg)
{
	str_alleyway_msg msg;
	str_bitcom_http_pro proto;

	for (;;) {
		proto.http_type = 1;

		msg.bitcom.data_lens = 1024;
		msg.bitcom.data = (char *)xmalloc(msg.bitcom.data_lens);
		if (NULL == msg.bitcom.data) {
			CRIT("Failed to alloc memory for heart beate frame.");
			continue;
		}

		msg.bitcom.data_lens = alleyway_bitcom_protocol(msg.bitcom.data,
														msg.bitcom.data_lens,
														(void *)&proto, 1, 1);
		vmq_sendto_tabitcom(msg, TABITCOM_HEARTBEAT);

		sleep(5);
	}

	return NULL;
}

int32_t ve_pfm_bitcom_init(void)
{
	pthread_t tid_ltcp;
	pthread_t tid_lhttp;
	pthread_t tid_chk_status;
	pthread_t tid_hb;
	pthread_t tid_pfm;

#if HTTPLONG_DOUBLE_TEST
	pthread_t tid_lhttp_test;
#endif

	tabitcom_pthread_ram_init();
	set_http_mode();
	tabitcom_mq_send_pthread_init();
	tabitcom_mq_resend_pthread_init();
	tabitcom_ftp_resend_pthread_init();
	tabitcom_http_resend_pthread_init();

	if(0 != pthread_create(&tid_ltcp, NULL, ltcp_thread, NULL)){
		ERROR("Failed to create tcp long thread");
		return -1;
	}

	if(0 != pthread_create(&tid_lhttp, NULL, lhttp_thread, NULL)){
		ERROR("Failed to create http long thread");
		return -2;
	}

#if HTTPLONG_DOUBLE_TEST
	if(0 != pthread_create(&tid_lhttp_test, NULL, lhttp_test_thread, NULL)){
		ERROR("Failed to create http long test thread");
		return -3;
	}
#endif

	if(0 != pthread_create(&tid_chk_status, NULL, chk_status_thread, NULL)){
		ERROR("Failed to create check status thread");
		return -4;
	}

	if(0 != pthread_create(&tid_hb, NULL, heartbeat_thread, NULL)){
		ERROR("Failed to create heart beat thread");
		return -5;
	}

	if(0 != pthread_create(&tid_pfm, NULL, pfm_thread, NULL)){
		ERROR("Failed to create platform main thread");
		return -6;
	}

	return 0;

}

C_CODE_END
