
#ifndef _TRAFFIC_FLOW_PROCESS_H_
#define _TRAFFIC_FLOW_PROCESS_H_

#include "upload.h"
//#include "alg_result.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"

/* ������������ͨ���� */
#define SQL_CREATE_TABLE_TRAFFIC_FLOW \
	"CREATE TABLE traffic_flow(ID INTEGER PRIMARY KEY,\
section_id VARCHAR(32),\
section_name VARCHAR(100),\
large_count INTEGER,\
small_count INTEGER,\
comm_count INTEGER,\
avg_volume INTEGER,\
avg_speed INTEGER,\
occupancy INTEGER,\
queue INTEGER,\
traval_time INTEGER,\
point_id VARCHAR(32),\
point_name VARCHAR(100),\
dev_id VARCHAR(32),\
dev_name VARCHAR(64),\
uptime VARCHAR(32),\
lan_num INTEGER,\
direction INTEGER,\
time_headway INTEGER,\
density INTEGER,\
car_len INTEGER,\
flag_send INTEGER,\
flag_store INTEGER);"

#define NUM_COLUMN_ETRAFFIC_FLOW 23

/* �������˽�ͨ���� */
#define SQL_CREATE_TABLE_TRAFFIC_FLOW_PEDESTRIAN \
	"CREATE TABLE traffic_flow_pedestrian(ID INTEGER PRIMARY KEY,\
flow_left2right INTEGER,\
flow_right2left INTEGER,\
point_id TEXT,\
point_name TEXT,\
dev_id TEXT,\
dev_name TEXT,\
uptime TEXT,\
flag_send INTEGER,\
flag_store INTEGER);"

#define DB_TRAFFIC_FLOW_PEDESTRIAN_DIR 	"traffic_flow_pedestrian"


typedef struct
{
	int 	ID;
	char 	section_id[32]; 	/* ·�α�� */
	char 	section_name[100]; 	/* ·������ */
	int 	large_count; 		/* ����ƽ�����ͳ����� */
	int 	small_count; 		/* ����ƽ��С�ͳ����� */
	int 	comm_count; 		/* ������ */
	int 	avg_volume; 		/* ����ƽ������ */
	int 	avg_speed; 			/* ����ƽ������ */
	int 	occupancy; 			/* ����ƽ��ʱ��ռ���� */
	int 	queue; 				/* ����ƽ���ŶԳ��� */
	int 	traval_time; 		/* ����ƽ������ʱ�� */
	char 	point_id[32]; 		/* �ɼ����� */
	char 	point_name[100]; 	/* �ɼ������� */
	char 	dev_id[32]; 		/* ����豸��� */
	char 	dev_name[64]; 		/* ����豸���� */
	char 	uptime[32]; 		/* ���ʱ�� */
	int 	lan_num; 			/* ������ */
	int 	direction; 			/* ������ */
	int 	time_headway; 		/* ƽ����ͷʱ�� */
	int 	density; 			/* �����ܶ� */
	int 	car_len; 			/* ����ƽ������ */
	int 	flag_send;			/* ������־��0������������1����Ҫ���� */
	int 	flag_store;			/* �洢��־��0������ʵʱ�洢��1��ʵʱ�洢 */
} DB_TrafficFlow; 				/* ���ݿ�ʹ�õĻ�������ͨ���ṹ�� */

typedef struct
{
	int 	ID;
	int 	flow_left2right; 	/* ���������� */
	int 	flow_right2left; 	/* ���������� */
	char 	point_id[32]; 		/* �ɼ����� */
	char 	point_name[100]; 	/* �ɼ������� */
	char 	dev_id[32]; 		/* ����豸��� */
	char 	dev_name[64]; 		/* ����豸���� */
	char 	uptime[32]; 		/* ���ʱ�� */
	int 	flag_send;			/* ������־��0������������1����Ҫ���� */
	int 	flag_store;			/* �洢��־��0������ʵʱ�洢��1��ʵʱ�洢 */
} DB_TrafficFlowPedestrian; 	/* ���ݿ�ʹ�õ����˽�ͨ���ṹ�� */


#ifdef  __cplusplus
extern "C"
{
#endif

int process_traffic_flow(const TrafficFlowStatistics *traffic_flow);
int process_traffic_flow_motor_vehicles(const VehicleFlow *statistics);
int process_traffic_flow_pedestrian(const PedestrianFlow *statistics);
int analyze_traffic_flow_motor_vehicle_info(
    DB_TrafficFlow *db_traffic_flow,
    const VehicleFlow *statistics, int lane_num);
int analyze_traffic_flow_pedestrian_info(
    DB_TrafficFlowPedestrian *db_traffic_flow,
    const PedestrianFlow *statistics);
int send_traffic_flow_motor_vehicle(const DB_TrafficFlow *db_traffic_flow);
int send_traffic_flow_pedestrian(
    const DB_TrafficFlowPedestrian *db_traffic_flow);
int send_traffic_flow_history(
    const DB_TrafficFlow *db_traffic_flow, int dest_mq, int num_record);
int format_mq_text_traffic_flow_motor_vehicle(
    char *mq_text, const DB_TrafficFlow *traffic_flow);
int format_mq_text_traffic_flow_pedestrian(
    char *mq_text, const DB_TrafficFlowPedestrian *traffic_flow);
int save_traffic_flow_info(
    const DB_TrafficFlow *db_flow_records,
    const VehicleFlow *statistics, const char *partition_path);
int save_traffic_flow_pedestrian_info(
    const DB_TrafficFlowPedestrian *db_flow_records,
    const PedestrianFlow *statistics, const char *partition_path);
int db_write_flow_records(char *db_flow_name, DB_TrafficFlow *records, pthread_mutex_t *mutex_db_records);
int db_write_traffic_flow_pedestrian(
    const char *db_name, const DB_TrafficFlowPedestrian *record);
int db_read_flow_records(char *db_name, void *records, pthread_mutex_t *mutex_db_records, char * sql_cond);
int db_read_traffic_flow_pedestrian_info(
    const char *db_name, DB_TrafficFlowPedestrian *record, const char *sql);
int db_delete_flow_records(const char *db_name, void *records, pthread_mutex_t *mutex_db_records);
int db_delete_traffic_flow_pedestrian_info(
    const char *db_name, const DB_TrafficFlowPedestrian *record);
int db_format_insert_sql_traffic_flow(char *sql, DB_TrafficFlow *traffic_flow);
int db_format_insert_sql_traffic_flow_pedestrian(
    char *sql, const DB_TrafficFlowPedestrian *traffic_flow);
int db_unformat_read_sql_traffic_flow(char *azResult[], DB_TrafficFlow* buf);
int db_unformat_traffic_flow_pedestrian_info(
    char *azResult[], DB_TrafficFlowPedestrian *record);

#ifdef  __cplusplus
}
#endif


#endif 	/* _TRAFFIC_FLOW_PROCESS_H_ */
