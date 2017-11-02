
#ifndef _TRAFFIC_RECORDS_PROCESS_H_
#define _TRAFFIC_RECORDS_PROCESS_H_


#include "upload.h"
//#include "alg_result.h"
#include "mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"
#include "mcfw/interfaces/ti_media_std.h"	/* WTF! */
#include "mcfw/interfaces/link_api/systemLink_m3vpss.h"
#include "mcfw/interfaces/link_api/jpeg_enc_info.h"
#include "mcfw/interfaces/link_api/videoAnalysisLink_parm.h"




/*********************大华车辆特征宏定义*****************************/

//车牌颜色
#define DAHUA_PLATE_BLUE      (0)                   //车牌蓝
#define DAHUA_PLATE_YELLOW    (1)                   //车牌黄
#define DAHUA_PLATE_WHITE     (2)                   //车牌白
#define DAHUA_PLATE_BLACK     (3)                   //车牌黑
#define DAHUA_PLATE_UNKNOW    (99)                  //车牌未识别
#define DAHUA_PLATE_OTHERS    (100)                 //车牌其它

//行驶方向
#define DAHUA_EAST_TO_WEST     (0)              //东向西
#define DAHUA_WEST_TO_EAST     (1)              //西向东
#define DAHUA_NORTH_TO_SOUTH   (2)              //南向北
#define DAHUA_SOUTH_TO_NORTH   (3)              //北向南
#define DAHUA_SOUTHEAST_TO_NORTHWEST   (4)        //东南向西北
#define DAHUA_NORTHWEST_TO_SOUTHEAST   (5)        //西北向东南
#define DAHUA_NORTHEAST_TO_SOUTHWEST   (6)        //东北向西南
#define DAHUA_SOUTHWEST_TO_NORTHEAST   (7)        //西南向东北

//车牌类型
#define DAHUA_PLATE_MINIATURE_VEHICLE  (0)        //小型车辆
#define DAHUA_PLATE_OVERSIZE_VEHICLE   (1)        //大型车辆
#define DAHUA_PLATE_MILITARY_VEHICLE   (2)        //军警型车辆
#define DAHUA_PLATE_FOREIGN_VEHICLE    (3)        //外籍车辆
#define DAHUA_PLATE_UNKONW_VEHICLE     (99)       //未识别车辆
#define DAHUA_PLATE_OTHERS_VEHICLE     (100)      //其它

//车辆类型
#define DAHUA_TYPE_UNKNOW_VEHICLE      (0)        //未识别
#define DAHUA_TYPE_MINIATURE_VEHICLE   (1)        //小型汽车
#define DAHUA_TYPE_OVERSIZE_VEHICLE    (2)        //大型汽车
#define DAHUA_TYPE_EMBASSY_VEHICLE     (3)        //使馆汽车
#define DAHUA_TYPE_CONSULATE_VEHICLE   (4)        //领馆汽车
#define DAHUA_TYPE_OVERSEAS_VEHICLE    (5)        //境外汽车
#define DAHUA_TYPE_FOREIGN_VEHICLE     (6)        //外籍汽车
#define DAHUA_TYPE_LOWSPEED_VEHICLE    (7)        //低速汽车
#define DAHUA_TYPE_TRACTOR_VEHICLE     (8)        //拖拉机
#define DAHUA_TYPE_TRAILER_VEHICLE     (9)        //挂车
#define DAHUA_TYPE_COACH_VEHICLE       (10)       //教练车
#define DAHUA_TYPE_TEMP_VEHICLE        (11)       //临时行驶车
#define DAHUA_TYPE_POLICE_VEHICLE      (12)       //警用汽车
#define DAHUA_TYPE_POLICEMOTOR_VEHICLE (13)       //警用摩托车
#define DAHUA_TYPE_MOTOR_VEHICLE       (14)       //普通摩托车
#define DAHUA_TYPE_LIGHTMOTOR_VEHICLE  (15)       //轻便摩托车

//车辆颜色
#define DAHUA_COLOUR_WHITE_VEHICLE            (0)        //白色
#define DAHUA_COLOUR_BLACK_VEHICLE            (1)        //黑色
#define DAHUA_COLOUR_RED_VEHICLE              (2)        //红色
#define DAHUA_COLOUR_YELLOW_VEHICLE           (3)        //黄色
#define DAHUA_COLOUR_SILVERGREY_VEHICLE       (4)        //银灰色
#define DAHUA_COLOUR_BLUE_VEHICLE             (5)        //蓝色
#define DAHUA_COLOUR_GREEN_VEHICLE            (6)        //绿色
#define DAHUA_COLOUR_ORANGE_VEHICLE           (7)        //橙色
#define DAHUA_COLOUR_PURPLE_VEHICLE           (8)        //紫色
#define DAHUA_COLOUR_SYAN_VEHICLE             (9)        //青色
#define DAHUA_COLOUR_PINK_VEHICLE             (10)       //粉色
#define DAHUA_COLOUR_UNKNOW_VEHICLE           (99)       //未识别
#define DAHUA_COLOUR_OTHERS_VEHICLE           (100)      //其他

//行车状态
#define DAHUA_STATE_NORMAL_VEHICLE         (1)      //正常
#define DAHUA_STATE_NONMOTOR_VEHICLE       (2)      //非机动车
#define DAHUA_STATE_ABNORMAL_VEHICLE       (3)      //异常
#define DAHUA_STATE_INCOMPLETE_VEHICLE     (4)      //残缺

//违法类型
#define DAHUA_ILLEGAL_OVERSPEED_VEHICLE         (1)      //超速
#define DAHUA_ILLEGAL_SUPERLOWSPEED_VEHICLE     (2)      //超低速
#define DAHUA_ILLEGAL_OTHERS_VEHICLE            (3)      //其他
#define DAHUA_ILLEGAL_JAYWALK_VEHICLE           (4)      //闯红灯
#define DAHUA_ILLEGAL_UNLANE_VEHICLE            (5)      //不按车道行驶
#define DAHUA_ILLEGAL_LINEBALL_VEHICLE          (6)      //压线
#define DAHUA_ILLEGAL_RETROGRADE_VEHICLE        (7)      //逆行
#define DAHUA_ILLEGAL_BICYCLELANE_VEHICLE       (8)      //非机动车道
#define DAHUA_ILLEGAL_FLAG_VEHICLE              (10)     //机动车违反禁令标志指示
#define DAHUA_ILLEGAL_PARKING_VEHICLE           (33)     //违章停车
#define DAHUA_ILLEGAL_LANECHANG_VEHICLE         (34)      //违章变道
#define DAHUA_ILLEGAL_PRESSYELLOW_VEHICLE       (35)      //压黄线
#define DAHUA_ILLEGAL_TRAFFICJAM_VEHICLE        (36)      //交通拥堵
#define DAHUA_ILLEGAL_TRAFFICSTOP_VEHICLE       (37)      //交通滞留
#define DAHUA_ILLEGAL_YELLOWLANE_VEHICLE        (38)      //黄牌车占道
#define DAHUA_ILLEGAL_CARLANE_VEHICLE           (39)      //有车占道
#define DAHUA_ILLEGAL_MANUALLYCAPTURE_VEHICLE   (40)      //手动抓拍
#define DAHUA_ILLEGAL_BUSLANE_VEHICLE           (41)      //占用公交车道
#define DAHUA_ILLEGAL_ASTERN_VEHICLE            (42)      //违章倒车
#define DAHUA_ILLEGAL_YELLOWWARK_VEHICLE        (43)      //闯黄灯
#define DAHUA_ILLEGAL_CARPORT_VEHICLE           (44)      //车位有车
#define DAHUA_ILLEGAL_NOCARPORT_VEHICLE         (45)      //车位无车
#define DAHUA_ILLEGAL_SMOKING_VEHICLE           (46)      //吸烟
#define DAHUA_ILLEGAL_PHONE_VEHICLE             (47)      //打手机
#define DAHUA_ILLEGAL_SAFETYBELT_VEHICLE        (48)      //不系安全带
#define DAHUA_ILLEGAL_COVERPLATE_VEHICLE        (49)      //遮挡车牌
#define DAHUA_ILLEGAL_YELLOWSTOP_VEHICLE        (50)      //黄网格违章停车
#define DAHUA_ILLEGAL_UNKNOW_VEHICLE            (51)      //未知


/***************************************************************/




/*********************BITCOM协议宏定义*****************************/
//车牌颜色
#define BITCOM_BLUE      (1)                   //车牌蓝
#define BITCOM_YELLOW    (3)                   //车牌黄
#define BITCOM_WHITE     (4)                   //车牌白
#define BITCOM_BLACK     (2)                   //车牌黑
#define BITCOM_OTHERS    (5)                   //车牌其它
#define BITCOM_UNKNOW    (0)                  //车牌未识别

//行驶方向
#define BITCOM_EAST_TO_WEST     (1)              //东向西
#define BITCOM_WEST_TO_EAST     (2)              //西向东
#define BITCOM_NORTH_TO_SOUTH   (3)              //南向北
#define BITCOM_SOUTH_TO_NORTH   (4)              //北向南
#define BITCOM_SOUTHEAST_TO_NORTHWEST   (7)        //东南向西北
#define BITCOM_NORTHWEST_TO_SOUTHEAST   (8)        //西北向东南
#define BITCOM_NORTHEAST_TO_SOUTHWEST   (5)        //东北向西南
#define BITCOM_SOUTHWEST_TO_NORTHEAST   (6)        //西南向东北

//车牌类型
#define BITCOM_LARGE_CAR                 1		   //大型汽车		  //可以检测
#define BITCOM_SMALL_CAR                 2		   //小型汽车          //可以检测
#define BITCOM_EMBASSY_CAR               3         //使馆汽车          //可以检测
#define BITCOM_CONSULATE_CAR 			 4	       //领馆汽车          //可以检测
#define BITCOM_FOREIGN_CAR 				 5         //境外汽车  
#define BITCOM_FOREIGNNATION_CAR 		 6         //外籍汽车
#define BITCOM_TRIWHEEL_MOTOBIKE 		 7         //两三轮摩托车 //可以检测
#define BITCOM_LIGHT_MOTOBIKE        	 8		   //轻便摩托车
#define BITCOM_EMBASSY_MOTOBIKE   		 9         //使馆摩托车
#define BITCOM_CONSULATE_MOTOBIKE 		 10        //领馆摩托车
#define BITCOM_FOREIGN_MOTOBIKE 	     11	       //境外摩托车
#define BITCOM_FOREIGNNATION_MOTOBIKE 	 12        //外籍摩托车
#define BITCOM_LOWSPEED_CAR 			 13        //低速载货汽车
#define BITCOM_TRACTOR_CAR  			 14        //拖拉机
#define BITCOM_GUA_CAR 					 15		   //挂车                //可以检测
#define BITCOM_XUE_CAR 					 16	       //教练汽车        //可以检测
#define BITCOM_XUE_MOTOBIKE 			 17        //教练摩托车
#define BITCOM_TESTING_CAR  			 18        //试验汽车
#define BITCOM_TESTING_MOTOBIKE 		 19        //试验摩托车
#define BITCOM_TEMPORARY_FOREIGN_CAR 	 20        //临时入境汽车
#define BITCOM_TEMPORARY_FOREIGN_MOTOBIKE 21       //临时入境摩托车
#define BITCOM_TEMPORARY_CAR 	         22        //临时入境行驶车
#define BITCOM_POLICE_CAR 				 23	       //警用汽车           //可以检测  
#define BITCOM_POLICE_MOTOBIKE			 24	       //警用摩托车
#define BITCOM_AGRICULTURE_CAR 			 25        //原农机号牌
#define BITCOM_GANG_CAR  				 26        //香港入境汽车     //可以检测 
#define BITCOM_AO_CAR  					 27	       //澳门入境汽车    //可以检测
#define BITCOM_ARMY_LARGE_CAR   		 28        //军队用大型汽车 //可以检测
#define BITCOM_ARMY_SMALL_CAR   		 29        //军队用小型汽车 //可以检测
#define BITCOM_WJ_LARGE_CAR	 			 30        //武警大型汽车    //可以检测
#define BITCOM_WJ_SMALL_CAR	 			 31        //武警小型汽车    //可以检测
#define BITCOM_UNKNOWN_CAR 				 41		   //为“无车牌”添加  //可以检测

//车辆类型
#define BITCOM_TYPE_MIDDLE_VEHICLE      (1)        //小型汽车
#define BITCOM_TYPE_MINIATURE_VEHICLE   (2)        //中型汽车
#define BITCOM_TYPE_OVERSIZE_VEHICLE    (3)        //大型汽车
#define BITCOM_TYPE_BIGBUS_VEHICLE      (4)        //大客车
#define BITCOM_TYPE_TRUCK_VEHICLE       (5)        //大货车
#define BITCOM_TYPE_OTHERS_VEHICLE      (6)        //其它车


//车辆颜色
#define BITCOM_COLOUR_WHITE_VEHICLE            (1)        //白色
#define BITCOM_COLOUR_BLACK_VEHICLE            (10)       //黑色
#define BITCOM_COLOUR_RED_VEHICLE              (5)        //红色
#define BITCOM_COLOUR_YELLOW_VEHICLE           (3)        //黄色
#define BITCOM_COLOUR_SILVERGREY_VEHICLE       (2)        //银灰色
#define BITCOM_COLOUR_BLUE_VEHICLE             (8)        //蓝色
#define BITCOM_COLOUR_GREEN_VEHICLE            (7)        //绿色
#define BITCOM_COLOUR_PURPLE_VEHICLE           (6)        //紫色
#define BITCOM_COLOUR_BROWN_VEHICLE            (4)        //粉色
#define BITCOM_COLOUR_OTHERS_VEHICLE           (11)       //其它

//行车状态
#define BITCOM_STATE_NORMAL_VEHICLE         (1)      //正常
#define BITCOM_STATE_NONMOTOR_VEHICLE       (2)      //非机动车
#define BITCOM_STATE_ABNORMAL_VEHICLE       (3)      //异常
#define BITCOM_STATE_INCOMPLETE_VEHICLE     (4)      //残缺

//违法类型
#define BITCOM_ILLEGAL_PARKING_VEHICLE           (1)      //违章停车
#define BITCOM_ILLEGAL_LINEBALL_VEHICLE          (8)      //压线
#define BITCOM_ILLEGAL_UNLANE_VEHICLE            (4)      //不按车道行驶
#define BITCOM_ILLEGAL_PRESSYELLOW_VEHICLE       (2)      //压黄线
#define BITCOM_ILLEGAL_LANECHANG_VEHICLE         (32)     //违章变道
#define BITCOM_ILLEGAL_RETROGRADE_VEHICLE        (64)     //逆行
#define BITCOM_ILLEGAL_JAYWALK_VEHICLE           (128)    //闯红灯
#define BITCOM_ILLEGAL_OVERSPEED_VEHICLE         (256)    //超速
#define BITCOM_ILLEGAL_CARLANE_VEHICLE           (512)    //有车占道
#define BITCOM_ILLEGAL_BICYCLELANE_VEHICLE       (1024)   //非机动车道
#define DAHUA_ILLEGAL_FLAG1_VEHICLE              (2048)   //拥堵时强行驶入交叉路口
#define DAHUA_ILLEGAL_FLAG2_VEHICLE              (4096)   //违反限时限行
#define DAHUA_ILLEGAL_FLAG3_VEHICLE              (8192)   //不礼让行人
#define DAHUA_ILLEGAL_FLAG4_VEHICLE              (16384)  //违章掉头
#define DAHUA_ILLEGAL_FLAG5_VEHICLE              (16)     //压实线


/***************************************************************/





/* 创建通行记录表，机动车、非机动车和行人都使用该表 */
#define SQL_CREATE_TABLE_TRAFFIC_RECORDS \
	"CREATE TABLE traffic_records(ID INTEGER PRIMARY KEY,\
plate_num VARCHAR(32),\
plate_type INTEGER,\
point_id VARCHAR(32),\
point_name VARCHAR(100),\
dev_id VARCHAR(32),\
lane_num INTEGER,\
speed INTEGER,\
time VARCHAR(32),\
collection_agencies VARCHAR(32),\
direction INTEGER,\
image_path VARCHAR(128),\
partition_path VARCHAR(64),\
color INTEGER,\
vehicle_logo INTEGER,\
objective_type INTEGER,\
coordinate_x INTEGER,\
coordinate_y INTEGER,\
width INTEGER,\
height INTEGER,\
pic_flag INTEGER,\
plate_color INTEGER,\
description VARCHAR(128),\
ftp_user VARCHAR(128),\
ftp_passwd VARCHAR(128),\
ftp_ip VARCHAR(128),\
ftp_port INTEGER,\
flag_send INTEGER,\
flag_store INTEGER,\
vehicle_type INTEGER);"
#define NUM_COLUMN_TRAFFIC_RECORDS 30 	//字段数目，需要与SQL_CREATE_ 宏保持一致。用于判断数据库中表的字段数是否与程序版本处理的字段数匹配

#define DB_TRAFFIC_RECORDS_NAME 	"traffic_records.db"
#define TRAFFIC_RECORDS_FTP_DIR 	"kakou"

typedef struct
{
	int 	ID;
	char 	plate_num[32]; 				/* 车牌号码 */
	int 	plate_type; 				/* 号牌类型 */
	char 	point_id[32]; 				/* 采集点编号 */
	char 	point_name[100]; 			/* 采集点名称 */
	char 	dev_id[32]; 				/* 设备编号 */
	int 	lane_num; 					/* 车道号 */
	int 	speed; 						/* 车速 */
	char 	time[32]; 					/* 抓拍时间 */
	char 	collection_agencies[32]; 	/* 采集机关代号 */
	int 	direction; 					/* 方向编号 */
	char 	image_path[256]; 			/* 图像相对路径 */
	char 	partition_path[64]; 		/* 存储的根路径 */
	int 	color; 						/* 车身颜色 */
	int 	vehicle_logo; 				/* 车标 */
	int 	objective_type; 			/* 目标类型 */
	int 	coordinate_x; 				/* 区域X坐标 */
	int 	coordinate_y; 				/* 区域Y坐标 */
	int 	width; 						/* 区域宽度 */
	int 	height; 					/* 区域高度 */
	int 	pic_flag; 					/* 车牌在第几张图片上 */
	int 	plate_color; 				/* 车牌颜色 */
	char 	description[128]; 			/* 通行描述 */
	char 	ftp_user[128]; 				/* FTP用户名 */
	char 	ftp_passwd[128]; 			/* FTP密码 */
	char 	ftp_ip[128]; 				/* FTP服务器IP地址 */
	int 	ftp_port; 					/* FTP服务器端口 */
	int 	flag_send;				/* 续传标志: 使用低3位:
		第0位表示消息:0,不需要续传消息；1，需要续传消息；
		第1位表示图片:0,不需要续传图片；1，需要续传图片； */
	int 	flag_store;				/* 存储标志: 0:不是实时存储，1，实时存储 */
	int 	vehicle_type; 				/* 车型 */
	int		obj_state;					/* 目标状态: 0 驶入 1 驶离 */
	uint32_t detect_coil_time;			/* 检测线圈触发时间 */
	int confidence;						//置信度
} DB_TrafficRecord; 					/* 数据库使用的通行记录结构体 */


typedef struct       //机动车协议
{
	int vehicleType;                //车辆类型
	char vehicleColor[12];			//车辆颜色
	int vehicleLogo;				//车标
	int	vehicleLength;				//车辆长度
	char vehicleNo[32];				//车牌号码
	int	vehicleNoType;				//车牌类型
	char vehicleNoColor[12];		//车牌颜色
}NetPoseVehicleProtocol;

typedef struct     //行人协议
{
	int faceLeft;  //人脸左侧位置
	int faceTop;   //人脸上侧位置
	int faceRight; //人脸右侧位置
	int faceBottom;//人脸下侧位置
	
}NetPoseOthersProtocol;


#ifdef  __cplusplus
extern "C"
{
#endif


int process_vm_records_motor_vehicle(const SystemVpss_SimcopJpegInfo *info); //机动车
int process_vm_records_others(const SystemVpss_SimcopJpegInfo *info);           //非机动车
                 



/******************处理卡口图片******************/
int analyze_traffic_records_picture(
    EP_PicInfo *pic_info, const SystemVpss_SimcopJpegInfo *info);      //机动车
int analyze_traffic_record_info_others(
    DB_TrafficRecord *db_traffic_record, const NoVehiclePoint *result,
    const EP_PicInfo *pic_info, const char *partition_path);            //非机动车

int send_traffic_records_picture_buf(const EP_PicInfo *pic_info); 
int save_traffic_records_picture_buf(
    const EP_PicInfo *pic_info, const char*partition_path);

/******************处理卡口信息******************/
int analyze_traffic_records_info_motor_vehicle(
    DB_TrafficRecord *db_traffic_record, const TrfcVehiclePlatePoint *result,
    const EP_PicInfo *pic_info, const char *partition_path);

int send_traffic_records_info(const DB_TrafficRecord *db_traffic_record);
int save_traffic_records_info(
    const DB_TrafficRecord *db_traffic_record,
    const EP_PicInfo *pic_info, const char *partition_path);



int format_mq_text_traffic_record(
    char *mq_text, const DB_TrafficRecord *record);
int format_mq_text_traffic_record_motor_vehicle(
    char *mq_text, const DB_TrafficRecord *record);
int format_mq_text_traffic_record_others(
    char *mq_text, const DB_TrafficRecord *record);


int db_write_traffic_records(char *db_traffic_name, DB_TrafficRecord *records, pthread_mutex_t *mutex_db_records);
int db_format_insert_sql_traffic_records(char *sql, DB_TrafficRecord *records);

int db_read_traffic_record(const char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records, char * sql_cond);
int db_query_traffic_records(char *db_name, void *records, pthread_mutex_t *mutex_db_records);

int db_add_column_traffic_records(char *db_name, void *records,	pthread_mutex_t *mutex_db_records);


int db_delete_traffic_records(char *db_name, void *records, pthread_mutex_t *mutex_db_records);
int db_unformat_read_sql_traffic_records(
    char *azResult[], DB_TrafficRecord *traffic_record);

int send_traffic_records_info_history(void *db_traffic_records, int dest_mq, int num_record);

int send_traffic_records_image_buf(const EP_PicInfo *pic_info);

int send_traffic_records_image_file(
    DB_TrafficRecord* db_traffic_record, EP_PicInfo *pic_info);

int save_traffic_records_info_others(
    const DB_TrafficRecord *db_traffic_records,
    const void *image_info, const char *partition_path);

int process_entrance_control(MSGDATACMEM *msg_alg_result_info);



int getime(char *nowtime);
int send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

int send_vm_records_motor_vehicle_to_NetPose_others(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

int send_vm_records_motor_vehicle_to_Dahua(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

int protocol_convert_NetPose_moter_vehicle(DB_TrafficRecord *db_traffic_record,NetPoseVehicleProtocol *VehicleProtocol);

int protocol_convert_NetPose_others(DB_TrafficRecord *db_traffic_record,NetPoseOthersProtocol *OthersProtocol);



int send_vm_records_motor_vehicle_to_NetPose_otherstmp(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

/*******************************************************************************
 * 函数名: alleyway_sendstatus_to_bitcom
 * 功  能: 出入口 发送bitcom协议
 * 参  数: 
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int alleyway_sendstatus_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

/*******************************************************************************
 * 函数名: alleyway_senddatas_to_bitcom
 * 功  能: 出入口 发送bitcom协议
 * 参  数: 
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int alleyway_senddatas_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);



/*******************************************************************************
 * 函数名: bitcom_sendto_dahua
 * 功  能: 发送数据到bitcom 平台
 * 参  数: 
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
void * alleyway_sendto_bitcom_thread(void * argv);

/*******************************************************************************
 * 函数名: bitcom_sendto_dahua
 * 功  能: 发送数据到大华平台
 * 参  数: 
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
void * bitcom_sendto_dahua(void * argv);
//int process_traffic_record_others(const NoVehiclePoint *result);

//int analyze_traffic_record_info_others(
//    DB_TrafficRecord *db_traffic_record, const NoVehiclePoint *result,
 //   const VD_PicInfo *pic_info, const char *partition_path);
 
//int send_traffic_record_image_buf(const VD_PicInfo *pic_info);
//int save_traffic_record_image(
//   const VD_PicInfo *pic_info, const char *partition_path);

int process_fillinlight_smart_control(MSGDATACMEM *msg_alg_result_info);

#ifdef  __cplusplus
}
#endif


#endif 	/* _TRAFFIC_RECORDS_PROCESS_H_ */
