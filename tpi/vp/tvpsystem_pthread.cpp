#include <stdio.h>
#include <pthread.h>
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
#include <sqlite3.h>
#include <sys/stat.h>

#include "tvpuniview_pthread.h"
#include "baokang_pthread.h"
#include "tvpnetposa_pthread.h"
#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "interface_ipnc.h"
#include "logger/log.h"


extern int gi_Baokang_illegally_park_switch;
extern int gi_Uniview_illegally_park_switch;

//The third platform vp pthread
void *tvpsystem_pthread(void * arg)
{
	pthread_t tvpuniview_pthread_pid;
	pthread_t pthread_baokang;


    if(gi_Uniview_illegally_park_switch)
    {
        if(pthread_create(&tvpuniview_pthread_pid, NULL,
                            tvpuniview_pthread, NULL))
        {
            ERROR("Create tpi tvpuniview_pthread failed!");
        }
    }
    if(gi_Baokang_illegally_park_switch)
    {
        if(pthread_create(&pthread_baokang, NULL,
                            baokang_pthread, NULL))
        {
            ERROR("Create tpi tvpuniview_pthread failed!");
        }
    }

	#if 0
	pthread_t tvpnetposa_pthread_pid;
	if(pthread_create(&tvpnetposa_pthread_pid, NULL, tvpnetposa_pthread, NULL))
	{
		ERROR("Create tpi tvpnetposa_pthread failed!");
	}
	#endif
	
	while(1)
	{
		sleep(10);
	}
    return NULL;
}
