#ifndef __PARK_KTETC_H__
#define __PARK_KTETC_H__

#include <string.h>
#include "park_records_process.h"
#include "platform.h"

#define KTETC_TIME_LENGTH (15)
#define KTETC_FLOWID_LENGTH (22)

#define KTETC_DRIVE_IN (0x01)
#define KTETC_DRIVE_OUT (0x00)

#define KTETC_ABSOLUTE_PATH ("/mnt/mmc/ktetc/")
#define KTETC_DB_PATH ("/mnt/mmc/ktetc/ktetc.db")

enum KTETC_MSG_TYPE
{
	KTETC_VIDEO = 1,
	KTETC_HIGHVIDEO,
	KTETC_GEOMAGNETISM,
	KTETC_ABNORMAL,
	KTETC_DEVICE_FAULT
};

typedef struct
{
	bool enable;
	string comType;  /**< 厂商编码 101 */
	string parkCode; /**< 停车场编码，试验场固定为9 位899000000 */
	string devCode;  /**< 设备号，6 位，自定义 */
	string psCode;   /**< 泊位号，西沙屯泊位序号范围：A0001-
						  A0009(视频桩和地磁)，A1001-A1020（高点
						  视频）,A2001-2020（高点视频）*/
	string url;
}ktetc_platform_info_t;

typedef struct
{
	string comType;      /**< 厂商编码 109 */
	string IDdataTime;   /**< 时间戳，格式：YYYYMMDDHHmmss，如
						      20171010123059，表示记录生成时间 */
	string flowId;		 /**< 记录惟一编码，格式：3 位厂商编码+1 位消
						      息类型代码+13 位时间+3 位序号，如
					          102320171011121314001，表示：102-优先
					          科技，3-地磁（1-高点视频，2-视频桩），
					          20171011121314-年月日时分秒，001-流水号 */
	string parkCode;     /**< 停车场编码，试验场固定为9 位899000000 */
	string devCode;		 /**< 设备号，6 位，自定义 */
	string psCode;		 /**< 泊位号，西沙屯泊位序号范围：A0001-
						 	  A0009(视频桩和地磁)，A1001-A1020（高点
							  视频）,A2001-2020（高点视频）*/
	unsigned int inOutState;   /**< 车辆驶入驶出状态，1 驶入，0 驶出 */
	string vehPlate;	 /**< 号牌号码，无牌、未识别、无法识别等用“-”或者“0”表示。*/
	unsigned int confidence;	 /**< 车牌置信度，范围0 到100，值越大，置信度越高。*/
	string plateColor;	 /**< 号牌颜色,符合《GA 24.7-2005 机动车登记
							  信息代码 第7 部分：号牌种类代码》*/
	string vehColor;	 /**< 车身颜色,符合《GA 24.8-2005 机动车登记
							  信息代码 第7 部分：车身颜色基本色调代码》*/
	unsigned int vehType;		 /**< 车辆类型:1 小型，2 中型，3 大型，4 其他 */
	string Image1;	 /**< 全景图相对存储路径,如:102320171011121314001\1.jpg*/
	string Image2;	 /**< 全景图相对存储路径,如:102320171011121314001\2.jpg*/
	string Image3;	 /**< 全景图相对存储路径,如:102320171011121314001\3.jpg*/
	string Image4;	 /**< 全景图相对存储路径,如:102320171011121314001\4.jpg*/
	string FiledataTime; /**< 时间，车辆驶入或者驶离时间，格式：YYYYMMDDHHmmss,如20171010133059*/
	string inTime;		 /**< 入场时间，当为驶离状态时，填写入场时间，格式：YYYYMMDDHHmmss,如20171010123059*/

	string plateFeature;
	const EP_PicInfo *pic_info;
}ktetc_t;

typedef struct
{
	string comType;      /**< 厂商编码 109 */
	string flowId;		 /**< 记录惟一编码，格式：3 位厂商编码+1 位消
						      息类型代码+13 位时间+3 位序号，如
					          102320171011121314001，表示：102-优先
					          科技，3-地磁（1-高点视频，2-视频桩），
					          20171011121314-年月日时分秒，001-流水号 */
	string parkCode;     /**< 停车场编码，试验场固定为9 位899000000 */
	string devCode;		 /**< 设备号，6 位，自定义 */
	string psCode;		 /**< 泊位号，西沙屯泊位序号范围：A0001-
						 	  A0009(视频桩和地磁)，A1001-A1020（高点
							  视频）,A2001-2020（高点视频）*/
	unsigned int alarmCode; /**< 异常状态:
								1 不规范停车（压线、斜停、跨位等）
								2 视频遮挡
								3 车位非法占用
								4.车牌置信度低
								5.车牌遮挡
								10.恶意欠费
								11.非会员停车
								12.设备故障
								99.其他*/
	string alarmTime; /**< 异常时间，格式：YYYYMMDDHHmmss,如20171010123059*/
	unsigned int alarmLevel; /**< 异常级别，1 低、2 中、3 高 */
}ktetc_alarm_t;

class CParkKtetc : public CPlatform
{
public:
	CParkKtetc(void);
	~CParkKtetc(void);
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
	virtual string get_platform_type_str(void) { return "ktetc"; }

public:
	void run_thread(void);
	void hb_thread(void);
	void retrans_thread(void);

	bool retransmition_enable(void) {return true;}

private:
    int alarm_handler(ParkAlarmInfo* alarm);
	int record_handler(const struct Park_record* record);
    int info_handler(struct Park_record* record);

	int get_record_time(char *record_time);

	int time_convert(unsigned int from, char *to);
	int time_convert(const char *from, char *to);

	int construct_flowId(const char *time, char *out);
	int construct_inTime(const struct Park_record *record, char *inTime);

	int bitcom2ktetc(const struct Park_record *record, ktetc_t *ktetc);
	int bitcom2ktetc_alarm(const ParkAlarmInfo *alarm, ktetc_alarm_t *ktetc_alarm);

	int save_record2database(const ktetc_t *k);
	int	save_record_pictures(const ktetc_t *k, const struct Park_record *r);
	int save_alarm2database(const ktetc_alarm_t *k);

	int send_record2platform(const ktetc_t *k);
	int send_alarm2platform(const ktetc_alarm_t *k);

	void ktetc_register();
private:
	ktetc_platform_info_t m_platform_info;
	pthread_t tid;
	pthread_t m_hb_tid;
	pthread_t m_retrans_tid;
};
#endif /* __PARK_KTETC_H__ */
