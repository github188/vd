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

#include "logger/log.h"
#include "Msg_Def.h"
#include "arm_config.h"
#include "commonfuncs.h"
#include "vd_msg_queue.h"
#include "tvpuniview_protocol.h"
#include "tvpuniview_records.h"

extern int gi_Uniview_illegally_park_switch;
extern PLATFORM_SET msgbuf_paltform;
extern ARM_config g_arm_config;
str_tvpuniview_records gstr_tvpuniview_records;

//tvpuniview send records
int tvpuniview_records_send(int ai_socket)
{
	int  li_send_lens = 0;
	int  li_recv_lens = 0;
	char lc_data[1024*1024] = {0};
	int  li_j = 0;
	struct timeval timeout = {3, 0};
	str_tvpuniview_protocol lstr_tvpuniview_protocol;

	if(ai_socket == -1) return -1;

	lstr_tvpuniview_protocol.frame_cmd = RT_RECORDS_CMD;
	lstr_tvpuniview_protocol.data.pic_num = 3;
	li_send_lens = tvpuniview_encode(&lstr_tvpuniview_protocol, lc_data);

	//send to the uniview server
	DEBUG("________^^^^%d_______", li_send_lens);


	send(ai_socket, lc_data, li_send_lens, 0);

	DEBUG("__________$$$$$_______");

	//recv records respond
	memset(lc_data, 0, sizeof(lc_data));
	setsockopt(ai_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	memset(lc_data, 0, sizeof(lc_data));
	li_recv_lens = recv(ai_socket, lc_data, 1024, 0);
	if(li_recv_lens > 0)
	{
		DEBUG("_______The records send respond:________");

		while(li_recv_lens--)
		{
			if(strncmp(lc_data+li_j, "<Result>", strlen("<Result>")) == 0)
			{
				DEBUG("result = %c, %c", lc_data[li_j+strlen("<Result>")], lc_data[li_j+strlen("<Result>")+1]);
				break;
			}
			li_j++;
		}

		return ai_socket;
	}

	return 0;
}

//tvpuniview send heart beate
int tvpuniview_heartbeate_send(int ai_socket)
{
	char lc_data[1024*1024] = {0};
	int  li_send_lens = 0;
	int  li_recv_lens = 0;
	int  li_i = 0;	
	struct timeval timeout = {9, 0};
	str_tvpuniview_protocol lstr_tvpuniview_protocol;

	//heart beate info
	lstr_tvpuniview_protocol.frame_cmd = HEART_BEATE_CMD;
	lstr_tvpuniview_protocol.frame_lens = 32;
	//sprintf(lstr_tvpuniview_protocol.heart_beate.dev_code, "%d", htonl(987654321));
	//strcpy(lstr_tvpuniview_protocol.heart_beate.dev_code, g_arm_config.basic_param.exp_device_id);
	strcpy(lstr_tvpuniview_protocol.heart_beate.dev_code, msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.DeviceId);
    DEBUG("dev_code = %s", lstr_tvpuniview_protocol.heart_beate.dev_code);
	li_send_lens = tvpuniview_encode(&lstr_tvpuniview_protocol, lc_data);

	printf("[UNIVIEW_HEART_BEATE]: ");
	for(li_i=0; li_i<li_send_lens; li_i++)
	{
		printf("%02x ", lc_data[li_i]);
	}
	printf("\n");

	//send to the uniview server
	send(ai_socket, lc_data, li_send_lens, 0);

	//recv heart beate respond
	memset(lc_data, 0, sizeof(lc_data));
	setsockopt(ai_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	memset(lc_data, 0, sizeof(lc_data));
	li_recv_lens = recv(ai_socket, lc_data, 1024, 0);
	if(li_recv_lens > 0)
	{
		DEBUG("_______The heart beate respond:________");

		return ai_socket;
	}

	close(ai_socket);

	return -1;
}


//tvpuniview login
int tvpuniview_login(char *ac_saddr, int ai_port)
{
	int socket_fd = -1;
	int li_ret = -1;
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

	return socket_fd;
}


//tvpuniview init
// get the ip address & port from global buffer
int tvpuniview_init(char *ip, int *port)
{
#if 0
	FILE *fd = NULL;
	char lc_tmp[128] = {0};
	char lc_port[32] = {0};
	int  li_port = 0;

	fd = fopen("/mnt/nand/uniview.cfg", "a+");

	fgets(lc_tmp, sizeof(lc_tmp), fd);

	DEBUG("_____uniview IP : %s_____", lc_tmp);

	sscanf(lc_tmp, "%[^:]:%s", ac_ip, lc_port);

	li_port = atoi(lc_port);

	fclose(fd);

	return li_port;
#endif
    strcpy(ip, msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.ServerIp);
    *port = msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.MsgPort;
	return 0;
}

//tvpuniview records pthread
void *tvpuniview_records_pthread(void *arg)
{
	static int socket_server = -1;
	int heartbeate_flag = 0;
	char lc_ip[32] = {0};
	int  li_port = 0;

	for(;;)
	{
		usleep(1000*1000*1);

		//tvpuniview Login
		while(socket_server == -1)
		{
			tvpuniview_init(lc_ip, &li_port);

			DEBUG("The vp uniview is login: ip = %s, port = %d", lc_ip, li_port);

			if((socket_server=tvpuniview_login(lc_ip, li_port)) > 0)
			{
				DEBUG("The vp uniview is login successful!");

				break;
			}
			else
			{
				sleep(2);
			}
		}

		//DEBUG("socket_server=%d", socket_server);

		//send heartbeate to uniview
		if((((heartbeate_flag+1) % 30) == 0) && socket_server > 0)
		{
			DEBUG("The heartbeate sendto uniview!");

			socket_server = tvpuniview_heartbeate_send(socket_server);
			heartbeate_flag = 0;
		}
		heartbeate_flag++;

		//send records to univew
		if(gstr_tvpuniview_records.flag == 1)
		{
			tvpuniview_records_send(socket_server);

			gstr_tvpuniview_records.flag = 0;
		}
	}
	return NULL;
}
