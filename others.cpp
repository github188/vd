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
#include "Msg_Def.h"
#include "others.h"
#include "commonfuncs.h"
#include "sysctrl.h"
#include "traffic_records_process.h"
#include "ve/platform/bitcom/proto.h"
#include "ve/platform/bitcom/thread.h"
#include "platform_thread.h"
#include "logger/log.h"

extern SET_NET_PARAM g_set_net_param;

PLATFORM_SET msgbuf_paltform;
str_alleyway_status gstr_alleyway_status;

int gi_NetPose_vehicle_switch = 0;
int gi_Dahua_vehicle_switch = 0;
int gi_Bitcom_alleyway_switch = 0;
int gi_Uniview_illegally_park_switch = 0;
int gi_Baokang_illegally_park_switch = 0;
int gi_Zehin_park_switch = 0;

int flg_register_Dahua = 0;
int flg_register_Bitcom = 0;

int flg_register_NetPose_vehicle = 0;
int flg_register_NetPose_face = 0;

int NetPose_Platform_Vehicle_Enable = 0;
int NetPose_Platform_Face_Enable = 0;


int getime_for_dahua(char *nowtime)
{
    struct tm *pt;
	struct timeval  Tus;
	int cur_sec, cur_min, cur_hour, cur_day, cur_mouth, cur_year, cur_weekday;

	long ust;


	time_t t = time(NULL);
	pt = localtime(&t);
	memset(nowtime, 0, sizeof(*nowtime));

	gettimeofday(&Tus, NULL);
	cur_sec = pt->tm_sec;
	cur_min = pt->tm_min;
	cur_hour = pt->tm_hour;
	cur_day = pt->tm_mday;
	cur_mouth = pt->tm_mon + 1;
	cur_year = pt->tm_year + 1900;
	cur_weekday = pt->tm_wday;
	ust = (long)Tus.tv_usec/1000;

	printf("时间: %d:%d:%d.%ld \n", cur_hour, cur_min, cur_sec,ust);
	printf("日期: %d-%d-%d \n", cur_year, cur_mouth, cur_day);
	sprintf(nowtime,"\"Time\" : \"%d-%d-%d %d:%d:%d.%ld\"",cur_year, cur_mouth, cur_day,cur_hour, cur_min, cur_sec,ust);
	printf("Time is:%s\n",nowtime);

	return 0;
}


/***************************************************
 * 函数名:ListenThrFxn
 * 功  能: 网页配置平台参数监听函数
 * 参  数:
 * 返回值:
 ****************************************************/
void *ListenThrFxn(void* arg)
{
    int qid = Msg_Init(SYS_MSG_KEY);
	int ret = 0;

	msgbuf_paltform.Des = MSG_TYPE_MSG26;
	msgbuf_paltform.Src= MSG_TYPE_MSG25;
	msgbuf_paltform.Cmd = GET;

	//strncpy(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp, "172.24.10.40", strlen("172.24.10.40"));
	//strncpy(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.DeviceId, "1000000", strlen("1000000"));
	//msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort = 9211;
	//msgbuf_paltform.pf_switch.vehicle_switch = V_on;
	//msgbuf_paltform.pf_ftype = DaHua;

	//获取boa配置信息
	do
	{
	    TRACE_LOG_PLATFROM_INTERFACE("MSG_QUEUE send to boa : des=%d, src=%d, Cmd=%d, ftype=%d, utype=%d, vtype=%d", msgbuf_paltform.Des, msgbuf_paltform.Src, msgbuf_paltform.Cmd, \
		    msgbuf_paltform.pf_ftype, msgbuf_paltform.pf_utype, msgbuf_paltform.pf_vtype);

		ret = Msg_Send_Rsv(qid, MSG_TYPE_MSG25, &msgbuf_paltform, sizeof(msgbuf_paltform));

		TRACE_LOG_PLATFROM_INTERFACE("MSG_QUEUE recv from boa : des=%d, src=%d,"
                "Cmd=%d, ftype=%d, utype=%d, vtype=%d, itype=%d, ptype=%d,"
                "vehicle_switch=%d, face_switch=%d, getway_switch=%d,"
                "illegally_park_switch=%d, park_switch=%d",
            msgbuf_paltform.Des,
            msgbuf_paltform.Src,
            msgbuf_paltform.Cmd,
			msgbuf_paltform.pf_ftype,
            msgbuf_paltform.pf_utype,
            msgbuf_paltform.pf_vtype,
            msgbuf_paltform.pf_itype,
            msgbuf_paltform.pf_ptype,
            msgbuf_paltform.pf_switch.vehicle_switch,
            msgbuf_paltform.pf_switch.face_switch,
            msgbuf_paltform.pf_switch.getway_switch);

		sleep(1);
	}while(ret < 0);

    while(1)
    {
        //车辆识别
        if(msgbuf_paltform.pf_vtype == NETPOSA_V)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_car = %d ,NetPosaIP = %s, NetPosaID = %s, NetPosaPORT=%d", msgbuf_paltform.pf_vtype, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);
            gi_NetPose_vehicle_switch = msgbuf_paltform.pf_switch.vehicle_switch == V_on ? 1 : 0;
        }
        if(msgbuf_paltform.pf_vtype == DaHua_V)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_car = %d ,DaHuaIP = %s, DaHuaID = %s, DaHuaPORT=%d", msgbuf_paltform.pf_vtype, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.DeviceId, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort);

            gi_Dahua_vehicle_switch = msgbuf_paltform.pf_switch.vehicle_switch == V_on ? 1 : 0;
        }

        //人脸识别
        if(msgbuf_paltform.pf_ftype == NETPOSA)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_face = %d ,NetPosaIP = %s, NetPosaID = %s, NetPosaPORT=%d", msgbuf_paltform.pf_ftype, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId, msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);
        }
        if(msgbuf_paltform.pf_ftype == DaHua)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_face = %d ,DaHuaIP = %s, DaHuaID = %s, DaHuaPORT=%d", msgbuf_paltform.pf_ftype, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.DeviceId, msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort);
        }

        //出入口识别
        if(msgbuf_paltform.pf_gtype == BITCOM)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_alleyway = %d ,BITCOMIP = %s, BITCOMID = %s, BITCOMPORT=%d", msgbuf_paltform.pf_gtype, msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp, msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId, msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);

            gi_Bitcom_alleyway_switch = msgbuf_paltform.pf_switch.getway_switch == G_on ? 1 : 0;
        }

        if(msgbuf_paltform.pf_itype == UNIVIEW)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_vp = %d ,UNIVIEWIP = %s, UNIVIEWID = %s, UNIVIEWPORT=%d", msgbuf_paltform.pf_itype, msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.ServerIp, msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.DeviceId, msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.MsgPort);

            gi_Uniview_illegally_park_switch = msgbuf_paltform.pf_switch.illegally_park_switch == I_on ? 1 : 0;
        }

        if(msgbuf_paltform.pf_itype == BAOKANG)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_vp = %d" , msgbuf_paltform.pf_itype);

            gi_Baokang_illegally_park_switch = msgbuf_paltform.pf_switch.illegally_park_switch == I_on ? 1 : 0;;
        }

        if(msgbuf_paltform.pf_ptype == ZEHIN)
        {
            TRACE_LOG_PLATFROM_INTERFACE("PLATFORM_park = %d ,ZEHINIP = %s, ZEHINID = %s, ZEHINPORT=%d", msgbuf_paltform.pf_itype, msgbuf_paltform.Msg_buf.Zehin_Park_Mag.CenterIp, msgbuf_paltform.Msg_buf.Zehin_Park_Mag.DeviceId, msgbuf_paltform.Msg_buf.Zehin_Park_Mag.CmdPort);
            gi_Zehin_park_switch = msgbuf_paltform.pf_switch.park_switch == P_on ? 1 : 0;
        }


        //send platform info to dsp
		TRACE_LOG_PLATFROM_INTERFACE("vd %s send_dsp_msg PLATFORM_MSG msg_type=10\n",__func__);
		send_dsp_msg(&msgbuf_paltform,sizeof(PLATFORM_MSG),10);//消息类型，1，算法配置参数；2，隐形配置参数；3，车牌库；4，道闸控制输入参数；5，电子车牌触发参数；6，重启前记录回传；7，arm参数；8，抓拍请求；10，平台信息
//    	printf_with_ms("%s: after send_dsp_msg PLATFORM_MSG \n",__func__);


        //wait for new platform info
		while(1)
		{
			ret=Msg_Rsv(qid, MSG_TYPE_MSG25, &msgbuf_paltform, sizeof(msgbuf_paltform));
			if(ret<0)
			{
				usleep(10000);
				ERROR("vd %s recv PLATFORM_MSG failed\n",__func__);
			}
			else
			{
				break;
			}
		}
    }

	return NULL;
}


/***************************************************
 * 函数名:发送设备状态
 * 功  能: bitcom平台对接，只负责发送心跳
 * 参  数:
 * 返回值: 成功，返回0；出错，返回NULL
 ****************************************************/
void *StatusThrFxn(void * arg)
{
    static str_alleyway_status lstr_alleyway_status;

	while(1)
	{
	    if(flg_register_Bitcom == 1)
	    {
		//发送bitcom状态
		if((lstr_alleyway_status.FaultState!=gstr_alleyway_status.FaultState) || \
			(lstr_alleyway_status.SenseCoilState!=gstr_alleyway_status.SenseCoilState) || \
			(lstr_alleyway_status.FlashlLightState!=gstr_alleyway_status.FlashlLightState) || \
			(lstr_alleyway_status.IndicatoLightState!=gstr_alleyway_status.IndicatoLightState))
		{
		    alleyway_sendstatus_to_bitcom(NULL, NULL);
			lstr_alleyway_status.FaultState = gstr_alleyway_status.FaultState;
			lstr_alleyway_status.SenseCoilState = gstr_alleyway_status.SenseCoilState;
			lstr_alleyway_status.FlashlLightState = gstr_alleyway_status.FlashlLightState;
			lstr_alleyway_status.IndicatoLightState = gstr_alleyway_status.IndicatoLightState;
		}
	    }

		sleep(1);
	}

	return NULL;
}

/***************************************************
 * 函数名:NetPoseThrFxn
 * 功  能: 大华平台对接，只负责发送心跳
 * 参  数:
 * 返回值: 成功，返回0；出错，返回NULL
 ****************************************************/
void *DahuaThrFxn(void * arg)
{
	pthread_t dahua_thread;
	if(pthread_create(&dahua_thread, NULL, bitcom_sendto_dahua, NULL))
	{
		ERROR("Creat ListenThrFxn failed! \n");
	}

	TRACE_LOG_SYSTEM("In DahuaThrFxn\n");
	int sockfd;
	struct sockaddr_in client_addr;
	char buf[1024];
	fd_set rset;
	struct timeval tv;

	while(1)
	{
	    sleep(5);

		flg_register_Dahua = 0;

		if((msgbuf_paltform.pf_vtype == DaHua_V) && (msgbuf_paltform.pf_switch.vehicle_switch == V_on))
		{

			//************创建套接字发送心跳信息************//
			if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
			{
				TRACE_LOG_PLATFROM_INTERFACE("\nDaHua create socket failed ");
			    continue;
			}

			bzero(&client_addr, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort);
			client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp);


			if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
			{
			    close(sockfd);
			    TRACE_LOG_SYSTEM("\nDaHua connect failed ");
				continue;
			}


			while(1)
			{
			    if((msgbuf_paltform.pf_vtype == DaHua_V) && (msgbuf_paltform.pf_switch.vehicle_switch == V_on))
			    {

				    char nowtime[64];
				    memset(nowtime,0,64);
				    getime_for_dahua(nowtime);

				    //***********心跳消息体************//
				    char str_body[64];
				    memset(str_body,0,64);
				    sprintf(str_body,"{\r\n"\
					    "%s\n"\
					    "}\r\n",nowtime);
				    int len = strlen(str_body);
				    printf("str_body:\n");
				    printf("%s\n",str_body);


				    //************心跳消息头************//
				    char str_heartbeat[512];
				    memset(str_heartbeat,0,512);
				    sprintf(str_heartbeat,"POST /DH/Devices/%s$0/Keepalive HTTP/1.1\r\n"\
					    "Connection: Keep-alive\r\n"\
					    "Content-Length: %d\r\n"\
					    "Content-Type: application/json;charset=UTF-8\r\n"\
					    "Host: %s:%d\r\n\r\n",
					    msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.DeviceId,len,msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp,
					    msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort);

				    //************拼接************//
				    strcat(str_heartbeat,str_body);

				    int ret = send(sockfd,str_heartbeat,strlen(str_heartbeat),0);

				    TRACE_LOG_SYSTEM("DaHua send heartbeat : %s", str_heartbeat);
				    if(ret < 0)
				    {
				    	TRACE_LOG_SYSTEM("DaHua send heartbeat failed");
				    	close(sockfd);
					    break;
				    }

				    FD_ZERO(&rset);
				    FD_SET(sockfd, &rset);
				    tv.tv_sec= 1;
				    tv.tv_usec= 0;

				    int h= select(sockfd +1, &rset, NULL, NULL, &tv);

				    if (h < 0)//select failed
				    {
				    	TRACE_LOG_SYSTEM("DaHua select failed");
				    	close(sockfd);
					    break;
				    }
				    else if(h==0)//timeout
					{
						TRACE_LOG_SYSTEM("DaHua select timeout");
					}
				    else if (h > 0)//successful, to read data
				    {
				    	memset(buf, 0, 1024);
					    int i= read(sockfd, buf, 1023);
					    if (i==0)
					    {
					    	TRACE_LOG_SYSTEM("DaHua read failed");
					    	close(sockfd);
						    break;
					    }

					    TRACE_LOG_SYSTEM("\nDaHua recive heartbeat back : %s", buf);
					    flg_register_Dahua = 1;

				    }
			    }

				sleep(10);

			}

		}

	}

	return NULL;

}


/***************************************************
 * 函数名:NetPoseFaceThrFxn
 * 功  能: 东方网力平台对接，只负责发送注册信息和心跳(行人平台注册和心跳)
 * 参  数:
 * 返回值: 成功，返回0；出错，返回NULL
 ****************************************************/
void *NetPoseFaceThrFxn(void * arg)
{


	//进行平台注册
	while(1)
	{
	    sleep(5); //注册失败应隔两秒重新注册，心跳和发送过车消息应在注册成功之后进行
	    int sockfd;
		struct sockaddr_in client_addr;

		printf("Register NetPose Platform_Face : IP : %s Port : %d\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp,
			msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort);
		if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
		{
		    perror("#############Face socket error");
			continue;
		}

		bzero(&client_addr, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort);
		client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp);

		if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
		{
		    perror("################Face Connect Error");
			close(sockfd);
			continue;
		}

		printf("--------Connect NetPose_Platform_Face success!--------\n");


		/******************发送注册信息******************/

		//获取本机IP地址
		char ip_local[32];
		memset(ip_local,0,32);

		sprintf(ip_local,"%u.%u.%u.%u",
			g_set_net_param.m_NetParam.m_IP[0],
			g_set_net_param.m_NetParam.m_IP[1],
			g_set_net_param.m_NetParam.m_IP[2],
			g_set_net_param.m_NetParam.m_IP[3]);

		printf("In DfwlPlatformThrFxn:ip_local---%s\n",ip_local);

		//---------注册的消息体---------//
		char str_body[128];
		memset(str_body, 0, 128);

		sprintf(str_body,"deviceIp=%s&msgPort=%d&company=%s&videoRegFlag=False",ip_local,msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort,
			msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.Company);
		//		printf("str_body = %s\n", str_body);

		int len_body = strlen(str_body);

		//---------注册的请求行，若干请求头---------//
		char str_register[512];
		memset(str_register,0,512);
		sprintf(str_register,"POST /toll-face/home/regist?deviceId=%s HTTP/1.1\r\n"\
			"Content-Length: %d\r\n"\
			"Pragma: no-cache\r\n"\
			"Cache-Control: no-cache\r\n"\
			"Accept: text/html\r\n"\
			"Connection : Keep-alive\r\n\r\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.DeviceId,len_body);


		//---------拼接消息头跟消息体---------//

		strcat(str_register,str_body);

		printf("###############Face register info################\n");
		printf("%s\n",str_register);
		int ret = write(sockfd,str_register,strlen(str_register));
		if (ret < 0)
		{
		    perror("Face Post error");

			close(sockfd);
			continue;
		}

		close(sockfd);
		flg_register_NetPose_face = 1;

		#if 0
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		tv.tv_sec= 1;
		tv.tv_usec= 0;

		int h= select(sockfd +1, &rset, NULL, NULL, &tv); //等到1秒，如果socket还不可读，那么select将会返回0，重新发送请求

		if (h == 0)
		{
		    close(sockfd);
			continue;
		}

		if (h < 0)
		{
		    close(sockfd);
			printf("#################Face Select error.\n");
			continue;
		}

		if (h > 0)
		{
		    memset(buf, 0, 1024);
			int i= read(sockfd, buf, 1023);
			if (i==0)
			{
			    close(sockfd);
				printf("################Face target closed\n");
				continue;
			}
		    printf("********The Face register receive is ********\n");
			printf("%s\n", buf);
			printf("************************\n");

			close(sockfd);
			flg_register_NetPose_face = 1;
		}
	    #endif

		while(1 == flg_register_NetPose_face)
		{
		    sleep(30);
			int sockfd;
			static int count = 0;
			struct sockaddr_in client_addr;
			char buf[1024];
			fd_set rset;
			struct timeval tv;

			if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
			{
			    perror("###########Face socket error");
				if(++count == 3)
				{
				    count = 0;
					flg_register_NetPose_face = 0;

					close(sockfd);

					break;
				}
			    continue;
			}

			bzero(&client_addr, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort);
			client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp);

			if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
			{
			    perror("#############Face Connect Error");
				if(++count == 3)
				{
				    count = 0;
					flg_register_NetPose_face = 0;
					close(sockfd);
					break;
				}
			    close(sockfd);
				continue;
			}

			printf("--------Connect NetPose_Platform_Face success!--------\n");

			//------------心跳消息体------------//
			char str_body[312];
			memset(str_body,0,312);

			sprintf(str_body,"<Message>\r\n"\
				"<Device StateCode=\"1\">\r\n"\
				"<Status Name=\"Detector\" Value=\"1\" />\r\n"\
				"<Status Name=\"Flash\" Value=\"1\" />\r\n"\
				"<Status Name=\"Video\"  Value=\"1\" />\r\n"\
				"</Device>\r\n"\
				"</Message>\r\n");
			int len = strlen(str_body);

			//----------心跳消息头部----------//
			char str_heartbeat[512];
			memset(str_heartbeat,0,512);

			sprintf(str_heartbeat,"POST /toll-face/home/heartBeat?deviceId=%s HTTP/101\r\n"\
				"Content-Length: %d\r\n"\
				"Cache-Control: no-cache\r\n"\
				"Pragma: no-cache\r\n"\
				"Accept: text/html\r\n"\
				"Connection : Keep-alive\r\n\r\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.DeviceId,len);


			//----------消息拼接----------//

			strcat(str_heartbeat,str_body);

			printf("heartbeat info:%s\n",str_heartbeat);

			int ret = write(sockfd,str_heartbeat,strlen(str_heartbeat));

			if (ret < 0)
			{
			    perror("heartbeat write error");
				if(++count == 3)
				{
				    count = 0;
					flg_register_NetPose_face = 0;
					close(sockfd);
					break;
				}
			    close(sockfd);
				continue;
			}

			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			tv.tv_sec= 1;
			tv.tv_usec= 0;
			int h= select(sockfd +1, &rset, NULL, NULL, &tv);

			if (h == 0)
			{
			    if(++count == 3)
			    {
				count = 0;
				    flg_register_NetPose_face = 0;
				    close(sockfd);
				    break;
			    }

				close(sockfd);
				continue;
			}
		    if (h < 0)
		    {
			if(++count == 3)
			{
			    count = 0;
				flg_register_NetPose_face = 0;
				close(sockfd);
				break;
			}
			close(sockfd);
			    printf("Face Select error.\n");
			    continue;
		    }

			if (h > 0)
			{
			    memset(buf, 0, 1024);
				int i= read(sockfd, buf, 1023);
				if (i==0)
				{
				    if(++count == 3)
				    {
					count = 0;
					    flg_register_NetPose_face = 0;
					    close(sockfd);
					    break;
				    }
				    close(sockfd);
					printf("Face target closed\n");
					continue;
				}
			    printf("The Face heartbeat recieve is ********\n");
				printf("%s\n", buf);
				printf("************************\n");
				close(sockfd);
			}

		}

	}

	return NULL;
}

/***************************************************
 * 函数名:NetPoseThrFxn
 * 功  能: 东方网力平台对接，只负责发送注册信息和心跳
 * 参  数:
 * 返回值: 成功，返回0；出错，返回NULL
 ****************************************************/
void *NetPoseVehicleThrFxn(void * arg)
{


	//	if(Read_Platform_Info_File("/mnt/nand/platform_info") == -1)
	//	{
	//		printf("Read Platform info file failed !\n");
	//	}

	//	printf("After Read_Platform_Info_File\n");
	//	printf("The paltform_vehicle info is :\n");
	//	printf("NetPose DeviceId: %s IP : %s Port : %d\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId,msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
	//												msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);

	//进行平台注册
	while(1)
	{
	    sleep(5); //注册失败应隔两秒重新注册，心跳和发送过车消息应在注册成功之后进行
	    int sockfd;
		struct sockaddr_in client_addr;
		char buf[1024];
		fd_set rset;
		struct timeval tv;

		printf("Register NetPose Platform_Vehicle : IP : %s Port : %d\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
			msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);

		if((msgbuf_paltform.pf_vtype == NETPOSA_V)  && (msgbuf_paltform.pf_switch.vehicle_switch == V_on))
		{
		    if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
		    {
			perror("##############Vehicle socket error");
			    continue;
		    }

			bzero(&client_addr, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);
			client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp);

			if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
			{
			    perror("###############Vehicle Connect Error");
				close(sockfd);
				continue;
			}

			printf("--------Connect NetPose_Platform_Vehicle success!--------\n");


			/******************发送注册信息******************/

			//获取本机IP地址
			char ip_local[32];
			memset(ip_local,0,32);

			sprintf(ip_local,"%u.%u.%u.%u",
				g_set_net_param.m_NetParam.m_IP[0],
				g_set_net_param.m_NetParam.m_IP[1],
				g_set_net_param.m_NetParam.m_IP[2],
				g_set_net_param.m_NetParam.m_IP[3]);

			printf("In DfwlPlatformThrFxn:ip_local---%s\n",ip_local);

			//---------注册的消息体---------//
			char str_body[128];
			memset(str_body, 0, 128);

			sprintf(str_body,"deviceIp=%s&msgPort=%d&company=%s&videoRegFlag=False",ip_local,msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort,
				msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.Company);
			printf("NetPosestr_body = %s\n", str_body);


			int len_body = strlen(str_body);

			//---------注册的请求行，若干请求头---------//
			char str_register[512];
			memset(str_register,0,512);
			sprintf(str_register,"POST /toll-gate/home/regist?deviceId=%s HTTP/1.1\r\n"\
				"Content-Length: %d\r\n"\
				"Pragma: no-cache\r\n"\
				"Cache-Control: no-cache\r\n"\
				"Accept: text/html\r\n"\
				"Connection : Keep-alive\r\n\r\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId,len_body);

			//---------拼接消息头跟消息体---------//

			strcat(str_register,str_body);

			printf("##############Vehicle register info############\n");
			printf("%s\n",str_register);
			int ret = write(sockfd,str_register,strlen(str_register));
			if (ret < 0)
			{
			    close(sockfd);
				perror("Vehicle Post error");
				continue;
			}


			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			tv.tv_sec= 1;
			tv.tv_usec= 0;

			int h= select(sockfd +1, &rset, NULL, NULL, &tv); //等到1秒，如果socket还不可读，那么select将会返回0，重新发送请求

			if (h == 0)
			{
			    close(sockfd);
				continue;
			}

			if (h < 0)
			{
			    close(sockfd);
				printf("##############Vehicle Select error.\n");
				continue;
			}

			if (h > 0)
			{
			    memset(buf, 0, 1024);
				int i= read(sockfd, buf, 1023);
				if (i==0)
				{
				    close(sockfd);
					printf("############Vehicle target closed\n");
					continue;
				}
			    printf("********The Vehicle register receive is ********\n");
				printf("%s\n", buf);
				printf("************************\n");

				close(sockfd);
				flg_register_NetPose_vehicle = 1;
			}


			while((1== flg_register_NetPose_vehicle) && (msgbuf_paltform.pf_ftype == NETPOSA))
			{
			    sleep(30);
				int sockfd;
				static int count = 0;
				struct sockaddr_in client_addr;
				char buf[1024];
				fd_set rset;
				struct timeval tv;

				printf("Heartbeat NetPose Platform Vehicle: IP : %s Port : %d\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
					msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);

				if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
				{
				    perror("##############Vehicle socket error");
					if(++count == 3)
					{
					    count = 0;
						flg_register_NetPose_vehicle= 0;
						break;
					}
				    continue;
				}

				bzero(&client_addr, sizeof(client_addr));
				client_addr.sin_family = AF_INET;
				client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);
				client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp);

				if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
				{
				    perror("#############Vehicle Connect Error");
					if(++count == 3)
					{
					    count = 0;
						close(sockfd);
						flg_register_NetPose_vehicle= 0;
						break;
					}
				    close(sockfd);
					continue;
				}

				printf("--------Connect NetPose_Platform_Vehicle success!--------\n");

				//------------心跳消息体------------//
				char str_body[312];
				memset(str_body,0,312);

				sprintf(str_body,"<Message>\r\n"\
					"<Device StateCode=\"1\">\r\n"\
					"<Status Name=\"Detector\" Value=\"1\" />\r\n"\
					"<Status Name=\"Flash\" Value=\"1\" />\r\n"\
					"<Status Name=\"Video\"  Value=\"1\" />\r\n"\
					"</Device>\r\n"\
					"</Message>\r\n");
				int len = strlen(str_body);

				//----------心跳消息头部----------//
				char str_heartbeat[512];
				memset(str_heartbeat,0,512);

				sprintf(str_heartbeat,"POST /toll-gate/home/heartBeat?deviceId=%s HTTP/101\r\n"\
					"Content-Length: %d\r\n"\
					"Cache-Control: no-cache\r\n"\
					"Pragma: no-cache\r\n"\
					"Accept: text/html\r\n"\
					"Connection : Keep-alive\r\n\r\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId,len);


				//----------消息拼接----------//

				strcat(str_heartbeat,str_body);

				printf("heartbeat info:%s\n",str_heartbeat);

				int ret = write(sockfd,str_heartbeat,strlen(str_heartbeat));

				if (ret < 0)
				{
				    perror("heartbeat write error");
					if(++count == 3)
					{
					    count = 0;
						flg_register_NetPose_vehicle = 0;
						close(sockfd);
						break;
					}
				    close(sockfd);
					continue;
				}

				FD_ZERO(&rset);
				FD_SET(sockfd, &rset);
				tv.tv_sec= 1;
				tv.tv_usec= 0;
				int h= select(sockfd +1, &rset, NULL, NULL, &tv);

				if (h == 0)
				{
				    if(++count == 3)
				    {
					count = 0;
					    flg_register_NetPose_vehicle = 0;
					    close(sockfd);
					    break;
				    }

					close(sockfd);
					continue;
				}
			    if (h < 0)
			    {
				if(++count == 3)
				{
				    count = 0;
					flg_register_NetPose_vehicle = 0;
					close(sockfd);
					break;
				}
				close(sockfd);
				    printf("Vehicle Select error.\n");
				    continue;
			    }

				if (h > 0)
				{
				    memset(buf, 0, 1024);
					int i= read(sockfd, buf, 1023);
					if (i==0)
					{
					    if(++count == 3)
					    {
						count = 0;
						    flg_register_NetPose_vehicle = 0;
						    close(sockfd);
						    break;
					    }
					    close(sockfd);
						printf("Vehicle target closed\n");
						continue;
					}
				    printf("The Vehicle heartbeat recieve is ********\n");
					printf("%s\n", buf);
					printf("************************\n");
					close(sockfd);
				}


			}
		}

	}

	return NULL;
}

void *othersThrFxn(void *arg)
{
	set_thread("others");
	pthread_t NetPoseVehicleThread;
	pthread_t DahuaThread;
	pthread_t ListenThread;
	pthread_t TpiThread;

	pthread_attr_t attr;

	if (pthread_attr_init(&attr))
	{
	    ERROR("Failed to initialize thread attrs\n");
		return NULL;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	//2015/04/27 by shp
#ifndef PARK_ZEHIN_THIN
	if(pthread_create(&ListenThread, &attr, ListenThrFxn, NULL))
	{
		ERROR("Creat ListenThrFxn failed! \n");
	}

    // sleep three mins wait vd get configuration from system_server.
    sleep(3);
	//东方网力平台
	if (pthread_create(&NetPoseVehicleThread, &attr, NetPoseVehicleThrFxn, NULL))
	{
		ERROR("Failed to create NetPoseVehicleThrFxn thread\n");
	}

	//大华平台
	if (pthread_create(&DahuaThread, &attr, DahuaThrFxn, NULL))
	{
		ERROR("Failed to create DahuaThrFxn thread\n");
	}
#endif
	if (pthread_create(&TpiThread, &attr, platform_thread, NULL))
	{
		ERROR("Failed to create BitcomThrFxn thread\n");
	}

	pthread_attr_destroy(&attr);

	pthread_exit(NULL);

	return NULL;
}
