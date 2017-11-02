#ifndef __PARK_SONGLI_H__
#define __PARK_SONGLI_H__

#include "vd_msg_queue.h"
#include "interface_alg.h"
#include "park_records_process.h"
#include "platform.h"
#include "sem_util.h"

#undef min
#undef max
#include "mq_module.h"

enum class SONGLI_PIC_SRC
{
	FTP,
	ALIYUN
};

enum class SONGLI_STATUS
{
    DISABLE,
    ENABLE,
    READY,
    START, // mq create success
    REGISTERING, // begin send register
    REGISTERED, // registered
    RUNNING,
    LOSTHEARTBEAT,
    STOPPED
};

typedef struct
{
	bool enable;
    bool retransmition_enable;
	bool upload_light_action;
    unsigned int rotate_count;
	SONGLI_PIC_SRC pic_src;
}songli_platform_info_t;

class CParkSongli : public CPlatform
{
public:
	CParkSongli(void);
	~CParkSongli(void);
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
	virtual string get_platform_type_str(void) { return "songli"; }

    bool retrans_enable() { return m_platform_info.retransmition_enable; }

    class_mq_producer* get_upload_mq()
    {
        class_mq_producer* mq = NULL;
        pthread_mutex_lock(&upload_mutex);
        mq = m_mq_park_upload;
        pthread_mutex_unlock(&upload_mutex);
        return mq;
    }

    void set_upload_mq(class_mq_producer* mq)
    {
        pthread_mutex_lock(&upload_mutex);
        m_mq_park_upload = mq;
        pthread_mutex_unlock(&upload_mutex);
    }

    class_mq_consumer* get_down_mq()
    {
        class_mq_consumer* mq = NULL;
        pthread_mutex_lock(&down_mutex);
        mq = m_mq_park_down;
        pthread_mutex_unlock(&down_mutex);
        return mq;
    }

    void set_down_mq(class_mq_consumer* mq)
    {
        pthread_mutex_lock(&down_mutex);
        m_mq_park_down = mq;
        pthread_mutex_unlock(&down_mutex);
    }

    SONGLI_STATUS get_status()
    {
        SONGLI_STATUS status;
        pthread_rwlock_rdlock(&status_rwlock);
        status = m_songli_status;
        pthread_rwlock_unlock(&status_rwlock);
        return status;
    }

    void set_status(SONGLI_STATUS status)
    {
        pthread_rwlock_wrlock(&status_rwlock);
        m_songli_status = status;
        pthread_rwlock_unlock(&status_rwlock);
    }

public:
    void run_thread(void);
    void hb_thread(void);
    void retrans_thread(void);
    int send_msg_2_songli_platform(string &msg);

private:
    void start_mq_songli(void);
    void register2platform(void);
    void songli_hb(void);
    int alarm_handler(ParkAlarmInfo* alarm);
    int record_handler(struct Park_record* record);
    int info_handler(struct Park_record* record);
    int light_handler(Lighting_park *light_info);
    int park_send_record_songli(string &result);
    int park_send_picture_songli(EP_PicInfo pic_info[], const int pic_num);
    int save_picture4retrans(EP_PicInfo pic_info[], const int pic_num);
    int songli_database_init(void);
private:
	songli_platform_info_t m_platform_info;

    pthread_t tid;
    pthread_t songli_hb_tid;
    pthread_t songli_retrans_tid;

    pthread_mutex_t upload_mutex;
    pthread_mutex_t down_mutex;
	class_mq_producer *m_mq_park_upload;           //泊车上传json到中心服务
	class_mq_consumer *m_mq_park_down;

    pthread_rwlock_t status_rwlock;
    SONGLI_STATUS m_songli_status;
	SemHandl_t park_sem;
};
#endif
