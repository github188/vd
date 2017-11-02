#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sqlite3.h>
#include <uuid/uuid.h>
#include <curl/curl.h>

#include "logger/log.h"
#include "commonfuncs.h"
#include "../inc/tpzehin_records.h"
#include "../inc/tpzehin_common.h"
#include "tpzehin_pthread.h"
#include "spzehin_api.h"
#include "struction.h"
#include "sendFile.h"
#include "spsystem_api.h"
#include "park_util.h"

str_tpzehin_records gstr_tpzehin_records;
pthread_mutex_t g_zehin_record_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_zehin_alarm_mutex = PTHREAD_MUTEX_INITIALIZER;

extern bool g_light_controlled_by_platform;

//save picture for reget
int tpzehin_save_picture4reget(int i)
{
	str_spzehin_picreget_table picreget_tb = {0};
	str_spzehin_msgreget_table msgreget_tb = {0};
	str_tpzehin_records &records = gstr_tpzehin_records;

	picreget_tb.pic_serial = i + 1;
	picreget_tb.msgtable_id =
		spzehin_select_msgreget_table(spzehin_db, &msgreget_tb, SORT_DESC);
	sprintf(picreget_tb.msgrecord_id, "%llu", records.msgrecord_id);

    if ((records.picture_size[i] <= 0) ||
        (records.field.image_name[i][0] == '\0')) {
        return -1;
    }
	sprintf(picreget_tb.pic_size, "%d", records.picture_size[i]);
	sprintf(picreget_tb.pic_name, "%s", records.field.image_name[i]);

	// If there are pictures more than rotate_count in database, delete
	// the oldest one then save this one.
    DEBUG("count pic table %d rotate count %d",
           spzehin_count_picreget_table(spzehin_db), tpzehin_rotate_count());
	if (spzehin_count_picreget_table(spzehin_db) >= tpzehin_rotate_count())
	{
        str_spzehin_picreget_table picreget_check;
		int ret = spzehin_select_picreget_table(spzehin_db, &picreget_check, 1);
		if(ret > 0)
		{
			INFO("Delete record picture %s because up to limit %d",
				 picreget_check.pic_name, tpzehin_rotate_count());
			spzehin_delete_picreget_table(spzehin_db, ret);

            // update spsystem picture
            ret = spsystem_check_picture_exist(spsystem_db,
                    picreget_check.pic_name);
            if(ret > 0)
            {
                spsystem_update_picture_table(spsystem_db,
                        picreget_check.pic_name,
                        1,
                        PARK_ZEHIN);
            }
		}
	}

	int ret = spzehin_insert_picreget_table(spzehin_db, picreget_tb);
	if(ret == 0)
	{
		ret = spsystem_check_picture_exist(spsystem_db,
				records.field.image_name[i]);
		if(ret > 0)
		{
			spsystem_update_picture_table(spsystem_db,
					records.field.image_name[i],
					0,
					PARK_ZEHIN);
		}
		else
		{
			char values[12][128] = {{0}};
			strcpy(values[1], records.field.image_name[i]);
			strcpy(values[3], "1");
			strcpy(values[4], "0");
			strcpy(values[5], "1");
			strcpy(values[6], "1");

			park_save_picture(records.field.image_name[i],
					records.picture[i],
					records.picture_size[i]);
			spsystem_insert_picture_table(spsystem_db, values);
		}
	}
	INFO("Upload park picture insert into zehin picture database!");
	return 0;
}

//send message to zehin plateform
unsigned long long tpzehin_send_message(char ac_values[][128])
{
	int i = 0;
	unsigned long long li_ret = 0;
	char *ptr = NULL;
	stZehinDataMultiRealTimeData_T_ lstr_zehin_msg;
    memset(&lstr_zehin_msg, 0x0, sizeof(stZehinDataMultiRealTimeData_T_));

	//uploard message to zehin platform
	strcpy(lstr_zehin_msg.sTerminalSn, gstr_tpzehin_login.index.sTerminalSn);
	lstr_zehin_msg.iCurTime = 0;
	//lstr_zehin_msg.sCustomData = lstr_zehin_msg.sCustomData;
	lstr_zehin_msg.iSendSize = gstr_tpzehin_login.index_num;

	for (i = 0; i < lstr_zehin_msg.iSendSize; i++) {
		lstr_zehin_msg.DataPointValue[i].iDevIndex =
			gstr_tpzehin_login.index.iDevIndex;
		lstr_zehin_msg.DataPointValue[i].iDataIndex = i + 1;
	}

    INFO("%-12s\t%-10s\t%-12s\t%-12s\t%-10s\t%-6s\t%-20s\t%-16s\t%s",
         "space_code", "plate_num", "plate_color", "plate_status",
         "confidence", "eltype", "eltime", "plate_position", "uuid");
    INFO("%-12s\t%-10s\t%-12s\t%-12s\t%-10s\t%-6s\t%-20s\t%-16s\t%s",
         ac_values[0], ac_values[1], ac_values[2], ac_values[3],
         ac_values[4], ac_values[5], ac_values[6], ac_values[7], ac_values[8]);

	strcpy(lstr_zehin_msg.DataPointValue[0].sDataValue, ac_values[0]);
	strcpy(lstr_zehin_msg.DataPointValue[1].sDataValue, ac_values[1]);
	strcpy(lstr_zehin_msg.DataPointValue[2].sDataValue, ac_values[2]);
	strcpy(lstr_zehin_msg.DataPointValue[3].sDataValue, ac_values[3]);
	strcpy(lstr_zehin_msg.DataPointValue[4].sDataValue, ac_values[4]);
	strcpy(lstr_zehin_msg.DataPointValue[5].sDataValue, ac_values[5]);
	strcpy(lstr_zehin_msg.DataPointValue[6].sDataValue, ac_values[6]);
	strcpy(lstr_zehin_msg.DataPointValue[8].sDataValue, ac_values[7]);
	strcpy(lstr_zehin_msg.sIdentifies, ac_values[8]);

	ptr = vPaasT_DataMultiRealTimeDataEx(lstr_zehin_msg, 10);
	if (ptr != NULL) {
        li_ret = atoll(ptr);
        const char* id = lstr_zehin_msg.sIdentifies;

        if (li_ret > 0) {
            INFO("Upload zehin msg %s success %s", id, ptr);
        } else {
            INFO("Upload zehin msg %s failed %s", id, ptr);
        }
    } else {
		ERROR("Upload zehin msg %s failed!", lstr_zehin_msg.sIdentifies);
		li_ret = 0;
	}
	return li_ret;
}

//send picture to zehin plateform
int tpzehin_send_picture(const char *picname, const char *picbuf,
                         size_t piclen, unsigned long long msgrecordid,
                         const char picserial)
{
    if (NULL == picname) {
        ERROR("NULL is picname");
        return SF_UNKNOWNERROR;
    }

    if (NULL == picbuf) {
        ERROR("NULL is picbuf");
        return SF_UNKNOWNERROR;
    }

	if(piclen <= 0) {
		ERROR("picture length <= 0.");
		return SF_FILEISNULL;
	}

    int ret = 0;
	stZehinDataRecordImage_T_ recordImage;
	memset(&recordImage, 0x00, sizeof(stZehinDataRecordImage_T_));

    const char* url = tpzehin_get_sf_ip();
    char respurl[512] = {0};
    if ((strncmp(tpzehin_get_sf_ip(), "http://", 7) == 0) ||
        (strncmp(tpzehin_get_sf_ip(), "https://", 8) == 0)) {
        ret = SF_SendForPOSTData(url, picname, picbuf, piclen, respurl);
    } else {
#if 1 /************* XXX BELOW WILL BE ABANDONED **********/
        time_t now;
		struct tm *timenow;
		char upload_flag = 0;
		static struct stFileInfo zehin_pic;
		stZehinDataRecordImage_T_ recordImage;

		memset(&zehin_pic, 0, sizeof(zehin_pic));
		memset(&recordImage, 0, sizeof(stZehinDataRecordImage_T_));

		//init lstr_zehin_pic
		zehin_pic.iCamID = 111111;
		zehin_pic.iFileType = 0;
		zehin_pic.iCamIndex = 1;

		time(&now);
		timenow = localtime(&now);
		zehin_pic.stCreateDate.iYear    = timenow->tm_year;
		zehin_pic.stCreateDate.iMonth   = timenow->tm_mon + 1;
		zehin_pic.stCreateDate.iDay     = timenow->tm_mday;
		zehin_pic.stCreateDate.iHour    = timenow->tm_hour;
		zehin_pic.stCreateDate.iMinutes = timenow->tm_min;
		zehin_pic.stCreateDate.iSecond  = timenow->tm_sec;
		strcpy(zehin_pic.sFileName, picname);

		char url[256] = {0};
		upload_flag = sf_send(tpzehin_get_sf_ip(), tpzehin_get_sf_port(),
					  (char *)picbuf, piclen, zehin_pic, 10, url);
		if ((upload_flag == SF_SUCCESS) || (upload_flag == SF_FILEEXIST)) {
			//Send the URL to the zehin plateform
			strcpy(recordImage.sTerminalSn, tpzehin_get_spot_id());
			recordImage.iRetCode = 1;
			sprintf(recordImage.sRecordID, "%llu", msgrecordid);
			if (picserial == 1) {
				recordImage.iActiveURL = 1;
				recordImage.iActiveURL2 = 0;
				strcpy(recordImage.sImageURL, url);
				INFO("Picture RecordID : %s URL : %s", recordImage.sRecordID,
													   recordImage.sImageURL);
			} else if(picserial == 2) {
				recordImage.iActiveURL = 0;
				recordImage.iActiveURL2 = 1;
				strcpy(recordImage.sImageURL2, url);
				INFO("Picture RecordID : %s URL2 : %s", recordImage.sRecordID,
														recordImage.sImageURL2);
			}
			if (vPaasT_DataRecordImage(recordImage)) {
				DEBUG("Upload park picture to zehin plateform successful!");
				return 0;
			} else {
				ERROR("vPaasT_DataRecordImage failed.");
				return -1;
			}
		} else {
			ERROR("Upload park picture to zehin failed! %d", (int)upload_flag);
			return -1;
		}
#endif
    }

	if(ret == 0) {
		//Send the URL to the zehin plateform
		strcpy(recordImage.sTerminalSn, tpzehin_get_spot_id());
		recordImage.iRetCode = ret;
		sprintf(recordImage.sRecordID, "%llu", msgrecordid);
		if (picserial == 1) {
			recordImage.iActiveURL = 1;
			recordImage.iActiveURL2 = 0;

            if (NULL == respurl) {
                ERROR("respurl == NULL");
                return -1;
            }

			strcpy(recordImage.sImageURL, respurl);
            INFO("Picture sRecordID : %s URL  : %s",
                    recordImage.sRecordID, recordImage.sImageURL);
		} else if (picserial == 2) {
			recordImage.iActiveURL = 0;
			recordImage.iActiveURL2 = 1;

            if (NULL == respurl) {
                ERROR("respurl == NULL");
                return -1;
            }

			strcpy(recordImage.sImageURL2, respurl);
            INFO("Picture sRecordID : %s URL2 : %s",
                 recordImage.sRecordID, recordImage.sImageURL2);
		}

        if (vPaasT_DataRecordImage(recordImage)) {
            DEBUG("Upload park picture to zehin plateform successful!");
        } else {
            ERROR("vPaasT_DataRecordImage failed.");
        }
	} else {
        CURLcode error_code = static_cast<CURLcode>(ret);
        const char* strerror = curl_easy_strerror(error_code);
		ERROR("Upload picture to zehin failed! %d %s", error_code, strerror);
	}

	return ret;
}

/**
 * @brief msg reget
 *
 * @return 0 no msg to send
 *         1 send success.
 *         -1 send failed.
 */
int tpzehin_msg_reget(void)
{
	int msgnum = spzehin_count_msgreget_table(spzehin_db);
	//get message from msgreget table
    if (msgnum <= 0) {
        return 0;
    }

	str_spzehin_msgreget_table msgreget_table;
	memset(&msgreget_table, 0x00, sizeof(str_spzehin_msgreget_table));

    int msgid = spzehin_select_msgreget_table(spzehin_db, &msgreget_table);
    INFO("msgid = %d", msgid);
    if (msgid <= 0) {
        return 0;
    }

	char values[12][128] = {{0}};

    //upload message to zehin platform
    INFO("@@@reget@@@plate_num=%s", msgreget_table.plate_num);
    convert_enc_s("GBK", "UTF-8", msgreget_table.plate_num, 32, values[1], 64);
    strcpy(values[0], msgreget_table.berth_num);
    //strcpy(lc_values[1], msgreget_table.plate_num);
    strcpy(values[2], msgreget_table.plate_color);
    strcpy(values[3], msgreget_table.status);
    strcpy(values[4], msgreget_table.confidence);
    strcpy(values[5], msgreget_table.eltype);
    strcpy(values[6], msgreget_table.eltime);
    strcpy(values[7], msgreget_table.plate_position);
    strcpy(values[8], msgreget_table.uuid);
    unsigned long long record_id = tpzehin_send_message(values);
    if (record_id > 0) {
        DEBUG("resend sucessful record_id = %llu\n", record_id);
        char str_record_id[32] = {0};
        sprintf(str_record_id, "%llu", record_id);
        spzehin_update_picreget_talbe(spzehin_db, msgid, str_record_id);
        spzehin_delete_msgreget_table(spzehin_db, msgid);

		/**
		 * In normal case, light controlled by platform is cancelled at
		 * CParkZehin::record_notify or info_notify when receive the leave
		 * record, If the drive in record is retransmission, it can't be
		 * cancelled in normal case. so if send drive out record, check the
		 * light statue again and cancelled if necessary.
		 */
		if (g_light_controlled_by_platform) {
			light_ctl(LIGHT_ACTION_STOP, LIGHT_PRIO_PLATFORM, 0, 0, 0, 0, 255, 5, 0);
		}
        return 1;
    } else {
        return -1;
    }
}

/**
 * @brief pic reget
 *
 * @return 0 no pic to send
 *         1 send success
 *         -1 send failed.
 */
int tpzehin_pic_reget(void)
{
    int picnum = spzehin_count_picreget_table(spzehin_db);
    if (picnum <= 0) {
        return 0;
    }

	str_spzehin_picreget_table picreget_table;
	memset(&picreget_table, 0x00, sizeof(str_spzehin_picreget_table));

    sqlite3 *db = spzehin_db;
    str_spzehin_picreget_table *r = &picreget_table;

    int picid = spzehin_select_picreget_table(db, r);
    if (picid <= 0) {
        return 0;
    }

    INFO("picnum = %d picid = %d record_id = %s table_id = %d",
          picnum, picid, r->msgrecord_id, r->msgtable_id);

    //uploard picture to zehin platform
	unsigned char picbuf[500 * 1024] = {0};

    int piclens = atoi(picreget_table.pic_size);
    piclens = park_read_picture(r->pic_name, picbuf, piclens);

    if (piclens < 0) {
        /* read the picture failed, delete the picreget from table. */
        spzehin_delete_picreget_table(db, picid);

        // update spsystem picture
        int t = spsystem_check_picture_exist(spsystem_db, r->pic_name);
        if(t > 0) {
            spsystem_update_picture_table(spsystem_db, r->pic_name,
                                          1, PARK_ZEHIN);
        }
        return 0;
    } else {
        int ret = tpzehin_send_picture(r->pic_name, (char *)picbuf,
                                       piclens, atoll(r->msgrecord_id),
                                       r->pic_serial);
        if((ret == 0) || (ret == SF_FILEISNULL))
            // SF_FILEISNULL indicate the buf is invalid, drop it.
        {
            //delete park zehin  database
            spzehin_delete_picreget_table(db, picid);
            DEBUG("----------------delete the picreget");
            //update park system database
            spsystem_update_picture_table(spsystem_db, r->pic_name,
                                          1, PARK_ZEHIN);
            return 1;
        } else {
            return -1;
        }
    }
}

/**
 * @brief pic reget
 *
 * @return 0 no pic to send
 *         1 send success
 *         -1 send failed.
 */
int tpzehin_alarm_reget(void)
{
    str_spzehin_alarmreget_table alarm_tb = {0};

    int alarm_num = spzehin_count_alarmreget_table(spzehin_db);
    if (alarm_num <= 0) {
        return 0;
    }

    int alarm_id = spzehin_select_alarmreget_table(spzehin_db, &alarm_tb);
    if (alarm_id <= 0) {
        return 0;
    }

    // resend the alarmreget
    stZehinDataRealTimeAlarmEx_T_ DataRealTimeAlarm;
    //zehin alarm infomation upload
    strcpy(DataRealTimeAlarm.sTerminalSn, tpzehin_get_spot_id());
    DataRealTimeAlarm.iDevIndex = gstr_tpzehin_login.index.iDevIndex;
    DataRealTimeAlarm.iDataIndex = 8;
    DataRealTimeAlarm.iAlarmState = alarm_tb.category;
    strcpy(DataRealTimeAlarm.sCurTime, alarm_tb.time);
    sprintf(DataRealTimeAlarm.sCustomData, "%d", alarm_tb.level);
    sprintf(DataRealTimeAlarm.sIdentifies, "%s", alarm_tb.uuid);
    char * result = vPaasT_DataRealTimeAlarmEx(DataRealTimeAlarm, 5);

    INFO("%-12s\t%-10s\t%-24s\t%s",
         "category", "level", "time", "uuid");
    INFO("%-12d\t%-10d\t%-24s\t%s", alarm_tb.category,
                                    alarm_tb.level,
                                    alarm_tb.time,
                                    alarm_tb.uuid);
    DEBUG("ZEHIN alarm reget send result = %s", result);
    if(result != NULL) {
        spzehin_delete_alarmreget_table(spzehin_db, alarm_id);
        return 1;
    } else {
        return -1;
    }
}

//---------------------------------------------------------------
void *tpzehin_recordsreget_pthread(void *arg)
{
    int ret = 0;
	set_thread("zehin_reget");

    unsigned int interval_base = 10;
    srand((unsigned int)time(0));

	while(true)
	{
        /* this flag used to increase wait interval, if one of message picture
         * or alarm send success, it will be true, otherwise none of these send
         * success it will be false. */
        unsigned int send_status_flag = 0x00;

		//sleep interval time default is 10s.
        int range_min = interval_base - interval_base * 0.2;
        int range_max = interval_base + interval_base * 0.2;
        unsigned int interval = rand() % (range_max - range_min) + range_min;
		sleep(interval);

		if(!tpzehin_provisioned())
			continue;

        ret = tpzehin_msg_reget();
        if (ret == 1) {
            send_status_flag |= 0x11;
        } else if (ret == -1) {
            send_status_flag |= 0x10;
        }

        ret = tpzehin_pic_reget();
        if (ret == 1) {
            send_status_flag |= 0x11;
        } else if (ret == -1) {
            send_status_flag |= 0x10;
        }

        ret = tpzehin_alarm_reget();
        if (ret == 1) {
            send_status_flag |= 0x11;
        } else if (ret == -1) {
            send_status_flag |= 0x10;
        }

        /* has record to send and send success or no record to send */
        if ((send_status_flag == 0x11) || (send_status_flag == 0x00)) {
            interval_base = 10;
        } else { /* 0x10 has record to send and send failed. */
            if (interval_base < 10 * 12) { /* 10s 20s 40s 80s */
                interval_base = interval_base * 2;
            }
        }
	}
	return NULL;
}

//Third park zehin platform send park alarm
void *tpzehin_alarm_pthread(void *arg)
{
	set_thread("zehin_alarm");

	stZehinDataRealTimeAlarmEx_T_ alarm;

	int zehin_qid = msgget(0x7a656869, IPC_CREAT | 0666);
	zehin_msg_t zehin_msg;
    int msg_size=0;

	while (true) {
		msg_size = msgrcv(zehin_qid, &zehin_msg,
                    sizeof(zehin_msg_t) - sizeof(long), 100, 0);

		if (msg_size < 0) {
			ERROR("%s: msgrcv error ",__func__);
			usleep(10000);
			continue;
		}

        pthread_mutex_lock(&g_zehin_alarm_mutex);

		//zehin alarm infomation upload
		strcpy(alarm.sTerminalSn, tpzehin_get_spot_id());
		alarm.iDevIndex = gstr_tpzehin_login.index.iDevIndex;
		alarm.iDataIndex = 8;
		alarm.iAlarmState = gstr_tpzehin_records.alarm.category;
		strcpy(alarm.sCurTime, gstr_tpzehin_records.alarm.time);
		sprintf(alarm.sCustomData, "%d", gstr_tpzehin_records.alarm.level);

		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, gstr_tpzehin_records.alarm.uuid);
		sprintf(alarm.sIdentifies, "%s", gstr_tpzehin_records.alarm.uuid);

        INFO("%-12s\t%-10s\t%-24s\t%s",
             "category", "level", "time", "uuid");
        INFO("%-12d\t%-10d\t%-24s\t%s", gstr_tpzehin_records.alarm.category,
                                        gstr_tpzehin_records.alarm.level,
                                        gstr_tpzehin_records.alarm.time,
                                        gstr_tpzehin_records.alarm.uuid);

		char *result = NULL;
        if(!tpzehin_is_online()) {
            ERROR("ZEHIN Platform is not online.");
            str_spzehin_alarmreget_table alarm_reget;
			memcpy(&alarm_reget, &(gstr_tpzehin_records.alarm),
					sizeof(str_spzehin_alarmreget_table));
			spzehin_insert_alarmreget_table(spzehin_db, alarm_reget);
			goto unlock_and_continue;
        }

        for(int i = 0; i < 3; i++) {
            result = vPaasT_DataRealTimeAlarmEx(alarm, 5);
            if(NULL != result) {
				goto unlock_and_continue;
            }
        }

		// alreay try 3times but failed. save it and retry later.
		if((NULL == result) && tpzehin_retransmition_enable()) {
			DEBUG("insert alarm record into reget thread.")
            str_spzehin_alarmreget_table alarm;
			memcpy(&alarm, &(gstr_tpzehin_records.alarm),
					sizeof(str_spzehin_alarmreget_table));
			spzehin_insert_alarmreget_table(spzehin_db, alarm);

			goto unlock_and_continue;
		}

unlock_and_continue:
        pthread_mutex_unlock(&g_zehin_alarm_mutex);
	}
	return NULL;
}

//Third park zehin platform send parkpicture
static int tpzehin_picture_handler(unsigned long long record_id)
{
	str_tpzehin_records &records = gstr_tpzehin_records;

	// two pictures
	for(int i = 0; i < 2; i++) {
		int ret = 0;
        if(!vPaasT_GetIsOnLine()) {
            ERROR("tpzehin not online.");
            break;
        }

        //upload park picture to zehin platform
        ret = tpzehin_send_picture(records.field.image_name[i],
                (char *)records.picture[i],
                records.picture_size[i],
                records.msgrecord_id,
                i + 1);
        if(0 == ret) {
            DEBUG("zehin send picture [%d] ok. record_id=%llu, image_name=%s",
                   i, records.msgrecord_id, records.field.image_name[i]);
        } else {
            ERROR("zehin send picture [%d] failed."
                    "record_id=%llu, image_name=%s ret=%d\n", i,
                    records.msgrecord_id, records.field.image_name[i], ret);
        }

		if((ret != 0) && tpzehin_retransmition_enable()) {
			//insert park message into msgreget database
			tpzehin_save_picture4reget(i);
		}
	}
	return 0;
}

void tpzehin_construct_msg(char values[][128])
{
	str_tpzehin_records &records = gstr_tpzehin_records;
	DB_ParkRecord &field = records.field;

	//upload park message to zehin platform
	strcpy(values[0], field.point_id);
	convert_enc("GBK", "UTF-8", field.plate_num, 32, values[1], 64);
	sprintf(values[2], "%d", field.plate_color);
	sprintf(values[3], "%d", field.status);
	sprintf(values[4], "%d", field.confidence[0]);
	if (field.objectState == 0)
		field.objectState = 2;
	sprintf(values[5], "%d", field.objectState);
	sprintf(values[6], "%s", field.time);

	/* use the last pic's plate position for drive in */
	if (field.objectState == 1) {
		sprintf(values[7], "%d/%d/%d/%d", field.coordinate_x[1],
										  field.coordinate_y[1],
										  field.width[1],
										  field.height[1]);
	}
	/* use the first pic's plate position for drive out */
	else {
		sprintf(values[7], "%d/%d/%d/%d", field.coordinate_x[0],
										  field.coordinate_y[0],
										  field.width[0],
										  field.height[0]);
	}

	/* generate the uuid for this record */
	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse(uuid, values[8]);
}

void tpzehin_save_msg4reget(char values[][128])
{
	str_spzehin_msgreget_table reget_tb = {0};

	str_tpzehin_records &records = gstr_tpzehin_records;
	DB_ParkRecord &field = records.field;

	//insert park message into msgreget database
	records.msgrecord_id = 0;
	sprintf(reget_tb.berth_num, "%s", field.point_id);
	sprintf(reget_tb.plate_num, "%s", field.plate_num);
	sprintf(reget_tb.plate_color, "%d", field.plate_color);
	sprintf(reget_tb.status, "%d", field.status);
	sprintf(reget_tb.confidence, "%d", field.confidence[0]);
	sprintf(reget_tb.eltype, "%d", field.objectState);
	sprintf(reget_tb.eltime, "%s", field.time);
	sprintf(reget_tb.plate_position, "%s", values[7]);
	sprintf(reget_tb.uuid, "%s", values[8]);

	ERROR("Upload zehin message failed. insert into database!");
	//insert message into sqlite3 database
	spzehin_insert_msgreget_table(spzehin_db, reget_tb);

	// the record transfer failed, so the pictures should save too.
	for(int i = 0; i < 2; i++)
		tpzehin_save_picture4reget(i);
}

//Third park zehin platform send parkmessage
void *tpzehin_record_pthread(void *arg)
{
	set_thread("zehin_record");

	char values[12][128] = {{0}};

	str_tpzehin_records &records = gstr_tpzehin_records;

    int ret = 0;
	zehin_msg_t zehin_msg = {0};
	unsigned long long record_id = 0;
	int zehin_qid = msgget(0x7a656869, IPC_CREAT | 0666);
	size_t recv_msg_size = sizeof(zehin_msg_t) - sizeof(long);

	while (true) {
		ret = msgrcv(zehin_qid, &zehin_msg, recv_msg_size, 200, 0);
		if (ret < 0) {
			ERROR("msgrcv error %s", strerror(errno));
			usleep(10000);
			continue;
		}

        pthread_mutex_lock(&g_zehin_record_mutex);

		tpzehin_construct_msg(values);

		if (!tpzehin_is_online() ||
			(tpzehin_retransmition_enable() &&
			(spzehin_count_msgreget_table(spzehin_db) > 0))) {
			tpzehin_save_msg4reget(values);
			goto unlock_and_continue;
		}

		for (int i = 0; i < 3; i++) {
			record_id = tpzehin_send_message(values);
			if (record_id != 0) {
				records.msgrecord_id = record_id;
				if (zehin_msg.with_picture) {
					// send picture now
					tpzehin_picture_handler(record_id);
				}
				goto unlock_and_continue;
			}
		}

		if(!tpzehin_retransmition_enable()) {
			INFO("RETRANMITION IS NOT ENABLED. DROP THIS MSG.");
		} else {
			tpzehin_save_msg4reget(values);
		}

unlock_and_continue:
		pthread_mutex_unlock(&g_zehin_record_mutex);
	}
	return NULL;
}
