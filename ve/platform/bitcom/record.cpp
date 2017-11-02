#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <inttypes.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "partition_func.h"
#include "data_process.h"
#include "database_mng.h"
#include "traffic_records_process.h"
#include "PCIe_api.h"
#include "ftp.h"
#include "log_interface.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>
#include "global.h"
#include <json/json.h>    //add by lxd
#include "ctrl.h"
#include "platform/bitcom/proto.h"
#include "vd_msg_queue.h"
#include "sys/time_util.h"
#include "platform/bitcom/mq_resend.h"
#include "platform/bitcom/ftp_resend.h"
#include "ep_type.h"
#include "thread.h"
#include "record.h"
#include "types.h"
#include "logger/log.h"
#include "config/ve_cfg.h"
#include "time_stat.h"
#include "sys/xstring.h"
#include "event.h"

typedef struct mq_text{
	char data[MQ_TEXT_BUF_SIZE];
	bool valid;
}mq_text_t;

typedef struct mq_send{
	pthread_t thread;
	pthread_mutex_t mutex;
	mq_text_t text[32];
}mq_send_t;


static int32_t mq_send_put(const char *text);

static mq_send_t mq_send;
extern str_alleyway_status gstr_alleyway_status;

int32_t tabitcom_ftp_send(const EP_PicInfo *picinfo)
{
	if (0 != ftp_get_status(FTP_CHANNEL_PASSCAR)) {
		return -1;
	}

	if (0 != ftp_send_pic_buf(picinfo, FTP_CHANNEL_PASSCAR)) {
		return -1;
	}

	return 0;
}

static int32_t ftp_send_pic(EP_PicInfo *pic_info)
{
	sabitcom_ftp_resend_t item;
	int32_t i;

	if (0 == tabitcom_ftp_send(pic_info)) {
		return 0;
	}

	for (i = 0; i < FTP_PATH_DEPTH; ++i) {
		strcat(item.path, pic_info->path[i]);
	}

	item.size = pic_info->size;
	memcpy(item.pic, pic_info->buf, item.size);
	strncpy(item.name, pic_info->name, sizeof(item.name));

	tabitcom_ftp_resend_put(&item);

	return -1;
}


static int32_t platform_send_rg_records(EP_PicInfo *pic_info, DB_TrafficRecord *db)
{
	str_bitcom_http_pro proto;
	str_alleyway_msg msg;
	ssize_t len;

	memset(&msg, 0, sizeof(msg));

	DEBUG("pic_info->size: %d", pic_info->size);
	DEBUG("pic_info->buf: 0x%08X", (uintptr_t)pic_info->buf);

	event_item_t *ei = event_item_alloc();
	if (ei) {
		strlcpy(ei->ident, "record", sizeof(ei->ident));
		const char *type = (pic_info->size == 0) ? "simple" : "full";
		snprintf(ei->desc, sizeof(ei->desc), "%s, %s", type, db->plate_num);
		event_put(ei);
	}

	proto.http_type = 3;
	proto.db_traffic_record = db;
	proto.pic_info = pic_info;

	msg.bitcom.http.frm_len = 800 * 1024;
	msg.bitcom.http.frm = (uint8_t *)malloc(msg.bitcom.http.frm_len);
	if (!msg.bitcom.http.frm) {
		CRIT("alloc memory for msg.bitcom.http.frm failed.");
		return -1;
	}

	len = alleyway_bitcom_protocol((char *)msg.bitcom.http.frm,
								   msg.bitcom.http.frm_len,
								   &proto, 3, 1);
	msg.bitcom.http.frm_len = len;

	if (pic_info->buf) {
		msg.bitcom.http.pic = (uint8_t *)malloc(pic_info->size);
		if (!msg.bitcom.http.pic) {
			free(msg.bitcom.http.frm);
			CRIT("alloc memory for msg.bitcom.http.pic failed.");
			return -1;
		}
		msg.bitcom.http.pic_len = pic_info->size;
		memcpy(msg.bitcom.http.pic, pic_info->buf, msg.bitcom.http.pic_len);
	}

	if (0 != vmq_sendto_tabitcom(msg, 3)) {
		ERROR("Put into queue failed!");
		free(msg.bitcom.http.pic);
		free(msg.bitcom.http.frm);
		return -2;
	}

	return 0;

}

int32_t tabitcom_mq_send(const char *text)
{
	static int32_t sta_cnt = 0;
	static int32_t send_cnt = 0;
	char ip[32];
	uint16_t port;
	int32_t sockfd;

	/*
	 * When offline mq cannot timely return an error, so we need to detect
	 * the status of mq server
	 */
	snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
			 g_set_net_param.m_NetParam.m_MQ_IP[0],
			 g_set_net_param.m_NetParam.m_MQ_IP[1],
			 g_set_net_param.m_NetParam.m_MQ_IP[2],
			 g_set_net_param.m_NetParam.m_MQ_IP[3]);
	port = g_set_net_param.m_NetParam.m_MQ_PORT;

	if((sockfd = tcp_connect_timeout(ip, port, 1000)) == -1){
		ERROR("error: connect mq server failed!");
		return -1;
	}
	close(sockfd);

	if (0 != mq_get_status_traffic_record()) {
		return -1;
	}

	DEBUG("%d get mq status success.", ++sta_cnt);

	if (0 != mq_send_traffic_record((char *)text)) {
		return -1;
	}

	DEBUG("%d send mq success", ++send_cnt);
	return 0;
}

static int32_t mq_send_msg(DB_TrafficRecord *db)
{
	char content[MQ_TEXT_BUF_SIZE];

	format_mq_text_traffic_record(content, db);
	DEBUG("content: %s.", content);

	return mq_send_put(content);
}

static int32_t rg_parse_pic_name(EP_PicInfo *pic_info,
								 const SystemVpss_AlgResultInfo *alg)
{
	const VD_Time *tm;
	const TrfcVehiclePlatePoint *result;

	tm = &alg->EPtime;
	result = (const TrfcVehiclePlatePoint *)(alg->AlgResultInfo);

	pic_info->tm.year = tm->tm_year;
	pic_info->tm.month = tm->tm_mon;
	pic_info->tm.day = tm->tm_mday;
	pic_info->tm.hour = tm->tm_hour;
	pic_info->tm.minute = tm->tm_min;
	pic_info->tm.second = tm->tm_sec;
	pic_info->tm.msecond = tm->tm_msec;

	/* 路径按照用户的配置进行设置 */
	int len = 0;
	for (int j = 0; j < EP_FTP_URL_LEVEL.levelNum; j++) {
		switch (EP_FTP_URL_LEVEL.urlLevel[j]) {
			case SPORT_ID:      //地点编号
				len += sprintf(pic_info->path[j], "/%s", EP_POINT_ID);
				break;
			case DEV_ID:        //设备编号
				len += sprintf(pic_info->path[j], "/%s", EP_DEV_ID);
				break;
			case YEAR_MONTH:    //年/月
				len += sprintf(pic_info->path[j], "/%04d%02d",
							   pic_info->tm.year, pic_info->tm.month);
				break;
			case DAY:           //日
				len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.day);
				break;
			case EVENT_NAME:    //事件类型
				len += sprintf(pic_info->path[j], "/%s",
							   TRAFFIC_RECORDS_FTP_DIR);
				break;
			case HOUR:          //时
				len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.hour);
				break;
			case FACTORY_NAME:  //厂商名称
				len += sprintf(pic_info->path[j], "/%s", EP_MANUFACTURER);
				break;
			default:
				break;
		}
		DEBUG("pic_info.path : %s\n", pic_info->path[j]);
	}


	/* 文件名以时间和车道号命名 */
	snprintf(pic_info->name, sizeof(pic_info->name),
	         "%s%04d%02d%02d%02d%02d%02d%03d%02d.jpg",
	         EP_EXP_DEV_ID,
	         pic_info->tm.year,
	         pic_info->tm.month,
	         pic_info->tm.day,
	         pic_info->tm.hour,
	         pic_info->tm.minute,
	         pic_info->tm.second,
	         pic_info->tm.msecond,
	         result->laneNum);

	DEBUG("pic_info.name is : %s\n",pic_info->name);

	return 0;

}

static int32_t rg_report_motor_vehicle_msg(const SystemVpss_AlgResultInfo *alg)
{
	EP_PicInfo pic_info;
	char partition[MOUNT_POINT_LEN] = {0};
	DB_TrafficRecord db;
	const TrfcVehiclePlatePoint *result;

	INFO("Rcvd msg from dsp @ %s", ustime_msec());
	time_stat_update(VE_TS_DSP_MSG);

	result = (const TrfcVehiclePlatePoint *)alg->AlgResultInfo;

	rg_parse_pic_name(&pic_info, alg);

	analyze_traffic_records_info_motor_vehicle(&db, result, &pic_info,
											   partition);

	ve_cfg_t cfg;
	ve_cfg_get(&cfg);
	if (cfg.pfm.bitcom.mq_enable) {
		mq_send_msg(&db);
	}

	pic_info.buf = NULL;
	pic_info.size = 0;
	platform_send_rg_records(&pic_info, &db);

	return 0;
}

int32_t process_rg_mq(MSGDATACMEM *msg)
{
	uint8_t *ptr;
	off_t offset;
	SystemVpss_AlgResultInfo alg;

	DEBUG("vd get a alg msg.");

	ptr = (uint8_t *)get_dsp_result_cmem_pointer();
	if (!ptr){
		ERROR("Alg result virtual address is NULL\n");
		return -1;
	}

	offset = msg->addr_phy - get_dsp_result_addr_phy();

	memcpy(&alg, ptr + offset, sizeof(SystemVpss_AlgResultInfo));

	rg_report_motor_vehicle_msg(&alg);

	return 0;

}

static int32_t rg_parse_pic(EP_PicInfo *pic_info,
							const SystemVpss_SimcopJpegInfo *info)
{
	const VD_Time *tm;
	const TrfcVehiclePlatePoint *result;

	tm = &info->algResultInfo.EPtime;
	result = (const TrfcVehiclePlatePoint *)(&info->algResultInfo.AlgResultInfo);

	rg_parse_pic_name(pic_info,
					  (const SystemVpss_AlgResultInfo *)&info->algResultInfo);

	pic_info->buf = (void *) ((char *)info + SIZE_JPEG_INFO);
	pic_info->size = info->jpeg_buf_size;

	DEBUG("pic_info.name: %s",pic_info->name);
	DEBUG("pic_info.size: %d",pic_info->size);

	return 0;
}

static int32_t record_send_to_plateform(const SystemVpss_SimcopJpegInfo *info)
{
	EP_PicInfo pic_info;
	DB_TrafficRecord db;
	char partition_path[MOUNT_POINT_LEN];
	const TrfcVehiclePlatePoint *result;

	result = (TrfcVehiclePlatePoint *)&(info->algResultInfo.AlgResultInfo);


	memset(partition_path, 0, MOUNT_POINT_LEN);
	memset(&pic_info, 0, sizeof(EP_PicInfo));
	memset(&db, 0, sizeof(db));

	analyze_traffic_records_picture(&pic_info, info);
	analyze_traffic_records_info_motor_vehicle(
		&db, result, &pic_info, partition_path);

	platform_send_rg_records(&pic_info, &db);

	return 0;
}



int32_t process_rg_records_motor_vehicle(const SystemVpss_SimcopJpegInfo *info)
{
	EP_PicInfo pic_info;
	char partition[MOUNT_POINT_LEN] = {0};
	const TrfcVehiclePlatePoint *result;
	DB_TrafficRecord db;
	ve_cfg_t cfg;

	INFO("Rcvd a record from dsp @ %s", ustime_msec());
	time_stat_update(VE_TS_DSP_REC);

	/* send to plateform */
	record_send_to_plateform(info);

	ve_cfg_get(&cfg);

	if (cfg.pfm.bitcom.ftp_enable) {
		/* parse pic info */
		rg_parse_pic(&pic_info, info);

		result = (const TrfcVehiclePlatePoint *)(&info->algResultInfo.AlgResultInfo);
		analyze_traffic_records_info_motor_vehicle(&db, result, &pic_info,
												   partition);
		ftp_send_pic(&pic_info);
	}

	return 0;
}

static int32_t mq_send_put(const char *text)
{
	mq_text_t *p, *end;
	int32_t ret = -1;

	end = mq_send.text + numberof(mq_send.text);

	pthread_mutex_lock(&mq_send.mutex);
	for (p = mq_send.text; p < end; ++p) {
		if (!p->valid) {
			DEBUG("put a mq.");
			DEBUG("%s", text);
			p->valid = true;
			strcpy(p->data, text);
			ret = 0;
			break;
		}
	}
	pthread_mutex_unlock(&mq_send.mutex);

	return ret;
}

static void *mq_send_thread(void *arg)
{
	mq_send_t *mq = (mq_send_t *)arg;
	mq_text_t *p, *end;
	mq_text_t text;
	sabitcom_mq_resend_t item;

	p = mq->text;
	end = mq->text + numberof(mq->text);

	for (;;) {
		pthread_mutex_lock(&mq->mutex);
		if (p->valid) {
			memcpy(&text, p, sizeof(text));
			p->valid = false;
		}
		++p;
		if (p == end) {
			p = mq->text;
		}
		pthread_mutex_unlock(&mq->mutex);

		if (text.valid) {
			text.valid = false;
			DEBUG("mq will be sent.");
			DEBUG("%s", text.data);
			if (0 != tabitcom_mq_send(text.data)) {
				ERROR("mq send failed, save to db");
				strncpy(item.text, text.data, sizeof(item.text));
				tabitcom_mq_resend_put(&item);
			}
		}
		usleep(10000);
	}

	return NULL;
}


void rg_record_ram_init(void)
{
	/* Do something */
}

int32_t tabitcom_mq_send_ram_init(void)
{
	mq_text_t *p, *end;

	pthread_mutex_init(&mq_send.mutex, NULL);

	end = mq_send.text + numberof(mq_send.text);
	for (p = mq_send.text; p < end; ++p) {
		p->valid = false;
	}

	return 0;
}

int32_t tabitcom_mq_send_pthread_init(void)
{
	pthread_attr_t attr;

	if (0 != pthread_attr_init(&attr)) {
		CRIT("Failed to initialize thread attrs\n");
		return -1;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&mq_send.thread, &attr, mq_send_thread, &mq_send)) {
		CRIT("Failed to create mq send thread\n");
	}

	pthread_attr_destroy(&attr);

	return 0;

}
