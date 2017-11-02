
#ifndef _TRAFFIC_FLOW_PROCESS_H_
#define _TRAFFIC_FLOW_PROCESS_H_

#include "upload.h"
//#include "alg_result.h"
#include "../../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/interface_alg.h"

/* 创建机动车交通流表 */
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

/* 创建行人交通流表 */
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
	char 	section_id[32]; 	/* 路段编号 */
	char 	section_name[100]; 	/* 路段名称 */
	int 	large_count; 		/* 车道平均大型车辆数 */
	int 	small_count; 		/* 车道平均小型车辆数 */
	int 	comm_count; 		/* 过车数 */
	int 	avg_volume; 		/* 车道平均流量 */
	int 	avg_speed; 			/* 车道平均车速 */
	int 	occupancy; 			/* 车道平均时间占有率 */
	int 	queue; 				/* 车道平均排对长度 */
	int 	traval_time; 		/* 车道平均旅行时间 */
	char 	point_id[32]; 		/* 采集点编号 */
	char 	point_name[100]; 	/* 采集点名称 */
	char 	dev_id[32]; 		/* 检测设备编号 */
	char 	dev_name[64]; 		/* 检测设备名称 */
	char 	uptime[32]; 		/* 检测时间 */
	int 	lan_num; 			/* 车道号 */
	int 	direction; 			/* 方向编号 */
	int 	time_headway; 		/* 平均车头时距 */
	int 	density; 			/* 车辆密度 */
	int 	car_len; 			/* 车道平均车长 */
	int 	flag_send;			/* 续传标志：0，无需续传；1，需要续传 */
	int 	flag_store;			/* 存储标志：0，不是实时存储；1，实时存储 */
} DB_TrafficFlow; 				/* 数据库使用的机动车交通流结构体 */

typedef struct
{
	int 	ID;
	int 	flow_left2right; 	/* 左向右流量 */
	int 	flow_right2left; 	/* 右向左流量 */
	char 	point_id[32]; 		/* 采集点编号 */
	char 	point_name[100]; 	/* 采集点名称 */
	char 	dev_id[32]; 		/* 检测设备编号 */
	char 	dev_name[64]; 		/* 检测设备名称 */
	char 	uptime[32]; 		/* 检测时间 */
	int 	flag_send;			/* 续传标志：0，无需续传；1，需要续传 */
	int 	flag_store;			/* 存储标志：0，不是实时存储；1，实时存储 */
} DB_TrafficFlowPedestrian; 	/* 数据库使用的行人交通流结构体 */


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
