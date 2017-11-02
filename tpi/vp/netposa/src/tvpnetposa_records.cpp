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
#include "tvpnetposa_records.h"

str_tvpnetposa_records gstr_tvpnetposa_records;

void *tvpnetposa_records_pthread(void *arg)
{
	while(1)
	{
		sleep(10);
	}
	return NULL;
}



