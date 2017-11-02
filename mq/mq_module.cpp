/*
 * mq_module.cpp
 *
 *  Created on: 2013-4-11
 *      Author: shanhongwei
 */

#include <stdio.h>

#include "json/json.h"
#include "commontypes.h"
#include "commonfuncs.h"
#include "global.h"
#include "mq_module.h"
#include "mq_listen.h"
#include "ep_type.h"
#include "logger/log.h"
#include "interface_alg.h"

#if 1 /* include the declaration of recv_msg_park_down. */
#include "songli_descending.h"
#endif

using namespace std;

extern int flag_heart_mq; //MQ心跳标志
extern void *thread_mq_send(void *arg);

pthread_t thread_t_mq;
pthread_t thread_t_mq_songli;

static class_mq_producer *g_mq_upload; //参数上载会话;
static class_mq_producer *g_mq_pass_car; //过车会话;
static class_mq_producer *g_mq_illegal; //违法会话;
static class_mq_producer *g_mq_illegal_hisense; //违法会话_海信平台;
static class_mq_producer *g_mq_park; //泊车会话

static class_mq_producer *g_mq_broadcast_send; //广播发送会话;
static class_mq_producer *g_mq_event_alarm; //事件告警会话;

class_mq_producer *g_mq_park_upload;           //泊车上传json到中心服务
class_mq_consumer *g_mq_park_down;

class_mq_consumer *g_mq_broadcast_listen; //广播监听会话;
class_mq_consumer *g_mq_download; //参数下载会话;


//static class_mq_producer *g_mq_dev_stat; //设备状态会话;
//static class_mq_producer *g_mq_traffic_flow; //交通流发送会话;
//static class_mq_producer *g_mq_h264Up; //h264上传FTP会话;
//static class_mq_producer *g_mq_nocar_pass; //非机动车通行记录会话;
//static class_mq_producer *g_mq_nocar_illegal; //非机动车违法记录会话;
//static class_mq_producer *g_mq_nocar_flow; //行人流量会话;
//static class_mq_producer *g_mq_history_passcar; //过车历史记录;
//static class_mq_producer *g_mq_history_illegal; //违法历史记录;
//static class_mq_producer *g_mq_history_statistics;
//static class_mq_producer *g_mq_history_event;



extern SET_NET_PARAM g_set_net_param;

int g_flg_mq_start = 0; //MQ会话启动完成.		1:完成; 0: 未完成;
int g_flg_mq_start_songli = 0; //MQ会话启动完成.		1:完成; 0: 未完成;


int g_flg_mq_sta_ok = 0; //MQ各个会话状态相与的结果.
int mq_event_alarm_ok = 0; //事件告警会话标志;

//int mq_ok = 0; //MQ服务状态标志	置1为MQ服务可用。
int upgrade = 0; //设备升级

static class_mq_producer *mq_sender_instance[MQ_INSTANCE_COUNT];
static bool mq_lib_initialized = false;

/*
 * 功能：获取mq发送实例指针
 * 参数：
 *		[in]index -- 实例指针到索引号
 * 返回：
 *      成功返回实例指针， 失败返回NULL
 */
class_mq_producer *get_mq_producer_instance(int index)
{
	if (index >= MQ_INSTANCE_COUNT || index < 0)
	{
		return NULL;
	}

	return mq_sender_instance[index];
}

void * thread_mq(void *arg);
void * thread_start_mq(void *arg);


/*
 * 功能: 初始化ActiveMQ库
 */
void mq_lib_init(void)
{
	if (mq_lib_initialized)
	{
		return;
	}
	activemq::library::ActiveMQCPP::initializeLibrary();
	mq_lib_initialized = true;
}

/*******************************************************************************
 * 功能：关闭mq库
 * 参数：无
 * 返回：无
 *******************************************************************************/
void mq_lib_close(void)
{
	activemq::library::ActiveMQCPP::shutdownLibrary();
}

/*
 * 功能：获取mq版本号
 * 参数：
 *		[out]version --版本号指针
 * 返回：
 *      成功返回0
 */
int get_mq_version(char *version)
{
	ActiveMQConnectionMetaData *ver;
	std::string str;

	if (!mq_lib_initialized)
	{
		mq_lib_init();
	}
	ver = new ActiveMQConnectionMetaData();
	str = ver->getProviderVersion();

	sprintf(version, "%s", str.c_str());
	DEBUG("MQ_version = %s \n", str.c_str());
	delete ver;

	return 0;
}

/*
 * 停止mq业务逻辑线程
 */
void stop_mq_thread()
{
	mq_lib_close();
}
/*
 * 启动mq业务逻辑线程
 */
int start_mq_thread()
{
	pthread_attr_t attr;
	pthread_t MqStartThread;

	/* Initialize the thread attributes */
	if (pthread_attr_init(&attr))
	{
		DEBUG("Failed to initialize thread attrs\n");
		return FAILURE;
	}
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	printf_with_ms("to thread_start_mq:\n");
	if (pthread_create(&MqStartThread, &attr, thread_start_mq, NULL))
	{
		DEBUG("Failed to create thread_start_mq\n");
		perror("error");
		pthread_attr_destroy(&attr);
		return FAILURE;
	}

	printf_with_ms("after thread_start_mq\n");
	pthread_attr_destroy(&attr);

	return SUCCESS;
}

void * thread_start_mq(void *arg)
{
	struct sched_param schedParam;
	pthread_attr_t attr;

	pthread_t MqSendThread;
	printf_with_ms("to mq_lib_init:\n");
	mq_lib_init();
	printf_with_ms("after mq_lib_init:\n");
	for (int index = 0; index < MQ_INSTANCE_COUNT; index++)
	{
		mq_sender_instance[index] = NULL;
	}

	/* Initialize the thread attributes */
	if (pthread_attr_init(&attr))
	{
		DEBUG("Failed to initialize thread attrs\n");
		return NULL;
	}
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/* Force the thread to use custom scheduling attributes */
	//	if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED))
	//	{
	//		DEBUG("Failed to set schedule inheritance attribute\n");
	//		return FAILURE;
	//	}

	/* Set the thread to be fifo real time scheduled */
	//	if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO))
	//	{
	//		DEBUG("Failed to set FIFO scheduling policy\n");
	//		return FAILURE;
	//	}

	//#########################################/MQ发送数据线程/#########################################//
	schedParam.sched_priority = MQ_QUEUE_THREAD_PRIORITY;
	//	if (pthread_attr_setschedparam(&attr, &schedParam))
	//	{
	//		DEBUG("Failed to set scheduler parameters\n");
	//		return FAILURE;
	//	}

	if (pthread_create(&thread_t_mq, &attr, thread_mq, NULL))
	{
		DEBUG("Failed to create speech thread\n");
		perror("error");
		pthread_attr_destroy(&attr);
		return NULL;
	}
	if (pthread_create(&MqSendThread, &attr, thread_mq_send, NULL))
	{
		DEBUG("Failed to create speech thread\n");
		pthread_attr_destroy(&attr);
		return NULL;
	}

	pthread_attr_destroy(&attr);
//	debug("start_mq_thread passed.\n");
	log_send(LOG_LEVEL_STATUS,0,"MQ:","start_mq_thread passed!\n");
	return NULL;
}

/*
 * 心跳后，定时通知处理函数
 */
void sig_alarm(int sig, siginfo_t * info, void * test)
{
	flag_heart_mq = 0;
	return;
}

/*
 * 功能：MQ会话线程.
 * 参数：
 * 返回：
 */
void * thread_mq(void *arg)
{
	std::string uri_broker;
	std::string uri_dest;
	char str[256];
	char s_ip[128];

	//############  MQ参数配置       ################//

	sprintf((char *) s_ip, "tcp://%d.%d.%d.%d:%d",
	        g_set_net_param.m_NetParam.m_MQ_IP[0],
	        g_set_net_param.m_NetParam.m_MQ_IP[1],
	        g_set_net_param.m_NetParam.m_MQ_IP[2],
	        g_set_net_param.m_NetParam.m_MQ_IP[3],
	        g_set_net_param.m_NetParam.m_MQ_PORT);

//	DEBUG("MQ服务器地址: %s \n", s_ip);
	log_send(LOG_LEVEL_STATUS,0,"MQ:","The MQ IP address:%s\n",s_ip);

	uri_broker += "failover://(";
	uri_broker += s_ip;
	uri_broker += "?wireFormat=openwire";

	//The maximum inactivity duration (before which the socket is considered dead) in milliseconds.
	//On some platforms it can take a long time for a socket to appear to die, so we allow the broker to kill
	//connections if they are inactive for a period of time. Use by some transports to enable a keep alive heart beat feature.
	//Set to a value <= 0 to disable inactivity monitoring.
	uri_broker += "&wireFormat.MaxInactivityDuration=30000"; //失效检测时间. 现在此参数还无法配置成功.
	uri_broker += "&wireFormat.MaxInactivityDurationInitalDelay=10002"; //失效重连间隔时间.现在此参数还无法配置成功.
	uri_broker += "&soKeepAlive=true";
	uri_broker += "&transport.useAsyncSend=true";
	uri_broker += "&transport.useInactivityMonitor=true";
	uri_broker += "&keepAliveResponseRequired=false";
	uri_broker += ")";

	//###############/Failover Transport Options/###############//
	//If a send is blocked waiting on a failed connection to reconnect how long should it wait before failing the send, default is forever (-1).
	uri_broker += "?timeout=10001";
	uri_broker += "&initialReconnectDelay=10"; //How long to wait if the initial attempt to connect to the broker fails.
	uri_broker += "&maxReconnectDelay=20003"; //Maximum time that the transport waits before trying to connect to the Broker again.
	uri_broker += "&maxReconnectAttempts=0"; //Max number of times to attempt to reconnect before failing the transport, default is forever (0).
	uri_broker += "&connection.useAsyncSend=true"; //异步发送;
	//uri_broker += "&connection.alwaysSyncSend=false";

	//Time to wait on Message Sends for a Response, default value of zero indicates to wait forever.
	//Waiting forever allows the broker to have flow control over messages coming from this client
	//if it is a fast producer or there is no consumer such that the broker would run out of memory
	//if it did not slow down the producer.
	uri_broker += "&connection.sendTimeout=10002";

	//The amount of time to wait for a response from the broker when shutting down.
	// Normally we want a response to indicate that the client has been disconnected cleanly,
	//but we don't want to wait forever, however if you do, set this to zero.
	uri_broker += "&connection.closeTimeout=10002";

	//	uri_broker += "&keepAliveResponseRequired=false";
	//	uri_broker += "&";
	//	uri_broker += "&";
	//	uri_broker += "&";
	//		uri_broker += ")";

	uri_broker += "";

	//	uri_broker += "&soKeepAlive=true";
	//	uri_broker += "";//")";//)";//+")";

//	DEBUG("uri_broker = %s \n", uri_broker.c_str());
	log_send(LOG_LEVEL_STATUS,0,"MQ:","The MQ uri_broker = %s\n",uri_broker.c_str());


	log_send(LOG_LEVEL_STATUS,0,"MQ:","**********************Start Create MQ Session!!!!!**********************\n");


	//#########/参数上载会话/###########//
	sprintf(str, "%s", URI_UPDATA);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_upload = new class_mq_producer(uri_broker, uri_dest, "session_updata");


	//#########/过车会话/#########//
	sprintf(str, "%s", URI_PASS_CAR);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_pass_car = new class_mq_producer(uri_broker, uri_dest, "session_passcar");


	//##########/违法会话/###########//
	sprintf(str, "%s", URI_ILLEGAL);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_illegal = new class_mq_producer(uri_broker, uri_dest, "session_illegal");

	//##########/违法会话_海信平台/###########//
	sprintf(str, "%s", URI_VP_HISENSE);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_illegal_hisense = new class_mq_producer(uri_broker, uri_dest, "session_illegal_hisense");


	//##########/泊车会话/###########//
	sprintf(str, "%s", URI_PARK);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_park = new class_mq_producer(uri_broker, uri_dest, "session_park");


      //########/广播发送会话/############//
	sprintf(str, "%s", URI_BROADCAST_UP);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_broadcast_send = new class_mq_producer(uri_broker, uri_dest, "session_broadcast_up");


      //#####/事件告警发送会话/#############//
	sprintf(str, "%s", URI_EVENT_ALARM);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_event_alarm = new class_mq_producer(
	    uri_broker, uri_dest, "session_event_alarm");

//-----------------------------consumer-----------------------------//

	//######/广播监听会话/##############//
	sprintf(str, "%s", URI_BROADCAST_DOWN);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_broadcast_listen = new class_mq_consumer(
	    uri_broker, uri_dest, "session_broadcast_down", recv_msg_broadcast);


	//######/参数下载会话;/########//
	//	sprintf(str, "%s.%s", URI_DOWN, param_basic.m_strDeviceID);
	sprintf(str, "%s", URI_DOWN);
	uri_dest = str;
	DEBUG("uri_dest = %s \n", uri_dest.c_str());
	g_mq_download = new class_mq_consumer(
	    uri_broker, uri_dest, "session_downdata", recv_msg_down);

//-------------------------------------------------------------------//

	sleep(10);
//	DEBUG("开始连接MQ会话......\n");
	log_send(LOG_LEVEL_STATUS,0,"MQ:","**********************Start Connect MQ!!!!!**********************\n");

	mq_lib_init();

	mq_sender_instance[MQ_INSTANCE_UPLOAD] = g_mq_upload;           //参数上载
	mq_sender_instance[MQ_INSTANCE_PASS_CAR] = g_mq_pass_car;   //卡口
	mq_sender_instance[MQ_INSTANCE_ILLEGAL] = g_mq_illegal;	           //违停

	mq_sender_instance[MQ_INSTANCE_ILLEGAL_HISENSE] = g_mq_illegal_hisense;	   //违停_海信平台

	mq_sender_instance[MQ_INSTANCE_PARK] = g_mq_park;                  //泊车

	mq_sender_instance[MQ_INSTANCE_BROADCAST] = g_mq_broadcast_send;    //广播发送
	mq_sender_instance[MQ_INSTANCE_EVENT_ALARM] = g_mq_event_alarm;      //事件报警

	//###############/参数上载会话/###############//
	while (g_mq_upload->run() != 1)
	{
		g_mq_upload->close();
		sleep(5);
	}

	//###############/过车会话/###############//
	while (g_mq_pass_car->run() != 1)
	{
		g_mq_pass_car->close();
		sleep(5);
	}


	//###############/违法会话/###############//
	while (g_mq_illegal->run() != 1)
	{
		g_mq_illegal->close();
		sleep(5);
	}

	//###############/违法会话_海信平台/###############//
	while (g_mq_illegal_hisense->run() != 1)
	{
		g_mq_illegal_hisense->close();
		sleep(5);
	}

	//###############/泊车会话/###############//
	while (g_mq_park->run() != 1)
	{
		g_mq_park->close();
		sleep(5);
	}


	//###############/广播发送会话/###############//
	while (g_mq_broadcast_send->run() != 1)
	{
		g_mq_broadcast_send->close();
		sleep(5);
	}


	//###############/事件告警发送会话/###############//
	while (g_mq_event_alarm->run() != 1)
	{
		g_mq_event_alarm->close();
		sleep(5);
	}

//-----------------------------consumer-----------------------------//

	//###############/广播监听会话/###############//
	while (g_mq_broadcast_listen->run() != FLG_MQ_TRUE)
	{
		g_mq_broadcast_listen->close();
		sleep(1);
	}


	//###############/参数下载会话;/###############//
	while (g_mq_download->run() != FLG_MQ_TRUE)
	{
		g_mq_download->close();
		sleep(1);
	}

//------------------------------------------------------------------//


	g_flg_mq_start = 1;

	while (1)                //并没有调用 do_stat_check
	{

#ifdef MQ_SELF_KILL

#ifdef _DEBUG_OPEN_DEV_MQ_

		g_mq_upload->do_stat_check(); //参数上载会话;
		g_mq_download->do_stat_check();//参数下载会话

#endif

		g_mq_pass_car->do_stat_check(); //过车会话;
		g_mq_illegal->do_stat_check();//违法会话;
//		g_mq_traffic_flow->do_stat_check();//交通流发送会话;
		g_mq_event_alarm->do_stat_check();//事件告警会话;
		//		g_mq_nocar_illegal->do_stat_check();	//非机动车违法记录会话
//		g_mq_nocar_pass->do_stat_check();//	非机动车通行记录会话;

		g_mq_broadcast_listen->do_stat_check();//广播监听会话
		g_mq_broadcast_send->do_stat_check();//广播发送会话;

#endif

		sleep(1);
	}

	pthread_exit(NULL);
}
