
#ifndef _TRAFFIC_RECORDS_PROCESS_H_
#define _TRAFFIC_RECORDS_PROCESS_H_


#include "upload.h"
//#include "alg_result.h"
#include "mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"
#include "mcfw/interfaces/ti_media_std.h"	/* WTF! */
#include "mcfw/interfaces/link_api/systemLink_m3vpss.h"
#include "mcfw/interfaces/link_api/jpeg_enc_info.h"
#include "mcfw/interfaces/link_api/videoAnalysisLink_parm.h"




/*********************�󻪳��������궨��*****************************/

//������ɫ
#define DAHUA_PLATE_BLUE      (0)                   //������
#define DAHUA_PLATE_YELLOW    (1)                   //���ƻ�
#define DAHUA_PLATE_WHITE     (2)                   //���ư�
#define DAHUA_PLATE_BLACK     (3)                   //���ƺ�
#define DAHUA_PLATE_UNKNOW    (99)                  //����δʶ��
#define DAHUA_PLATE_OTHERS    (100)                 //��������

//��ʻ����
#define DAHUA_EAST_TO_WEST     (0)              //������
#define DAHUA_WEST_TO_EAST     (1)              //����
#define DAHUA_NORTH_TO_SOUTH   (2)              //����
#define DAHUA_SOUTH_TO_NORTH   (3)              //������
#define DAHUA_SOUTHEAST_TO_NORTHWEST   (4)        //����������
#define DAHUA_NORTHWEST_TO_SOUTHEAST   (5)        //��������
#define DAHUA_NORTHEAST_TO_SOUTHWEST   (6)        //����������
#define DAHUA_SOUTHWEST_TO_NORTHEAST   (7)        //�����򶫱�

//��������
#define DAHUA_PLATE_MINIATURE_VEHICLE  (0)        //С�ͳ���
#define DAHUA_PLATE_OVERSIZE_VEHICLE   (1)        //���ͳ���
#define DAHUA_PLATE_MILITARY_VEHICLE   (2)        //�����ͳ���
#define DAHUA_PLATE_FOREIGN_VEHICLE    (3)        //�⼮����
#define DAHUA_PLATE_UNKONW_VEHICLE     (99)       //δʶ����
#define DAHUA_PLATE_OTHERS_VEHICLE     (100)      //����

//��������
#define DAHUA_TYPE_UNKNOW_VEHICLE      (0)        //δʶ��
#define DAHUA_TYPE_MINIATURE_VEHICLE   (1)        //С������
#define DAHUA_TYPE_OVERSIZE_VEHICLE    (2)        //��������
#define DAHUA_TYPE_EMBASSY_VEHICLE     (3)        //ʹ������
#define DAHUA_TYPE_CONSULATE_VEHICLE   (4)        //�������
#define DAHUA_TYPE_OVERSEAS_VEHICLE    (5)        //��������
#define DAHUA_TYPE_FOREIGN_VEHICLE     (6)        //�⼮����
#define DAHUA_TYPE_LOWSPEED_VEHICLE    (7)        //��������
#define DAHUA_TYPE_TRACTOR_VEHICLE     (8)        //������
#define DAHUA_TYPE_TRAILER_VEHICLE     (9)        //�ҳ�
#define DAHUA_TYPE_COACH_VEHICLE       (10)       //������
#define DAHUA_TYPE_TEMP_VEHICLE        (11)       //��ʱ��ʻ��
#define DAHUA_TYPE_POLICE_VEHICLE      (12)       //��������
#define DAHUA_TYPE_POLICEMOTOR_VEHICLE (13)       //����Ħ�г�
#define DAHUA_TYPE_MOTOR_VEHICLE       (14)       //��ͨĦ�г�
#define DAHUA_TYPE_LIGHTMOTOR_VEHICLE  (15)       //���Ħ�г�

//������ɫ
#define DAHUA_COLOUR_WHITE_VEHICLE            (0)        //��ɫ
#define DAHUA_COLOUR_BLACK_VEHICLE            (1)        //��ɫ
#define DAHUA_COLOUR_RED_VEHICLE              (2)        //��ɫ
#define DAHUA_COLOUR_YELLOW_VEHICLE           (3)        //��ɫ
#define DAHUA_COLOUR_SILVERGREY_VEHICLE       (4)        //����ɫ
#define DAHUA_COLOUR_BLUE_VEHICLE             (5)        //��ɫ
#define DAHUA_COLOUR_GREEN_VEHICLE            (6)        //��ɫ
#define DAHUA_COLOUR_ORANGE_VEHICLE           (7)        //��ɫ
#define DAHUA_COLOUR_PURPLE_VEHICLE           (8)        //��ɫ
#define DAHUA_COLOUR_SYAN_VEHICLE             (9)        //��ɫ
#define DAHUA_COLOUR_PINK_VEHICLE             (10)       //��ɫ
#define DAHUA_COLOUR_UNKNOW_VEHICLE           (99)       //δʶ��
#define DAHUA_COLOUR_OTHERS_VEHICLE           (100)      //����

//�г�״̬
#define DAHUA_STATE_NORMAL_VEHICLE         (1)      //����
#define DAHUA_STATE_NONMOTOR_VEHICLE       (2)      //�ǻ�����
#define DAHUA_STATE_ABNORMAL_VEHICLE       (3)      //�쳣
#define DAHUA_STATE_INCOMPLETE_VEHICLE     (4)      //��ȱ

//Υ������
#define DAHUA_ILLEGAL_OVERSPEED_VEHICLE         (1)      //����
#define DAHUA_ILLEGAL_SUPERLOWSPEED_VEHICLE     (2)      //������
#define DAHUA_ILLEGAL_OTHERS_VEHICLE            (3)      //����
#define DAHUA_ILLEGAL_JAYWALK_VEHICLE           (4)      //�����
#define DAHUA_ILLEGAL_UNLANE_VEHICLE            (5)      //����������ʻ
#define DAHUA_ILLEGAL_LINEBALL_VEHICLE          (6)      //ѹ��
#define DAHUA_ILLEGAL_RETROGRADE_VEHICLE        (7)      //����
#define DAHUA_ILLEGAL_BICYCLELANE_VEHICLE       (8)      //�ǻ�������
#define DAHUA_ILLEGAL_FLAG_VEHICLE              (10)     //������Υ�������־ָʾ
#define DAHUA_ILLEGAL_PARKING_VEHICLE           (33)     //Υ��ͣ��
#define DAHUA_ILLEGAL_LANECHANG_VEHICLE         (34)      //Υ�±��
#define DAHUA_ILLEGAL_PRESSYELLOW_VEHICLE       (35)      //ѹ����
#define DAHUA_ILLEGAL_TRAFFICJAM_VEHICLE        (36)      //��ͨӵ��
#define DAHUA_ILLEGAL_TRAFFICSTOP_VEHICLE       (37)      //��ͨ����
#define DAHUA_ILLEGAL_YELLOWLANE_VEHICLE        (38)      //���Ƴ�ռ��
#define DAHUA_ILLEGAL_CARLANE_VEHICLE           (39)      //�г�ռ��
#define DAHUA_ILLEGAL_MANUALLYCAPTURE_VEHICLE   (40)      //�ֶ�ץ��
#define DAHUA_ILLEGAL_BUSLANE_VEHICLE           (41)      //ռ�ù�������
#define DAHUA_ILLEGAL_ASTERN_VEHICLE            (42)      //Υ�µ���
#define DAHUA_ILLEGAL_YELLOWWARK_VEHICLE        (43)      //���Ƶ�
#define DAHUA_ILLEGAL_CARPORT_VEHICLE           (44)      //��λ�г�
#define DAHUA_ILLEGAL_NOCARPORT_VEHICLE         (45)      //��λ�޳�
#define DAHUA_ILLEGAL_SMOKING_VEHICLE           (46)      //����
#define DAHUA_ILLEGAL_PHONE_VEHICLE             (47)      //���ֻ�
#define DAHUA_ILLEGAL_SAFETYBELT_VEHICLE        (48)      //��ϵ��ȫ��
#define DAHUA_ILLEGAL_COVERPLATE_VEHICLE        (49)      //�ڵ�����
#define DAHUA_ILLEGAL_YELLOWSTOP_VEHICLE        (50)      //������Υ��ͣ��
#define DAHUA_ILLEGAL_UNKNOW_VEHICLE            (51)      //δ֪


/***************************************************************/




/*********************BITCOMЭ��궨��*****************************/
//������ɫ
#define BITCOM_BLUE      (1)                   //������
#define BITCOM_YELLOW    (3)                   //���ƻ�
#define BITCOM_WHITE     (4)                   //���ư�
#define BITCOM_BLACK     (2)                   //���ƺ�
#define BITCOM_OTHERS    (5)                   //��������
#define BITCOM_UNKNOW    (0)                  //����δʶ��

//��ʻ����
#define BITCOM_EAST_TO_WEST     (1)              //������
#define BITCOM_WEST_TO_EAST     (2)              //����
#define BITCOM_NORTH_TO_SOUTH   (3)              //����
#define BITCOM_SOUTH_TO_NORTH   (4)              //������
#define BITCOM_SOUTHEAST_TO_NORTHWEST   (7)        //����������
#define BITCOM_NORTHWEST_TO_SOUTHEAST   (8)        //��������
#define BITCOM_NORTHEAST_TO_SOUTHWEST   (5)        //����������
#define BITCOM_SOUTHWEST_TO_NORTHEAST   (6)        //�����򶫱�

//��������
#define BITCOM_LARGE_CAR                 1		   //��������		  //���Լ��
#define BITCOM_SMALL_CAR                 2		   //С������          //���Լ��
#define BITCOM_EMBASSY_CAR               3         //ʹ������          //���Լ��
#define BITCOM_CONSULATE_CAR 			 4	       //�������          //���Լ��
#define BITCOM_FOREIGN_CAR 				 5         //��������  
#define BITCOM_FOREIGNNATION_CAR 		 6         //�⼮����
#define BITCOM_TRIWHEEL_MOTOBIKE 		 7         //������Ħ�г� //���Լ��
#define BITCOM_LIGHT_MOTOBIKE        	 8		   //���Ħ�г�
#define BITCOM_EMBASSY_MOTOBIKE   		 9         //ʹ��Ħ�г�
#define BITCOM_CONSULATE_MOTOBIKE 		 10        //���Ħ�г�
#define BITCOM_FOREIGN_MOTOBIKE 	     11	       //����Ħ�г�
#define BITCOM_FOREIGNNATION_MOTOBIKE 	 12        //�⼮Ħ�г�
#define BITCOM_LOWSPEED_CAR 			 13        //�����ػ�����
#define BITCOM_TRACTOR_CAR  			 14        //������
#define BITCOM_GUA_CAR 					 15		   //�ҳ�                //���Լ��
#define BITCOM_XUE_CAR 					 16	       //��������        //���Լ��
#define BITCOM_XUE_MOTOBIKE 			 17        //����Ħ�г�
#define BITCOM_TESTING_CAR  			 18        //��������
#define BITCOM_TESTING_MOTOBIKE 		 19        //����Ħ�г�
#define BITCOM_TEMPORARY_FOREIGN_CAR 	 20        //��ʱ�뾳����
#define BITCOM_TEMPORARY_FOREIGN_MOTOBIKE 21       //��ʱ�뾳Ħ�г�
#define BITCOM_TEMPORARY_CAR 	         22        //��ʱ�뾳��ʻ��
#define BITCOM_POLICE_CAR 				 23	       //��������           //���Լ��  
#define BITCOM_POLICE_MOTOBIKE			 24	       //����Ħ�г�
#define BITCOM_AGRICULTURE_CAR 			 25        //ԭũ������
#define BITCOM_GANG_CAR  				 26        //����뾳����     //���Լ�� 
#define BITCOM_AO_CAR  					 27	       //�����뾳����    //���Լ��
#define BITCOM_ARMY_LARGE_CAR   		 28        //�����ô������� //���Լ��
#define BITCOM_ARMY_SMALL_CAR   		 29        //������С������ //���Լ��
#define BITCOM_WJ_LARGE_CAR	 			 30        //�侯��������    //���Լ��
#define BITCOM_WJ_SMALL_CAR	 			 31        //�侯С������    //���Լ��
#define BITCOM_UNKNOWN_CAR 				 41		   //Ϊ���޳��ơ����  //���Լ��

//��������
#define BITCOM_TYPE_MIDDLE_VEHICLE      (1)        //С������
#define BITCOM_TYPE_MINIATURE_VEHICLE   (2)        //��������
#define BITCOM_TYPE_OVERSIZE_VEHICLE    (3)        //��������
#define BITCOM_TYPE_BIGBUS_VEHICLE      (4)        //��ͳ�
#define BITCOM_TYPE_TRUCK_VEHICLE       (5)        //�����
#define BITCOM_TYPE_OTHERS_VEHICLE      (6)        //������


//������ɫ
#define BITCOM_COLOUR_WHITE_VEHICLE            (1)        //��ɫ
#define BITCOM_COLOUR_BLACK_VEHICLE            (10)       //��ɫ
#define BITCOM_COLOUR_RED_VEHICLE              (5)        //��ɫ
#define BITCOM_COLOUR_YELLOW_VEHICLE           (3)        //��ɫ
#define BITCOM_COLOUR_SILVERGREY_VEHICLE       (2)        //����ɫ
#define BITCOM_COLOUR_BLUE_VEHICLE             (8)        //��ɫ
#define BITCOM_COLOUR_GREEN_VEHICLE            (7)        //��ɫ
#define BITCOM_COLOUR_PURPLE_VEHICLE           (6)        //��ɫ
#define BITCOM_COLOUR_BROWN_VEHICLE            (4)        //��ɫ
#define BITCOM_COLOUR_OTHERS_VEHICLE           (11)       //����

//�г�״̬
#define BITCOM_STATE_NORMAL_VEHICLE         (1)      //����
#define BITCOM_STATE_NONMOTOR_VEHICLE       (2)      //�ǻ�����
#define BITCOM_STATE_ABNORMAL_VEHICLE       (3)      //�쳣
#define BITCOM_STATE_INCOMPLETE_VEHICLE     (4)      //��ȱ

//Υ������
#define BITCOM_ILLEGAL_PARKING_VEHICLE           (1)      //Υ��ͣ��
#define BITCOM_ILLEGAL_LINEBALL_VEHICLE          (8)      //ѹ��
#define BITCOM_ILLEGAL_UNLANE_VEHICLE            (4)      //����������ʻ
#define BITCOM_ILLEGAL_PRESSYELLOW_VEHICLE       (2)      //ѹ����
#define BITCOM_ILLEGAL_LANECHANG_VEHICLE         (32)     //Υ�±��
#define BITCOM_ILLEGAL_RETROGRADE_VEHICLE        (64)     //����
#define BITCOM_ILLEGAL_JAYWALK_VEHICLE           (128)    //�����
#define BITCOM_ILLEGAL_OVERSPEED_VEHICLE         (256)    //����
#define BITCOM_ILLEGAL_CARLANE_VEHICLE           (512)    //�г�ռ��
#define BITCOM_ILLEGAL_BICYCLELANE_VEHICLE       (1024)   //�ǻ�������
#define DAHUA_ILLEGAL_FLAG1_VEHICLE              (2048)   //ӵ��ʱǿ��ʻ�뽻��·��
#define DAHUA_ILLEGAL_FLAG2_VEHICLE              (4096)   //Υ����ʱ����
#define DAHUA_ILLEGAL_FLAG3_VEHICLE              (8192)   //����������
#define DAHUA_ILLEGAL_FLAG4_VEHICLE              (16384)  //Υ�µ�ͷ
#define DAHUA_ILLEGAL_FLAG5_VEHICLE              (16)     //ѹʵ��


/***************************************************************/





/* ����ͨ�м�¼�����������ǻ����������˶�ʹ�øñ� */
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
#define NUM_COLUMN_TRAFFIC_RECORDS 30 	//�ֶ���Ŀ����Ҫ��SQL_CREATE_ �걣��һ�¡������ж����ݿ��б���ֶ����Ƿ������汾������ֶ���ƥ��

#define DB_TRAFFIC_RECORDS_NAME 	"traffic_records.db"
#define TRAFFIC_RECORDS_FTP_DIR 	"kakou"

typedef struct
{
	int 	ID;
	char 	plate_num[32]; 				/* ���ƺ��� */
	int 	plate_type; 				/* �������� */
	char 	point_id[32]; 				/* �ɼ����� */
	char 	point_name[100]; 			/* �ɼ������� */
	char 	dev_id[32]; 				/* �豸��� */
	int 	lane_num; 					/* ������ */
	int 	speed; 						/* ���� */
	char 	time[32]; 					/* ץ��ʱ�� */
	char 	collection_agencies[32]; 	/* �ɼ����ش��� */
	int 	direction; 					/* ������ */
	char 	image_path[256]; 			/* ͼ�����·�� */
	char 	partition_path[64]; 		/* �洢�ĸ�·�� */
	int 	color; 						/* ������ɫ */
	int 	vehicle_logo; 				/* ���� */
	int 	objective_type; 			/* Ŀ������ */
	int 	coordinate_x; 				/* ����X���� */
	int 	coordinate_y; 				/* ����Y���� */
	int 	width; 						/* ������ */
	int 	height; 					/* ����߶� */
	int 	pic_flag; 					/* �����ڵڼ���ͼƬ�� */
	int 	plate_color; 				/* ������ɫ */
	char 	description[128]; 			/* ͨ������ */
	char 	ftp_user[128]; 				/* FTP�û��� */
	char 	ftp_passwd[128]; 			/* FTP���� */
	char 	ftp_ip[128]; 				/* FTP������IP��ַ */
	int 	ftp_port; 					/* FTP�������˿� */
	int 	flag_send;				/* ������־: ʹ�õ�3λ:
		��0λ��ʾ��Ϣ:0,����Ҫ������Ϣ��1����Ҫ������Ϣ��
		��1λ��ʾͼƬ:0,����Ҫ����ͼƬ��1����Ҫ����ͼƬ�� */
	int 	flag_store;				/* �洢��־: 0:����ʵʱ�洢��1��ʵʱ�洢 */
	int 	vehicle_type; 				/* ���� */
	int		obj_state;					/* Ŀ��״̬: 0 ʻ�� 1 ʻ�� */
	uint32_t detect_coil_time;			/* �����Ȧ����ʱ�� */
	int confidence;						//���Ŷ�
} DB_TrafficRecord; 					/* ���ݿ�ʹ�õ�ͨ�м�¼�ṹ�� */


typedef struct       //������Э��
{
	int vehicleType;                //��������
	char vehicleColor[12];			//������ɫ
	int vehicleLogo;				//����
	int	vehicleLength;				//��������
	char vehicleNo[32];				//���ƺ���
	int	vehicleNoType;				//��������
	char vehicleNoColor[12];		//������ɫ
}NetPoseVehicleProtocol;

typedef struct     //����Э��
{
	int faceLeft;  //�������λ��
	int faceTop;   //�����ϲ�λ��
	int faceRight; //�����Ҳ�λ��
	int faceBottom;//�����²�λ��
	
}NetPoseOthersProtocol;


#ifdef  __cplusplus
extern "C"
{
#endif


int process_vm_records_motor_vehicle(const SystemVpss_SimcopJpegInfo *info); //������
int process_vm_records_others(const SystemVpss_SimcopJpegInfo *info);           //�ǻ�����
                 



/******************������ͼƬ******************/
int analyze_traffic_records_picture(
    EP_PicInfo *pic_info, const SystemVpss_SimcopJpegInfo *info);      //������
int analyze_traffic_record_info_others(
    DB_TrafficRecord *db_traffic_record, const NoVehiclePoint *result,
    const EP_PicInfo *pic_info, const char *partition_path);            //�ǻ�����

int send_traffic_records_picture_buf(const EP_PicInfo *pic_info); 
int save_traffic_records_picture_buf(
    const EP_PicInfo *pic_info, const char*partition_path);

/******************��������Ϣ******************/
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
 * ������: alleyway_sendstatus_to_bitcom
 * ��  ��: ����� ����bitcomЭ��
 * ��  ��: 
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int alleyway_sendstatus_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);

/*******************************************************************************
 * ������: alleyway_senddatas_to_bitcom
 * ��  ��: ����� ����bitcomЭ��
 * ��  ��: 
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int alleyway_senddatas_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info);



/*******************************************************************************
 * ������: bitcom_sendto_dahua
 * ��  ��: �������ݵ�bitcom ƽ̨
 * ��  ��: 
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
void * alleyway_sendto_bitcom_thread(void * argv);

/*******************************************************************************
 * ������: bitcom_sendto_dahua
 * ��  ��: �������ݵ���ƽ̨
 * ��  ��: 
 * ����ֵ: �ɹ�������0����������-1
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
