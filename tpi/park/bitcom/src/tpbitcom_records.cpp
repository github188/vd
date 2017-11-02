#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "logger/log.h"
#include "ftp.h"
#include "commonfuncs.h"
#include "park_records_process.h"
#include "tpbitcom_records.h"
#include "spbitcom_api.h"
#include "data_process.h"
#include "tpbitcom_common.h"
#include "spsystem_api.h"
#include "park_util.h"

#include "refhandle.h"
str_tpbitcom_records gstr_tpbitcom_records;

//---------------------------------------------------------------
void *tpbitcom_recordsreget_pthread(void *arg)
{
	int  li_msgnum = 0;
	int  li_msgid = 0;
	char lc_msgbuf[1024] = {0};
	int  li_picnum = 0;
	int  li_picid = 0;
	unsigned char lc_picbuf[1024*500] = {0};
	int  li_ret = 0;
	str_spbitcom_picreget_table lstr_spbitcom_picreget_table;
	EP_PicInfo lstr_picinfo;

	while(1)
	{
		//sleep 2s
		usleep(1000*1000*2);

		//get message from msgreget table
		if((li_msgnum=spbitcom_count_msgreget_table(spbitcom_db)) > 0)
		{
	        if((li_msgid=spbitcom_select_msgreget_table(spbitcom_db, lc_msgbuf)) <= 0)
	        	continue;

			//upload message to bitcom platform3
			li_ret = mq_send_park_record(lc_msgbuf);

			if(li_ret == 0)
				spbitcom_delete_msgreget_table(spbitcom_db, li_msgid);

			DEBUG("Park bitcom message xuchuan!!!!!");
        }

		//get picture from msgreget table
		if((li_picnum=spbitcom_count_picreget_table(spbitcom_db)) > 0)
		{
	        if((li_picid=spbitcom_select_picreget_table(spbitcom_db, &lstr_spbitcom_picreget_table)) <= 0)
	        	continue;

			//upload picture to bitcom platform
			sscanf(lstr_spbitcom_picreget_table.pic_path, "%[^/]/%[^/]/%[^/]/%[^/]/%[^/]/%[^/]",
					lstr_picinfo.path[0], lstr_picinfo.path[1], lstr_picinfo.path[2],
					lstr_picinfo.path[3], lstr_picinfo.path[4], lstr_picinfo.path[5]);
			strcpy(lstr_picinfo.name, lstr_spbitcom_picreget_table.pic_name);
			lstr_picinfo.size = atoi(lstr_spbitcom_picreget_table.pic_size);
			int pic_len = park_read_picture(lstr_picinfo.name, lc_picbuf, lstr_picinfo.size);

            if (pic_len < 0) {
                /* read the picture failed, delete the picreget from table. */
                spbitcom_delete_picreget_table(spbitcom_db, li_picid);

                // update spsystem picture
                int ret = spsystem_check_picture_exist(spsystem_db,
                                          lstr_picinfo.name);
                if(ret > 0) {
                    spsystem_update_picture_table(spsystem_db,
                            lstr_picinfo.name,
                            1,
                            PARK_BITCOM);
                }
                continue;
            }

			lstr_picinfo.buf = lc_picbuf;
            li_ret = send_park_records_image_buf(&lstr_picinfo, 1);
			if(li_ret == 0)
			{
                spbitcom_delete_picreget_table(spbitcom_db, li_picid);
                spsystem_update_picture_table(spsystem_db,
                        lstr_spbitcom_picreget_table.pic_name,
                        1,
                        PARK_BITCOM);
               DEBUG("Park bitcom picture xuchuan sucessful!!!!!");
			}
            else
            {
               DEBUG("Park bitcom picture xuchuan failed!!!!!");
            }
		}

		if((li_msgnum==0) && (li_picnum==0)) gstr_tpbitcom_records.reget_flag = 0;
    }
	return NULL;
}


//Third park bitcom platform send parkpicture
void *tpbitcom_picture_pthread(void *arg)
{
	int li_i = 0;
	int li_j = 0;
	static char lsc_upload_flag = 0;
	static char lsc_upload_count = 0;
	str_spbitcom_picreget_table lstr_spbitcom_picreget_table;

    str_tpbitcom_records &records = gstr_tpbitcom_records;
	memset(&lstr_spbitcom_picreget_table, 0, sizeof(lstr_spbitcom_picreget_table));

	while(1)
	{
		usleep(1000*100);

		if(records.picexist_flag == 1)
		{
			lsc_upload_flag = lsc_upload_count = 0;
			do
			{
				//insert park message into msgreget database
				if(lsc_upload_count++ >= 3)
				{
					if(!tpbitcom_retransmition_enable())
						break;

					for(li_j=0; li_j<2; li_j++)
					{
						// If there are more than 10 pictures in flash, delete
						// the oldest one then save this one.
						if (spbitcom_count_picreget_table(spbitcom_db) >= tpbitcom_rotate_count())
						{
                            DEBUG("more than %d pictures in database, delete one.", tpbitcom_rotate_count());
							int ret = spbitcom_select_picreget_table(spbitcom_db, &lstr_spbitcom_picreget_table);
							if(ret > 0)
							{
								spbitcom_delete_picreget_table(spbitcom_db, ret);

                                // update spsystem picture
                                ret = spsystem_check_picture_exist(spsystem_db,
                                        lstr_spbitcom_picreget_table.pic_name);
                                if(ret > 0)
                                {
                                    spsystem_update_picture_table(spsystem_db,
                                            lstr_spbitcom_picreget_table.pic_name,
                                            1,
                                            PARK_BITCOM);
                                }
							}
						}

						memset(&lstr_spbitcom_picreget_table, 0, sizeof(lstr_spbitcom_picreget_table));
						sprintf(lstr_spbitcom_picreget_table.pic_size, "%d", records.picture_size[li_j]);
						for(li_i=0; li_i<EP_FTP_URL_LEVEL.levelNum; li_i++)
						{
							strcat(lstr_spbitcom_picreget_table.pic_path, records.picture_info[li_j].path[li_i]);
							strcat(lstr_spbitcom_picreget_table.pic_path, "/");
						}
						sprintf(lstr_spbitcom_picreget_table.pic_name, "%s", records.picture_info[li_j].name);
						DEBUG("lstr_spbitcom_picreget_table.pic_path=%s", lstr_spbitcom_picreget_table.pic_path);
                        int ret = spbitcom_insert_picreget_table(spbitcom_db, lstr_spbitcom_picreget_table);
                        if(ret == 0)
                        {
                            ret = spsystem_check_picture_exist(spsystem_db,
                                    records.field.image_name[li_j]);
                            if(ret > 0)
                            {
                                spsystem_update_picture_table(spsystem_db,
                                        records.field.image_name[li_j],
                                        0,
                                        PARK_BITCOM);
                            }
                            else
                            {
                                char values[12][128] = {{0}};
                                strcpy(values[1], records.field.image_name[li_j]);
                                strcpy(values[3], "0");
                                strcpy(values[4], "1");
                                strcpy(values[5], "1");
                                strcpy(values[6], "1");

                                spsystem_insert_picture_table(spsystem_db, values);
                                park_save_picture(records.field.image_name[li_j],
                                        records.picture[li_j],
                                        records.picture_size[li_j]);
                            }
                        }
						DEBUG("Upload park picture insert into bitcom database!");
					}

					break;
				}

				//upload parpk message to bitcom platform
				if((lsc_upload_flag=send_park_records_image_buf(gstr_tpbitcom_records.picture_info, 2)) == 0)
				{
					//Send the URL to the bitcom plateform
					DEBUG("Upload park picture to bitcom plateform succcessful!");
				}

				DEBUG("****************park bitcom picture lsc_upload_flag=%d****************", lsc_upload_flag);

			}while(lsc_upload_flag);

			gstr_tpbitcom_records.picexist_flag = 0;
		}
	}
	return NULL;
}


//Third park bitcom platform send parkmessage
void *tpbitcom_message_pthread(void *arg)
{
	static char lsc_upload_flag = 0;
	static char lsc_upload_count = 0;
	str_spbitcom_msgreget_table lstr_spbitcom_msgreget_table;

	memset(&lstr_spbitcom_msgreget_table, 0, sizeof(str_spbitcom_msgreget_table));

	while(1)
	{
		usleep(1000*100);

		if(gstr_tpbitcom_records.msgexist_flag == 1)
		{
			lsc_upload_count = lsc_upload_flag =0;
			do
			{
				//insert park message into msgreget database
				if(lsc_upload_count++ >= 3)
				{
					if(!tpbitcom_retransmition_enable())
						break;

					gstr_tpbitcom_records.reget_flag = 1;

					strcpy(lstr_spbitcom_msgreget_table.message, gstr_tpbitcom_records.message);
					spbitcom_insert_msgreget_table(spbitcom_db, lstr_spbitcom_msgreget_table);

					DEBUG("Upload park message insert into bitcom database = %s!", lstr_spbitcom_msgreget_table.message);

					break;
				}

				//upload park message to bitcom platform
				if((lsc_upload_flag=send_park_record_info(&gstr_tpbitcom_records.field)) == 0)
				{
					DEBUG("Upload park message to bitcom plateform succcessful!");
				}

				DEBUG("****************park bitcom message lsc_upload_flag=%d****************", lsc_upload_flag);

			}while(lsc_upload_flag);

			gstr_tpbitcom_records.msgexist_flag = 0;
		}
	}
	return NULL;
}


