/**
 * @file platform_thread.h
 * @brief load platform_set.cfg and start platforms.
 * @author felixdu
 * @date 2017-02-09
 * @version 0.1
 * <pre><b>copyright: bitcom</b></pre>
 * <pre><b>email: </b>durd_bitcom@163.com</pre>
 * <pre><b>company: </b>http://www.microsoft.com</pre>
 * <pre><b>All rights reserved.</b></pre>
 */
#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <list>
#include <map>
#include <json/json.h>
using namespace std;

enum class EDeviceType
{
	PARK, /**< 泊车 */
	VP,   /**< 违停 */
	VM    /**< 简易卡口 */
};

enum class EPlatformType
{
	PARK_BITCOM, /**< 比特平台 基于MQ FTP */
	PARK_ZEHIN,  /**< 特易停平台 基于VPAAS */
	PARK_SONGLI, /**< 第三方接入平台 基于MQ 阿里云存储 FTP等 */
    PARK_HTTP,   /**< 北京首钢对接的HTTP POST 平台 */
	PARK_KTETC,  /**< 北京易路行技术有限公司 */

	VP_BAOKANG, /**< 宝康平台 */
	VP_UNIVIEW, /**< 宇视平台 */
	VP_BITCOM   /**< 比特平台 */
};

/**
 * @brief 平台控制的总控制
 *
 * 包括平台的启动，是否需要处理消息，设置/读取配置信息等。
 */
class CPlatform
{
public:
    CPlatform():m_running(false) {}
    ~CPlatform() {}
	virtual int run(void) = 0;
	virtual int stop(void) = 0;
	virtual int subscribe(void) = 0;
	virtual int unsubscribe(void) = 0;
	virtual bool subscribed(void) = 0;
	virtual int notify(int type, void* data) = 0;
	virtual int set_platform_info(void *) = 0;
	virtual int set_platform_info(const Json::Value & root) = 0;
	virtual int set_platform_info(const string & json_str) = 0;
	virtual void* get_platform_info(void) = 0;
	virtual string get_platform_info_str(void) = 0;
	virtual Json::Value get_platform_info_json(void) = 0;
    virtual bool is_enable(void) = 0;
	bool is_running(void) { return m_running; }
	EPlatformType get_platform_type(void) { return m_platform_type; };
	virtual string get_platform_type_str(void) = 0;
protected:
	bool m_running;
	EPlatformType m_platform_type;
};

/**
 * 所有的平台都put到这个list里，不过同一时刻只会代表一种设备类型在运行，
 * 所以只会有一种设备类型的平台。
 */
extern list<CPlatform*> g_platform_list;
extern EDeviceType g_device_type;
#endif
