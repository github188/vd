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
#include "tvpnetposa_protocol.h"
#include "tvpnetposa_pthread.h"
#include "tvpnetposa_records.h"
#include "tvpnetposa_register.h"
#include "logger/log.h"


//tvpuniview pthread function
void *tvpnetposa_pthread(void *arg)
{
	str_vp_msg lstr_vp_msg;
	pthread_t tvpnetposa_register_id;
	int msg_size=0;

	memset(&lstr_vp_msg, 0, sizeof(lstr_vp_msg));

	//create tvpnetposa register pthread
	if(pthread_create(&tvpnetposa_register_id, NULL, tvpnetposa_register_pthread, NULL))
	{
		ERROR("Create tpi tvpnetposa_register_pthread failed!");
	}
	
	//create tvpnetposa records pthread
//	if(pthread_create(&tvpnetposa_records_id, NULL, tvpnetposa_records_pthread, NULL))
//	{
//		ERROR("Create tpi tvpnetposa_records_pthread failed!");
//	}

	while(1)
	{
		if(gstr_tvpnetposa_records.flag==0)
		{
			msg_size=msgrcv(MSG_VP_ID, &lstr_vp_msg, sizeof(str_vp_msg) - sizeof(long), MSG_VP_NETPOSA_TYPE, 0);
			if(msg_size<0)
			{
				ERROR("%s: msgrcv error ",__func__);
				usleep(10000);
				continue;
			}
			gstr_tvpnetposa_records.flag = 1;
		}
	}
	return NULL;
}
