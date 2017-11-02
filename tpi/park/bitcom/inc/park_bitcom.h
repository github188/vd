#ifndef __PARK_BITCOM_H__
#define __PARK_BITCOM_H__

#include "platform.h"

typedef struct
{
	bool enable;
    bool retransmition_enable;
    unsigned int rotate_count;
}bitcom_platform_info_t;

class CParkBitcom : public CPlatform
{
public:
	CParkBitcom(void);
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
	virtual string get_platform_type_str(void) { return "bitcom"; }
private:
	bitcom_platform_info_t m_platform_info;
};

#endif
