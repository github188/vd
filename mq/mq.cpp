/*
 * commonfuncs.h
 *
 *  Created on: 2013-4-8
 *      Author: shanhongwei
 */

#include <stdio.h>
#include <string.h>
#include "mq.h"
#include "ep_type.h"
#include "arm_config.h"
#include "camera_config.h"
#include "log_interface.h"
#include "logger/log.h"

extern ARM_config g_arm_config; //arm参数结构体全局变量
extern Camera_config g_camera_config; //arm参数结构体全局变量
extern SET_NET_PARAM g_set_net_param;



/**************************************************************************

 * 功能：mq消费者构造函数
 * 参数：
 *		brokerURI:		mq服务器连接uri
 *		destURI: 		mq连接的topic/queue
 *		session_name:	会话名称
 *		recv_msg:		接收到消息后的回调函数
 *		useTopic:		是否topic
 *		clientAck:		客户端确认方式.  true--客户端确认, false--自动确认
 * 返回：
 *		无

**************************************************************************/

class_mq_consumer::class_mq_consumer(
    const std::string& brokerURI,const std::string& destURI,
    const char *session_name, void(* recv_msg)(const Message* message),
    bool useTopic, bool clientAck) :
	connection(NULL), session(NULL), destination(NULL), consumer(NULL),
	useTopic(useTopic), brokerURI(brokerURI), destURI(destURI),
	clientAck(clientAck), flg_conn_state(FLG_MQ_FALSE),
	flg_mq_error(FLG_MQ_ERROR_NO), session_name(session_name),
	recv_msg(recv_msg)
{
	flg_check_mq = 0;
	flg_run = FLG_MQ_FALSE;
	pthread_mutex_init(&mutex, NULL);
	flg_transport_sta = FLG_MQ_FALSE;
	flg_conn_state = FLG_MQ_FALSE;
	count = 0;
}

/**************************************************************************
 * 功能：
 * 参数：
 * 返回：
**************************************************************************/
class_mq_consumer::~class_mq_consumer() throw ()
{
	//	int sta;
	//	void *ret;

	this->clean_up();

}
/*
 * 功能：MQ消息 接收消费者开始运行;
 * 参数：无
 * 返回：
 */
int class_mq_consumer::run()
{
	pthread_mutex_lock(&mutex);
	try
	{
		INFO("消费者类实例<%s> :dest_uri=%s,brok=%s\n", session_name,
		         destURI.c_str(), brokerURI.c_str());
		ActiveMQConnectionFactory* connectionFactory =
		    new ActiveMQConnectionFactory(brokerURI);

		// Create a Connection
		connection = connectionFactory->createConnection();
		delete connectionFactory;
		connectionFactory = NULL;

		this->connection->setExceptionListener(this);          //1MQ异常监听，如果出现异常会调用onException

		ActiveMQConnection* amqConnection =
		    dynamic_cast<ActiveMQConnection*> (connection);
		if (amqConnection != NULL)
		{
			amqConnection->addTransportListener(this);             //1addTransportListener  中有transportInterrupted(MQ断线异常) 和transportResumed
														//1如果只是单纯的MQ断开并不会调用onExceptiong函数来处理异常，其他的异常
														//1调用onException来处理
		}

		connection->start();

		// Create a Session
		if (clientAck)
		{
			session = connection->createSession(Session::CLIENT_ACKNOWLEDGE);
		}
		else
		{
			session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
		}

		// Create the destination (Topic or Queue)
		if (useTopic)
		{
			destination = session->createTopic(destURI);
		}
		else
		{
			destination = session->createQueue(destURI);
		}

		// Create a MessageConsumer from the Session to the Topic or Queue
		consumer = session->createConsumer(destination);
		consumer->setMessageListener(this);

		ALERT("MQ: The_consumer_instance <%s>: MQ Create Success!",
			  session_name);

	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		ALERT("MQ: The_consumer_instance <%s>: Connect MQ failed!",
			  session_name);
		onException(e);

		flg_conn_state = FLG_MQ_FALSE;
		pthread_mutex_unlock(&mutex);
		return FLG_MQ_FALSE;

	}
	flg_mq_error = FLG_MQ_ERROR_NO;
	flg_check_mq = 1;
	flg_run = FLG_MQ_TRUE;
	flg_conn_state = FLG_MQ_TRUE;
	ALERT("MQ: The_consumer_instance <%s>: Connect MQ Success!",
		  session_name);
	pthread_mutex_unlock(&mutex);
	return FLG_MQ_TRUE;
}

/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_consumer::onMessage(const Message* message) throw ()
{
	pthread_mutex_lock(&mutex);
	try
	{
		recv_msg(message);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
	}
	pthread_mutex_unlock(&mutex);
	return;
}

/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_consumer::close()
{
	this->clean_up();
	flg_check_mq = 0;
	DEBUG("The class_mq_consumer.%s:监听会话close()", session_name);
}

/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_consumer::clean_up()
{
	// Destroy resources.
	pthread_mutex_lock(&mutex);
	try
	{
		if (destination != NULL)
			delete destination;
	}
	catch (CMSException& e)
	{
	}
	destination = NULL;

	try
	{
		if (consumer != NULL)
			delete consumer;
	}
	catch (CMSException& e)
	{
	}
	consumer = NULL;

	// Close open resources.
	try
	{
		if (session != NULL)
			session->close();
		if (connection != NULL)
			connection->close();
	}
	catch (CMSException& e)
	{
	}

	// Now Destroy them
	try
	{
		if (session != NULL)
			delete session;
	}
	catch (CMSException& e)
	{
	}
	session = NULL;

	try
	{
		if (connection != NULL)
			delete connection;
	}
	catch (CMSException& e)
	{
	}
	connection = NULL;
	flg_conn_state = FLG_MQ_FALSE;
	flg_run = FLG_MQ_FALSE;
	flg_transport_sta = FLG_MQ_FALSE;
	pthread_mutex_unlock(&mutex);
}
/*
 * 功能：异常处理
 * 参数:
 * 返回：
 */
void class_mq_consumer::onException(const CMSException& ex AMQCPP_UNUSED)
{

	DEBUG("consumer instance<%s> : exception!\n",session_name);
	flg_mq_error = FLG_MQ_ERROR_YES;
}

/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_consumer::transportInterrupted()
{
	flg_transport_sta = FLG_MQ_FALSE;
	flg_conn_state = FLG_MQ_FALSE;
}


/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_consumer::transportResumed()
{
	flg_transport_sta = FLG_MQ_TRUE;
	if (flg_run == FLG_MQ_TRUE)
	{
		DEBUG("%s:监听会话run_do()恢复<<<<<<<<<<<<<<<<<<<<<<\n", session_name);
		flg_conn_state = FLG_MQ_TRUE;
	}
}



int class_mq_consumer::do_stat_check(void)
{
	if (get_mq_conn_state() != FLG_MQ_TRUE)
	{
		count++;
		if (count % 10 == 0)
		{
			DEBUG("%s 已经失效时间=%d 秒!\n",session_name,count);
		}
	}
	else
	{
		count = 0;
	}

	if (count > MAX_MQ_CHECK_RESET || (get_mq_conn_state() != FLG_MQ_TRUE
	                                   && flg_transport_sta == FLG_MQ_TRUE))
	{
		DEBUG("%s 超时 kill self！\n", session_name);
		char str[512];
		sprintf(str, "%s 超时 kill self！\n", session_name);

		for (int i = 0; i < 1; i++)
		{
			close();
			usleep(100);
			if (run() == FLG_MQ_TRUE)
			{
				count = 0;
				break;
			}
		}
	}

	return 0;
}





/*
 * 功能：
 * 参数：
 * 返回：//获得此实例MQ是否可用;	FLG_MQ_TRUE:可用; FLG_MQ_FALSE:不可用;
 */
int class_mq_consumer::get_mq_conn_state(void)
{
	int sta;
	pthread_mutex_lock(&mutex);
	sta = flg_conn_state;
	pthread_mutex_unlock(&mutex);
	return sta;
}




/**************************************************************************
 * 功能：构造函数
 * 参数：
 * 		brokerURI:	   连接mq服务器的uri
 * 		destURI:  	   topic/queue的名称
 * 		session_name:   会话名称
 * 		useTopic:	          是否topic，不是则采用queue
 *		clientAck:	   客户端确认方式.  true--客户端确认, false--自动确认
 * 返回：无
 *
**************************************************************************/
class_mq_producer::class_mq_producer(
    const std::string& brokerURI, const std::string& destURI,
    const char *session_name, bool useTopic, bool clientAck) :
	connection(NULL), session(NULL), destination(NULL), producer(NULL),
	useTopic(useTopic), clientAck(clientAck), brokerURI(brokerURI),
	destURI(destURI), flg_mq_error(FLG_MQ_ERROR_NO), session_name(session_name)
{
	pthread_mutex_init(&mutex_state, NULL);
	flg_check_mq = 0;
	flg_run = FLG_MQ_FALSE;
	count = 0;
	flg_transport_sta = FLG_MQ_FALSE;
	flg_conn_state = FLG_MQ_FALSE;
}

/**************************************************************************
 * 功能：析构函数
 * 参数：
 * 返回：
**************************************************************************/
class_mq_producer::~class_mq_producer() throw ()
{
	this->clean_up();
}



/**************************************************************************
 * 功能：	MQ消息生产者运行
 * 参数：无
 * 返回： 1--成功, 0--失败
**************************************************************************/

int class_mq_producer::run()
{
	pthread_mutex_lock(&mutex_state);
	try
	{
		auto_ptr<ActiveMQConnectionFactory> connectionFactory(
		    new ActiveMQConnectionFactory(brokerURI));

		connectionFactory->setUseAsyncSend(true);

		connectionFactory->setSendTimeout(10056);

		// Create a Connection
		try
		{
			connection = connectionFactory->createConnection();

			if(connection!=NULL)
			{
				this->connection->setExceptionListener(this);

				ActiveMQConnection* amqConnection =
					dynamic_cast<ActiveMQConnection*> (connection);

				if (amqConnection != NULL)
				{
					amqConnection->addTransportListener(this);

				}
                //1第一次进行连接或者断线进行重连
                //连接成功都会进入transportResumed函数
				connection->start();


			}
			else
			{
				flg_conn_state = FLG_MQ_FALSE;
				pthread_mutex_unlock(&mutex_state);
				return 0;
			}
		}
		catch (CMSException& e)
		{
			DEBUG("create mq factory connection false!\n");
			e.printStackTrace();
			onException(e);
			throw e;
		}

		DEBUG("getSendTimeout = %d \n", connectionFactory->getSendTimeout());

		// Create a Session
		if (clientAck)
		{
			session = connection->createSession(Session::CLIENT_ACKNOWLEDGE);
		}
		else
		{
			session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
		}

		// Create the destination (Topic or Queue)
		if (useTopic)
		{
			destination = session->createTopic(destURI);
		}
		else
		{
			destination = session->createQueue(destURI);
		}

		producer = session->createProducer(destination);
		producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);
		flg_mq_error = FLG_MQ_ERROR_NO;

	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		log_send(LOG_LEVEL_STATUS,0,"MQ:","The_producer_instance <%s> :Connect MQ failed!\n",session_name);
		onException(e);
		flg_conn_state = FLG_MQ_FALSE;
		pthread_mutex_unlock(&mutex_state);
		return 0;
	}
	flg_mq_error = FLG_MQ_ERROR_NO;
	flg_check_mq = 1;

	flg_run = FLG_MQ_TRUE;
	flg_conn_state = FLG_MQ_TRUE;
	log_send(LOG_LEVEL_STATUS,0,"MQ:","The_producer_instance <%s> : Connect MQ Success!\n",session_name);
	pthread_mutex_unlock(&mutex_state);
	return 1;
}


/**************************************************************************
 * 功能：	MQ消息生产者关闭
 * 参数：无
 * 返回：
**************************************************************************/
void class_mq_producer::close()
{
	this->clean_up();
	flg_conn_state = FLG_MQ_FALSE;
	flg_check_mq = 0;
	log_send(LOG_LEVEL_STATUS,0,"MQ:","The class_mq_producer.%s  ===========Close!!!!===========\n",session_name);
}

/*
 * 功能：
 * 参数：
 * 返回：
 */
void class_mq_producer::clean_up()
{
	int flag_connection_valid=1;

	// Destroy resources.
	pthread_mutex_lock(&mutex_state);

	try
	{
		if (destination != NULL)
		{
			DEBUG("to delete destination %p\n", destination);
			delete destination;
		}

	}
	catch (CMSException& e)
	{
		DEBUG("delete destination : Exception\n");
		e.printStackTrace();
	}
	destination = NULL;

	try
	{
		if (producer != NULL)
		{
			DEBUG("to delete producer \n");
			delete producer;
		}

	}
	catch (CMSException& e)
	{
		DEBUG("delete producer : Exception\n");
		e.printStackTrace();
	}
	producer = NULL;

	// Close open resources.
	try
	{
		if (session != NULL)
		{
			DEBUG("to close session\n");
			session->close();
			flag_connection_valid=1;
		}
		else
		{
			flag_connection_valid=0;
		}

		if (connection != NULL)
		{
			DEBUG("to close connection\n");

			connection->close();
		}

	}
	catch (CMSException& e)
	{
		DEBUG("close : Exception\n");
		e.printStackTrace();
	}

	try
	{
		if (session != NULL)
		{
			DEBUG("to delete session\n");
			delete session;
		}

	}
	catch (CMSException& e)
	{
		DEBUG("delete session : Exception\n");
		e.printStackTrace();
	}
	session = NULL;

	try
	{
		if (connection != NULL)
		{
			delete connection;
		}

	}
	catch (CMSException& e)
	{
		DEBUG("delete connection : Exception\n");
		e.printStackTrace();
	}
	connection = NULL;
	flg_conn_state = FLG_MQ_FALSE;
	flg_run = FLG_MQ_FALSE;
	DEBUG("pthread_mutex_unlock\n");
	pthread_mutex_unlock(&mutex_state);

	log_send(LOG_LEVEL_STATUS,0,"MQ:","class_mq_producer <%s> ====cleanup!!!!!====   flg_conn_state and flg_run set to false!!\n",session_name);
}
/**************************************************************************
 * 功能：异常监听函数，如果MQ服务器出现异常则会进入此函数处理
 		      前提是在run 中设置了	this->connection->setExceptionListener(this);
 * 参数：CMSException类的引用
 * 返回：
**************************************************************************/
void class_mq_producer::onException(const CMSException& ex AMQCPP_UNUSED)
{
	log_send(LOG_LEVEL_STATUS,0,"MQ:","producer instance <%s>: exception!\n",session_name);

	flg_mq_error = FLG_MQ_ERROR_YES;
}


/**************************************************************************
 * 功能：MQ 连接中断 处理，如果设备与MQ服务器连接中断，并不会进onExceptin处理
 			而直接进入此函数，要调用此函数 需要在run 中做如下操作:
 			ActiveMQConnection* amqConnection =
			    dynamic_cast<ActiveMQConnection*> (connection);
			if (amqConnection != NULL)
			{
				amqConnection->addTransportListener(this);
			}
			在addTransportListener 中会有transportInterrupted和transportResumed

 * 参数：
 * 返回：
**************************************************************************/
void class_mq_producer::transportInterrupted()
{
	//pthread_mutex_lock(&mutex_state);
	flg_conn_state = FLG_MQ_FALSE;
	flg_transport_sta = FLG_MQ_FALSE;

	log_send(LOG_LEVEL_STATUS,0,"MQ:","The class_mq_producer Interrupted!!!!!  <<<<<<<<<<<<<<<%s>>>>>>>>>>>>>>>\n",session_name);

	//pthread_mutex_unlock(&mutex_state);

}


/**************************************************************************
 * 功能：MQ 连接中断 处理，如果设备与MQ服务器重新连接，并不会进入onException
 			而直接进入此函数
 * 参数：
 * 返回：
**************************************************************************/
void class_mq_producer::transportResumed()
{
	log_send(LOG_LEVEL_STATUS,0,"MQ:", "The  class_mq_producer  Restored!!!!! <<<<<<<<<<<<<<<%s>>>>>>>>>>>>>>\n",session_name);
	flg_transport_sta = FLG_MQ_TRUE;

	flg_run = flg_conn_state = FLG_MQ_TRUE;
	DEBUG("%s:生产者类,创建MQ会话run_do()恢复>>>>>>>>>>>>>>>>>>>>===================================>>>\n",
	         session_name);
}


/*
 * 功能：
 * 参数：
 * 返回：
 */
int class_mq_producer::do_stat_check(void)
{
	if (get_mq_conn_state() != FLG_MQ_TRUE)
	{
		count++;
		if (count % 10 == 0)
		{
			DEBUG("%s 已经失效时间=%d 秒!\n", session_name, count);
		}

	}
	else
	{
		count = 0;
	}

	if (count > MAX_MQ_CHECK_RESET || (get_mq_conn_state() != FLG_MQ_TRUE
	                                   && flg_transport_sta == FLG_MQ_TRUE))
	{

		DEBUG("%s 超时 kill self！\n", session_name);
		char str[512];
		sprintf(str, "%s 超时 kill self！\n", session_name);
		//write_err_log_noflash(str);
		for (int i = 0; i < 1; i++)
		{
			close();
			usleep(100);
			if (run() == 1)
			{
				count = 0;
				break;
			}
		}
	}

	return 0;
}



/*
 * 功能：发送带int属性值的textMessage
 * 参数：
 * 		msg: 消息内容
 * 		type: 消息类型
 * 		property: 属性名称
 * 		value: 属性值
 * 返回：    >0:成功; <0失败
 */
int class_mq_producer::send_msg_with_property_text(char *msg, char *type,
        const char *property, int value)
{
	TextMessage* msg_text;
	char str[512];
	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);
	msg_text = session->createTextMessage();

	try
	{
		//		msg_text->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		msg_text->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		if (type != NULL)
		{
			msg_text->setCMSType(type);
		}

		if (property != NULL)
		{
			msg_text->setIntProperty(property, value);
		}

		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_text() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		DEBUG(str);
		return FLG_MQ_FALSE;
	}
	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);

	return FLG_MQ_TRUE;
}


/*
 * 功能：发送带int属性值的textMessage
 * 参数：
 * 		msg: 消息内容
 * 		type: 消息类型
 * 		property: 属性名称
 * 		value: 属性值
 * 		property2: 属性名称
 * 		value2: 属性值
 * 返回：    >0:成功; <0失败
 */
int class_mq_producer::send_msg_with_property_text(char *msg, char *type,
        const char *property, int value,const char *property2, int value2)
{
	TextMessage* msg_text;
	char str[512];
	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);
	msg_text = session->createTextMessage();

	try
	{
		//		msg_text->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		msg_text->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		if (type != NULL)
		{
			msg_text->setCMSType(type);
		}

		if (property != NULL)
		{
			msg_text->setIntProperty(property, value);
		}

		if (property2 != NULL)
		{
			msg_text->setIntProperty(property2, value2);
		}

		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_text() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		DEBUG(str);
		return FLG_MQ_FALSE;
	}
	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);

	return FLG_MQ_TRUE;
}


/*
 * 功能：发送带int属性值的textMessage
 * 参数：
 * 		msg: 消息内容
 * 		type: 消息类型
 * 		property: 属性名称
 * 		value: 属性值
 * 		property2: 属性名称
 * 		value2: 属性值
 * 返回：    >0:成功; <0失败
 */
int class_mq_producer::send_msg_with_property_text(char *msg, char *type,
        const char *property, int value,const char *property2, char* value2)
{
	TextMessage* msg_text;
	char str[512];
	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);
	msg_text = session->createTextMessage();

	try
	{
		//		msg_text->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		msg_text->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		if (type != NULL)
		{
			msg_text->setCMSType(type);
		}

		if (property != NULL)
		{
			msg_text->setIntProperty(property, value);
		}

		if (property2 != NULL)
		{
			msg_text->setStringProperty(property2, value2);
		}

		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_text() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		DEBUG(str);
		return FLG_MQ_FALSE;
	}
	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);

	return FLG_MQ_TRUE;
}

/*
 * 功能：发送带string属性值的textMessage
 * 参数：
 * 		msg: 消息内容
 * 		type: 消息类型
 * 		property: 属性名称
 * 		value: 属性值
 * 返回：    >0:成功; <0失败
 */
int class_mq_producer::send_msg_with_property_text(char *msg, char *type,
        const char *property, char *value)
{
	char str[512];
	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);

	TextMessage* msg_text = session->createTextMessage();
	try
	{
		//msg_text->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		msg_text->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		if (type != NULL)
		{
			msg_text->setCMSType(type);
		}

		if (property != NULL)
		{
			msg_text->setStringProperty(property, value);
		}

		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_text() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		return FLG_MQ_FALSE;
	}

	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}

/*
 * 功能：发送带string属性值的textMessage
 * 参数：
 * 		msg: 消息内容
 * 		type: 消息类型
 * 		property: 属性名称
 * 		value: 属性值
 * 		property也: 属性名称
 * 		value2: 属性值
 * 返回：    >0:成功; <0失败
 */
int class_mq_producer::send_msg_with_property_text(char *msg, char *type,
        const char *property, char *value, const char *property2, int value2)
{
	char str[512];
	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);

	TextMessage* msg_text = session->createTextMessage();
	try
	{
		//		msg_text->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		msg_text->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		if (type != NULL)
		{
			msg_text->setCMSType(type);
		}

		if (property != NULL)
		{
			msg_text->setStringProperty(property, value);
		}
		if (property2 != NULL)
		{
			msg_text->setIntProperty(property2, value2);
		}

		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_text() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		return FLG_MQ_FALSE;
	}

	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}

/***********************************************************************
Function:			send_msg_test()
Description:		发送文本消息内容，setText参数为string类
Return:			 >0: 成功; <0:失败
Author:			lxd
Date:			2014.9.22
***********************************************************************/

int class_mq_producer::send_msg_text_string(const std::string& msg)
{
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		log_send(LOG_LEVEL_STATUS,0,"VD:","生产者类[%s],连接已断开",session_name);
		return FLG_MQ_FALSE;
	}
	pthread_mutex_lock(&mutex_state);
	TextMessage* msg_text = session->createTextMessage();
	try
	{
		if (msg.empty() == 0)
		{
			msg_text->setText(msg);
		}

		producer->send(msg_text);
	//	delete msg_text;
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		delete msg_text;
		pthread_mutex_unlock(&mutex_state);
		return FLG_MQ_FALSE;
	}
	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}





/*
 * 功能：发送textMessage
 * 参数：
 * 		msg: 文本消息内容
 * 返回：
 * 		>0: 成功; <0:失败
 */
int class_mq_producer::send_msg_text(char *msg)
{
	usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}
	pthread_mutex_lock(&mutex_state);
	TextMessage* msg_text = session->createTextMessage();
	try
	{
		if (msg != NULL)
		{
			msg_text->setText(msg);
		}
		producer->send(msg_text);
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		delete msg_text;
		pthread_mutex_unlock(&mutex_state);
		return FLG_MQ_FALSE;
	}
	//usleep(1000);
	delete msg_text;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}

/*
 * 功能：发送附带2个属性的 bytemessage
 * 参数：
 * 		msg:消息指针
 * 		len:消息字节数
 * 		str_property0: 第一个属性名称
 * 		property_value0:第一个属性值
 * 	 	str_property1: 第二个属性名称
 * 		property_value1:第二个属性值
 * 返回： >0:成功;<0失败
 */
int class_mq_producer::send_msg_with_property_byte(void *msg, int len,
        char* type, const char *str_property0, int property_value0,
        const char *str_property1, int property_value1)
{
	BytesMessage* message = NULL;
	char str[512];

	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("生产者类[%s],  连接已经断开...\n", session_name);
		return FLG_MQ_FALSE;
	}

	pthread_mutex_lock(&mutex_state);
	try
	{
		message = session->createBytesMessage();
	}
	catch (CMSException& e)
	{
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		sprintf(str, "[%s] send_msg_with_property_byte() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		delete message;
		return FLG_MQ_FALSE;
	}

	try
	{
		if (str_property0 != NULL)
		{
			message->setIntProperty(str_property0, property_value0);
		}
		if (str_property1 != NULL)
		{
			message->setIntProperty(str_property1, property_value1);
		}

		//message->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		message->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		message->setCMSType(type);

		message->setCMSExpiration(1000*6*5);
		if (msg != NULL)
		{
			message->writeBytes((const unsigned char *) msg, 0, len);
		}
		producer->send(message);

		flg_mq_error = FLG_MQ_ERROR_NO;
	}
	catch (CMSException& e)
	{
		onException(e);
		e.printStackTrace();
		pthread_mutex_unlock(&mutex_state);

		sprintf(str, "[%s] send_msg_with_property_byte() type=%d 失败!\n",
		        session_name, *type);
		//		write_err_log_noflash(str);
		delete message;
		return FLG_MQ_FALSE;

	}
	delete message;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}

/**
 * 功能：发送byteMessage
 * 参数：
 * 		msg: 消息指针
 * 		len: 消息字节数
 *      enum_type: 消息类型
 * 返回：
 */
int class_mq_producer::send_msg_byte(char *msg, int len, char enum_type)
{
	BytesMessage* message;
	char type[3];

	//usleep(10000);
	if (flg_conn_state != FLG_MQ_TRUE)
	{
		DEBUG("send msg faile ,  连接已经断开...\n");
		return FLG_MQ_FALSE;
	}
	pthread_mutex_lock(&mutex_state);
	try
	{
		message = session->createBytesMessage();
	}
	catch (CMSException& e)
	{

		DEBUG("message= session->createBytesMessage()faile....\n");
		e.printStackTrace();
		onException(e);
		pthread_mutex_unlock(&mutex_state);
		return FLG_MQ_FALSE;
	}

	//	DEBUG("producter msg.....\n");
	sprintf(type, "%d", enum_type);
	try
	{
		//pMessage_queue->setIntProperty("IsRequest", 0);
		//message->setCMSCorrelationID(g_arm_config.basic_param.device_id);
		message->setCMSCorrelationID(g_set_net_param.m_NetParam.m_DeviceID);
		message->setCMSType(type);
		message->writeBytes((const unsigned char *) msg, 0, len);

		if (flg_conn_state != FLG_MQ_TRUE)
		{
			DEBUG("send msg faile ,  连接已经断开...\n");
			delete message;
			pthread_mutex_unlock(&mutex_state);
			return FLG_MQ_FALSE;
		}
		//		send( Message* message, int deliveryMode, int priority, long long timeToLive ) = 0;
		producer->send(message);

		DEBUG("生产者发送完消息:%s  >> \n", msg);

		flg_mq_error = FLG_MQ_ERROR_NO;
	}
	catch (CMSException& e)
	{
		DEBUG("send msg faile.....\n");
		onException(e);
		e.printStackTrace();
		pthread_mutex_unlock(&mutex_state);

		delete message;
		return FLG_MQ_FALSE;

	}

	delete message;
	pthread_mutex_unlock(&mutex_state);
	return FLG_MQ_TRUE;
}




/*
 * 功能：
 * 参数：
 * 返回：
 */
int class_mq_producer::get_error(void) //获得MQ异常发生;
{
	int sta;
	pthread_mutex_lock(&mutex_state);
	sta = flg_mq_error;
	pthread_mutex_unlock(&mutex_state);
	return sta;
}

/*
 * 功能: 获取该实例连接状态
 * 参数: 无
 * 返回: FLG_MQ_TRUE:正常; FLG_MQ_FALSE:断开;
 */
int class_mq_producer::get_mq_conn_state(void)
{
	int sta;
	pthread_mutex_lock(&mutex_state);
	sta = flg_conn_state;
	pthread_mutex_unlock(&mutex_state);
	return sta;
}
