#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sqlite3.h>
#include <sys/stat.h>

#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "tpbitcom_records.h"
#include "spbitcom_api.h"
#include "logger/log.h"
#include "tpbitcom_common.h"

//create the multi directory
static int create_multi_dir(const char *path)
{
	int i, len;

	len = strlen(path);
	char dir_path[len+1];
	dir_path[len] = '\0';

	strncpy(dir_path, path, len);

	for (i=0; i<len; i++)
	{
		if (dir_path[i] == '/' && i > 0)
		{
			dir_path[i]='\0';
			if (access(dir_path, F_OK) < 0)
			{
				if (mkdir(dir_path, 0755) < 0)
				{
					return -1;
				}
			}
			dir_path[i]='/';
		}
	}

	return 0;
}


//tpbitcom pthread function
void *tpbitcom_pthread(void *arg)
{
	str_park_msg lstr_park_msg;
	pthread_t tpbitcom_message_id;
	pthread_t tpbitcom_picture_id;
	pthread_t tpbitcom_recordsreget_id;
	int msg_size = 0;

	memset(&lstr_park_msg, 0, sizeof(str_park_msg));

	//init park bitcom database
	create_multi_dir("/home/records/park/bitcom/database/");
	create_multi_dir("/home/records/park/bitcom/picture/");
	spbitcom_open("/home/records/park/bitcom/database/spbitcom.db");
	spbitcom_create_msgreget_table(spbitcom_db);
	spbitcom_create_picreget_table(spbitcom_db);

	//create send park message pthread
	if(pthread_create(&tpbitcom_message_id, NULL, tpbitcom_message_pthread, NULL))
	{
		ERROR("Create tpi tpbitcom_message_pthread failed!");
	}

	//create send park picture pthread
	if(pthread_create(&tpbitcom_picture_id, NULL, tpbitcom_picture_pthread, NULL))
	{
		ERROR("Create tpi tpbitcom_picture_pthread failed!");
	}

	if(tpbitcom_retransmition_enable())
	{
		//create reget records pthread
		if(pthread_create(&tpbitcom_recordsreget_id, NULL, tpbitcom_recordsreget_pthread, NULL))
		{
			ERROR("Create tpi tpbitcom_recordsreget_pthread failed!");
		}
	}

	while(1)
	{
		if((gstr_tpbitcom_records.msgexist_flag==0) && (gstr_tpbitcom_records.picexist_flag==0))
		{
			DEBUG("bitcom bitcom bitcom bitcom recv");

			msg_size = msgrcv(MSG_PARK_ID, &lstr_park_msg, sizeof(str_park_msg) - sizeof(long), MSG_PARK_BITCOM_TYPE, 0);

			if(msg_size<0)
			{
				log_send(LOG_LEVEL_FAULT,0,"VD:","After rcv ,error : %s,type=%d\n",__func__,lstr_park_msg.type);
				DEBUG("VD recived message error : %s,type=%d",__func__,lstr_park_msg.type);
				usleep(10000);
				continue;
			}

            INFO("bitcom recv message %d", lstr_park_msg.data.bitcom.type);

			if(gstr_tpbitcom_records.pic_flag == 0)
			{
				gstr_tpbitcom_records.msgexist_flag = 1;
				gstr_tpbitcom_records.picexist_flag = 0;
			}
			else
			{
				gstr_tpbitcom_records.msgexist_flag = 1;
                gstr_tpbitcom_records.picexist_flag = 1;
			}
		}
		DEBUG("bitcom bitcom bitcom bitcom");
		usleep(1000*10);
	}
	return NULL;
}
