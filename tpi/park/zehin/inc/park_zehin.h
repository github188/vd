#ifndef __PARK_ZEHIN_H__
#define __PARK_ZEHIN_H__

#include "platform.h"

typedef struct _platform_info_t
{
	bool enable;
	bool retransmition_enable;
    bool send_status_when_retransmition;
	bool space_status_enable;
	bool light_ctl_enable; // enable light controlled from platform.
	unsigned int rotate_count;
	string device_id;
	string center_ip;
	string sf_ip;
	unsigned int cmd_port;
	unsigned int heartbeat_port;
	unsigned int sf_port;
}zehin_platform_info_t;

class CParkZehin : public CPlatform
{
public:
	CParkZehin(void);
	virtual int run(void);
	virtual int stop(void);
	virtual int subscribe(void);
	virtual int unsubscribe(void);
	virtual bool subscribed(void);
	virtual int notify(int type, void *data);
	virtual int set_platform_info(void *p);
	virtual void* get_platform_info(void);
	virtual int set_platform_info(const string & json_str);
	virtual int set_platform_info(const Json::Value & root);
	virtual string get_platform_info_str(void);
	virtual Json::Value get_platform_info_json(void);
    virtual bool is_enable(void) { return m_platform_info.enable; }
	virtual string get_platform_type_str(void) { return "zehin"; }

private:
    int alarm_notify(void* data);
    int info_notify(void* data);
    int record_notify(void* data);
private:
	zehin_platform_info_t m_platform_info;
};

#endif
