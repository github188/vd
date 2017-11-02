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
#include "tvpnetposa_register.h"

str_tvpnetposa_register gstr_tvpnetposa_register;


//tvpnetposa login
int tvpnetposa_heartbeate(char *ac_saddr, int ai_port)
{
	int socket_fd = -1;
	int li_ret = 0;
	int li_lens = 0;
	char lc_data[1024] = {0};
	struct timeval timeout = {3, 0};
	struct sockaddr_in s_addr;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		return li_ret;
	}

	bzero(&s_addr, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(ac_saddr);
	s_addr.sin_port = htons(ai_port);

	if(connect(socket_fd, (struct sockaddr *)(&s_addr), sizeof(struct sockaddr)) < 0)
	{
		close(socket_fd);

		return li_ret;
	}

	li_lens = tvpnetposa_heartbeate_encode(lc_data, 0);
	TRACE_LOG_SYSTEM("[send]: tvpnetposa_heartbeate_encode = %s", lc_data);

	//send to the netposa server
	send(socket_fd, lc_data, 1024, 0);

	//recv netposa register respond
	memset(lc_data, 0, sizeof(lc_data));
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	li_lens = recv(socket_fd, lc_data, 1024, 0);
	if(li_lens > 0)
	{
		TRACE_LOG_SYSTEM("[recv]: tvpnetposa_heartbeate_encode respond = %s", lc_data);

		li_ret = tvpnetposa_heartbeate_decode(lc_data, li_lens);
	}

	close(socket_fd);

	return li_ret;
}


//tvpnetposa login
int tvpnetposa_login(char *ac_saddr, int ai_port)
{
	int socket_fd = -1;
	int li_ret = 0;
	int li_lens = 0;
	char lc_data[1024] = {0};
	struct timeval timeout = {3, 0};
	struct sockaddr_in s_addr;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		return li_ret;
	}

	bzero(&s_addr, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(ac_saddr);
	s_addr.sin_port = htons(ai_port);

	if(connect(socket_fd, (struct sockaddr *)(&s_addr), sizeof(struct sockaddr)) < 0)
	{
		close(socket_fd);

		return li_ret;
	}

	li_lens = tvpnetposa_register_encode(lc_data, 0);
	TRACE_LOG_SYSTEM("[send]: tvpnetposa_register_encode = %s", lc_data);

	//send to the netposa server
	send(socket_fd, lc_data, 1024, 0);

	//recv netposa register respond
	memset(lc_data, 0, sizeof(lc_data));
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	li_lens = recv(socket_fd, lc_data, 1024, 0);
	if(li_lens > 0)
	{
		TRACE_LOG_SYSTEM("[recv]: tvpnetposa_register_encode respond = %s", lc_data);

		li_ret = tvpnetposa_register_decode(lc_data, li_lens);
	}

	close(socket_fd);

	return li_ret;
}

//tvpuniview register init
int register_init()
{
	sprintf(gstr_tvpnetposa_register.ip, "192.168.1.181");
	gstr_tvpnetposa_register.port = 1213;
	sprintf(gstr_tvpnetposa_register.company, "bitcom");

	return 0;
}

//tvpuniview register pthread function
void *tvpnetposa_register_pthread(void *arg)
{
	int li_sec = 0;
	static int li_count = 0;

	register_init();

	for(;;)
	{
		sleep(1);

		//netposa register
		while(gstr_tvpnetposa_register.flag == 0)
		{
			TRACE_LOG_SYSTEM("The tvpnetposa is login!!!!!!!");

			if((gstr_tvpnetposa_register.flag=tvpnetposa_login((char*)"192.168.1.181", 1213)) <= 0)
			{
				sleep(3);
			}
		}

		//netposa heart beate
		if((li_sec%(gstr_tvpnetposa_register.heart_cycle-1)) == 0)
		{
			int ret = (tvpnetposa_heartbeate((char*)"192.168.1.181", 1213)<=0) ? ++li_count : 0;
            li_count = ret;

			TRACE_LOG_SYSTEM("_______li_count = %d_______!!!!!!!", li_count);
		}
		li_sec++;

		gstr_tvpnetposa_register.flag = li_count>=3 ? 0 : gstr_tvpnetposa_register.flag;
	}
	return NULL;
}
