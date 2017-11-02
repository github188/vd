/*
 * mq.h
 *
 *  Created on: 2013-4-8
 *      Author: shanhongwei
 */

#ifndef MQ_H_
#define MQ_H_

#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/core/ActiveMQConnection.h>
#include <activemq/core/ActiveMQConnectionMetaData.h>
#include <activemq/transport/DefaultTransportListener.h>
#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Integer.h>
#include <activemq/util/Config.h>
#include <decaf/util/Date.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <decaf/lang/Long.h>
#include <decaf/util/Date.h>

#include "ep_type.h"
#include "arm_config.h"

using namespace activemq;
using namespace activemq::core;
using namespace activemq::transport;
using namespace decaf::lang;
using namespace decaf::util;
using namespace decaf::util::concurrent;
using namespace cms;
using namespace std;

extern ARM_config g_arm_config; //arm参数结构体全局变量
extern char g_SerialNum[50];
extern char g_SoftwareVersion[50];

#define FLG_MQ_TRUE 			88		//MQ连接上;
#define FLG_MQ_FALSE 			-44		//MQ连接断开;
#define FLG_MQ_ERROR_YES		-11		//有异常发生
#define FLG_MQ_ERROR_NO			11		//无异常发生
#define MAX_MQ_CHECK_RESET	(60*10*1)	//MQ出现连续此段时间内无正常连接,就需重新创建会话。

namespace mq
{

class class_mq_consumer: public ExceptionListener,
	public MessageListener,
	public DefaultTransportListener
{
private:
	Connection* connection;
	Session* session;
	Destination* destination;
	MessageConsumer* consumer;
	bool useTopic;
	std::string brokerURI;
	std::string destURI;
	bool clientAck;

	int flg_conn_state; //会话连接状态;
	int flg_mq_error; //异常发生与否;
	const char *session_name; //实例名称用于调试显示类的实例名称,无真实作用.
	pthread_mutex_t mutex;

	int count; //断网检测次数;
	int flg_check_mq; //检查MQ会话在线并保持功能。
	int flg_run; //MQ会话创建成功标志;
	int flg_transport_sta; //失效传输通路状态;

	//	private:

	class_mq_consumer(const class_mq_consumer&);
	class_mq_consumer& operator=(const class_mq_consumer&);

	void clean_up();

public:

	class_mq_consumer()
	{
		pthread_mutex_init(&mutex, NULL);
	}
	class_mq_consumer(
	    const std::string& brokerURI, const std::string& destURI,
	    const char *session_name, void(*recv_msg)(const Message* message),
	    bool useTopic = true, bool clientAck = false);

	virtual ~class_mq_consumer() throw ();

	virtual void onException(const CMSException& ex AMQCPP_UNUSED);

	void close();

	int run();

	virtual void onMessage(const Message* message) throw ();

	void (*recv_msg)(const Message* message);

	//	void (* recv_msg_text)(const char *msg_text);
	//	void (* recv_msg_byte)(const char *msg_byte,int len);

	virtual void transportInterrupted();

	virtual void transportResumed();

	int get_mq_conn_state(void); //获得此实例MQ是否可用;	FLG_MQ_TRUE:可用; FLG_MQ_FALSE:不可用;

	int do_stat_check(void);

};

class class_mq_producer: public ExceptionListener,
	public DefaultTransportListener
{
private:

	Connection* connection;
	Session* session;
	Destination* destination;
	MessageProducer* producer;
	bool useTopic;
	bool clientAck;

	std::string brokerURI;
	std::string destURI;
	int flg_conn_state; //会话连接状态;
	int flg_mq_error; //异常发生与否;
	const char *session_name; //实例名称用于调试显示类的实例名称,无真实作用.

	pthread_mutex_t mutex_state; //状态改变, 发送进行同步控制
	int count; //断网检测次数;
	int flg_check_mq; //检查MQ会话在线并保持功能。
	int flg_run; //MQ会话创建成功标志;
	int flg_transport_sta; //失效传输通路状态;

private:

	class_mq_producer(const class_mq_producer&);
	class_mq_producer& operator=(const class_mq_producer&);
	void clean_up();

public:

	class_mq_producer()
	{
		pthread_mutex_init(&mutex_state, NULL);
	}

	class_mq_producer(
	    const std::string& brokerURI, const std::string& destURI,
	    const char *session_name, bool useTopic = true, bool clientAck = false);

	virtual ~class_mq_producer() throw ();

	void close();

	virtual void onException(const CMSException& ex AMQCPP_UNUSED);

	int send_msg_text(char *msg);

	int send_msg_text_string(const std::string& msg);
	int send_msg_with_property_text(char *msg, char *type,
	                                const char *str_property, int property_value);

	int send_msg_with_property_text(char *msg, char *type,
	                                const char *str_property, char *property_value);

	int send_msg_with_property_text(char *msg, char *type,
	                                const char *str_property, int property_value,
	                                const char *str_property2, int property_value2);

	int send_msg_with_property_text(char *msg, char *type,
	                                const char *property, int value, const char *property2,
	                                char* value2);

	int send_msg_with_property_text(char *msg, char *type,
	                                const char *str_property, char *property_value,
	                                const char *str_property2, int property_value2);
	//		int send_msg_with_property_byte(void *msg, int len, char *type, map<
	//		        string, int> *pMap);

	int send_msg_with_property_byte(void *msg, int len, char *type,
	                                const char *str_property0, int property_value0,
	                                const char *str_property1, int property_value1);

	int send_msg_byte(char *msg, int len, char enum_type);
	//		virtual void run();
	int run();

	virtual void transportInterrupted();
	virtual void transportResumed();

	int get_mq_conn_state(void); //获得此实例MQ是否可用;	 FLG_MQ_TRUE:可用; FLG_MQ_FALSE:不可用;
	int get_error(void); //获得MQ异常发生;


	int do_stat_check(void);
};
}

using namespace mq;

#endif /* MQ_H_ */
