#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "types.h"
#include "Msg_Def.h"
#include "commonfuncs.h"
#include "http_resend.h"
#include "logger/log.h"


C_CODE_BEGIN

typedef struct http_resend{
	sqlite_t sqlite;
	pthread_mutex_t mutex;
	pthread_t thread;
	char pic_dir[256];
	int32_t nr_record;
	int32_t backoff_ptr;
	bool on;
}http_resend_t;

static void *http_resend_pthread(void *arg);
static ssize_t http_recombine(uint8_t *dest, size_t sz_dest,
							  tabitcom_http_info_t *info,
							  char *json,  uint8_t *pic,
							  size_t pic_len);
extern ssize_t tabitcom_http_record(const uint8_t *data, size_t len);


static const uint32_t backoff_time[] = {8, 16, 32, 64, 128, 256};
static http_resend_t http_resend;
extern PLATFORM_SET msgbuf_paltform;

int32_t tabitcom_http_resend_ram_init(const char *db_path,
									  const char *pic_dir,
									  int32_t nr_record)
{
	pthread_mutex_init(&http_resend.mutex, NULL);
	sqlite_open(&http_resend.sqlite, db_path);
	strncpy(http_resend.pic_dir, pic_dir, sizeof(http_resend.pic_dir));
	sabitcom_http_resend_tab_create(&http_resend.sqlite);
	http_resend.nr_record = nr_record;
	http_resend.on = true;
	http_resend.backoff_ptr = 0;

	return 0;
}

int32_t tabitcom_http_resend_set_switch(bool on)
{
	pthread_mutex_lock(&http_resend.mutex);
	http_resend.on = on;
	pthread_mutex_unlock(&http_resend.mutex);
	return 0;
}

int32_t tabitcom_http_resend_set_record_num(int32_t nr)
{
	pthread_mutex_lock(&http_resend.mutex);
	http_resend.nr_record = nr;
	pthread_mutex_unlock(&http_resend.mutex);
	return 0;
}

int32_t tabitcom_http_resend_pthread_init(void)
{
	pthread_attr_t attr;

	if (0 != pthread_attr_init(&attr)) {
		ERROR("Failed to initialize thread attrs\n");
		return -1;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&http_resend.thread,
					   &attr,
					   http_resend_pthread,
					   &http_resend)) {
		ERROR("Failed to create speech thread\n");
	}

	pthread_attr_destroy(&attr);

	return 0;
}


static void *http_resend_pthread(void *arg)
{
	int32_t id;
	char path[512];
	uint8_t http[SIZE_JPEG_BUFFER];
	ssize_t len;
	FILE *file;
	sabitcom_http_resend_t item;
	tabitcom_http_info_t info;
	http_resend_t *hr = (http_resend_t *)arg;

	for (;;) {
		sleep(backoff_time[hr->backoff_ptr]);
		pthread_mutex_lock(&hr->mutex);

		do {
			if (!hr->on) {
				break;
			}

			id = sabitcom_http_resend_select(&hr->sqlite, &item);
			if (id <= 0) {
				break;
			}

			snprintf(path, sizeof(path), "%s/%s", hr->pic_dir, item.name);
			file = fopen(path, "rb");
			if (NULL == file) {
				ERROR("Open %s failed", path);
				/* 图片文件打开失败, 删除记录 */
				sabitcom_http_resend_del_id(&hr->sqlite, id);
				break;
			}

			len = fread(item.pic, sizeof(uint8_t), sizeof(item.pic), file);
			fclose(file);

			if (0 == len) {
				/* 长度为0读取失败 */
				ERROR("Read pic file failed, %s", path);
				/* 图片文件读取失败删除记录 */
				sabitcom_http_resend_del_id(&hr->sqlite, id);
				break;
			}

			strncpy(info.usr_agent, "ice_wind", sizeof(info.usr_agent));
			strncpy(info.dev_id,
					msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId,
					sizeof(info.dev_id));
			strncpy(info.serv_ip,
					msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp,
					sizeof(info.serv_ip));
			info.port = msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort;

			len = http_recombine(http, sizeof(http), &info, item.json,
								 item.pic, len);
			if (len <= 0) {
				ERROR("Http recombine failed");
				sabitcom_http_resend_del_id(&hr->sqlite, id);
				remove(path);
				break;
			}

			if(0 == tabitcom_http_record(http, len)){
				DEBUG("Delete http record %d.", id);
				sabitcom_http_resend_del_id(&hr->sqlite, id);
				remove(path);
				hr->backoff_ptr = 0;
			} else {
				++hr->backoff_ptr;
				if (numberof(backoff_time) == hr->backoff_ptr) {
					hr->backoff_ptr = 0;
				}
			}
		}while (0);

		pthread_mutex_unlock(&hr->mutex);
	}

	return NULL;
}

int32_t tabitcom_http_resend_put(const sabitcom_http_resend_t *item)
{
	int32_t ret, i;
	sabitcom_http_name_t del_name[10];
	char buf[512];
	FILE *file;

	pthread_mutex_lock(&http_resend.mutex);

	if (!http_resend.on) {
		pthread_mutex_unlock(&http_resend.mutex);
		return -1;
	}

	ret = sabitcom_http_resend_del_redundant(&http_resend.sqlite,
											http_resend.nr_record,
											del_name,
											numberof(del_name));
	for (i = 0; i < ret; ++i) {
		snprintf(buf, sizeof(buf), "%s/%s", http_resend.pic_dir, del_name[i]);
		remove(buf);
	}

	ret = sabitcom_http_resend_insert(&http_resend.sqlite, item);
	if (SQLITE_OK == ret) {
		snprintf(buf, sizeof(buf), "%s/%s", http_resend.pic_dir, item->name);
		file = fopen(buf, "wb");
		if (file) {
			fwrite(item->pic, sizeof(uint8_t), item->size, file);
			fclose(file);
		}
	}

	pthread_mutex_unlock(&http_resend.mutex);

	return ret;
}

static ssize_t http_record_get_boundary(const char *record,
										char *buf, size_t sz_buf)
{
	const char *boundary, *content_type, *src;
	char *dest, *dest_end;

	content_type = strstr(record, "Content-Type: ");
	if (!content_type) {
		return -1;
	}

	boundary = strstr(content_type, "boundary=");
	if (!boundary) {
		return -1;
	}

	src = boundary + strlen("boundary=");
	dest = buf;
	dest_end = buf + sz_buf;
	memset(buf, 0, sz_buf);
	while (('\n' != *src) && (dest < dest_end)){
		*dest++ = *src++;
	}

	return (ssize_t)(dest - buf);
}

/**
 * http_record_get_json - 解析http协议中的字符串
 * @data: 分隔符之后的报文
 * @data_len: 报文长度
 * @boundary: 分割符
 * @buf: json串缓存
 * @sz_buf: json串缓存大小
 *
 * Return:
 *  >0: json长度, 不包含结束符
 *  其他: 错误
 */
static ssize_t http_record_get_json(const char *data, const char *boundary,
									char *buf, size_t sz_buf)
{
	const char ident[] = "Content-Type: application/json;charset=UTF-8\r\n";
	const char *src, *src_end;
	char *dest, *dest_end;

	/* 找标识字符串 */
	src = strstr(data, ident);
	if (!src) {
		TRACE_LOG_SYSTEM("get json ident failed!");
		return -1;
	}
	src += strlen(ident);

	/* 找下一个分隔符, src与src_end之间的是json串 */
	src_end = strstr(src, boundary);
	if (!src_end) {
		TRACE_LOG_SYSTEM("find boundary failed!");
		return -1;
	}

	dest = buf;
	dest_end = buf + sz_buf;

	/* 缓存大小判断, 最终放到缓存中的json串包含结束符, 所以要减1 */
	if ((src_end - src) > (sz_buf - 1)) {
		TRACE_LOG_SYSTEM("buffer is too small!");
		return -1;
	}

	while (src < src_end) {
		*dest++ = *src++;
	}

	*dest = 0;
	return (ssize_t)(dest - buf);
}

int32_t tabitcom_http_get_resend_info(sabitcom_http_resend_t *item,
									  const char *http,
									  const uint8_t *pic,
									  size_t sz_pic)
{
	char buf[256];
	struct tm *tm;
	struct timeval tv;
	time_t rawtime;
	static int32_t cnt = 0;

	if(http_record_get_boundary(http, buf, sizeof(buf)) < 0){
		ERROR("Get boundary failed!");
		return -1;
	}

	if(http_record_get_json(http, buf, item->json, sizeof(item->json)) < 0){
		ERROR("Get json failed!");
		return -1;
	}

	item->size = min(sz_pic, sizeof(item->pic));
	memcpy(item->pic, pic, item->size);

	gettimeofday(&tv, NULL);
	time(&rawtime);
	tm = localtime(&rawtime);
	snprintf(item->name, sizeof(item->name),
			 "%04d%02d%02d%02d%02d%02d%03d%d.jpg",
			 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
			 tm->tm_min, tm->tm_sec, (int32_t)tv.tv_usec / 1000, ++cnt);

	return 0;
}

static ssize_t http_recombine(uint8_t *dest, size_t sz_dest,
							  tabitcom_http_info_t *info,
							  char *json,  uint8_t *pic,
							  size_t pic_len)
{
	char boundary[128];
	char hdr[1024];
	char json_body[1024];
	uint8_t image_body[1024 * 1024];
	uint8_t *p, *p_end;
	time_t rawtime;
	size_t len, image_body_len;

	time(&rawtime);
	sprintf(boundary, "----------COMPANYMSG%012x", (uint32_t)rawtime);

	/* json体 */
	snprintf(json_body, sizeof(json_body),
			 "--%s\r\n"
			 "Content-Type: application/json;charset=UTF-8\r\n"
			 "%s\r\n"
			 "--%s",
			 boundary,
			 json,
			 boundary);

	/* 图片体 */
	p = image_body;
	p_end = image_body + sizeof(image_body);
	p += snprintf((char *)p, (size_t)(p_end - p), "Content-Type: image/jpeg\r\n");
	len = min(pic_len, (size_t)(p_end - p));
	memcpy(p, pic, len);
	p += len;
	p += snprintf((char *)p, p_end - p, "--%s--", boundary);
	image_body_len = (size_t)(p - image_body);

	/* http头 */
	snprintf(hdr, sizeof(hdr),
			 "POST /COMPANY/Devices/%s/Datas HTTP/1.1\r\n"
			 "Content-Type: multipart/form-data;boundary=%s\r\n"
			 "Content-Length: %d\r\n"
			 "Host: %s:%d\r\n"
			 "Connection: Keep-alive\r\n"
			 "User-Agent: %s\r\n"
			 "\r\n",
			 info->dev_id,
			 boundary,
			 strlen(json_body) + image_body_len,
			 info->serv_ip, info->port,
			 info->usr_agent);

	p = dest;
	p_end = dest + sz_dest;

	p += snprintf((char *)p, (size_t)(p_end - p), "%s%s", hdr, json_body);
	len = min((size_t)(p_end - p), image_body_len);
	memcpy(p, image_body, len);
	p += len;

	return (ssize_t)(p - dest);
}


C_CODE_END


