#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <file_msg_drv.h>

#include "interface.h"
#include "serial_ctrl.h"
#include "lib_i2c.h"
#include "i2c_sem_util.h" //add by dsl, 2014-1-3
#include "sysctrl.h"
#include "sys_msg_drv.h"
#include "ApproDrvMsg.h"
#include "Appro_interface.h"
#include "Msg_Def.h"
#include "commonfuncs.h"
#include "logger/log.h"

/* add by dsl, 2013-9-24 */
static volatile int runflag = 0;
CallBack pFunc = NULL;
void *signalDetectThread(void * arg);
/* end added, dsl, 2013-9-24 */


#define TIMEUNIT	(10*1000) /*10ms*/ // modified by zdy, 2013-10-17, origin 100ms
#define SIGTIMER	(SIGUSR1)
static pthread_t tTimer = -1;
pthread_cond_t timerCond;
pthread_mutex_t timerMtx;
int timerOn = 0;
int timeToSleep = -1;
int md44_fd = -1;

static sem_t *tty_sem = NULL;

void timer_func(int sig)
{
	printf("alei@%s(L%d): at %s() catch TIMEOUT\n", __FILE__, __LINE__, __func__);
}


void * timerLoop(void * arg)
{
	set_thread("timerLoop");
	timeToSleep = -1;
	timerOn = 1;
	int thisSleepTime = 0;

	while(timerOn)
	{
		pthread_mutex_lock(&timerMtx);
		while ((timeToSleep == -1)&&(timerOn == 1))
		{
			pthread_cond_wait(&timerCond, &timerMtx);
		}
		thisSleepTime = (timeToSleep > TIMEUNIT ?  TIMEUNIT : timeToSleep);
		timeToSleep = (timeToSleep > TIMEUNIT ? timeToSleep - TIMEUNIT : 0);
		pthread_mutex_unlock(&timerMtx);

		if (timerOn == 0)
			break;

		usleep(thisSleepTime);

		pthread_mutex_lock(&timerMtx);
		if (timeToSleep == 0)   //timeout
		{
			kill(getpid(), SIGTIMER);
			timeToSleep = -1; //timer off
		}
		else if (timeToSleep < 0)     //timer off
		{
			//do nothing
		}
		else if (timeToSleep > 0)
		{
			//continue;
		}
		pthread_mutex_unlock(&timerMtx);
	}

	timeToSleep = -1;
	timerOn = 0;
	return 0;
}

static int start_timer(int usec)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGTIMER);
	if(sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0)
	{
		printf("alei@%s(L%d): at %s() unblock singal failed.\n", __FILE__, __LINE__, __func__);
		return -1;
	}
	if(signal(SIGTIMER, timer_func) < 0)
	{
		printf("alei@%s(L%d): at %s() register singal failed.\n", __FILE__, __LINE__, __func__);
		return -1;
	}

	//timerOn = 1;
	pthread_mutex_lock(&timerMtx);
	timeToSleep = usec;
	pthread_mutex_unlock(&timerMtx);
	pthread_cond_signal(&timerCond);

	return 0;
}


static int stop_timer()
{
	pthread_mutex_lock(&timerMtx);
	timeToSleep = -1;
	pthread_mutex_unlock(&timerMtx);
	//pthread_cond_signal(&timerCond);

	return 0;
}


int Init_Com_To_File()
{
	if (InitFileMsgDrv(FILE_MSG_KEY, FILE_VD_MSG) < 0)
	{
		ERROR("Error when InitFileMsgDrv.\n");
		return -1;
	}

	DEBUG("InitFileMsgDrv done.\n");

	return 0;
}

void Deinit_Com_To_File()
{
	CleanupFileMsgDrv();
}

net_config *get_net_config()
{
#if 0
	start_timer(3000000);
	static net_config net_cfg;

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		stop_timer();
		return NULL;
	}

	net_cfg.ip = pSysInfo->lan_config.net.ip;
	net_cfg.netmask = pSysInfo->lan_config.net.netmask;
	net_cfg.gateway = pSysInfo->lan_config.net.gateway;
	/*alei: modify to get the MAC address */
	memset(net_cfg.mac, '\0', sizeof(net_cfg.mac));
	sprintf(net_cfg.mac, "%02x:%02x:%02x:%02x:%02x:%02x", pSysInfo->lan_config.net.MAC[0],
	        pSysInfo->lan_config.net.MAC[1], pSysInfo->lan_config.net.MAC[2],
	        pSysInfo->lan_config.net.MAC[3], pSysInfo->lan_config.net.MAC[4],
	        pSysInfo->lan_config.net.MAC[5]);

	stop_timer();

	return &net_cfg;
#endif	
	return NULL;
}

int set_net_config(net_config *net)
{
	start_timer(3000000);
	int error_num = 0;
	if (ControlSystemData(SFIELD_SET_IP, (void *) &net->ip.s_addr,
	                      sizeof(net->ip.s_addr)) < 0)
	{
		printf("[Error:] IP set failure!\n");
		error_num = -1;
	}
	if (ControlSystemData(SFIELD_SET_GATEWAY, (void *) &net->gateway.s_addr,
	                      sizeof(net->gateway.s_addr)) < 0)
	{
		printf("[Error:] Gateway set failure!\n");
		error_num = -1;
	}
	if (ControlSystemData(SFIELD_SET_NETMASK, (void *) &net->netmask.s_addr,
	                      sizeof(net->netmask.s_addr)) < 0)
	{
		printf("[Error:] Netmask set failure!\n");
		error_num = -1;
	}
	/*	not set mac address, zdy, 2013-09-09 */
	// if (ControlSystemData(SFIELD_SET_MAC, (void *) &net->mac,
	//                       strlen(net->mac)) < 0)
	// {
	//     printf("[Error:] Mac set failure!\n");
	//     error_num = -1;
	// }
	stop_timer();
	return error_num;
}

H264_config *get_h264_config()
{
	start_timer(3000000);
	int ch = -1;
	static H264_config h264_configure;

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		stop_timer();
		return NULL;
	}
	//TODO to be added
	for (ch = 0; ch < 2; ch++)
	{
		h264_configure.h264_channel[ch].h264_on = pSysInfo->h264codecre_config[ch].nH264Enable;
		h264_configure.h264_channel[ch].cast = (int)pSysInfo->h264codecre_config[ch].unimul_config.UMEnable;

		/* get IP address for this channel */
		/*if this is an IPv6 address, then how to convert?*/
		char ipAddr[30] = "";
		strncpy(ipAddr, pSysInfo->h264codecre_config[ch].unimul_config.IPaddr_UM, sizeof(ipAddr));
		//printf("H264_channel[%d].IP=%s.\n", ch, ipAddr);

		int ip[4];
		sscanf(ipAddr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
		h264_configure.h264_channel[ch].ip[0] = ip[0];
		h264_configure.h264_channel[ch].ip[1] = ip[1];
		h264_configure.h264_channel[ch].ip[2] = ip[2];
		h264_configure.h264_channel[ch].ip[3] = ip[3];

		/*@alei: just for debug*/
		int index = 0;
		for ( ; index < 4; index++)
		{
			// printf("H264_channel[%d] IP[%d] = %d.\n", ch, index, h264_configure.h264_channel[ch].ip[index]);
		}

		h264_configure.h264_channel[ch].port =
		    (ch == 0 ? pSysInfo->h264codecre_config[0].unimul_config.Port_UM : pSysInfo->h264codecre_config[1].unimul_config.Port_UM);
		/*@alei: just for debug*/
		// printf("CH[%d] port=%d.\n", ch, h264_configure.h264_channel[ch].port);

		h264_configure.h264_channel[ch].rate = (ch == 0 ? pSysInfo->lan_config.nMpeg41bitrate/1000 : pSysInfo->lan_config.nMpeg42bitrate/1000);
		/*@alei: just for debug*/
		// printf("CH[%d] rate=%d.\n", ch, h264_configure.h264_channel[ch].rate);

		/*	modified by zdy, 2013-09-29, framerate = 15 */
		h264_configure.h264_channel[ch].fps = 15; //(ch == 0 ? pSysInfo->lan_config.nFrameRate1 : pSysInfo->lan_config.nFrameRate2);
		/* end modified, zdy, 2013-09-29 */
		/*@alei: just for debug*/
		// printf("CH[%d] fps=%d.\n", ch, h264_configure.h264_channel[ch].fps);

		h264_configure.h264_channel[ch].osd_info.color.r = pSysInfo->osd_info_inter[ch].color_inter.r;
		h264_configure.h264_channel[ch].osd_info.color.g = pSysInfo->osd_info_inter[ch].color_inter.g;
		h264_configure.h264_channel[ch].osd_info.color.b = pSysInfo->osd_info_inter[ch].color_inter.b;
		/*@alei: just for debug*/
		// printf("CH[%d] r=%d g=%d b=%d\n", ch, h264_configure.h264_channel[ch].osd_info.color.r,
		// h264_configure.h264_channel[ch].osd_info.color.g,
		// h264_configure.h264_channel[ch].osd_info.color.b);

		/*FIXME: missing time and so on */


		h264_configure.h264_channel[ch].width = pSysInfo->h264codecre_config[ch].width;
		h264_configure.h264_channel[ch].height = pSysInfo->h264codecre_config[ch].height;
		/*@alei: just for debug*/
		// printf("CH[%d] width=%d height=%d.\n", ch, h264_configure.h264_channel[ch].width,
		// 			h264_configure.h264_channel[ch].height);

		/*	add by zdy, 2013-07-30	*/
		int item = 0;
		for ( ; item < 8; ++item)
		{
			h264_configure.h264_channel[ch].osd_info.osd_item[item].switch_on = pSysInfo->osd_info_inter[ch].osd_item_inter[item].switch_on;
			h264_configure.h264_channel[ch].osd_info.osd_item[item].x = pSysInfo->osd_info_inter[ch].osd_item_inter[item].x;
			h264_configure.h264_channel[ch].osd_info.osd_item[item].y = pSysInfo->osd_info_inter[ch].osd_item_inter[item].y;
			h264_configure.h264_channel[ch].osd_info.osd_item[item].is_time = pSysInfo->osd_info_inter[ch].osd_item_inter[item].is_time;
			strcpy(h264_configure.h264_channel[ch].osd_info.osd_item[item].content, pSysInfo->osd_info_inter[ch].osd_item_inter[item].content);

			// printf("h264_configure.h264_channel[%d].osd_info.osd_item[%d].switch_on = %d\n", ch, item, h264_configure.h264_channel[ch].osd_info.osd_item[item].switch_on);
			// printf("h264_configure.h264_channel[%d].osd_info.osd_item[%d].x = %d\n", ch, item, h264_configure.h264_channel[ch].osd_info.osd_item[item].x);
			// printf("h264_configure.h264_channel[%d].osd_info.osd_item[%d].y = %d\n", ch, item, h264_configure.h264_channel[ch].osd_info.osd_item[item].y);
			// printf("h264_configure.h264_channel[%d].osd_info.osd_item[%d].is_time = %d\n", ch, item, h264_configure.h264_channel[ch].osd_info.osd_item[item].is_time);
			// printf("h264_configure.h264_channel[%d].osd_info.osd_item[%d].content = %s\n", ch, item, h264_configure.h264_channel[ch].osd_info.osd_item[item].content);
		}
		/*	end added 	*/

	}

	stop_timer();

	return &h264_configure;
}


int set_h264_config(H264_config *h264_configure)
{
	return 0; 	/* ÔÝÊ±ÆÁ±Îµô */

	//fprintf(stderr, "######################step into %s() ############################\n", __func__);
	int restartflag = 0;
	time_t time1, time2;
	int ret = SUCCESS;

	unsigned char h264enable1 = h264_configure->h264_channel[0].h264_on;
	unsigned char h264enable2 = h264_configure->h264_channel[1].h264_on;

	//init_env();
	start_timer(120*1000*1000);
	printf("inside: %s\n", __func__);
	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		ret = FAIL;
		goto exit;
	}

	time1 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d\n", __FILE__, __LINE__, __func__, (int)time1);


	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	char cast1 = h264_configure->h264_channel[0].cast;
	char ip1[20];
	memset(ip1, 0, sizeof(ip1));
	sprintf(ip1, "%d.%d.%d.%d", h264_configure->h264_channel[0].ip[0],
	        h264_configure->h264_channel[0].ip[1],
	        h264_configure->h264_channel[0].ip[2],
	        h264_configure->h264_channel[0].ip[3]);
	printf("IP1:%s\n", ip1);
	unsigned short int port1 = h264_configure->h264_channel[0].port;


	if (cast1 == 1)   //if CH0 enable
	{
		if (cast1 != (int)pSysInfo->h264codecre_config[0].unimul_config.UMEnable
		        || strcmp(ip1, pSysInfo->h264codecre_config[0].unimul_config.IPaddr_UM) != 0
		        || port1 != pSysInfo->h264codecre_config[0].unimul_config.Port_UM)
		{

			if (ControlSystemData(SFIELD_SET_UM_ENABLE1, (void *) &cast1, sizeof(cast1)) < 0)
			{
				//deinit_env();
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;


			if (ControlSystemData(SFIELD_SET_UM_IP1, (void *) ip1, strlen(ip1)) < 0)
			{
				//deinit_env();
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;


			if (port1 % 2 == 0)
			{
				if (ControlSystemData(SFIELD_SET_UM_PORT1, (void *) &port1, sizeof(port1)) < 0)
				{
					//deinit_env();
					ret = FAIL;
					goto exit;
				}
			}
			else
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;

			restartflag = 1;

		}
	}
	else if (cast1 == 0)     //if ch0 unable,
	{
		if(cast1 != (int)pSysInfo->h264codecre_config[0].unimul_config.UMEnable)   //original enable
		{
			if (ControlSystemData(SFIELD_SET_UM_ENABLE1, (void *) &cast1, sizeof(cast1)) < 0)
			{
				//deinit_env();
				ret = FAIL;
				goto exit;
			}
			restartflag = 1;
		}
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	if (h264enable2 == 1)  //dual stream, do the follwing
	{
		char cast2 = h264_configure->h264_channel[1].cast;
		char ip2[20];
		memset(ip2, 0, sizeof(ip2));
		sprintf(ip2, "%d.%d.%d.%d", h264_configure->h264_channel[1].ip[0],
		        h264_configure->h264_channel[1].ip[1],
		        h264_configure->h264_channel[1].ip[2],
		        h264_configure->h264_channel[1].ip[3]);
		printf("IP2:%s\n", ip2);
		unsigned short int port2 = h264_configure->h264_channel[1].port;

		if (cast2 == 1)   //if CH1 enable
		{
			if (cast2 != (int)pSysInfo->h264codecre_config[1].unimul_config.UMEnable
			        || strcmp(ip2, pSysInfo->h264codecre_config[1].unimul_config.IPaddr_UM) != 0
			        || port2 != pSysInfo->h264codecre_config[1].unimul_config.Port_UM)
			{

				if (ControlSystemData(SFIELD_SET_UM_ENABLE2, (void *) &cast2, sizeof(cast2)) < 0)
				{
					//deinit_env();
					ret = FAIL;
					goto exit;
				}

				time2 = time(NULL);
				printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
				time1 = time2;


				if (ControlSystemData(SFIELD_SET_UM_IP2, (void *) ip2, strlen(ip2)) < 0)
				{
					//deinit_env();
					ret = FAIL;
					goto exit;
				}

				time2 = time(NULL);
				printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
				time1 = time2;


				if (port2 % 2 == 0)
				{
					if (ControlSystemData(SFIELD_SET_UM_PORT2, (void *) &port2, sizeof(port2)) < 0)
					{
						//deinit_env();
						ret = FAIL;
						goto exit;
					}
				}
				else
				{
					ret = FAIL;
					goto exit;
				}

				time2 = time(NULL);
				printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
				time1 = time2;
				restartflag = 1;

			}
		}
		else if (cast2 == 0)     //if ch1 unable,
		{
			if(cast2 != (int)pSysInfo->h264codecre_config[1].unimul_config.UMEnable)   //original enable
			{
				if (ControlSystemData(SFIELD_SET_UM_ENABLE2, (void *) &cast2, sizeof(cast2)) < 0)
				{
					//deinit_env();
					ret = FAIL;
					goto exit;
				}
				time2 = time(NULL);
				printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
				time1 = time2;
				restartflag = 1;
			}
		}

	}
	else if (h264enable2 == 0)     //CH1 is unable
	{
		//do nothing
	}



	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	int bitrate1 = h264_configure->h264_channel[0].rate;
	bitrate1 *= 1000;

	if (ControlSystemData(SFIELD_SET_MPEG41_BITRATE, (void *) &bitrate1,
	                      sizeof(bitrate1)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	__u8 framerate1 = 15;//h264_configure->h264_channel[0].fps;
	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_FRAMERATE1, (void *) &framerate1,
	                      sizeof(framerate1)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	// osd color r 1
	__u8 r1 = h264_configure->h264_channel[0].osd_info.color.r;
	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_R0, (void *) &r1, sizeof(r1))
	        < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	// osd color g 1
	__u8 g1 = h264_configure->h264_channel[0].osd_info.color.g;
	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_G0, (void *) &g1, sizeof(g1))
	        < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	// osd color b 1
	// printf("osd b 1...........\n");

	__u8 b1 = h264_configure->h264_channel[0].osd_info.color.b;


	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_B0, (void *) &b1, sizeof(b1))
	        < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;



	if (h264enable2)
	{
		int bitrate2 = h264_configure->h264_channel[1].rate;
		bitrate2 *= 1000;

		if (ControlSystemData(SFIELD_SET_MPEG42_BITRATE, (void *) &bitrate2,
		                      sizeof(bitrate2)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;
		// framerate 1

		// framerate 2
		__u8 framerate2 = 15;//h264_configure->h264_channel[1].fps;
		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_FRAMERATE2, (void *) &framerate2,
		                      sizeof(framerate2)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		// osd color r 2
		// printf("osd r 2...........\n");

		__u8 r2 = h264_configure->h264_channel[1].osd_info.color.r;


		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_R1, (void *) &r2, sizeof(r2))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		// osd color g 2
		// printf("osd g 2...........\n");

		__u8 g2 = h264_configure->h264_channel[1].osd_info.color.g;

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_G1, (void *) &g2, sizeof(g2))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		// osd color b 2
		// printf("osd b 2...........\n");

		__u8 b2 = h264_configure->h264_channel[1].osd_info.color.b;

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_B1, (void *) &b2, sizeof(b2))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;
	}

	/*	add by zdy, 2013-07-30	*/
	char switchon[2][8];
	int posx[2][8];
	int posy[2][8];
	char istime[2][8];
	char content[2][8][64];

	memset(content, 0, sizeof(content));
	int ch = 0;
	int item = 0;


	for (ch = 0; ch < 2; ch++)
	{
		item = 0;
		for (; item < 8; item++)
		{
			switchon[ch][item] = h264_configure->h264_channel[ch].osd_info.osd_item[item].switch_on;
			posx[ch][item] = h264_configure->h264_channel[ch].osd_info.osd_item[item].x;
			posy[ch][item] = h264_configure->h264_channel[ch].osd_info.osd_item[item].y;
			istime[ch][item] = h264_configure->h264_channel[ch].osd_info.osd_item[item].is_time;
			strcpy(content[ch][item], h264_configure->h264_channel[ch].osd_info.osd_item[item].content);

			// printf("switchon[%d][%d] = %d\n", ch, item, switchon[ch][item]);
			// printf("posx[%d][%d] = %d\n", ch, item, posx[ch][item]);
			// printf("posy[%d][%d] = %d\n", ch, item, posy[ch][item]);
			// printf("istime[%d][%d] = %d\n", ch, item, istime[ch][item]);
			// printf("content[%d][%d] = %s\n", ch, item, content[ch][item]);
		}
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;
	// osd switch

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH00, (void *)&switchon[0][0], sizeof(switchon[0][0])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH01, (void *)&switchon[0][1], sizeof(switchon[0][1])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH02, (void *)&switchon[0][2], sizeof(switchon[0][2])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH03, (void *)&switchon[0][3], sizeof(switchon[0][3])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH04, (void *)&switchon[0][4], sizeof(switchon[0][4])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH05, (void *)&switchon[0][5], sizeof(switchon[0][5])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH06, (void *)&switchon[0][6], sizeof(switchon[0][6])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
	if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH07, (void *)&switchon[0][7], sizeof(switchon[0][7])) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	if(h264enable2)
	{
		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH10, (void *)&switchon[1][0], sizeof(switchon[1][0])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH11, (void *)&switchon[1][1], sizeof(switchon[1][1])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH12, (void *)&switchon[1][2], sizeof(switchon[1][2])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH13, (void *)&switchon[1][3], sizeof(switchon[1][3])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH14, (void *)&switchon[1][4], sizeof(switchon[1][4])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		//printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH15, (void *)&switchon[1][5], sizeof(switchon[1][5])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH16, (void *)&switchon[1][6], sizeof(switchon[1][6])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_SWITCH17, (void *)&switchon[1][7], sizeof(switchon[1][7])) < 0)
		{
			ret = FAIL;
			goto exit;
		}
	}


	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;
	// osd item 1
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 1");
	if (switchon[0][0] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X00, (void *)&posx[0][0], sizeof(posx[0][0])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y00, (void *)&posy[0][0], sizeof(posy[0][0])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME00, (void *)&istime[0][0], sizeof(istime[0][0])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][0] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT00, (void *)&content[0][0], strlen(content[0][0])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}
	}

	// osd item 2
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 2");

	if (switchon[0][1] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X01, (void *)&posx[0][1], sizeof(posx[0][1])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y01, (void *)&posy[0][1], sizeof(posy[0][1])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME01, (void *)&istime[0][1], sizeof(istime[0][1])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][1] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT01, (void *)&content[0][1], strlen(content[0][1])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 3
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 3");

	if (switchon[0][2] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X02, (void *)&posx[0][2], sizeof(posx[0][2])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y02, (void *)&posy[0][2], sizeof(posy[0][2])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME02, (void *)&istime[0][2], sizeof(istime[0][2])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][2] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT02, (void *)&content[0][2], strlen(content[0][2])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 4
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 4");

	if (switchon[0][3] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X03, (void *)&posx[0][3], sizeof(posx[0][3])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y03, (void *)&posy[0][3], sizeof(posy[0][3])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME03, (void *)&istime[0][3], sizeof(istime[0][3])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][3] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT03, (void *)&content[0][3], strlen(content[0][3])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 5
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 5");

	if (switchon[0][4] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X04, (void *)&posx[0][4], sizeof(posx[0][4])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y04, (void *)&posy[0][4], sizeof(posy[0][4])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME04, (void *)&istime[0][4], sizeof(istime[0][4])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][4] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT04, (void *)&content[0][4], strlen(content[0][4])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 6
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 6");

	if (switchon[0][5] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X05, (void *)&posx[0][5], sizeof(posx[0][5])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y05, (void *)&posy[0][5], sizeof(posy[0][5])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME05, (void *)&istime[0][5], sizeof(istime[0][5])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][5] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT05, (void *)&content[0][5], strlen(content[0][5])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 7

	if (switchon[0][6] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X06, (void *)&posx[0][6], sizeof(posx[0][6])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y06, (void *)&posy[0][6], sizeof(posy[0][6])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME06, (void *)&istime[0][6], sizeof(istime[0][6])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][6] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT06, (void *)&content[0][6], strlen(content[0][6])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	// osd item 8
	// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream1 osd item 8");

	if (switchon[0][7] == 1)
	{
		if (ControlSystemData(SFIELD_SET_OSDINFO_X07, (void *)&posx[0][7], sizeof(posx[0][7])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_Y07, (void *)&posy[0][7], sizeof(posy[0][7])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME07, (void *)&istime[0][7], sizeof(istime[0][7])) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		if (istime[0][7] == 0)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT07, (void *)&content[0][7], strlen(content[0][7])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
		}

	}

	if (h264enable2)
	{
		// osd item 1
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 1");

		if (switchon[1][0] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X10, (void *)&posx[1][0], sizeof(posx[1][0])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y10, (void *)&posy[1][0], sizeof(posy[1][0])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME10, (void *)&istime[1][0], sizeof(istime[1][0])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (istime[1][0] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT10, (void *)&content[1][0], strlen(content[1][0])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
				// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			}

		}

		// osd item 2
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 2");

		if (switchon[1][1] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X11, (void *)&posx[1][1], sizeof(posx[1][1])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y11, (void *)&posy[1][1], sizeof(posy[1][1])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME11, (void *)&istime[1][1], sizeof(istime[1][1])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (istime[1][1] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT11, (void *)&content[1][1], strlen(content[1][1])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
				// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			}

		}

		// osd item 3
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 3");

		if (switchon[1][2] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X12, (void *)&posx[1][2], sizeof(posx[1][2])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y12, (void *)&posy[1][2], sizeof(posy[1][2])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME12, (void *)&istime[1][2], sizeof(istime[1][2])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (istime[1][2] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT12, (void *)&content[1][2], strlen(content[1][2])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
				// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			}
		}

		// osd item 4
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 4");

		if (switchon[1][3] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X13, (void *)&posx[1][3], sizeof(posx[1][3])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			if (ControlSystemData(SFIELD_SET_OSDINFO_Y13, (void *)&posy[1][3], sizeof(posy[1][3])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME13, (void *)&istime[1][3], sizeof(istime[1][3])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);

			if (istime[1][3] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT13, (void *)&content[1][3], strlen(content[1][3])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
				// printf("alei@%s(L%d): in %s()\n", __FILE__, __LINE__, __func__);
			}
		}

		// osd item 5
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 5");

		if (switchon[1][4] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X14, (void *)&posx[1][4], sizeof(posx[1][4])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y14, (void *)&posy[1][4], sizeof(posy[1][4])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME14, (void *)&istime[1][4], sizeof(istime[1][4])) < 0)
			{
				ret = FAIL;
				goto exit;
			}


			if (istime[1][4] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT14, (void *)&content[1][4], strlen(content[1][4])) < 0)
				{
					ret = FAIL;
					goto exit;
				}

			}
		}

		// osd item 6
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 6");

		if (switchon[1][5] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X15, (void *)&posx[1][5], sizeof(posx[1][5])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y15, (void *)&posy[1][5], sizeof(posy[1][5])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME15, (void *)&istime[1][5], sizeof(istime[1][5])) < 0)
			{
				ret = FAIL;
				goto exit;
			}


			if (istime[1][5] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT15, (void *)&content[1][5], strlen(content[1][5])) < 0)
				{
					ret = FAIL;
					goto exit;
				}

			}
		}

		// osd item 7
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 7");

		if (switchon[1][6] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X16, (void *)&posx[1][6], sizeof(posx[1][6])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_Y16, (void *)&posy[1][6], sizeof(posy[1][6])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME16, (void *)&istime[1][6], sizeof(istime[1][6])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (istime[1][6] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT16, (void *)&content[1][6], strlen(content[1][6])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
			}

		}

		// osd item 8
		// printf("alei@%s(L%d): in %s() %s\n", __FILE__, __LINE__, __func__, "stream2 osd item 8");
		if (switchon[1][7] == 1)
		{
			if (ControlSystemData(SFIELD_SET_OSDINFO_X17, (void *)&posx[1][7], sizeof(posx[1][7])) < 0)
			{
				ret = FAIL;
				goto exit;
			}
			if (ControlSystemData(SFIELD_SET_OSDINFO_Y17, (void *)&posy[1][7], sizeof(posy[1][7])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (ControlSystemData(SFIELD_SET_OSDINFO_ISTIME17, (void *)&istime[1][7], sizeof(istime[1][7])) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			if (istime[1][7] == 0)
			{
				if (ControlSystemData(SFIELD_SET_OSDINFO_CONTENT17, (void *)&content[1][7], strlen(content[1][7])) < 0)
				{
					ret = FAIL;
					goto exit;
				}
			}
		}
	}
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;
	/*	end added 	*/


	// width , height
	int width1, width2, height1, height2;
	// __u8 codemode;
	memcpy(&width1, &h264_configure->h264_channel[0].width,
	       sizeof(h264_configure->h264_channel[0].width));
	memcpy(&width2, &h264_configure->h264_channel[1].width,
	       sizeof(h264_configure->h264_channel[1].width));
	memcpy(&height1, &h264_configure->h264_channel[0].height,
	       sizeof(h264_configure->h264_channel[0].height));
	memcpy(&height2, &h264_configure->h264_channel[1].height,
	       sizeof(h264_configure->h264_channel[1].height));
	// printf("w*h=%d*%d, w*h=%d*%d\n", width1, height1, width2, height2);
	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;
	if (ControlSystemData(SFIELD_SET_RES_WIDTH1, (void *) &width1,
	                      sizeof(width1)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;


	if (ControlSystemData(SFIELD_SET_RES_HEIGHT1, (void *) &height1,
	                      sizeof(height1)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	if (h264enable2)
	{
		if (ControlSystemData(SFIELD_SET_RES_WIDTH2, (void *) &width2,
		                      sizeof(width2)) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		if (ControlSystemData(SFIELD_SET_RES_HEIGHT2, (void *) &height2,
		                      sizeof(height2)) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;
	}

	// unsigned char h264enable1 = h264_configure->h264_channel[0].h264_on;
	// unsigned char h264enable2 = h264_configure->h264_channel[1].h264_on;

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	if (ControlSystemData(SFIELD_SET_H264_ENABLE1, (void *)&h264enable1, sizeof(h264enable1)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	if (ControlSystemData(SFIELD_SET_H264_ENABLE2, (void *)&h264enable2, sizeof(h264enable2)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	time2 = time(NULL);
	printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
	time1 = time2;

	if(h264enable2 == 1)
	{
		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		printf("Stream 2 is enabled\n");
		unsigned char videomode = 1;
		if (ControlSystemData(SFIELD_SET_VIDEO_MODE, (void *) &videomode, sizeof(videomode)) < 0)//dual Stream
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		printf("Stream 2 is enabled, videomode = %d\n", videomode);
		unsigned char codecmode = 0;
		if (ControlSystemData(SFIELD_SET_VIDEOCODECCOMBO, (void *) &codecmode, sizeof(codecmode)) < 0)//H264+H264
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		printf("Stream 2 is enabled, codecmode = %d\n", codecmode);
		unsigned char codecres = 0;
		if (width1 == 1920 && width2 == 720 )
		{
			codecres = 0;
			if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;

			printf("Stream 2 is enabled, codecres = %d\n", codecres);
		}
		else if (width1 == 1920 && width2 == 1920 )
		{
			codecres = 1;
			if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));

		}
		else if (width1 == 2560 && width2 == 720 )
		{
			codecres = 2;
			if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;

		}
		else if (width1 == 3264 && width2 == 720 )
		{
			codecres = 3;
			if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;

		}
		else//User defined resolution
		{
			codecres = 4;
			if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
			{
				ret = FAIL;
				goto exit;
			}

			time2 = time(NULL);
			printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
			time1 = time2;

		}
	}
	else
	{
		printf("Stream 2 is disabled\n");
		unsigned char videomode = 0;
		if (ControlSystemData(SFIELD_SET_VIDEO_MODE, (void *) &videomode, sizeof(videomode)) < 0)//Single Stream
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;


		printf("Stream 2 is disabled, videomode = %d\n", videomode);

		unsigned char codecmode = 0;
		if (ControlSystemData(SFIELD_SET_VIDEOCODECCOMBO, (void *) &codecmode, sizeof(codecmode)) < 0)//H264
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;


		printf("Stream 2 is disabled, codecmode = %d\n", codecmode);

		unsigned char codecres = 0;
		if (width1 == 1920)
		{
			codecres = 0;//2M
		}
		else if (width1 == 2560)
		{
			codecres = 1;//5M
		}
		else
		{
			codecres = 2;//8M
		}
		printf("Stream 2 is disabled, codecres = %d\n", codecres);

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

		if (ControlSystemData(SFIELD_SET_VIDEOCODECRES, (void *) &codecres, sizeof(codecres)) < 0)
		{
			ret = FAIL;
			goto exit;
		}

		time2 = time(NULL);
		printf("alei@%s(L%d):at %s() time=%d time_escape=%d\n", __FILE__, __LINE__, __func__, (int)time2, (int)(time2-time1));
		time1 = time2;

	}

	if (restartflag == 1)
	{
		unsigned char param = 0;
		if (ControlSystemData(SFIELD_SET_RESTART, (void *) &param, sizeof(unsigned char)) < 0)
		{
			ret = FAIL;
			goto exit;
		}
	}
	//deinit_env();
exit:
	stop_timer();
	//fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n\n\nexit from %s().\n\n\n\n\n\n\n\n", __func__);
	return ret;
}

/// get camera config
Camera_config *get_camera_config()
{
	start_timer(3000000);
	// printf("inside: %s\n", __func__);
	static Camera_config camcfg;
	memset(&camcfg, '\0', sizeof(camcfg));
	//init_env();
	char ipaddr[30];
	memset(ipaddr, 0, sizeof(ipaddr));
	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		//deinit_env();
		stop_timer();
		return NULL;
	}

	// printf("exposure flag..........\n");
	memcpy(&camcfg.exp_mode.flag, &pSysInfo->cam_set.nExposureCtrl,
	       sizeof(pSysInfo->cam_set.nExposureCtrl));

	// printf("exposure.........\n");
	memcpy(&camcfg.exp_manu.exp, &pSysInfo->cam_set.nExposure,
	       sizeof(pSysInfo->cam_set.nExposure));

	/*FIXME: @alei:no light_dt_mode */
	// printf("gain........\n");
	memcpy(&camcfg.exp_manu.gain, &pSysInfo->cam_set.nAGC,
	       sizeof(pSysInfo->cam_set.nAGC));

	// printf("exposure max........\n");
	memcpy(&camcfg.exp_auto.exp_max, &pSysInfo->cam_set.nExposureMax,
	       sizeof(pSysInfo->cam_set.nExposureMax));

	// printf("exposure mid........\n");
	memcpy(&camcfg.exp_auto.exp_mid, &pSysInfo->cam_set.nExposureMid,
	       sizeof(pSysInfo->cam_set.nExposureMid));

	// printf("exposure min........\n");
	memcpy(&camcfg.exp_auto.exp_min, &pSysInfo->cam_set.nExposureMin,
	       sizeof(pSysInfo->cam_set.nExposureMin));

	// printf("gain max........\n");
	memcpy(&camcfg.exp_auto.gain_max, &pSysInfo->cam_set.nAGCMax,
	       sizeof(pSysInfo->cam_set.nAGCMax));

	// printf("gain mid........\n");
	memcpy(&camcfg.exp_auto.gain_mid, &pSysInfo->cam_set.nAGCMid,
	       sizeof(pSysInfo->cam_set.nAGCMid));

	// printf("gain min........\n");
	memcpy(&camcfg.exp_auto.gain_min, &pSysInfo->cam_set.nAGCMin,
	       sizeof(pSysInfo->cam_set.nAGCMin));

	/*FIXME: @alei: no s_ExpWnd*/


	// printf("white balance........\n");
	memcpy(&camcfg.awb_mode.flag, &pSysInfo->cam_set.nWhiteBalance,
	       sizeof(pSysInfo->cam_set.nWhiteBalance));

	// printf("red gain........\n");
	memcpy(&camcfg.awb_manu.gain_r, &pSysInfo->cam_set.nRedGain,
	       sizeof(pSysInfo->cam_set.nRedGain));

	// printf("green gain........\n");
	memcpy(&camcfg.awb_manu.gain_g, &pSysInfo->cam_set.nGreenGain,
	       sizeof(pSysInfo->cam_set.nGreenGain));

	// printf("blue gain........\n");
	memcpy(&camcfg.awb_manu.gain_b, &pSysInfo->cam_set.nBlueGain,
	       sizeof(pSysInfo->cam_set.nBlueGain));

	// printf("contrast........\n");
	memcpy(&camcfg.color_param.contrast, &pSysInfo->cam_set.nContrast,
	       sizeof(pSysInfo->cam_set.nContrast));

	// printf("luminance........\n");
	memcpy(&camcfg.color_param.luma, &pSysInfo->cam_set.nLuminance,
	       sizeof(pSysInfo->cam_set.nLuminance));

	// printf("saturation........\n");
	memcpy(&camcfg.color_param.saturation, &pSysInfo->cam_set.nSaturation,
	       sizeof(pSysInfo->cam_set.nSaturation));

	/*	following add by zdy, 2013-10-28 */
	// syn
	memcpy(&camcfg.syn_info.is_syn_open, &pSysInfo->cam_set.PhaseEnable,
	       sizeof(pSysInfo->cam_set.PhaseEnable));

	memcpy(&camcfg.syn_info.phase, &pSysInfo->cam_set.PhaseValue,
	       sizeof(pSysInfo->cam_set.PhaseValue));

	// camcfg.syn_info.is_syn_open = pSysInfo->cam_set.PhaseEnable;
//    camcfg.syn_info.phase = pSysInfo->cam_set.PhaseValue;
	/*	end, zdy, 2013-10-28 */
	//deinit_env();

	stop_timer();
	return &camcfg;
}
/// set camera config
int set_camera_config(Camera_config *camcfg)
{
	int ret = SUCCESS;
	start_timer(3000000);
	//init_env();
	// printf("inside: %s\n", __func__);
	// exposure flag, manual/auto
	char exposureflag = camcfg->exp_mode.flag;
	// printf("exposure flag..........\n");
	if (ControlSystemData(SFIELD_SET_EXPOSURECTRL, (void *) &exposureflag,
	                      sizeof(exposureflag)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	// manual exposure + manual gain
	if (exposureflag == 0)
	{
		/*@alei type error*/
		// manual exposure
		int exposure = camcfg->exp_manu.exp;
		// printf("exposure.........\n");
		// printf("alei@exposure=%d\n", exposure);
		if (ControlSystemData(SFIELD_SET_EXPOSURE, (void *) &exposure,
		                      sizeof(exposure)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// manual gain
		int gain = camcfg->exp_manu.gain;
		// printf("gain........\n");
		if (ControlSystemData(SFIELD_SET_AGC, (void *) &gain, sizeof(gain)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
	}
	// auto exposure + auto gain
	else if (exposureflag == 1)
	{
		// exposure max
		int exposuremax = camcfg->exp_auto.exp_max;
		// printf("exposure max........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MAX, (void *) &exposuremax,
		                      sizeof(exposuremax)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// exposure mid
		int exposuremid = camcfg->exp_auto.exp_mid;
		// printf("exposure mid........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MID, (void *) &exposuremid,
		                      sizeof(exposuremid)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// exposure min
		int exposuremin = camcfg->exp_auto.exp_min;
		// printf("exposure min........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MIN, (void *) &exposuremin,
		                      sizeof(exposuremin)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain max
		int gainmax = camcfg->exp_auto.gain_max;
		// printf("gain max........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MAX, (void *) &gainmax,
		                      sizeof(gainmax)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain mid
		int gainmid = camcfg->exp_auto.gain_mid;
		// printf("gain mid........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MID, (void *) &gainmid,
		                      sizeof(gainmid)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain min
		int gainmin = camcfg->exp_auto.gain_min;
		// printf("gain min........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MIN, (void *) &gainmin,
		                      sizeof(gainmin)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
	}
	// auto exposure + manual gain
	else if (exposureflag == 2)
	{
		// manual gain
		int gain = camcfg->exp_manu.gain;
		// printf("gain........\n");
		if (ControlSystemData(SFIELD_SET_AGC, (void *) &gain, sizeof(gain)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// exposure max
		int exposuremax = camcfg->exp_auto.exp_max;
		// printf("exposure max........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MAX, (void *) &exposuremax,
		                      sizeof(exposuremax)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// exposure mid
		int exposuremid = camcfg->exp_auto.exp_mid;
		// printf("exposure mid........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MID, (void *) &exposuremid,
		                      sizeof(exposuremid)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// exposure min
		int exposuremin = camcfg->exp_auto.exp_min;
		// printf("exposure min........\n");
		if (ControlSystemData(SFIELD_SET_EXPOSURE_MIN, (void *) &exposuremin,
		                      sizeof(exposuremin)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
	}
	// manual exposure + auto gain
	else if (exposureflag == 3)
	{
		int exposure = camcfg->exp_manu.exp;
		// printf("exposure.........\n");
		// printf("alei@exposure=%d\n", exposure);
		if (ControlSystemData(SFIELD_SET_EXPOSURE, (void *) &exposure,
		                      sizeof(exposure)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain max
		int gainmax = camcfg->exp_auto.gain_max;
		// printf("gain max........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MAX, (void *) &gainmax,
		                      sizeof(gainmax)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain mid
		int gainmid = camcfg->exp_auto.gain_mid;
		// printf("gain mid........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MID, (void *) &gainmid,
		                      sizeof(gainmid)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// gain min
		int gainmin = camcfg->exp_auto.gain_min;
		// printf("gain min........\n");
		if (ControlSystemData(SFIELD_SET_AGC_MIN, (void *) &gainmin,
		                      sizeof(gainmin)) < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
	}


	// white balance flag, manual/auto
	char wbflag = (camcfg->awb_mode.flag == 0 ? 1 : 0);
	// printf("white balance........\n");
	if (ControlSystemData(SFIELD_SET_WHITE_BALANCE, (void *) &wbflag,
	                      sizeof(wbflag)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	if (wbflag == 1)
	{
		// red gain
		__u16 rgain = camcfg->awb_manu.gain_r;
		// printf("red gain........\n");
		if (ControlSystemData(SFIELD_SET_RED_GAIN, (void *) &rgain, sizeof(rgain))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// green gain
		__u16 ggain = camcfg->awb_manu.gain_g;
		// printf("green gain........\n");
		if (ControlSystemData(SFIELD_SET_GREEN_GAIN, (void *) &ggain, sizeof(ggain))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}

		// blue gain
		__u16 bgain = camcfg->awb_manu.gain_b;
		// printf("blue gain........\n");
		if (ControlSystemData(SFIELD_SET_BLUE_GAIN, (void *) &bgain, sizeof(bgain))
		        < 0)
		{
			//deinit_env();
			ret = FAIL;
			goto exit;
		}
	}

	// contrast
	__u16 contrast = camcfg->color_param.contrast;
	// printf("contrast........\n");
	if (ControlSystemData(SFIELD_SET_CONTRAST_HI, (void *) &contrast,
	                      sizeof(contrast)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	// luminance
	__u16 luminance = camcfg->color_param.luma;
	// printf("luminance........\n");
	if (ControlSystemData(SFIELD_SET_LUMINANCE, (void *) &luminance,
	                      sizeof(luminance)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	// saturation
	__u16 saturation = camcfg->color_param.saturation;
	// printf("saturation........\n");
	if (ControlSystemData(SFIELD_SET_SATURATION, (void *) &saturation,
	                      sizeof(saturation)) < 0)
	{
		//deinit_env();
		ret = FAIL;
		goto exit;
	}

	/*	following add by zdy, 2013-10-28 */
	// syn
	char is_syn_open = camcfg->syn_info.is_syn_open;

	fprintf(stderr, "zdy@func(%s) is_syn_open=%d\n\n\n", __func__, is_syn_open);

	if (is_syn_open == 1)
	{
		int phase = camcfg->syn_info.phase;
		fprintf(stderr, "zdy@func(%s) phase=%d\n\n\n", __func__, phase);

		if (phase < 0)
			phase = 0;
		if (phase > 19)
			phase = 19;

		if (ControlSystemData(SFIELD_SET_PHASE_VALUE, (void *)&phase, sizeof(phase)) < 0)
		{
			printf("alei@%s(L%d): at %s() call SFIELD_SET_PHASE_VALUE error!\n", __FILE__, __LINE__, __func__);
			ret = FAIL;
			goto exit;
		}
	}

	if (ControlSystemData(SFIELD_SET_PHASE_ENABLE, (void *)&is_syn_open, sizeof(is_syn_open)) < 0)
	{
		printf("alei@%s(L%d): at %s() call SFIELD_SET_PHASE_ENABLE error!\n", __FILE__, __LINE__, __func__);
		ret = FAIL;
		goto exit;
	}
	/*	end, zdy, 2013-10-28 */

exit:
	stop_timer();
	//deinit_env();
	return ret;
}


NTP_CONFIG_PARAM *get_ntp_config()
{
	start_timer(3000000);
	// printf("zdy@%s(L%d): at %s() ntp.\n", __FILE__, __LINE__, __func__);
	static NTP_CONFIG_PARAM ntp_para;

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		stop_timer();
		return NULL;
	}

	unsigned char flag = pSysInfo->osd_config[2].dstampenable;
	if (flag == 2)
		ntp_para.useNTP = 1;
	else
		ntp_para.useNTP = 0;

	unsigned short distance = pSysInfo->ntp_config.NTPDistance;
	// printf("zdy@%s(L%d): at %s() ntp.distance=%d\n", __FILE__, __LINE__, __func__,distance);

	// memcpy(&ntp_para.NTP_distance, &pSysInfo->ntp_config.NTPDistance, sizeof(pSysInfo->ntp_config.NTPDistance));
	ntp_para.NTP_distance = distance;

	char ipAddr[30] = "";
	strncpy(ipAddr, pSysInfo->lan_config.net.ntp_server, sizeof(ipAddr));
	//printf("H264_channel[%d].IP=%s.\n", ch, ipAddr);

	int ip[4];
	sscanf(ipAddr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
	ntp_para.NTP_server_ip[0] = (BYTE)ip[0];
	ntp_para.NTP_server_ip[1] = (BYTE)ip[1];
	ntp_para.NTP_server_ip[2] = (BYTE)ip[2];
	ntp_para.NTP_server_ip[3] = (BYTE)ip[3];

	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.useNTP=%d\n", __FILE__, __LINE__, __func__,ntp_para.useNTP);
	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.NTP_distance=%d\n", __FILE__, __LINE__, __func__,ntp_para.NTP_distance);
	// printf("zdy@%s(L%d): at %s() ntp.pSysInfo->ntp_config.NTPDistance=%d\n", __FILE__, __LINE__, __func__,pSysInfo->ntp_config.NTPDistance);
	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.ntp_server=%d.%d.%d.%d\n", __FILE__, __LINE__, __func__,ntp_para.NTP_server_ip[0],ntp_para.NTP_server_ip[1],ntp_para.NTP_server_ip[2],ntp_para.NTP_server_ip[3]);

	stop_timer();
	return &ntp_para;
}

int set_ntp_config(NTP_CONFIG_PARAM *ntp_config)
{
	int ret = SUCCESS;
	start_timer(3000000);
	unsigned char ntpenable = ntp_config->useNTP;

	unsigned char flag = 0;
	if (ntpenable == 1)
		flag = 2;

	if (ControlSystemData(SFIELD_SET_NTP_ENABLE, (void *)&ntpenable, sizeof(ntpenable)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	if(ControlSystemData(SFIELD_SET_DSTAMPENABLE3, &flag, sizeof(flag)) < 0)
	{
		ret = FAIL;
		goto exit;
	}

	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.useNTP=%d\n", __FILE__, __LINE__, __func__,ntp_config->useNTP);

	unsigned int distance = ntp_config->NTP_distance;

	// printf("zdy@%s(L%d): at %s() ntp.distance=%d\n", __FILE__, __LINE__, __func__,distance);
	if (ControlSystemData(SFIELD_SET_NTP_INTERVAL, (void *)&distance, sizeof(distance)) < 0)
	{
		ret = FAIL;
		goto exit;
	}
	printf("zdy@%s(L%d): at %s() ntp.ntp_para.NTP_distance=%d\n", __FILE__, __LINE__, __func__,ntp_config->NTP_distance);

	char ip[20];
	memset(ip, 0, sizeof(ip));
	sprintf(ip, "%d.%d.%d.%d", ntp_config->NTP_server_ip[0],
	        ntp_config->NTP_server_ip[1],
	        ntp_config->NTP_server_ip[2],
	        ntp_config->NTP_server_ip[3]);
	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.ntp_server=%d.%d.%d.%d\n", __FILE__, __LINE__, __func__,ntp_config->NTP_server_ip[0],ntp_config->NTP_server_ip[1],ntp_config->NTP_server_ip[2],ntp_config->NTP_server_ip[3]);

//	unsigned char v2 = 0;

	if (ControlSystemData(SFIELD_SET_SNTP_SERVER, (void *)ip, strlen(ip)) < 0)
	{
		ret = FAIL;
		goto exit;
	}
	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.ntp_server=%d.%d.%d.%d\n", __FILE__, __LINE__, __func__,ntp_config->NTP_server_ip[0],ntp_config->NTP_server_ip[1],ntp_config->NTP_server_ip[2],ntp_config->NTP_server_ip[3]);

	if (ControlSystemData(SFIELD_START_NTPCLIENT, (void *)&distance, sizeof(distance)) < 0)
	{
		ret = FAIL;
		goto exit;
	}
	// printf("zdy@%s(L%d): at %s() ntp.ntp_para.ntp_server=%d.%d.%d.%d\n", __FILE__, __LINE__, __func__,ntp_config->NTP_server_ip[0],ntp_config->NTP_server_ip[1],ntp_config->NTP_server_ip[2],ntp_config->NTP_server_ip[3]);
exit:
	stop_timer();
	return ret;
}


int power_down()
{
	int ret = SUCCESS;
	start_timer(3000000);
	unsigned char value = 0; // modified by zdy, 2013-08-14
	if (ControlSystemData(SFIELD_SET_POWERDOWN, (void *)&value, sizeof(value)) < 0)
	{
		ret = FAIL;
		goto exit;
	}
exit:
	stop_timer();
	return ret;
}

int get_prepos_station(void)
{
	DOWNLOAD_MSG msgbuf;

	static int flag_first=1;
	static int qPTZId;
	int msg_len= 0;
	if(flag_first==1)
	{
		flag_first=0;
		qPTZId = Msg_Init(PAR_MSG_KEY);	
	}
		
	msgbuf.Des = MSG_TYPE_MSG18;
	msgbuf.Src = MSG_TYPE_MSG20;
	msgbuf.Cmd = GET_PREPOS_STATION;
	printf("VD request GET_PREPOS_STATION\n");
//    Msg_Send( qPTZId, &msgbuf, sizeof(DOWNLOAD_MSG));
	
	msgsnd(qPTZId,&msgbuf,sizeof(DOWNLOAD_MSG)-sizeof(long),0);
	msgbuf.Cmd = 0xff;
//	msg_len = Msg_Rsv(qPTZId,MSG_TYPE_MSG3, &msgbuf,sizeof(DOWNLOAD_MSG));
	msg_len = msgrcv( qPTZId, &msgbuf, sizeof(msgbuf)-sizeof(long),MSG_TYPE_MSG20, 0);
	printf("VD recive GET_PREPOS_STATION1 %d %d %d\n",msg_len,msgbuf.Cmd,msgbuf.Msg_buf.prepos_station);
	return msgbuf.Msg_buf.prepos_station;
}

Version_Detail *get_version_detail()
{
	start_timer(3000000);
	// printf("inside : %s\n", __func__);
	static Version_Detail ver_detail;

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		stop_timer();
		return NULL;
	}

	strcpy(ver_detail.HdVer, pSysInfo->camera_detail_info.HardwareVer);
	printf("zdy@%s(L@%d): in %s(); HardwareVer = %s\n", __FILE__, __LINE__, __func__, ver_detail.HdVer);

	strcpy(ver_detail.FPGAVer, pSysInfo->camera_detail_info.FPGAVer);
	// printf("zdy@%s(L@%d): in %s(); FPGAVer = %s\n", __FILE__, __LINE__, __func__, ver_detail.FPGAVer);

	strcpy(ver_detail.McuVersion, pSysInfo->camera_detail_info.McuVersion);
	// printf("zdy@%s(L@%d): in %s(); McuVersion = %s\n", __FILE__, __LINE__, __func__, ver_detail.McuVersion);


	strcpy(ver_detail.SysServerVer, pSysInfo->camera_detail_info.SysServerVer);
	// printf("zdy@%s(L@%d): in %s(); SysServerVer = %s\n", __FILE__, __LINE__, __func__, ver_detail.SysServerVer);

	strcpy(ver_detail.serialnum, pSysInfo->camera_detail_info.SerialNum);
	printf("zdy@%s(L@%d): in %s(); SerialNum = %s\n", __FILE__, __LINE__, __func__, ver_detail.serialnum);

	strcpy(ver_detail.devicetype, pSysInfo->camera_detail_info.DeviceType);
	printf("zdy@%s(L@%d): in %s(); DeviceType = %s\n", __FILE__, __LINE__, __func__, ver_detail.devicetype);

	stop_timer();

	return &ver_detail;
}

//Fan_Ctrl *get_fan_ctrl()
//{
//	start_timer(3000000);
//	// printf("inside : %s\n", __func__);
//	static Fan_Ctrl fan_ctrl;
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("Error when read share memory.\n");
//		stop_timer();
//		return NULL;
//	}
//
//	fan_ctrl.UpperTemp = pSysInfo->fan_temp.UpperTemp;
//	fan_ctrl.LowerTemp = pSysInfo->fan_temp.LowerTemp;
//
//	stop_timer();
//	return &fan_ctrl;
//}
//
//int set_fan_ctrl(Fan_Ctrl *fan_ctrl)
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	// printf("inside: %s\n", __func__);
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("Error when read share memory.\n");
//		ret = FAIL;
//		goto exit;
//	}
//
//	// upper temp
//	// printf("upper temp...........\n");
//	// printf("zdy@%s(L%d): in %s(); uppertemp = %d\n", __FILE__, __LINE__, __func__, fan_ctrl->UpperTemp);
//	int uppertemp = fan_ctrl->UpperTemp;
//	if (ControlSystemData(SFIELD_SET_FAN_UPPERTEMP, (void *) &uppertemp, sizeof(uppertemp))
//	        < 0)
//	{
//		ret = FAIL;
//		goto exit;
//	}
//
//	// lower temp
//	// printf("lower temp...........\n");
//	// printf("zdy@%s(L%d): in %s(); lowertemp = %d\n", __FILE__, __LINE__, __func__, fan_ctrl->LowerTemp);
//	int lowertemp = fan_ctrl->LowerTemp;
//	if (ControlSystemData(SFIELD_SET_FAN_LOWERTEMP, (void *) &lowertemp, sizeof(lowertemp))
//	        < 0)
//	{
//		ret = FAIL;
//		goto exit;
//	}
//exit:
//	stop_timer();
//	return ret;
//}
//
//int get_fan_status()
//{
//	start_timer(3000000);
//	int ret = SUCCESS;
//
//	unsigned char value = 0;
//
//// fprintf(stderr, "zdy@file(%s) L(%d) func(%s)\n", __FILE__, __LINE__, __func__);
//	// get fan status
//	ControlSystemData(SFIELD_GET_FAN_STATUS, (void *)&value, sizeof(value));
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("Error when read share memory.\n");
//		ret = FAIL;
//		goto exit;
//	}
//
//	ret = pSysInfo->fan_temp.FanStatus;
//	// printf("status = %d\n", status);
//// fprintf(stderr, "zdy@file(%s) L(%d) func(%s) status=%d\n", __FILE__, __LINE__, __func__, status);
//exit:
//	stop_timer();
//	return ret;
//}
//#if 1
//Camera_info *get_lens_ctrl()
//{
//	start_timer(3000000);
//	static Camera_info cam_info;
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("Error when read share memory.\n");
//		stop_timer();
//		return NULL;
//	}
//
//	cam_info.iris_size = pSysInfo->cam_set.nIris;
//	cam_info.iris_type = pSysInfo->lens_ctrl.iris_type;
//
//	stop_timer();
//	return &cam_info;
//}
//
//int set_lens_ctrl(Camera_info *camera_info)
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	unsigned char focustype = camera_info->focus_type;
//
//	unsigned char start = 1;
//	unsigned char stop = 0;
//
//	unsigned char irisval = 0;
//	int iris_type = camera_info->iris_type;
//	printf("zdy@%s(L@%d): in %s(); iris_type = %d\n", __FILE__, __LINE__, __func__, iris_type);
//
//	// int iris_type = 1;
//	if (iris_type != 0)
//	{
//		if (ControlSystemData(SFIELD_SET_IRIS_TYPE, (void *)&iris_type, sizeof(iris_type)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//		// printf("zdy@%s(L@%d): in %s(); focustype = %d\n", __FILE__, __LINE__, __func__, camera_info->focus_type);
//		// printf("zdy@%s(L@%d): in %s(); focustype = %d\n", __FILE__, __LINE__, __func__, focustype);
//
//		unsigned char irissize = camera_info->iris_size;
//		// printf("zdy@%s(L@%d): in %s(); irissize = %d\n", __FILE__, __LINE__, __func__, camera_info->iris_size);
//
//		if (iris_type == 1)
//		{
//			switch (irissize)
//			{
//			case FULL_OPEN:
//				break;
//			case F1_4:
//				break;
//			case F1_6:
//				break;
//			case F1_8:
//				break;
//			case F2_0:
//				irisval = 1;
//				break;
//			case F2_2:
//				irisval = 2;
//				break;
//			case F2_5:
//				irisval = 3;
//				break;
//			case F2_8:
//				irisval = 4;
//				break;
//			case F3_2:
//				irisval = 5;
//				break;
//			case F3_5:
//				irisval = 6;
//				break;
//			case F4_0:
//				irisval = 7;
//				break;
//			case F4_5:
//				irisval = 8;
//				break;
//			case F5_0:
//				irisval = 9;
//				break;
//			case F5_6:
//				irisval = 10;
//				break;
//			case F6_3:
//				irisval = 11;
//				break;
//			case F7_1:
//				irisval = 12;
//				break;
//			case F8:
//				irisval = 13;
//				break;
//			case F9:
//				irisval = 14;
//				break;
//			case F10:
//				irisval = 15;
//				break;
//			case F11:
//				irisval = 16;
//				break;
//			case F13:
//				irisval = 17;
//				break;
//			case F14:
//				irisval = 18;
//				break;
//			case F16:
//				irisval = 19;
//				break;
//			case F18:
//				irisval = 20;
//				break;
//			case F20:
//				irisval = 21;
//				break;
//			case F22:
//				irisval = 22;
//				break;
//			default:
//				break;
//			}
//		}
//		else if (iris_type == 2)
//		{
//			switch (irissize)
//			{
//			case FULL_OPEN:
//				break;
//			case F1_4:
//				break;
//			case F1_6:
//				break;
//			case F1_8:
//				break;
//			case F2_0:
//				irisval = 1;
//				break;
//			case F2_2:
//				irisval = 2;
//				break;
//			case F2_5:
//				irisval = 3;
//				break;
//			case F2_8:
//				irisval = 4;
//				break;
//			case F3_2:
//				irisval = 5;
//				break;
//			case F3_5:
//				irisval = 6;
//				break;
//			case F4_0:
//				irisval = 7;
//				break;
//			case F4_5:
//				irisval = 8;
//				break;
//			case F5_0:
//				irisval = 9;
//				break;
//			case F5_6:
//				irisval = 10;
//				break;
//			case F6_3:
//				irisval = 11;
//				break;
//			case F7_1:
//				irisval = 12;
//				break;
//			case F8:
//				irisval = 13;
//				break;
//			case F9:
//				irisval = 14;
//				break;
//			case F10:
//				irisval = 15;
//				break;
//			case F11:
//				irisval = 16;
//				break;
//			case F13:
//				irisval = 17;
//				break;
//			case F14:
//				irisval = 18;
//				break;
//			case F16:
//				irisval = 19;
//				break;
//			case F18:
//				irisval = 20;
//				break;
//			case F20:
//				irisval = 21;
//				break;
//			case F22:
//				irisval = 22;
//				break;
//			default:
//				break;
//			}
//		}
//		else if (iris_type == 3)
//		{
//			switch (irissize)
//			{
//			case FULL_OPEN:
//				break;
//			case F1_4:
//				break;
//			case F1_6:
//				break;
//			case F1_8:
//				break;
//			case F2_0:
//				irisval = 1;
//				break;
//			case F2_2:
//				irisval = 2;
//				break;
//			case F2_5:
//				irisval = 3;
//				break;
//			case F2_8:
//				irisval = 4;
//				break;
//			case F3_2:
//				irisval = 5;
//				break;
//			case F3_5:
//				irisval = 6;
//				break;
//			case F4_0:
//				irisval = 7;
//				break;
//			case F4_5:
//				irisval = 8;
//				break;
//			case F5_0:
//				irisval = 9;
//				break;
//			case F5_6:
//				irisval = 10;
//				break;
//			case F6_3:
//				irisval = 11;
//				break;
//			case F7_1:
//				irisval = 12;
//				break;
//			case F8:
//				irisval = 13;
//				break;
//			case F9:
//				irisval = 14;
//				break;
//			case F10:
//				irisval = 15;
//				break;
//			case F11:
//				irisval = 16;
//				break;
//			case F13:
//				irisval = 17;
//				break;
//			case F14:
//				irisval = 18;
//				break;
//			case F16:
//			case F18:
//			case F20:
//			case F22:
//				irisval = 19;
//				break;
//			default:
//				break;
//			}
//		}
//
//		printf("zdy@%s(L@%d): in %s(); irisval = %d\n", __FILE__, __LINE__, __func__, irisval);
//
//		if (ControlSystemData(SFIELD_SET_IRIS, (void *)&irisval, sizeof(irisval)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//	}
//	else
//	{
//		printf("zdy@%s(L@%d): in %s(); focus = %d\n", __FILE__, __LINE__, __func__, focustype);
//		switch (focustype)
//		{
//#if 0	/*If system_server does this, response will be very slow.*/
//		case 0:
//		{
//			// printf("zdy@%s(L@%d): in %s(); stop = %d\n", __FILE__, __LINE__, __func__, stop);
//			if (ControlSystemData(SFIELD_SET_FOCUS_NEAR, (void *)&stop, sizeof(stop)) < 0)
//			{
//				ret = FAIL;
//				goto exit;
//			}
//			break;
//		}
//		case 1:
//		{
//			// printf("zdy@%s(L@%d): in %s(); near = %d\n", __FILE__, __LINE__, __func__, start);
//			if (ControlSystemData(SFIELD_SET_FOCUS_NEAR, (void *)&start, sizeof(start)) < 0)
//			{
//				ret = FAIL;
//				goto exit;
//			}
//			break;
//		}
//		case 2:
//		{
//			// printf("zdy@%s(L@%d): in %s(); far = %d\n", __FILE__, __LINE__, __func__, start);
//			if (ControlSystemData(SFIELD_SET_FOCUS_FAR, (void *)&start, sizeof(start)) < 0)
//			{
//				ret = FAIL;
//				goto exit;
//			}
//			break;
//		}
//#else
//		case 0:
//		{
//			stopFocus();
//			break;
//		}
//		case 1:
//		{
//			focusNear(1);
//			break;
//		}
//		case 2:
//		{
//			focusFar(1);
//			break;
//		}
//#endif
//		default:
//			break;
//		}
//	}
//exit:
//	stop_timer();
//
//	return ret;
//}
//
//#endif
//
//int get_serial_config(Serial_config_t conf[])
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("Error when read share memory.\n");
//		ret = FAIL;
//		goto exit;
//	}
//
//	int index = 0;
//	for ( ; index < 3; index++)
//	{
//		conf[index].dev_type = pSysInfo->seri_config[index].Devtype;
//		conf[index].baudrate = pSysInfo->seri_config[index].Baudrate;
//		conf[index].parity = pSysInfo->seri_config[index].Parity;
//		conf[index].databit = pSysInfo->seri_config[index].Databit;
//		conf[index].stopbit = pSysInfo->seri_config[index].Stopbit;
//	}
//exit:
//	stop_timer();
//	return ret;
//}
//
//
//int set_serial_config(Serial_config_t conf[])
//{
//	int ret = SUCCESS;
//	int index = 0;
//	int dev_type = 0;
//	int baudrate = 0;
//	char parity = 0, databit = 0, stopbit = 0;
//	int hasSignalDetect = 0;
//
//	start_timer(30000000);
//
//	for ( ; index < 3; index++)
//	{
//		dev_type = conf[index].dev_type;
//		// printf("alei@%s(L%d): in %s() set dev_type=%d.\n", __FILE__, __LINE__, __func__, dev_type);
//		if (ControlSystemData(SFIELD_SET_DEVTYPE1 + index, (void *) &dev_type, sizeof(dev_type)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		if (conf[index].dev_type == DEV_REDLIGHT_DETECT)
//		{
//			hasSignalDetect = 1;
//			if(runflag == 0)
//			{
//				//printf("Start thread!!\n");
//				pthread_t signalThread;
//				unsigned char data;
//
//				if (ControlSystemData(SFIELD_START_SIGNALDETECT, (void*)&data, sizeof(unsigned char)) < 0)
//				{
//					printf("[libinterface]: enable signal detect function error!!\n");
//					ret = -1;
//					goto exit;
//				}
//
//				runflag = 1;
//				if (pthread_create(&signalThread, NULL, signalDetectThread, NULL) < 0)
//				{
//					printf("[libinterface]: start signal detect thread error!!\n");
//					ret = -1;
//					goto exit;
//				}
//			}
//		}
//
//		baudrate = conf[index].baudrate;
//		// printf("alei@%s(L%d): in %s() set baudrate=%d.\n", __FILE__, __LINE__, __func__, baudrate);
//		if (ControlSystemData(SFIELD_SET_BAUDRATE1 + index, (void *) &baudrate, sizeof(baudrate)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		parity = conf[index].parity;
//		// printf("alei@%s(L%d): in %s() set parity=%d.\n", __FILE__, __LINE__, __func__, parity);
//		if (ControlSystemData(SFIELD_SET_PARITY1 + index, (void *) &parity, sizeof(parity)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		databit = conf[index].databit;
//		// printf("alei@%s(L%d): in %s() set databit=%d.\n", __FILE__, __LINE__, __func__, databit);
//		if (ControlSystemData(SFIELD_SET_DATABIT1 + index, (void *) &databit, sizeof(databit)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		stopbit = conf[index].stopbit;
//		// printf("alei@%s(L%d): in %s() set stopbit=%d.\n", __FILE__, __LINE__, __func__, stopbit);
//		if (ControlSystemData(SFIELD_SET_STOPBIT1 + index, (void *) &stopbit, sizeof(stopbit)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//	}
//
//	if (!hasSignalDetect)
//		runflag = 0;
//exit:
//	stop_timer();
//	return ret;
//}
//
//
//int get_in_config(IO_config_t conf[])
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	int index = 0;
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("[libinterface]: Error when read share memory.\n");
//		ret = FAIL;
//		goto exit;
//	}
//
//	for ( ; index < 4; index++)
//	{
//		conf[index].mode = pSysInfo->i_config[index].mode;
//		conf[index].trigger_type = pSysInfo->i_config[index].trigger_type;
//		conf[index].direction = pSysInfo->i_config[index].direction;
//	}
//exit:
//	stop_timer();
//	return ret;
//}
//
//int set_in_config(IO_config_t conf[])
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	int index = 0;
//	char mode = 0,
//	     trigger_type = 0,
//	     direction = 0;
//
//	for ( ; index < 4; index++)
//	{
//		// printf("alei@%s(L%d): in %s() set in mode for %d.\n", __FILE__, __LINE__, __func__, index + 1);
//		mode = conf[index].mode;
//		if (ControlSystemData(SFIELD_SET_INMODE1 + index, (void *) &mode, sizeof(mode)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		// printf("alei@%s(L%d): in %s() set in trigger type for %d.\n", __FILE__, __LINE__, __func__, index + 1);
//		trigger_type = conf[index].trigger_type;
//		if (ControlSystemData(SFIELD_SET_INTRIGGERTYPE1 + index, (void *) &trigger_type, sizeof(trigger_type)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		if (1 == mode)
//		{
//			// printf("alei@%s(L%d): in %s() set in direction for %d.\n", __FILE__, __LINE__, __func__, index + 1);
//			direction = conf[index].direction;
//			if (ControlSystemData(SFIELD_SET_INDIRECTION1 + index, (void *) &direction, sizeof(direction)) < 0)
//			{
//				ret = FAIL;
//				goto exit;
//			}
//		}
//	}
//exit:
//	stop_timer();
//	return ret;
//}
//
//
//int get_out_config(IO_config_t conf[])
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	int index = 0;
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//	{
//		printf("[libinterface]: Error when read share memory.\n");
//		ret = FAIL;
//		goto exit;
//	}
//
//	for ( ; index < 8; index++)
//	{
//		conf[index].mode = pSysInfo->o_config[index].mode;
//		conf[index].trigger_type = pSysInfo->o_config[index].trigger_type;
//		conf[index].direction = pSysInfo->o_config[index].direction;
//	}
//exit:
//	stop_timer();
//	return ret;
//}
//
//int set_out_config(IO_config_t conf[])
//{
//	int ret = 0;
//	start_timer(3000000);
//	int index = 0;
//	unsigned char mode = 0,
//	              trigger_type = 0,
//	              direction = 0;
//
//	for ( ; index < 8; index++)
//	{
//		mode = conf[index].mode;
//		// printf("alei@%s(L%d): in %s() set out mode%d=%d.\n", __FILE__, __LINE__, __func__, index + 1, mode);
//		if (ControlSystemData(SFIELD_SET_OUTMODE1 + index, (void *) &mode, sizeof(mode)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		trigger_type = conf[index].trigger_type;
//		// printf("alei@%s(L%d): in %s() set out trigger_type%d=%d.\n", __FILE__, __LINE__, __func__, index + 1, trigger_type);
//		if (ControlSystemData(SFIELD_SET_OUTTRIGGERTYPE1 + index, (void *) &trigger_type, sizeof(trigger_type)) < 0)
//		{
//			ret = FAIL;
//			goto exit;
//		}
//
//		if (1 == mode)
//		{
//			direction = conf[index].direction;
//			// printf("alei@%s(L%d): in %s() set out direction%d=%d.\n", __FILE__, __LINE__, __func__, index + 1, direction);
//			if (ControlSystemData(SFIELD_SET_OUTDIRECTION1 + index, (void *) &direction, sizeof(direction)) < 0)
//			{
//				ret = FAIL;
//				goto exit;
//			}
//		}
//	}
//exit:
//	stop_timer();
//	return ret;
//}

int Init_Com_To_Sys()
{
	if (SysDrvInit(SYS_VD_MSG) < 0)
	{
		printf("SysDrvInit Fail\n");
		return -1;
	}

	debug("SysDrvInit done.\n");

	pthread_cond_init(&timerCond, NULL);
	pthread_mutex_init(&timerMtx, NULL);

	if(tTimer == -1)
	{
		if (pthread_create(&tTimer, NULL, timerLoop, NULL) < 0)
		{
			printf("[libinterface]: L%d at %s() start timer error!", __LINE__, __func__);
			return FAIL;
		}
		//sleep(1);
	}

	if (tty_sem == NULL)
	{
		//printf("initialize sem\n");
		I2CSemInit();
		tty_sem = sem_open(SEM_NAME_TTY, OPEN_FLAG, OPEN_MODE, INIT_V);
	}

	return 0;
}


void Deinit_Com_To_Sys()
{
	SysDrvExit();

	/* add by dsl, 2013-10-12 */
	timerOn = 0;
	pthread_cond_signal(&timerCond);
	pthread_join(tTimer, NULL);

	tTimer = -1;

	pthread_cond_destroy(&timerCond);
	pthread_mutex_destroy(&timerMtx);
	/* end added, dsl, 2013-10-12 */

	if (md44_fd != -1)
	{
		close(md44_fd);
		md44_fd = -1;
	}

	if (NULL != tty_sem)
	{
		I2CSemClose(tty_sem);
		//DestroyI2CSem(tty_sem);
		tty_sem = NULL;
	}
}

/* following add by dsl, 2013-8-21 */
int flash_control(Flash_control_t * cmd)
{
	int ret = SUCCESS;
	start_timer(3000000);
	if (ControlSystemData(SFIELD_FLASH_CONTROL, (void *)cmd, sizeof(Flash_control_t)) < 0)
	{
		ret = FAIL;
	}

	stop_timer();

	return ret;


}

/*end added, dsl, 2013-8-21 */
/*	Following add by zdy, 2013-08-23	*/
//int mcu_upgrade(const char *path)
//{
//	int ret = SUCCESS;
//	// printf("zdy@file(%s) line(%d) func(%s) path = %s\n", __FILE__, __LINE__, __func__, path);
//	if (ControlSystemData(SFIELD_MCU_UPGRADE, (void *)path, strlen(path)) < 0)
//	{
//		ret = FAIL;
//	}
//	return ret;
//}
//int mcu_fpga_upgrade(const char *path)
//{
//	int ret = SUCCESS;
//	// printf("zdy@file(%s) line(%d) func(%s) path = %s\n", __FILE__, __LINE__, __func__, path);
//	if (ControlSystemData(SFIELD_MCU_FPGA_UPGRADE, (void *)path, strlen(path)) < 0)
//	{
//		ret = FAIL;
//	}
//	return ret;
//}
/*	end added, zdy, 2013-08-23	*/


///< SIP, 2013-08-22
int functions()
{
	InitFileMsgDrv(FILE_MSG_KEY, MSG_TYPE_MSG7);
	GetSysInfo();
	CleanupFileMsgDrv();
	return 0;
}

void Init_Appro(void)
{
	if (ApproDrvInit(VD_MSG_TYPE))
	{
		exit(1);
	}

	debug("ApproDrvInit done.\n");

	if (func_get_mem(NULL))
	{
		ApproDrvExit();
		exit(1);
	}

	debug("func_get_mem done.\n");
}

void Clean_Appro(void)
{
	ApproInterfaceExit();
}

// int get_cmd_info(char *cmd, char *result)
// {
//     FILE *fp;
//
//     if ((fp = popen(cmd, "r")) == NULL)
//     {
//         // cout << "[Warn:] popen error!" << endl;
//         pclose(fp);
//         return 1;
//     }
//
//     if (fgets(result, 100, fp) == NULL)
//     {
//         pclose(fp);
//         return -1;
//     }
//     else
//     {
//         int j = 0;
//         for (j = 0; j < (int) strlen(result); j++)
//         {
//             if (result[j] == '\n')
//                 result[j] = 0;
//         }
//
//         pclose(fp);
//         return 0;
//     }
// }
//
//int get_sys_status()
//{
//	// start_timer(3000000);
//	char cmd_result[100];
//	char *sys_grep; //origin string sys_grep
//	bzero(cmd_result, sizeof(cmd_result));
//	get_cmd_info((char *) "ps|grep system", cmd_result);
//	sys_grep = cmd_result;
//	// cout << "System_server status: " << sys_grep << endl;
//	// if (sys_grep.find("system_server") == sys_grep.npos)
//	//     return -1;
//	// else
//	// stop_timer();
//	return 0;
//}
//
//int zoom_fun(int zoom_code)
//{
//	// start_timer(3000000);
//
//	// unsigned char code;
//	// switch (zoom_code)
//	// {
//	// case 0:
//	//     code = 0;
//	//     if (ControlSystemData(SFIELD_SET_ZOOM_WIDE, (void *) &code, 1) < 0)
//	//     {
//	//         // cout << "[Error:] PTZ set failure!" << endl;
//	//         return -1;
//	//     }
//	//     break;
//	// case 1:
//	//     code = 1;
//	//     if (ControlSystemData(SFIELD_SET_ZOOM_WIDE, (void *) &code, 1) < 0)
//	//     {
//	//         // cout << "[Error:] PTZ set failure!" << endl;
//	//         return -1;
//	//     }
//	//     break;
//	// case 2:
//	//     code = 1;
//	//     if (ControlSystemData(SFIELD_SET_ZOOM_TELE, (void *) &code, 1) < 0)
//	//     {
//	//         // cout << "[Error:] PTZ set failure!" << endl;
//	//         return -1;
//	//     }
//	//     break;
//	// default:
//	//     break;
//	// }
//	// stop_timer();
//	return 0;
//}
//
//int set_alarm(int alarm_code)
//{
//	unsigned char code;
//	if (!alarm_code)
//	{
//		code = 0;
//		if (ControlSystemData(SFIELD_SET_ALARM_ENABLE, (void *)&code, sizeof(code)) < 0)
//		{
//			fprintf(stderr, "ERROR: zdy@file(%s) L(%d) func(%s) alarm enable set Fail!!\n", __FILE__, __LINE__, __func__);
//			return -1;
//		}
//		if (ControlSystemData(SFIELD_SET_EXT_ALARM, (void *)&code, sizeof(code)) < 0)
//		{
//			fprintf(stderr, "ERROR: zdy@file(%s) L(%d) func(%s) ext alarm set Fail!!\n", __FILE__, __LINE__, __func__);
//			return -1;
//		}
//	}
//	else if (alarm_code == 1)
//	{
//		code = 1;
//		if (ControlSystemData(SFIELD_SET_ALARM_ENABLE, (void *)&code, sizeof(code)) < 0)
//		{
//			fprintf(stderr, "ERROR: zdy@file(%s) L(%d) func(%s) alarm enable set Fail!!\n", __FILE__, __LINE__, __func__);
//			return -1;
//		}
//		if (ControlSystemData(SFIELD_SET_EXT_ALARM, (void *)&code, sizeof(code)) < 0)
//		{
//			fprintf(stderr, "ERROR: zdy@file(%s) L(%d) func(%s) ext alarm set Fail!!\n", __FILE__, __LINE__, __func__);
//			return -1;
//		}
//	}
//	else if (alarm_code == 2)
//	{
//		fprintf(stderr, "ERROR: zdy@file(%s) L(%d) func(%s)\n", __FILE__, __LINE__, __func__);
//		system("echo 1 > /sys/class/gpio/gpio92/value");
//	}
//	return 0;
//}



/*	following add by zdy, 2013-08-28	*/
Temp_Detail_t *get_temp_detail()
{
	start_timer(60000000);
	static Temp_Detail_t temp_detail;
	memset(&temp_detail, 0, sizeof(temp_detail));

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		stop_timer();
		return NULL;
	}
	float value = 0;

//	if (ControlSystemData(SFIELD_GET_DRIVER_TEMP, (void *)&value, sizeof(value)))
//	{
//		stop_timer();
//		return NULL;
//	}
	if (ControlSystemData(SFIELD_GET_8147_TEMP, (void *)&value, sizeof(value)))
	{
		stop_timer();
		return NULL;
	}


//	temp_detail.FpgaTemp = pSysInfo->camera_detail_info.FpgaTemp;
	temp_detail.Temp8147 = pSysInfo->camera_detail_info.Temp8147;
//	temp_detail.DriverTemp = pSysInfo->camera_detail_info.DriverTemp;
//	temp_detail.McuTemp = pSysInfo->camera_detail_info.McuTemp;
//	temp_detail.CCDTemp = pSysInfo->camera_detail_info.CCDTemp;
//	temp_detail.Temp6678 = pSysInfo->camera_detail_info.Temp6678;

//	temp_detail.McuCurrent = pSysInfo->camera_detail_info.McuCurrent;
//	temp_detail.McuVoltage = pSysInfo->camera_detail_info.McuVoltage;
//	temp_detail.CCDCurrent = pSysInfo->camera_detail_info.CCDCurrent;
//	temp_detail.CCDVoltage = pSysInfo->camera_detail_info.CCDVoltage;
//	temp_detail.FPGACurrent = pSysInfo->camera_detail_info.FPGACurrent;
//	temp_detail.FPGAVoltage = pSysInfo->camera_detail_info.FPGAVoltage;
//	temp_detail.Current8147 = pSysInfo->camera_detail_info.Current8147;
//	temp_detail.Voltage8147 = pSysInfo->camera_detail_info.Voltage8147;
//	temp_detail.Current6678 = pSysInfo->camera_detail_info.Current6678;
//	temp_detail.Voltage6678 = pSysInfo->camera_detail_info.Voltage6678;

	printf("zdy@%s(L@%d): in %s(); Temp8147 = %f\n", __FILE__, __LINE__, __func__, temp_detail.Temp8147);
//	printf("zdy@%s(L@%d): in %s(); DriverTemp = %f\n", __FILE__, __LINE__, __func__, temp_detail.DriverTemp);

	stop_timer();
	return &temp_detail;
}
/*	end added, zdy, 2013-08-28	*/

/*	following add by zdy, 2013-09-03	*/
//int harddisk_power_onoff(int id, int action)
//{
//	start_timer(3000000);
//	unsigned char value = 1;
//	int ret = SUCCESS;
//
//	if (id == 0 && action == 0)
//	{
//// printf("zdy@%s(L@%d): in %s(); id = %d, action = %d\n", __FILE__, __LINE__, __func__, id, action);
//		if (ControlSystemData(SFIELD_SET_CANLICHE_OFF1, (void *)&value, sizeof(value)) < 0)
//			ret = FAIL;
//	}
//	else if (id == 0 && action == 1)
//	{
//		if (ControlSystemData(SFIELD_SET_CANLICHE_ON1, (void *)&value, sizeof(value)) < 0)
//			ret = FAIL;
//	}
//	else if (id == 1 && action == 0)
//	{
//		// printf("zdy@%s(L@%d): in %s(); id = %d, action = %d\n", __FILE__, __LINE__, __func__, id, action);
//		if (ControlSystemData(SFIELD_SET_CANLICHE_OFF2, (void *)&value, sizeof(value)) < 0)
//			ret = FAIL;
//	}
//	else if (id == 1 && action == 1)
//	{
//		if (ControlSystemData(SFIELD_SET_CANLICHE_ON2, (void *)&value, sizeof(value)) < 0)
//			ret = FAIL;
//	}
//	else
//	{
//		printf("error harddisk!\n");
//		ret = FAIL;
//	}
//exit:
//	stop_timer();
//	return ret;
//}
/*	end added, zdy, 2013-09-03	*/

/* add by dsl, 2013-9-6 */
int set_current_time(time_t curtime)
{
#if 1
	//clock_t t_in = times(NULL);
	//fprintf(stderr, "inside: file(%s)@Line(%d)@func(%s) current_time=%d value=%d\n", __FILE__, __LINE__, __func__, time(NULL), (long)curtime);

	int status = -1;
	int ret = SUCCESS;
	//time_t curtime = (time_t)value;

	start_timer(3000000);

	if (-1 == stime(&curtime))
	{
		printf("%s(%d) at %s() call stime() fail!!!\n", __FILE__, __LINE__, __func__);
		ret = FAIL;
		goto exit;
	}

	//fprintf(stderr, "in: file(%s)@Line(%d)@func(%s) time=%d\n", __FILE__, __LINE__, __func__, time(NULL));

	status = system("hwclock -uw");

	if (status == -1)
	{
		printf("%s(%d) at %s() call system() fail!!!\n", __FILE__, __LINE__, __func__);
		ret = FAIL;
		goto exit;
	}
	else
	{
		if (WIFEXITED(status))
		{
			if (WEXITSTATUS(status) == 0)
			{
				//do nothing
			}
			else
			{
				printf("%s(%d) at %s(): fail!!!\n", __FILE__, __LINE__, __func__);
				ret = FAIL;
				goto exit;
			}
		}
		else
		{
			printf("%s(%d) at %s(): fail!!!\n", __FILE__, __LINE__, __func__);
			ret = FAIL;
			goto exit;
		}
	}

	// fprintf(stderr, "in: file(%s)@Line(%d)@func(%s) time=%d\n", __FILE__, __LINE__, __func__, time(NULL));
	// clock_t t_out = times(NULL);
	// printf("alei@%s: It takes %f second in %s().\n", __FILE__, (double)(t_out-t_in)/CLOCKS_PER_SEC,  __func__);

exit:
	stop_timer();
	return ret;

#else
	// clock_t t_in = times(NULL);
	fprintf(stderr, "inside: file(%s)@Line(%d)@func(%s) time=%d\n", __FILE__, __LINE__, __func__, time(NULL));

	start_timer(3000000);
	int ret = 0;
	time_t value = curtime;

	//printf("zdy@%s(L@%d): in %s(); value = %d\n", __FILE__, __LINE__, __func__, value);
	if (ControlSystemData(SFIELD_SET_CURRENTTIME, (void*)&value, sizeof(value)) < 0)
	{
		printf("[libinterface] set system time error!!!\n");
		ret = -1;
	}

	/*clock_t t_out = times(NULL);

	printf("alei@%s: It takes %f second in %s().\n", __FILE__, (double)(t_out-t_in)/CLOCKS_PER_SEC,  __func__);*/
	fprintf(stderr, "outside: file(%s)@Line(%d)@func(%s) time=%d\n", __FILE__, __LINE__, __func__, time(NULL));

exit:
	stop_timer();
	return ret;
#endif
}
/* end added, dsl, 2013-9-6 */

/*	following add by zdy, 2013-09-22	*/
int restore_factory_set(void)
{
	int ret = SUCCESS;
	start_timer(3000000);
	__u8 value = 0;
	printf("zdy@%s(L%d): at %s() factory reset camera.\n", __FILE__, __LINE__, __func__);

	if(ControlSystemData(SFIELD_RESET_CAMERA, (void *)&value, sizeof(value)) < 0)
	{
		ret = FAIL;
		goto exit;
	}
exit:
	stop_timer();
	return ret;
}
/*	end added, zdy, 2013-09-22	*/
/* add by dsl, 2013-9-18 */
//void *signalDetectThread(void * arg)
//{
//	//CallBack func = (CallBack)arg;
//	signal_status status;
//	//pthread_detach(pthread_self());
//	printf("[libinterface]: signal detect thread start.\n");
//
//	while (runflag)
//	{
//		memset(&status, '\0', sizeof(signal_status));
//
//		if (ControlSystemData(SFIELD_GET_SIGNALSTATUS, (void*)&status, sizeof(signal_status)) < 0)
//		{
//			if (pFunc != NULL)
//			{
//				pFunc(NULL);
//			}
//			continue;
//		}
//
//		if (pFunc != NULL)
//		{
//			pFunc(&status);
//		}
//	}
//	runflag = 0;
//	printf("[libinterface]: signal detect thread exit.\n");
//	return 0;
//
//}
//
//int set_signal_detect_callback(CallBack func)
//{
//	pFunc = func;
//	return 0;
//}
//
///* end added, dsl, 2013-9-18 */
//
///* add by dsl, 2013-11-5 */
//int set_camera_brightness(Brightness_control * arg)
//{
//	printf("alei@%s(L%d):at %s()\n", __FILE__, __LINE__, __func__);
//	int ret = SUCCESS;
//	int brightness = 0;
//
//	start_timer(3000000);
//
//	if (NULL == arg)
//	{
//		printf("[libinterface] %s(): invalid argument\n", __func__);
//		ret = FAIL;
//		goto exit;
//	}
//
//	brightness = arg->brightness;
//	if (brightness < 1 || brightness > 255)
//	{
//		printf("[libinterface] %s(): invalid argument.\nPlease specify a value between 1 and 255\n", __func__);
//		ret = FAIL;
//		goto exit;
//	}
//
//	if (ControlSystemData(SFIELD_SET_CURRENT_LUMINANCE, (void *)&brightness, sizeof(int)) < 0)
//	{
//		printf("alei@%s(L%d): at %s() call SFIELD_SET_CURRENT_LUMINANCE error!\n", __FILE__, __LINE__, __func__);
//		ret = FAIL;
//		goto exit;
//	}
//
//exit:
//	stop_timer();
//	return ret;
//}
/* end added, dsl, 2013-11-5 */

/* add by dsl, 2013-12-16 */
#define STROBE_DELAY_TIME_MAX	(80000)

int set_strobe_control(strobe_t * arg)
{
	int ret = SUCCESS;

	start_timer(3000000);

	if (NULL == arg)
	{
		printf("[libinterface] %s(): invalid argument(null)\n", __func__);
		ret = FAIL;
		goto exit;
	}

	if (0 != arg->enable || 1 != arg->enable)
	{
		printf("[libinterface] %s(): invalid argument(enable=%d)\n", __func__, arg->enable);
		ret = FAIL;
		goto exit;
	}

	if (0 != arg->polarity || 1 != arg->polarity)
	{
		printf("[libinterface] %s(): invalid argument(polarity=%d)\n", __func__, arg->polarity);
		ret = FAIL;
		goto exit;
	}

	if (arg->delay_time < 0 || arg->delay_time > STROBE_DELAY_TIME_MAX)
	{
		printf("[libinterface] %s(): invalid argument(delay_time=%u)\n", __func__, arg->delay_time);
		ret = FAIL;
		goto exit;
	}

	unsigned char ledctrl = 1;
	ledctrl |= arg->polarity<<4 | arg->enable<<5;

	if (ControlSystemData(SFIELD_SET_LED_CTRL, (void *)&ledctrl, sizeof(unsigned char)) < 0)
	{
		printf("alei@%s(L%d): at %s() call SFIELD_SET_LED_CTRL error!\n", __FILE__, __LINE__, __func__);
		ret = FAIL;
		goto exit;
	}

	unsigned int delay_time = arg->delay_time;
	if (ControlSystemData(SFIELD_SET_LED_DELAYTIME, (void *)&delay_time, sizeof(unsigned int)) < 0)
	{
		printf("alei@%s(L%d): at %s() call SFIELD_SET_LED_DELAYTIME error!\n", __FILE__, __LINE__, __func__);
		ret = FAIL;
		goto exit;
	}

exit:
	stop_timer();
	return ret;
}

strobe_t* get_strobe_ctrl()
{
	static strobe_t param;

	start_timer(3000000);

	SysInfo *pSysInfo = GetSysInfo();
	if (pSysInfo == NULL)
	{
		printf("Error when read share memory.\n");
		//deinit_env();
		stop_timer();
		return NULL;
	}
	memset(&param, 0, sizeof(strobe_t));

	unsigned char ledctrl = 0;
	memcpy(&ledctrl, &pSysInfo->led_info.LedCtrl, sizeof(unsigned char));
	param.enable = (ledctrl>>5)&0x1;
	param.polarity= (ledctrl>>4)&0x1;

	memcpy(&param.delay_time, &pSysInfo->led_info.LedDelayTime, sizeof(unsigned int));
	printf("alei@%s(L%d): at %s() enable=%u, polarity=%u, delay_time=%u.\n", __FILE__, __LINE__, __func__, param.enable, param.polarity, param.delay_time);

//exit:
	stop_timer();
	return &param;

}
/* end added, dsl, 2013-12-16 */

///*add by dsl, 2013-12-24 */
//static int find_port(int devType, int *baudrate, char *parity, int *databit, int *stopbit)
//{
//	int ports[] = { 1, 3, 4};
//	int index = 0;
//
//	SysInfo *pSysInfo = GetSysInfo();
//	if (pSysInfo == NULL)
//		return -1;
//
//	for (index = 0; index < 3; index++)
//	{
//		if (devType == pSysInfo->seri_config[index].Devtype)
//		{
//			*baudrate = pSysInfo->seri_config[index].Baudrate;
//			*parity = (pSysInfo->seri_config[index].Parity == 2 ? 'E' : pSysInfo->seri_config[index].Parity == 1 ? 'O' : 'N');
//			*databit = pSysInfo->seri_config[index].Databit;
//			*stopbit = pSysInfo->seri_config[index].Stopbit;
//			break;
//		}
//
//	}
//
//	if (3 == index)
//		return -1;
//	else
//		return ports[index];
//
//}
//
//int mrs485_command_send(unsigned char* buf_send, int num_send, int port_num, char O_S, int baudrate, char parity, int databit, int stopbit)
//{
//	static pre_port = -1;
//	//int fd = -1;
//	int i = 0;
//	int nread = -1;
//	int gpio_num = -1;
//	char cmd[100]="";
//
//
//	if (pre_port != port_num || md44_fd == -1)
//	{
//		// if ((fd = Open_Port(port_num, O_S, O_RDWR|O_NOCTTY|O_NDELAY)) < 0) //  open /dev/ttyo2
//		if (md44_fd != -1)
//		{
//			close(md44_fd);
//			md44_fd = -1;
//		}
//		if ((md44_fd = Open_Port(port_num, O_S, O_RDWR|O_NOCTTY)) < 0) //  open /dev/ttyo2
//		{
//			printf("Uart open error!\n");
//			return -1;
//		}
//		/*if((Set_Opt(fd, 9600, 8, 'N', 1)) < 0) // set baund rate, data bit....
//		{
//			perror("Set_Opt RS485 error");
//			return -2;
//		}*/
//		if((Set_Opt(md44_fd, baudrate, databit, parity, stopbit)) < 0) // set baund rate, data bit....
//		{
//			perror("Set_Opt RS485 error");
//			return -2;
//		}
//
//	}
//	switch (port_num)
//	{
//	case 1:
//		gpio_num = 24;
//		break;
//	case 3:
//		gpio_num = 26;
//		break;
//	case 4:
//		gpio_num = 27;
//		break;
//	default:
//		gpio_num = -1;
//		break;
//	}
//
//	sprintf(cmd,"/sys/class/gpio/gpio%d/value", gpio_num);
//	if (access(cmd, 0) == -1)
//	{
//		//printf("Create uart2 gpio0[%d] ......\n", gpio_num);
//		memset(cmd, 0, 100);
//		sprintf(cmd, "echo %d > /sys/class/gpio/export", gpio_num);
//		if(-1 == system(cmd))
//		{
//			printf("echo %d > /sys/class/gpio/export error.\n", gpio_num);
//		}
//
//		memset(cmd, 0, 100);
//		sprintf(cmd, "echo out > /sys/class/gpio/gpio%d/direction", gpio_num);
//		if (-1 == system(cmd))
//		{
//			printf("echo out > /sys/class/gpio/gpio%d/direction error.\n", gpio_num);
//		}
//		printf("Create uart%d gpio0[27] done.\n", port_num);
//	}
//	else
//	{
//		printf("Uart%d IO is controled by gpio0[%d]\n", port_num, gpio_num);
//	}
//
//	printf("echo 1 > /sys/class/gpio/gpio%d/value -----write enable\n", gpio_num);
//
//	memset(cmd, 0, 100);
//	sprintf(cmd, "echo 1 > /sys/class/gpio/gpio%d/value", gpio_num);
//
//	if ( -1 == system(cmd) )
//	{
//		printf("echo 1 > /sys/class/gpio/gpio%d/value error\n", gpio_num);
//	}
//
//	for ( i = 0; i < num_send; i++)
//		printf("cmd_send[%d]=0x%x.\n", i, buf_send[i]);
//
//	if (write(md44_fd, buf_send, num_send) < 0)             //Ð´´®¿Ú
//	{
//		perror("write error!\n");
//		close(md44_fd);
//		md44_fd = -1;
//		return -1;
//	}
//
//	//close(fd);
//	return 0;
//}
//
//int md44_control(char *buf, int len)
//{
//	int ret = SUCCESS;
//	start_timer(3000000);
//	int i = 0;
//	printf("in %s().\n", __func__);
//
//	for (i = 0; i < len; i++)
//	{
//		printf("\tbuf[%d]=0x%x\n", i, buf[i]);
//	}
//
//	int port = 0;
//	int baudrate, databit, stopbit;
//	char parity;
//
//	port = find_port(DEVICE_MD44, &baudrate, &parity, &databit, &stopbit);
//	if (-1 == port)
//	{
//		printf("alei@%s(L%d): at %s() no MD44 device.\n", __FILE__, __LINE__, __func__);
//		return -1;
//	}
//
//
//	if(0 != mrs485_command_send(buf, len, port, 'O', baudrate, parity, databit, stopbit))
//	{
//		printf("alei@%s(L%d): at %s() send buffer failed.\n", __FILE__, __LINE__, __func__);
//		return -1;
//
//	}
//#if 0
//	if (ControlSystemData(SFIELD_MD44_CONTROL, (void *)buf, len) < 0)
//	{
//		ret = FAIL;
//	}
//#endif
//
//	stop_timer();
//	return ret;
//}
///* end added, dsl, 2013-12-24 */
//
//
///*add by dsl, 2014-1-6 */
//int Heart()
//{
//	printf("inside: %s\n", __func__);
//	unsigned char buf_send[7];
//	memset(buf_send, 0, sizeof(buf_send));
//
//	buf_send[0] = 0xFF;
//	buf_send[1] = 0x32;
//	buf_send[2] = 0x00;
//	buf_send[3] = 0x00;
//	buf_send[4] = 0x00;
//	buf_send[5] = buf_send[1] + buf_send[2] + buf_send[3] + buf_send[4];
//	if (tty_sem == NULL)
//	{
//		printf("initialize sem\n");
//		I2CSemInit();
//		tty_sem = sem_open(SEM_NAME_TTY, OPEN_FLAG, OPEN_MODE, INIT_V);
//	}
//	I2CSemWait(tty_sem);
//	command_send(buf_send, 6, 5, 'O');
//	I2CSemRelease(tty_sem);
//	return 0;
//}
//
//int stopMcuHeart()
//{
//	printf("inside: %s\n", __func__);
//	unsigned char buf_send[7];
//	memset(buf_send, 0, sizeof(buf_send));
//
//	buf_send[0] = 0xFF;
//	buf_send[1] = 0x32;
//	buf_send[2] = 0x01;
//	buf_send[3] = 0x00;
//	buf_send[4] = 0x00;
//	buf_send[5] = buf_send[1] + buf_send[2] + buf_send[3] + buf_send[4];
//
//	if (tty_sem == NULL)
//	{
//		printf("initialize sem\n");
//		I2CSemInit();
//		tty_sem = sem_open(SEM_NAME_TTY, OPEN_FLAG, OPEN_MODE, INIT_V);
//	}
//	I2CSemWait(tty_sem);
//	command_send(buf_send, 6, 5, 'O');
//	I2CSemRelease(tty_sem);
//	return 0;
//}
//
//int openMcuHeart()
//{
//	printf("inside: %s\n", __func__);
//	unsigned char buf_send[7];
//	memset(buf_send, 0, sizeof(buf_send));
//
//	buf_send[0] = 0xFF;
//	buf_send[1] = 0x32;
//	buf_send[2] = 0x01;
//	buf_send[3] = 0x01;
//	buf_send[4] = 0x00;
//	buf_send[5] = buf_send[1] + buf_send[2] + buf_send[3] + buf_send[4];
//
//	if (tty_sem == NULL)
//	{
//		printf("initialize sem\n");
//		I2CSemInit();
//		tty_sem = sem_open(SEM_NAME_TTY, OPEN_FLAG, OPEN_MODE, INIT_V);
//	}
//	I2CSemWait(tty_sem);
//	command_send(buf_send, 6, 5, 'O');
//	I2CSemRelease(tty_sem);
//	return 0;
//}
/* end added, dsl, 2014-1-6 */
