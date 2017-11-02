#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <errno.h>
#include <json/json.h>
#include <inttypes.h>
#include "traffic_records_process.h"
#include "commonfuncs.h"
#include "others.h"
#include "Msg_Def.h"
#include "proto.h"
#include "thread.h"
#include "dev/led.h"
#include "logger/log.h"
#include "ftp.h"
#include "types.h"
#include "sys/heap.h"
#include "sys/time_util.h"
#include "sys/xstring.h"
#include "config/ve_cfg.h"

extern PLATFORM_SET msgbuf_paltform;

int respond_tcp(char *buf, int len, str_bitcom_pro *proto)
{

	char data[1024];
	uint32_t cs;
	size_t l;
	std::string ask;
	Json::Value root;
	Json::StyledWriter writer;

	root["ret"] = 1;
	root["desc"] = gstr_tabitcom_status.SenseCoilState;

	ask = writer.write(root);

	l = ask.size();
	if ((l + 1) > sizeof(data)) {
		ERROR("Ask data buffer is too small");
		return -1;
	}

	memcpy(data, ask.c_str(), l);
	data[l] = 0;

	cs = get_sum_chk((uint8_t *)data, l);

	l = snprintf(buf, len, "%s,%s,%d,%d,%d,%s",
				 proto->hdr, proto->daddr, VE_DTS_TYPE_ASK, cs, l, data);

	return min(len - 1, (int)l);

}


ssize_t status_http(char *buf, size_t bufsz)
{
	char *hdr;
	Json::Value root;
	Json::StyledWriter writer;
	std::string  payload;
	ssize_t len;

	/*
	 * Http payload
	 */
	/* Fault */
	root["FaultState"] = gstr_tabitcom_status.FaultState;
	/* Sensor coil status */
	root["SenseCoilState"] = gstr_tabitcom_status.SenseCoilState;
	/* White light */
	root["FlashlLightState"] = gstr_tabitcom_status.FlashlLightState;
	/* Indicate light */
	root["IndicatoLightState"] = gstr_tabitcom_status.IndicatoLightState.all;
	payload = writer.write(root);


	/*
	 * Http header
	 */
	hdr = buf;
	len = snprintf(hdr, bufsz,
				   "POST /COMPANY/Devices/%s/DeviceStatus HTTP/1.1\r\n"
				   "Content-Type: application+json\r\n"
				   "Content-Length: %d\r\n"
				   "Host: %s:%d\r\n"
				   "Connection: Keep-alive\r\n"
				   "User-Agent: ice_wind\r\n\r\n"
				   "%s",
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId,
				   payload.length(),
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp,
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort,
				   payload.c_str());

	return min(len, (ssize_t)(bufsz - 1));
}

static int32_t direction_adjust(int32_t dir, int32_t obj_state)
{
	int32_t ret;

	/* 目标状态, 0: 与相机同方向, 1: 与相机反方向 */
	ret = (0 == obj_state) ? dir : (19 - dir);

	INFO("obj state: %d.", obj_state);
	INFO("the original direction is %d.", dir);
	INFO("the direction after adjust is %d.", ret);

	return ret;
}




ssize_t record_http(char *buf, size_t size, str_bitcom_http_pro *proto)
{
	DB_TrafficRecord *rec;
	EP_PicInfo *pic_info;
	char boundary[64], eboundary[64];
	char pic_hdr[512], json_hdr[512], pic_end[8];
	char plate_num[32];
	ssize_t len, left;
	ssize_t pic_hdr_len, pic_end_len, json_hdr_len, eboundary_len;
	ustime_t tm;
	Json::Value root;
	Json::StyledWriter writer;
	std::string json_payload;
	char *wp;

	ASSERT(proto->db_traffic_record);
	ASSERT(proto->pic_info);

	rec = proto->db_traffic_record;
	pic_info = proto->pic_info;

	/*
	 * Boundary
	 */
	ustime(&tm);
	snprintf(boundary, sizeof(boundary),
			 "----------COMPANYMSG%012x", (uint32_t)tm.sec);

	/*
	 * Convert vehicle information to json string
	 */
	ssize_t l = convert_enc_s("GBK", "UTF-8",
							  rec->plate_num, strlen(rec->plate_num),
							  plate_num, sizeof(plate_num));
	plate_num[l > 0 ? l : 0] = 0;

	/* 
	 * Record serial number, add for test
	 */
	static uint32_t sn = 0;
	root["sn"] = sn++;
	/* Longitude */
	root["Longitude"] = 0.0;
	/* Latitude */
	root["Latitude"] = 0.0;
	/* Car status, 0: real time, 1: history */
	root["VehicleInfoState"] = 0;
	/* Use pic url, it's zero if not used */
	root["IsPicUrl"] = 0;
	/* Lane number */
	root["LaneIndex"] = rec->lane_num;
	/* Plate position in first pic */
	root["position"][0u] = rec->coordinate_x;
	root["position"][1] = rec->coordinate_y;
	root["position"][2] = rec->width;
	root["position"][3] = rec->height;
	/* Direction */
	root["direction"] = direction_adjust(rec->direction, rec->obj_state);
	/* First Plate number */
	root["PlateInfo1"] = plate_num;
	/* Second plate number */
	root["PlateInfo2"] = "鲁B88888";
	/* Plate color */
	root["PlateColor"] = rec->plate_color;
	/* Plate type */
	root["PlateType"] = rec->plate_type;
	/* Car pass time, format: YYYY-MM-DD HH(24):MI:S, this is string */
	root["PassTime"] = rec->time;
	/* Speed */
	root["VehicleSpeed"] = (double)rec->speed;
	root["LaneMiniSpeed"] = 0.0;
	root["LaneMaxSpeed"] = 0.0;
	/* Vehicle type */
	root["VehicleType"] = rec->vehicle_type;
	root["VehicleSubType"] = 0;
	root["VehicleColor"] = "H";
	root["VehicleColorDepth"] = 0;
	root["VehicleLength"] = 0;
	root["VehicleState"] = 1;
	root["PicCount"] = 1;
	root["PicType"][0u] = 1;
	root["PlatePicUrl"] = "";
	root["VehiclePic1Url"] = "";
	root["VehiclePic2Url"] = "";
	root["VehiclePic3Url"] = "";
	root["CombinedPicUrl"] = "";
	root["AlarmAction"] = 1;
	root["DetectCoilTime"] = rec->detect_coil_time;
	root["Confidence"] = rec->confidence;
	json_payload = writer.write(root);

	/*
	 * Make header
	 */
	json_hdr_len = snprintf(json_hdr, sizeof(json_hdr),
							"\r\n"
							"--%s\r\n"
							"Content-Type: application/json;charset=UTF-8\r\n",
							boundary);
	pic_hdr_len = snprintf(pic_hdr, sizeof(pic_hdr),
						   "--%s\r\n"
						   "Content-Type: image/jpeg\r\n",
						   boundary);
	pic_end_len = snprintf(pic_end, sizeof(pic_end), "\r\n");

	eboundary_len = snprintf(eboundary, sizeof(eboundary),
							 "--%s--", boundary);

	len = json_hdr_len + eboundary_len + json_payload.size();
	if (pic_info->buf) {
		len += pic_hdr_len + pic_info->size + pic_end_len;
	}

	wp = buf;
	/* Remain a byte for eof */
	left = size - 1;
	len = snprintf(wp, left,
				   "POST /COMPANY/Devices/%s/Datas HTTP/1.1\r\n"
				   "Content-Type: multipart/form-data;boundary=%s\r\n"
				   "Content-Length: %d\r\n"
				   "Host: %s:%d\r\n"
				   "Connection: Keep-alive\r\n"
				   "User-Agent: ice_wind\r\n"
				   "\r\n",
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId,
				   boundary, len,
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp,
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);
	len = min(len, left - 1);
	left -= len;
	wp += len;

	/*
	 * Copy json header
	 */
	len = min(left, json_hdr_len);
	memcpy(wp, json_hdr, len);
	left -= len;
	wp += len;

	/*
	 * Copy json body
	 */
	len = min(left, (ssize_t)json_payload.size());
	memcpy(wp, json_payload.c_str(), len);
	left -= len;
	wp += len;

	if (pic_info->buf) {
		/*
		 * Copy image header
		 */
		len = min(left, pic_hdr_len);
		memcpy(wp, pic_hdr, len);
		left -= len;
		wp += len;

		/*
		 * Copy image
		 */
		len = min(left, pic_info->size);
		memcpy(wp, pic_info->buf, len);
		left -= len;
		wp += len;

		/*
		 * Append a crlf
		 */
		len = min(left, pic_end_len);
		memcpy(wp, pic_end, len);
		left -= len;
		wp += len;
	}

	/*
	 * Copy end boundary
	 */
	len = min(left, eboundary_len);
	memcpy(wp, eboundary, len);
	left -= len;
	wp += len;

	/* Add the eof */
	*wp = 0;

	return size - left;
}

static ssize_t heartbeat_http(char *buf, size_t bufsz)
{
	char nowtime[64];
	char body[128];
	ssize_t len;

	ustime_sec_s(nowtime, sizeof(nowtime));
	len = snprintf(body, sizeof(body),
				   "{\r\n\"Time\":\"%s\"\n}\r\n", nowtime);
	len = snprintf(buf, bufsz,
				   "POST /COMPANY/Devices/%s/Keepalive HTTP/1.1\r\n"
				   "Content-Type: application/json\r\n"
				   "Content-Length: %d\r\n"
				   "Host: %s:%d\r\n"
				   "Connection: Keep-alive\r\n\r\n"
				   "%s",
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId,
				   len,
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp,
				   msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort,
				   body);

	return min(len, (ssize_t)(bufsz - 1));
}


int pro_encode(char *ac_buff, int ai_lens,  void *aos_bitcom_pro, int ai_protype)
{
	int li_ret = 0;

	str_bitcom_http_pro *pstr_bitcom_http_pro = NULL;

	str_bitcom_pro *pstr_bitcom_pro = NULL;

	switch (ai_protype) {
		case 1:
			li_ret = heartbeat_http(ac_buff, ai_lens);
			break;

		case 2:
			li_ret = status_http(ac_buff, ai_lens);
			break;

		case 3:
			pstr_bitcom_http_pro = (str_bitcom_http_pro *)aos_bitcom_pro;
			li_ret = record_http(ac_buff, ai_lens, pstr_bitcom_http_pro);
			break;

		case 4:
			pstr_bitcom_pro = (str_bitcom_pro *)aos_bitcom_pro;
			li_ret = respond_tcp(ac_buff, ai_lens, pstr_bitcom_pro);
			break;
	}

	return li_ret;
}


const uint8_t* dts_find_head(const uint8_t *data, size_t len)
{
	const uint8_t *p;

	ASSERT(data);

	p = data;
	while (len--) {
		if (('I' == *p) && ('P' == *(p + 1))
			&& ('N' == *(p + 2)) && 'C' == *(p + 3)) {
			return p;
		}

		++p;
	}

	return NULL;
}

int32_t dts_get_frm_info(const uint8_t *hdr, size_t len, dts_head_t *info)
{
	const uint8_t *p, *comma;
	int32_t cnt;
	char buf[128];
	size_t l;

	ASSERT(hdr);

	p = hdr;
	comma = hdr;
	cnt = 0;

	while (len--) {
		if (',' == *comma++) {

			++cnt;
			l = (size_t)(comma - p) - 1;

			switch (cnt) {
				case 1:
					/*
					 * Start of frame
					 */
					l = min(l, sizeof(info->sof) - 1);
					memcpy(info->sof, p, l);
					info->sof[l] = 0;

					break;

				case 2:
					/*
					 * Destination address
					 */
					l = min(l, sizeof(info->daddr) - 1);
					memcpy(info->daddr, p, min(l, sizeof(info->daddr)));
					info->daddr[l] = 0;
					break;

				case 3:
					/*
					 * Mesage type
					 */
					l = min(l, sizeof(buf) - 1);
					memcpy(buf, p, l);
					buf[l] = 0;
					info->type = strtol(buf, NULL, 10);
					break;

				case 4:
					/*
					 * Check summary
					 */
					l = min(l, sizeof(buf) - 1);
					memcpy(buf, p, l);
					buf[l] = 0;
					info->cs = strtol(buf, NULL, 10);
					break;

				case 5:
					/*
					 * Data region length
					 */
					l = min(l, sizeof(buf) - 1);
					memcpy(buf, p, l);
					buf[l] = 0;
					info->data_len = strtol(buf, NULL, 10);
					break;

				default:
					break;
			}
			p = comma;

			if (5 == cnt) {
				info->head_len = (size_t)(comma - hdr);
				return 0;
			}
		}
	}

	return -1;
}

uint32_t get_sum_chk(const uint8_t *data, size_t len)
{
	uint32_t cs;
	const uint8_t *p, *end;

	ASSERT(data);

	cs = 0;
	for (p = data, end = data + len; p < end; ++p) {
		cs += *p;
	}

	return cs;
}

static int_fast32_t led_line_json_chk(Json::Value &line, int_fast32_t nlines,
		int_fast32_t nlines_max)
{
	(void)nlines;

	if (line["LineNumber"].isNull()) {
		ERROR("LineNumber is not exist in led json");
		return -1;
	}

	if (line["LineNumber"].type() != Json::intValue) {
		ERROR("Type of LineNumber is not intValue");
		return -1;
	}

	int_fast32_t idx = line["LineNumber"].asInt();
	if (idx >= nlines_max) {
		ERROR("An error line number %d in led json", idx);
		return -1;
	}

	if (line["Action"].isNull()) {
		ERROR("Action is not exist in led json");
		return -1;
	}

	if (line["Action"].type() != Json::intValue) {
		ERROR("Type of Action is not intValue");
		return -1;
	}

	int_fast32_t action = line["Action"].asInt();
	if ((action < 1) || (action > 2)) {
		ERROR("An error line action %d in led json", action);
		return -1;
	}

	if (action == 2) {
		return 0;
	}

	if (line["LedMode"].isNull()) {
		ERROR("LedMode is not exist in led json");
		return -1;
	}

	if (line["LedMode"].type() != Json::intValue) {
		ERROR("Type of LedMode is not intValue");
		return -1;
	}

	int_fast32_t mode = line["LedMode"].asInt();
	if ((mode < 1) || (mode > 2)) {
		ERROR("An error line mode %d in led json", mode);
		return -1;
	}

	if (line["RollingSpeed"].isNull()) {
		ERROR("RollingSpeed is not exist in led json");
		return -1;
	}

	if (line["RollingSpeed"].type() != Json::intValue) {
		ERROR("Type of RollingSpeed is not intValue");
		return -1;
	}

	if (line["Text"].isNull()) {
		ERROR("Text is not exist in led json");
		return -1;
	}

	if (line["Text"].type() != Json::stringValue) {
		ERROR("Type of Text is not stringValue");
		return -1;
	}

	return 0;
}

static int_fast32_t parse_led_v09(Json::Value &jsLed, led_line_t **led_lines,
								  uint8_t *lines_wp)
{
	const char *space = " ";

	/*
	 * ve_led_get_max_line_num return zero means that
	 * the led model is disabled. If the led model is
	 * disabled, do not insert to led buffer
	 */
	if (ve_led_get_max_line_num() > 0) {
		led_line_t *led_line = led_line_malloc();
		if (led_line) {
			led_line->idx = 0;
			led_line->mode = LED_MODE_LINE;
			led_line->color.r = 255;
			led_line->volume = 100;

			led_line->action = (jsLed["LedAction"].asInt() == 1) ?
				LED_ACTION_DISP : LED_ACTION_CLR;
			led_line->duration = jsLed["LedDuration"].asInt();

			/*
			 * If the field of "LedContent" is not exist,
			 * copy " " to buffer
			 */
			const char *s = jsLed["LedContent"].isNull() ?
				space : jsLed["LedContent"].asCString();
			char gbk[1024];

			convert_enc("UTF-8", "GBK", s, strlen(s) + 1, gbk, sizeof(gbk));
			strlcpy(led_line->text, gbk, sizeof(led_line->text));

			led_lines[(*lines_wp)++] = led_line;
		}
	}

	return 0;
}

static int_fast32_t parse_led_v10(Json::Value &jsLed, led_line_t **led_lines,
								  uint8_t *lines_wp)
{
	/*
	 * ve_led_get_max_line_num return zero means that
	 * the led model is disabled. If the led model is
	 * disabled, do not insert to led buffer
	 */
	if (jsLed["Lines"].size() != jsLed["LinesNumber"].asUInt()) {
		ERROR("Lines.size not equal LinesNumber, %d:%d",
			  jsLed["Lines"].size(), jsLed["LinesNumber"].asUInt());
		return -1;
	}

	int_fast32_t nlines = ve_led_get_max_line_num();
	nlines = min(nlines, (int_fast32_t)jsLed["Lines"].size());
	if (nlines <= 0) {
		ERROR("Led total %d lines", nlines);
		return -1;
	}

	Json::Value jslines = jsLed["Lines"];
	int_fast32_t volume = jsLed["Volume"].asInt();
	int_fast32_t mode = jsLed["Mode"].asInt() == 1 ?
		LED_MODE_LINE : LED_MODE_PAGE;

	for (int_fast32_t i = 0; i < nlines; ++i) {
		Json::Value jsline = jslines[i];

		if(led_line_json_chk(jsline, nlines, ve_led_get_max_line_num()) != 0){
			ERROR("Check led line's json failed");
			continue;
		}

		led_line_t *led_line = led_line_malloc();
		if (led_line) {
			led_line->idx = jsline["LineNumber"].asInt();
			led_line->action = 0;

			if (jsline["Action"].asInt() == 2) {
				led_line->action |= LED_ACTION_CLR;
			} else {
				led_line->volume = volume;
				led_line->mode = mode;
				led_line->style = (jsline["LedMode"].asInt() == 1) ?
					LED_STYLE_IMMEDIATE : LED_STYLE_SHIFT_LEFT;

				if ((!jsline["Duration"].isNull())
					&& (jsline["Duration"].type() == Json::intValue)) {
					led_line->duration = jsline["Duration"].asInt();
				} else {
					led_line->duration = 0;
				}

				if ((!jsline["Cover"].isNull())
					&& (jsline["Cover"].type() == Json::intValue)) {
					led_line->cover = (bool)jsline["Cover"].asInt();
				} else {
					led_line->cover = false;
				}

				if ((!jsline["color"].isNull())
					&& (jsline["color"].type() == Json::stringValue)) {
					if (rgb_str2bin(&led_line->color,
							jsline["color"].asString().c_str()) != 0) {
						led_line->color.rgb = 0x00FF0000;
					}
				} else {
					led_line->color.rgb = 0x00FF0000;
				}

				led_line->time_per_col = jsline["RollingSpeed"].asInt();

				if (!jsline["Text"].isNull()) {
					const char *utf8 = jsline["Text"].asCString();
					char gbk[1024];

					ssize_t l = convert_enc_s("UTF-8", "GBK",
											  utf8, strlen(utf8),
											  gbk, sizeof(gbk));
					if (l >= 0) {
						gbk[l] = 0;
					} else {
						ERROR("Convert utf8 to gbk failed");
						const char dft_disp[] = "Welcome";
						strlcpy(gbk, dft_disp, sizeof(gbk));
					}

					strlcpy(led_line->text, gbk, sizeof(led_line->text));
					led_line->action |= LED_ACTION_DISP;

				}

				if (!jsline["Audio"].isNull()) {
					led_line->action |= LED_ACTION_AUDIO;
					const char *audio = jsline["Audio"].asCString();
					strlcpy(led_line->audio, audio, sizeof(led_line->audio));
				}
			}

			led_lines[(*lines_wp)++] = led_line;

		}
	}

	return 0;
}

int pro_decode(char *ac_buff, int ai_lens, str_bitcom_pro *os_bitcom_pro, char ac_protype)
{
	Json::Reader reader;
	Json::Value root;
	Json::Value jsRoadGate;
	Json::Value jsLed;
	Json::Value jsAudio;
	Json::Value jsCapture;
	Json::Value jsLock;
	Json::Value jsVip;
	size_t len;
	char json[1024 * 1024];
	dts_head_t hdr_info;
	int_fast32_t ret;

	if (ac_protype == PAB_TCP_PRO) {
		if (0 == dts_get_frm_info((uint8_t *)ac_buff, ai_lens, &hdr_info)) {
			snprintf(os_bitcom_pro->hdr, sizeof(os_bitcom_pro->hdr),
					 "%s", hdr_info.sof);
			os_bitcom_pro->type = hdr_info.type;


			if ((hdr_info.data_len + 1) > sizeof(json)) {
				ERROR("Json buffer is too small");
				return -1;
			}

			memcpy(json, ac_buff + hdr_info.head_len, hdr_info.data_len);
			json[hdr_info.data_len] = 0;

			if (reader.parse(json, root)) {
				if ((!root["Version"].isNull())
					&& (root["Version"].type() == Json::stringValue)) {
					sscanf(root["Version"].asCString(), "%d.%d",
						  &os_bitcom_pro->ver.major,
						  &os_bitcom_pro->ver.minor);
				} else {
					os_bitcom_pro->ver.major = 0;
					os_bitcom_pro->ver.minor = 0;
					ERROR("Can't get version from ve down frame");
				}

				DEBUG("VE down version is %d.%d",
					  os_bitcom_pro->ver.major, os_bitcom_pro->ver.minor);

				os_bitcom_pro->roadgate.valid_flag = 0;
				if (root["RoadGate"].type() != Json::nullValue) {
					jsRoadGate = root["RoadGate"];
					os_bitcom_pro->roadgate.barriergate = jsRoadGate["BarrierGate"].asInt();
					os_bitcom_pro->roadgate.bgkeeptime = jsRoadGate["BGKeepTime"].asInt();
					os_bitcom_pro->roadgate.bgmode = jsRoadGate["BGMode"].asInt();
					os_bitcom_pro->roadgate.valid_flag = 1;
				}

				os_bitcom_pro->led.valid_flag = 0;
				if (root["Led"].type() != Json::nullValue) {
					jsLed = root["Led"];

					led_line_t **led_lines = os_bitcom_pro->led.lines;
					uint8_t *lines_wp = &os_bitcom_pro->led.nlines;

					*lines_wp = 0;


					/* The old version */
					if (os_bitcom_pro->ver.major < 1) {
						ret = parse_led_v09(jsLed, led_lines, lines_wp);
					} else {
						ret = parse_led_v10(jsLed, led_lines, lines_wp);
					}

					if (ret == 0) {
						os_bitcom_pro->led.valid_flag = 1;
					}
				}

				os_bitcom_pro->voice.valid_flag = 0;
				if (root["Audio"].type() != Json::nullValue) {
					jsAudio = root["Audio"];
					stringstream sstraudio(jsAudio["VoiceContent"].asString());
					sstraudio >> os_bitcom_pro->voice.content;
					os_bitcom_pro->voice.volume = jsAudio["VoiceVolume"].asInt();
					os_bitcom_pro->voice.valid_flag = 1;
				}

				os_bitcom_pro->capture.valid_flag = 0;
				if (root["Capture"].type() != Json::nullValue) {
					jsCapture = root["Capture"];
					os_bitcom_pro->capture.mode = jsCapture["CaptureMode"].asInt();
					os_bitcom_pro->capture.reg = jsCapture["CaptureReg"].asInt();
					os_bitcom_pro->capture.picnum = jsCapture["CapturePicNum"].asInt();
					os_bitcom_pro->capture.valid_flag = 1;
				}

				os_bitcom_pro->lock.valid = false;
				if (root["Lock"].type() != Json::nullValue) {
					jsLock = root["Lock"];
					os_bitcom_pro->lock.action = jsLock["LockAction"].asInt();
					os_bitcom_pro->lock.valid = true;
				}

				os_bitcom_pro->vip.valid = false;
				if (root["ParkVipInfo"].type() != Json::nullValue) {
					jsVip = root["ParkVipInfo"];
					len = jsVip.toStyledString().length();

					os_bitcom_pro->vip.data = (char *)xmalloc(len + 1);
					if (NULL == os_bitcom_pro->vip.data) {
						CRIT("Failed to alloc memory for vip data");
						return -1;
					}

					os_bitcom_pro->vip.data_len = len + 1;
					memcpy(os_bitcom_pro->vip.data,
						   jsVip.toStyledString().c_str(), len);
					os_bitcom_pro->vip.data[len] = 0;

					os_bitcom_pro->vip.valid = true;
				}
			}

			return 0;

		}
	} else if (ac_protype == PAB_HTTP_PRO) {

	}

	return -1;

}

int alleyway_bitcom_protocol(char *ac_buff, int ai_lens, void *aos_bitcom_pro,
							 char ac_protype, char ac_prostyle)
{
	int li_ret = 0;

	str_bitcom_pro *pstr_bitcom_pro = NULL;

	switch (ac_prostyle) {
		case PRO_ENCODE:
			li_ret = pro_encode(ac_buff, ai_lens, aos_bitcom_pro, ac_protype);
			break;

		case PRO_DECODE:
			pstr_bitcom_pro = (str_bitcom_pro *)aos_bitcom_pro;
			li_ret = pro_decode(ac_buff, ai_lens, pstr_bitcom_pro, ac_protype);
			break;
	}

	return li_ret;
}

static int32_t tcp_connect(const char *ip, uint16_t port, int32_t retry)
{
	int32_t sockfd = -1;
	struct sockaddr_in serv_addr;

	while ((retry--) > 0) {
		usleep(30000);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (-1 == sockfd) {
			continue;
		}

		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		serv_addr.sin_addr.s_addr = inet_addr(ip);

		if (0 == connect(sockfd, (struct sockaddr *)(&serv_addr),
						 sizeof(struct sockaddr))) {
			TRACE_LOG_SYSTEM("tcp[%d] connect %s:%d successful!", sockfd, ip, port);
			return sockfd;
		}

		close(sockfd);
	}

	TRACE_LOG_SYSTEM("tcp connect %s:%d failed!", ip, port);

	return -1;
}


static int32_t tcp_send(int32_t sockfd, const uint8_t *data, size_t len)
{
	size_t left = len;
	ssize_t ret;
	const uint8_t *p = data;

	TRACE_LOG_SYSTEM("tcp[%d] will send %d bytes", sockfd, left);
	while (left > 0) {
		ret = send(sockfd, p, left, 0);
		if (ret >= 0) {
			left -= ret;
			p += ret;
		} else if ((EAGAIN != ret) && (EWOULDBLOCK != ret) && (EINTR != ret)) {
			TRACE_LOG_SYSTEM("tcp[%d] left %d bytes send failed!", sockfd, left);
			return -1;
		}
	}

	TRACE_LOG_SYSTEM("tcp[%d] has sent %d bytes", sockfd, len);

	return 0;
}

/**
 * tcp_recv - tcp接收
 *
 * @sockfd: 套接字
 * @data: 接收缓存
 * @size: 接收缓存大小
 * @timeout: 超时时间, 毫秒
 *
 * Return:
 *  >0 - 接收到的字节数
 *  -1 - 套接字失效
 */
static ssize_t tcp_recv(int32_t sockfd, uint8_t *data, size_t size, int32_t timeout)
{
	fd_set rset;
	struct timeval tv;
	ssize_t ret;

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout & 1000) * 1000;

	ret = select(sockfd + 1, &rset, NULL, NULL, &tv);
	if (-1 == ret) {
		TRACE_LOG_SYSTEM("socket[%d] select failed.", sockfd);
		return -1;
	} else if (0 == ret) {
		TRACE_LOG_SYSTEM("socket[%d] select timeout.", sockfd);
		return 0;
	}

	if (!FD_ISSET(sockfd, &rset)) {
		TRACE_LOG_SYSTEM("socket[%d] do not have data to read.", sockfd);
		return -1;
	}

	ret = recv(sockfd, data, size, 0);
	if ((ret <= 0) && (ret != EAGAIN) && (ret != EWOULDBLOCK) && (ret != EINTR)) {
		/* 套接字失效 */
		TRACE_LOG_SYSTEM("socket[%d] is error.", sockfd);
		return -1;
	}

	TRACE_LOG_SYSTEM("socket[%d] recv %d bytes.", sockfd, ret);

	return ret;
}

static int32_t tcp_socket_chk(int32_t sockfd, int32_t timeout)
{
	fd_set rset;
	struct timeval tv;
	uint8_t dummy[128];

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout & 1000) * 1000;

	if (-1 == select(sockfd + 1, &rset, NULL, NULL, &tv)) {
		TRACE_LOG_SYSTEM("socket[%d] select failed.", sockfd);
		return -1;
	}

	if (FD_ISSET(sockfd, &rset)) {
		TRACE_LOG_SYSTEM("socket[%d] has something to read.", sockfd);
		if (recv(sockfd, dummy, sizeof(dummy), 0) <= 0) {
			TRACE_LOG_SYSTEM("socket[%d] is error.", sockfd);
			return -1;
		}
	}

	return 0;
}

static void http_close(http_client_t *hc)
{
	close(hc->sockfd);
	hc->sockfd = -1;
}

void http_client_init(http_client_t *hc, const char *ip, uint16_t port,
					  bool tcplong, int32_t retry, int32_t recv_timeout)
{
	strncpy(hc->ip, ip, sizeof(hc->ip));
	hc->port = port;
	hc->retry = retry;
	hc->sockfd = -1;
	hc->tcplong = tcplong;
	hc->recv_timeout = recv_timeout;
	pthread_mutex_init(&hc->mutex, NULL);

}

void http_client_set_server_param(http_client_t *hc,
								  const char *ip, uint16_t port)
{
	if ((port != hc->port) || (0 != strncmp(hc->ip, ip, strlen(ip)))) {
		TRACE_LOG_SYSTEM("%s:%d change to %s:%d!", hc->ip, hc->port, ip, port);
		strncpy(hc->ip, ip, sizeof(hc->ip));
		hc->port = port;
		if (-1 != hc->sockfd) {
			close(hc->sockfd);
			hc->sockfd = -1;
		}
	}
}

int32_t http_send_recv(http_client_t *hc,
					   const uint8_t *send_buf, size_t send_len,
					   uint8_t *recv_buf, size_t sz_recv_buf)
{
	bool sockok;
	ssize_t ret = -1;
	int32_t retry = hc->retry;

	pthread_mutex_lock(&hc->mutex);

	while ((retry--) > 0) {

		sockok = false;

		if (hc->tcplong && (-1 != hc->sockfd)) {
			if (0 == tcp_socket_chk(hc->sockfd, 50)) {
				sockok = true;
			}
		}

		if (!sockok) {
			hc->sockfd = tcp_connect(hc->ip, hc->port, hc->retry);
			if (-1 == hc->sockfd) {
				pthread_mutex_unlock(&hc->mutex);
				return -1;
			}
		}

		if (-1 == tcp_send(hc->sockfd, send_buf, send_len)) {
			http_close(hc);
			continue;
		}

		ret = tcp_recv(hc->sockfd, recv_buf, sz_recv_buf, hc->recv_timeout);
		if (-1 == ret) {
			http_close(hc);
			continue;
		} else if (ret > 0) {
			break;
		}
	}

	if ((!hc->tcplong) && (-1 != hc->sockfd)) {
		http_close(hc);
	}

	pthread_mutex_unlock(&hc->mutex);
	return (retry < 0) ? -1 : ret;
}
