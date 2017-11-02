#ifndef _TVPUNIVIEW_PROTOCOL_H_
#define _TVPUNIVIEW_PROTOCOL_H_

#define MAX_XML_LENS_1M    (1024*1024*1)
#define MAX_PIC_LENS_5M    (1024*1024*5)
#define MAX_PIC_NUM_4      (4)


#define HEART_BEATE_CMD    (101)
#define RT_RECORDS_CMD     (115)



//tvp uniview data
typedef struct
{
	/* 卡口相机编号 */
    char szDevCode[32];
	
	/* 记录ID号 */
	char szRecordID[17];

	/* 卡口编号 */
	char szTollgateID[32];

	/* 经过时刻 */
	char szPassTime[32];

	/* 地点编码，可以为空，最大32位 */
	char szPlaceCode[32];

    /* 车道编号 */
    int ulLaneID;

    /* 车牌号码 */
    char szCarPlate[32];

    /* 车牌颜色 0-白色1-黄色 2-蓝色 3-黑色 4-其他 */
    int ulPlateColor;

	/* 车牌数量 */
	int ulPlateNum;

    /* 车速 */
    int ulVehSpeed;

	/* 限速 */
	int ulLimitedSpeed;

	/* 识别状态 0－识别成功 1－不成功 2－不完整 */
	int ulIdentifyStatus;

    /* 识别时间，单位毫秒 */
    int ulIdentifyTime;

    /* 车牌种类 1－单排 2－武警 3－警用 4－双排  5－其他 */
    char szPlateType[3];

    /* 车身颜色 A：白，B：灰，C：黄，D：粉，E：红，F：紫，G：绿，H：蓝，I：棕，J：黑，K：橙，L：青，M：银，N：银白，Z：其他 */
    char szVehBodyColor;

    /* 车身颜色深浅 0-未知，1-浅，2-深 */
    int ulVehColorDepth;

    /* 行驶方向 1-东向西，2-西向东，3-南向北，4-北向南，5-东南向西北，6-西北向东南，7-东北向西南，8-西南向东北 */
    int ulVehDirection;

    /* 车长，单位：厘米 */
    int ulVehLength;

    /* 车辆类型 0-未知，1-小型车，2-中型车，3-大型车，4-其他*/
    int ulVehtype;

    /* 车道类型 0-机动车道，1-非机动车道 */
    int ulLaneType;

    /* 车辆品牌 */
    char szVehBrand[3];

    /* 行人衣着颜色 */
    int ulDressColor;

    /* 违法类型 0-卡口，1-超速，4-闯红灯 */
    int ulTransgressType;

    /* 红灯时间 */
    int ulRedLightTime;

    /* 所有图片数量 或者URL数量*/
    int ulPicNum;

    /* 采集设备编码，可以为空，最大32位 */
    char szEquipmentCode[64];  

	/*应用类型*/
	int ulApplicationType;

	/*全局合成标志城*/
	int ulGlobalComposeFlag;

    /* 保留字段 */
    int aulReserved[4];
	
}str_tvpuniview_xml;



//tvp uniview data
typedef struct
{
	int  xml_lens;
	str_tvpuniview_xml xml;
	int  pic_num;
	int  pic_lens[MAX_PIC_NUM_4];
	char pic_data[MAX_PIC_NUM_4][MAX_PIC_LENS_5M];
}str_tvpuniview_data;

//tvp uniview heart beate
typedef struct
{
	char dev_code[32];
}str_tvpuniview_heartbeate;

//tvpuniview frame 
typedef struct
{
	//------------
	unsigned long frame_head;
	unsigned long frame_tail;
	unsigned long frame_lens;
	unsigned long version;
	unsigned long frame_cmd;

	str_tvpuniview_data data;
	str_tvpuniview_heartbeate heart_beate;
}str_tvpuniview_protocol;



extern int tvpuniview_encode(str_tvpuniview_protocol *as_tvpuniview_protocol, char *oc_data);





#endif




