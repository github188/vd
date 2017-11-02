#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "tvpuniview_protocol.h"
#include "tvpuniview_pthread.h"
#include "tvpuniview_records.h"
#include "logger/log.h"


//tvpuniview pthread function
void *tvpuniview_pthread(void *arg)
{
	str_vp_msg lstr_vp_msg;
	pthread_t tvpuniview_records_id;
	int msg_size=0;

	memset(&lstr_vp_msg, 0, sizeof(lstr_vp_msg));
	
	//create tvpuniview records pthread
	if(pthread_create(&tvpuniview_records_id, NULL, tvpuniview_records_pthread, NULL))
	{
		ERROR("Create tpi tvpuniview_records_pthread failed!");
	}

	while(1)
	{
		//if(gstr_tvpuniview_records.flag==0)
		//{
			msg_size=msgrcv(MSG_VP_ID, &lstr_vp_msg, sizeof(str_vp_msg) - sizeof(long), MSG_VP_UNIVIEW_TYPE, 0);
			if(msg_size<0)
			{
				ERROR("%s: msgrcv error ",__func__);
				usleep(10000);
				continue;
			}
			gstr_tvpuniview_records.flag = 1;
		//}
	}
	return NULL;
}
