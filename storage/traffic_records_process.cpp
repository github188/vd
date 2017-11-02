
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "storage_common.h"
#include "disk_mng.h"
#include "partition_func.h"
#include "data_process.h"
#include "database_mng.h"
#include "traffic_records_process.h"
#include "PCIe_api.h"
#include "ftp.h"
#include "log_interface.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>
#include "global.h"
#include <json/json.h>    //add by lxd
#include "ctrl.h"
#include "ve/platform/bitcom/proto.h"
#include "vd_msg_queue.h"
#include "sys/time_util.h"
#include "sys/tcp_util.h"
#include "logger/log.h"
#include "ve/config/cam_param.h"
#include "ve/dev/roadgate/roadgate.h"
#include "ve/dev/roadgate/roadgate_data.h"

extern PLATFORM_SET msgbuf_paltform;
extern str_alleyway_status gstr_alleyway_status;
extern int flg_register_NetPose_face;
extern int flg_register_NetPose_vehicle;
extern int flg_register_Bitcom;
extern int flg_register_Dahua;
extern int gi_filllight_smart;

/*******************************************************************************
 * 函数名: alleyway_sendstatus_to_bitcom
 * 功  能: 出入口 发送bitcom协议
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int alleyway_sendstatus_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{
	Json::Value root;
	Json::StyledWriter style_writer;

	root["FaultState"] = gstr_alleyway_status.FaultState;						            //设备故障状态
	root["SenseCoilState"] = gstr_alleyway_status.SenseCoilState;							      //线圈状态
	root["FlashlLightState"] = gstr_alleyway_status.FlashlLightState;                 //补光灯状态
	root["IndicatoLightState"] = gstr_alleyway_status.IndicatoLightState;               //指示灯状态

	std::string  str_vehicalinfo = style_writer.write(root);

	std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;

	//协议消息
	char str_vehical[1024*1024];
	memset(str_vehical,0,1024*1024);
	sprintf(str_vehical,"POST /COMPANY/Devices/%s$0/DeviceStatus HTTP/1.1\r\n"\
						"Content-Type: application+json\r\n"\
						"Content-Length: %d\r\n"\
					  "Host: %s:%d\r\n"\
					  "Connection: Keep-alive\r\n"\
					  "User-Agent: ice_wind\r\n\r\n",
						msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId,
						str_vehicalinfo.length(),
						msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp, msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);

	//***********拼接***********//
	strcat(str_vehical,str_vehicalinfo.c_str()); //消息头 + 过车信息

	TRACE_LOG_PLATFROM_INTERFACE("alleyway bitcom status = \n%s", str_vehical);

	//----------------发送数据----------------//

	//创建套接字并连接服务器
	int sockfd;
	fd_set rset;

	struct sockaddr_in client_addr;
	if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
	{
		return -1;
	}

	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);
	client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp); //IP需要动态赋予


	if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
	 {
		  close(sockfd);
		  return -1;
	 }


	while(1)
	{
		//在发送的时候需要考虑套接字缓冲区中是否有数据，需要先清空缓冲区
		int ret = send(sockfd,str_vehical, strlen(str_vehical), 0);
		if (ret == -1)
		{
			close(sockfd);
			return -1;
		}

		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);

		struct timeval tv;
		tv.tv_sec= 2;
		tv.tv_usec= 0;
		int h= select(sockfd +1, &rset, NULL, NULL, &tv); //过车消息发送完毕之后等待接收服务器的回复消息，如果2s内没有收到回复消息则跳过继续执行

		if (h <= 0)
		{
			close(sockfd);
			return -1;
		}

		if (h > 0)
		{
			char buf[1024];
			memset(buf, 0, 1024);
			int i= read(sockfd, buf, 1023);
			if (i <= 0)
			{
				close(sockfd);

				return -1;
			}
		}
		}

}


/*******************************************************************************
 * 函数名: alleyway_sendto_bitcom_thread
 * 功  能: 发送数据到大华平台
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
void * alleyway_sendto_bitcom_thread(void * argv)
{

	//创建套接字并连接服务器
	int sockfd;
	fd_set rset;

	while(1)
	{
		sleep(5);

		struct sockaddr_in client_addr;
		if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
		{
			continue;
		}

		gstr_bitcom_data.flag = 0;
		bzero(&client_addr, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort);
		client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp); //IP需要动态赋予

		if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
		 {
			  close(sockfd);

			  continue;
		 }

		while(1)
		{
			sleep(1);

			if(gstr_bitcom_data.flag == 1)
			{
				//在发送的时候需要考虑套接字缓冲区中是否有数据，需要先清空缓冲区
				int ret = send(sockfd , gstr_bitcom_data.buf, gstr_bitcom_data.buf_lens, 0);
				if (ret == -1)
				{
					close(sockfd);
					break;
				}

				TRACE_LOG_PLATFROM_INTERFACE("bitcom alleyway  send datas : %s", gstr_bitcom_data.buf);

				FD_ZERO(&rset);
				FD_SET(sockfd, &rset);

				struct timeval tv;
				tv.tv_sec= 2;
				tv.tv_usec= 0;
				int h= select(sockfd +1, &rset, NULL, NULL, &tv); //过车消息发送完毕之后等待接收服务器的回复消息，如果2s内没有收到回复消息则跳过继续执行

				if (h <= 0)
				{
					close(sockfd);

					break;
				}

				if (h > 0)
				{
					char buf[1024];
					memset(buf, 0, 1024);
					int i= read(sockfd, buf, 1023);
					if (i <= 0)
					{
						close(sockfd);

						break;
					}

					memset(gstr_bitcom_data.buf, 0, sizeof(gstr_bitcom_data.buf));
					gstr_bitcom_data.buf_lens = 0;
					gstr_bitcom_data.flag = 0;

				}

			}
		}

	}
    return NULL;
}


/*******************************************************************************
 * 函数名: alleyway_senddatas_to_bitcom
 * 功  能: 出入口 发送bitcom协议
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int alleyway_senddatas_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{

	//获取时间秒数
	char lt[48];
	char lc_position[20] = {0};
	memset(lt,0,48);
	long  ltime = time(NULL);
//	sprintf(lt,"----------BITCOMMSG0000%x",ltime); //固定为27个字符
	sprintf(lt,"----------COMPANYMSG%012x",(unsigned int)ltime); //固定为32个字符
	printf("%s:lt is %s\n",__func__,lt);
	//***********过车消息(信息)***********//

	char str_tmp[128];
	memset(str_tmp,0,128);

	sprintf(lc_position, "%d,%d,%d,%d", db_traffic_record->coordinate_x, db_traffic_record->coordinate_y, db_traffic_record->width, db_traffic_record->height);
	sprintf(str_tmp,"%s\r\n"\
					"Content-Type: application/json;charset=UTF-8\r\n\r\n",lt);
	int len_tmp = strlen(str_tmp);

	char PlateColor[12] = "0";
	sprintf(PlateColor,"%d",db_traffic_record->plate_color);

	char PlateNum[64] = "0";
	convert_enc("GBK", "UTF-8", db_traffic_record->plate_num,32,PlateNum,64);

	Json::Value root;
	Json::StyledWriter style_writer;

	root["Longitude"] = 0.0;						                        //抓拍点经度(不支持)
	root["Latitude"] = 0.0;							                        //抓拍点纬度(不支持)
	root["VehicleInfoState"] = 0;                               //过车信息状态，0:实时过车 1:历史过车 int
	root["IsPicUrl"] = 0;                   			              //图片类型,0:在消息体中包含图片，1:通过图片地址寻找 出入口为0
	root["LaneIndex"] = db_traffic_record->lane_num;            //车道号 int

	//	root["position"] = lc_position;                                 //车牌在第一张图片的位置
	root["position"][0u] = (db_traffic_record->coordinate_x);                                 //车牌在第一张图片的位置
	root["position"][1] = (uint)db_traffic_record->coordinate_y;
	root["position"][2] = db_traffic_record->width;
	root["position"][3] = db_traffic_record->height;
	root["direction"] = db_traffic_record->direction;                               //车牌号  字符串
	root["PlateInfo1"] = PlateNum;                               //主车牌号  字符串
	root["PlateInfo2"] = "鲁B88888";             				//辅车牌号  字符串  //暂时未赋值

	root["PlateColor"] = db_traffic_record->plate_color;//PlateColor;                            //车牌颜色  字符串
	root["PlateType"] = db_traffic_record->plate_type;          //车牌类型  int
	root["PassTime"] = db_traffic_record->time;                 //过车时间 格式YYYY-MM-DD HH(24):MI:S  字符串
	root["VehicleSpeed"] = (double)db_traffic_record->speed;    //车辆速度 double
	root["LaneMiniSpeed"] = 0.0;                                //车道低限速，无此功能填0.0  double
	root["LaneMaxSpeed"] = 0.0;                                 //车道高限速，无此功能填0.0  double
	root["VehicleType"] = db_traffic_record->vehicle_type;      //车辆类型 int
	root["VehicleSubType"] = 0;                                 //车辆子类型
	root["VehicleColor"] = db_traffic_record->color;            //车辆颜色 int
	root["VehicleColorDepth"] = 0;        //车辆颜色深浅
	root["VehicleLength"] = 0;                                  //车辆长度，不支持填0   int
	root["VehicleState"] = 1;								    //行车状态  int
	root["PicCount"] = 1;                                       //图片张数 int

//	root["PicType"] = "[1]";                                        //这个怎么赋值
	root["PicType"][0u] = 1;                                        //按照数组赋值

	root["PlatePicUrl"] = "";                                 //车牌照片url
	root["VehiclePic1Url"] = "";                              //车辆照片url
	root["VehiclePic2Url"] = "";
	root["VehiclePic3Url"] = "";
	root["CombinedPicUrl"] = "";  			                      //图片地址
	root["AlarmAction"] = 1;              			                //违法类型

	std::string  str_vehicalinfo = style_writer.write(root);
	std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;


	//***********过车消息体(图片)***********//
	char str_vehicalpic_head[72];
	memset(str_vehicalpic_head,0,72);
	sprintf(str_vehicalpic_head,"--%s\r\n"\
						   "Content-Type: image/jpeg\r\n",lt);

	int len_str_vehicalpic_head = strlen(str_vehicalpic_head);

	char str_vehicalpic_tail[48];
	memset(str_vehicalpic_tail,0,48);
	sprintf(str_vehicalpic_tail,"--%s--\r\n",lt);

	int len_str_vehicalpic_tail = strlen(str_vehicalpic_tail);

	//***********过车消息头***********//
	char str_vehical[1024*1024];
	memset(str_vehical,0,1024*1024);
	sprintf(str_vehical,"POST /COMPANY/Devices/%s$0/Datas HTTP/1.1\r\n"\
						"Content-Type: multipart/form-data;boundary=%s\r\n"\
						"Content-Length: %d\r\n"\
						"Host: %s:%d\r\n"\
						"Connection: Keep-alive\r\n"\
						"User-Agent: ice_wind\r\n\r\n"\
						"--%s\r\n"\
					     "Content-Type: application/json;charset=UTF-8\r\n",

						msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.DeviceId, \
						lt, \
						len_tmp + str_vehicalinfo.length() + len_str_vehicalpic_head + pic_info->size + len_str_vehicalpic_tail , \
						msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp, msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.MsgPort, \
						lt);

	//len_tmp + str_vehicalinfo.length() + len_str_vehicalpic_head + pic_info->size + len_str_vehicalpic_tail,

	//***********拼接***********//
	strcat(str_vehical,str_vehicalinfo.c_str()); //消息头 + 过车信息

	strcat(str_vehical,str_vehicalpic_head);     //消息头 + 过车信息 + 过车图片头信息

	int len = strlen(str_vehical);               //消息头 + 过车信息 + 过车图片头信息的字符串长度

	memcpy(str_vehical + len,pic_info->buf,pic_info->size);  //消息头 + 过车信息 + 过车图片头信息 + 图片

	memcpy(str_vehical + len + pic_info->size,str_vehicalpic_tail,len_str_vehicalpic_tail); //消息头 + 过车信息 + 过车图片头信息 + 图片 + 消息尾  总大小


	TRACE_LOG_PLATFROM_INTERFACE("alleyway bitcom datas = \n%s", str_vehical);


	//bitcom发送大华数据赋值
	if(gstr_bitcom_data.flag == 0)
	{
		gstr_bitcom_data.buf_lens = len + pic_info->size + len_str_vehicalpic_tail;
		memcpy(gstr_bitcom_data.buf, str_vehical, gstr_bitcom_data.buf_lens);

		gstr_bitcom_data.flag = 1;
	}
    return 0;
}

/*******************************************************************************
 * 函数名: bitcom_to_dahua_illegal
 * 功  能: bitcom to dahua 违法类型
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_illegal(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->color)
	{
		//违章停车
		case BITCOM_ILLEGAL_PARKING_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_PARKING_VEHICLE;
		break;

		//压线
		case BITCOM_ILLEGAL_LINEBALL_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_LINEBALL_VEHICLE;
		break;

		//不按车道行驶
		case BITCOM_ILLEGAL_UNLANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_UNLANE_VEHICLE;
		break;

		//压黄线
		case BITCOM_ILLEGAL_PRESSYELLOW_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_PRESSYELLOW_VEHICLE;
		break;

		//违章变道
		case BITCOM_ILLEGAL_LANECHANG_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_LANECHANG_VEHICLE;
		break;

		//逆行
		case BITCOM_ILLEGAL_RETROGRADE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_RETROGRADE_VEHICLE;
		break;

		//闯红灯
		case BITCOM_ILLEGAL_JAYWALK_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_JAYWALK_VEHICLE;
		break;

		//超速
		case BITCOM_ILLEGAL_OVERSPEED_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_OVERSPEED_VEHICLE;
		break;

		//有车占道
		case BITCOM_ILLEGAL_CARLANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_CARLANE_VEHICLE;
		break;

		//非机动车道
		case BITCOM_ILLEGAL_BICYCLELANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_BICYCLELANE_VEHICLE;
		break;

		//机动车违反禁令标志指示
		case DAHUA_ILLEGAL_FLAG1_VEHICLE:
		case DAHUA_ILLEGAL_FLAG2_VEHICLE:
		case DAHUA_ILLEGAL_FLAG3_VEHICLE:
		case DAHUA_ILLEGAL_FLAG4_VEHICLE:
		case DAHUA_ILLEGAL_FLAG5_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_FLAG_VEHICLE;
		break;

		//其它
		default:
			as_traffic_record->color = DAHUA_ILLEGAL_OTHERS_VEHICLE;
		break;

	}

	return 0;
}


/*******************************************************************************
 * 函数名: bitcom_to_dahua_vehiclecolour
 * 功  能: bitcom to dahua 车辆颜色转换
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_vehiclecolour(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->color)
	{
		//白色
		case BITCOM_COLOUR_WHITE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_WHITE_VEHICLE;
		break;

		//黑色
		case BITCOM_COLOUR_BLACK_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_BLACK_VEHICLE;
		break;

		//红色
		case BITCOM_COLOUR_RED_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_RED_VEHICLE;
		break;

		//黄色
		case BITCOM_COLOUR_YELLOW_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_YELLOW_VEHICLE;
		break;

		//银灰色
		case BITCOM_COLOUR_SILVERGREY_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_SILVERGREY_VEHICLE;
		break;

		//蓝色
		case BITCOM_COLOUR_BLUE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_BLUE_VEHICLE;
		break;

		//绿色
		case BITCOM_COLOUR_GREEN_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_GREEN_VEHICLE;
		break;

		//紫色
		case BITCOM_COLOUR_PURPLE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_PURPLE_VEHICLE;
		break;

		//棕色
		case BITCOM_COLOUR_BROWN_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_OTHERS_VEHICLE;
		break;

		//其它
		case BITCOM_COLOUR_OTHERS_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_OTHERS_VEHICLE;
		break;

		//未识别
		default:
			as_traffic_record->color = DAHUA_COLOUR_UNKNOW_VEHICLE;
		break;

	}
	return 0;
}



/*******************************************************************************
 * 函数名: bitcom_to_dahua_vehicletype
 * 功  能: bitcom to dahua 车辆类型转换
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_vehicletype(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->plate_type)
	{
		//小型车
		case BITCOM_SMALL_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_MINIATURE_VEHICLE;
		break;

		//大型车
		case BITCOM_LARGE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_OVERSIZE_VEHICLE;
		break;

		//使馆汽车
		case BITCOM_EMBASSY_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_EMBASSY_VEHICLE;
		break;

		//领馆汽车
		case BITCOM_CONSULATE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_CONSULATE_VEHICLE;
		break;

		//境外汽车
		case BITCOM_FOREIGN_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_OVERSEAS_VEHICLE;
		break;

		//外籍汽车
		case BITCOM_FOREIGNNATION_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_FOREIGN_VEHICLE;
		break;

		//低速汽车
		case BITCOM_LOWSPEED_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_LOWSPEED_VEHICLE;
		break;

		//拖拉机
		case BITCOM_TRACTOR_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TRACTOR_VEHICLE;
		break;

		//挂车
		case BITCOM_GUA_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TRAILER_VEHICLE;
		break;

		//教练车
		case BITCOM_XUE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_COACH_VEHICLE;
		break;

		//临时行驶车
		case BITCOM_TEMPORARY_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TEMP_VEHICLE;
		break;

		//警用汽车
		case BITCOM_POLICE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_POLICE_VEHICLE;
		break;

		//警用摩托车
		case BITCOM_POLICE_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_POLICEMOTOR_VEHICLE;
		break;

		//普通摩托
		case BITCOM_TRIWHEEL_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_MOTOR_VEHICLE;
		break;

		//轻便摩托车
		case BITCOM_LIGHT_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_LIGHTMOTOR_VEHICLE;
		break;

		//未识别
		case BITCOM_UNKNOWN_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_UNKNOW_VEHICLE;
		break;

		default:
		break;
	}

	return 0;
}



/*******************************************************************************
 * 函数名: bitcom_to_dahua_platetype
 * 功  能: bitcom to dahua 车牌类型
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_platetype(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->plate_type)
	{
		//小型车
		case BITCOM_SMALL_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_MINIATURE_VEHICLE;
		break;

		//大型车
		case BITCOM_LARGE_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_OVERSIZE_VEHICLE;
		break;

		//境外车
		case BITCOM_FOREIGNNATION_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_FOREIGN_VEHICLE;
		break;

		//未识别
		case BITCOM_UNKNOWN_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_UNKONW_VEHICLE;
		break;

		//军警型车辆
		case BITCOM_POLICE_CAR:
		case BITCOM_POLICE_MOTOBIKE:
		case BITCOM_ARMY_LARGE_CAR:
		case BITCOM_ARMY_SMALL_CAR:
		case BITCOM_WJ_LARGE_CAR:
		case BITCOM_WJ_SMALL_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_MILITARY_VEHICLE;
		break;

		default:
			as_traffic_record->plate_type = DAHUA_PLATE_OTHERS_VEHICLE;
		break;
	}

	return 0;
}


/*******************************************************************************
 * 函数名: bitcom_to_dahua_direction
 * 功  能: bitcom to dahua 车牌行驶方向转换
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_direction(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->direction)
	{
		//东向西
		case BITCOM_EAST_TO_WEST:
			as_traffic_record->direction = DAHUA_EAST_TO_WEST;
		break;

		//西向东
		case BITCOM_WEST_TO_EAST:
			as_traffic_record->direction = DAHUA_WEST_TO_EAST;
		break;

		//南向北
		case BITCOM_NORTH_TO_SOUTH:
			as_traffic_record->direction = DAHUA_NORTH_TO_SOUTH;
		break;

		//北向南
		case BITCOM_SOUTH_TO_NORTH:
			as_traffic_record->direction = DAHUA_SOUTH_TO_NORTH;
		break;

		//东南向西北
		case BITCOM_SOUTHEAST_TO_NORTHWEST:
			as_traffic_record->direction = DAHUA_SOUTHEAST_TO_NORTHWEST;
		break;

		//西北向东南
		case BITCOM_NORTHWEST_TO_SOUTHEAST:
			as_traffic_record->direction = DAHUA_NORTHWEST_TO_SOUTHEAST;
		break;

		//东北向西南
		case BITCOM_NORTHEAST_TO_SOUTHWEST:
			as_traffic_record->direction = DAHUA_NORTHEAST_TO_SOUTHWEST;
		break;

		//西南向东北
		case BITCOM_SOUTHWEST_TO_NORTHEAST:
			as_traffic_record->direction = DAHUA_SOUTHWEST_TO_NORTHEAST;
		break;

	}
	return 0;
}

/*******************************************************************************
 * 函数名: bitcom_to_dahua_platecolour
 * 功  能: bitcom to dahua 车牌颜色转换
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_platecolour(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->plate_color)
	{
		case BITCOM_BLUE:
			as_traffic_record->plate_color = DAHUA_PLATE_BLUE;
		break;

		case BITCOM_YELLOW:
			as_traffic_record->plate_color = DAHUA_PLATE_YELLOW;
		break;

		case BITCOM_WHITE:
			as_traffic_record->plate_color = DAHUA_PLATE_WHITE;
		break;

		case BITCOM_BLACK:
			as_traffic_record->plate_color = DAHUA_PLATE_BLACK;
		break;

		case BITCOM_OTHERS:
			as_traffic_record->plate_color = DAHUA_PLATE_OTHERS;
		break;

		case BITCOM_UNKNOW:
			as_traffic_record->plate_color = DAHUA_PLATE_UNKNOW;
		break;

		default:
			as_traffic_record->plate_color = DAHUA_PLATE_UNKNOW;
		break;
	}
	return 0;
}


/*******************************************************************************
 * 函数名: bitcom_to_dahua_protocol
 * 功  能: bitcom to dahua 协议转换
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int bitcom_to_dahua_protocol(DB_TrafficRecord *as_traffic_record)
{
	//车牌类型转换
	bitcom_to_dahua_platetype(as_traffic_record);

	//车牌颜色转换
	bitcom_to_dahua_platecolour(as_traffic_record);

	//行驶方向转换
	bitcom_to_dahua_direction(as_traffic_record);

	//车辆类型转换
	bitcom_to_dahua_vehicletype(as_traffic_record);

	//车辆颜色转换
	bitcom_to_dahua_vehiclecolour(as_traffic_record);

	return 0;
}



/*******************************************************************************
 * 函数名: process_VM_record
 * 功  能: 处理单个机动车通行记录
 * 参  数: result，要处理的结果
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int process_vm_records_motor_vehicle(const SystemVpss_SimcopJpegInfo *info)
{
	log_state("VD:","In process_traffic_record_motor_vehicle!\n");

	if (info == NULL)
		return -1;

	const TrfcVehiclePlatePoint *result =
	    (TrfcVehiclePlatePoint *) &(info->algResultInfo.AlgResultInfo);

	log_state("VD:","result: %s\n", result->strResult);

	printf_with_ms("in %s: recv vm record\n",__func__);

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);

	EP_PicInfo pic_info;
	DB_TrafficRecord db_traffic_record;
	memset(&pic_info,0,sizeof(EP_PicInfo));
	memset(&db_traffic_record,0,sizeof(DB_TrafficRecord));
	int flag_send = 0;

	do /* 处理图片 */
	{
		analyze_traffic_records_picture(&pic_info, info);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			if (send_traffic_records_picture_buf(&pic_info) < 0)
			{
				flag_send |= 0x02; 	/* 图片上传失败，需要续传 */
				log_send(LOG_LEVEL_FAULT,0,"VD:","Send traffic record picture of motor vehicle failed !\n");
				TRACE_LOG_SYSTEM("ftp VM send picture failed!");
			}
			else
			{
				TRACE_LOG_SYSTEM("ftp VM send picture successful!");
			}
		}

		if (((flag_send & 0x02) != 0) || (EP_DISK_SAVE_CFG.vehicle == 1))
		{
			if (disk_status == 0)
			{
				disk_status = save_traffic_records_picture_buf(&pic_info, partition_path);

				if (disk_status < 0)
				{
					log_error("VD:","Save traffic record picture of motor vehicle failed !\n");
				}
			}
			else
			{
				log_error("VD:","The disk is not available, traffic record picture of motor vehicle is discarded !\n");

			}
		}
	}
	while (0);

	do /* 处理信息 */
	{
		analyze_traffic_records_info_motor_vehicle(
		    &db_traffic_record, result, &pic_info, partition_path);

		TRACE_LOG_SYSTEM("picture1=%s", pic_info.name);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			flag_send |= 0x01; 		/* 先设置为需要续传 */

			if (((flag_send & 0x02) == 0) &&
			        (send_traffic_records_info(&db_traffic_record) == 0))
			{
				flag_send = 0x00; 	/* 如果上传成功，再恢复为无需上传 */
			}
		}


		if (((flag_send & 0x01) != 0) || (EP_DISK_SAVE_CFG.vehicle == 1))
		{
			if (disk_status == 0)
			{
				db_traffic_record.flag_send 	= flag_send;
				db_traffic_record.flag_store 	= EP_DISK_SAVE_CFG.vehicle;

				if (save_traffic_records_info(&db_traffic_record,
				                             &pic_info, partition_path) < 0)
				{
					log_error("VD:","Save traffic record information of motor vehicle failed !\n");
				}
			}
			else
			{
				log_error("VD:","The disk is not available, traffic record information of motor vehicle is discarded !\n");
			}
		}

		TRACE_LOG_SYSTEM("flg_register_NetPose_vehicle=%d, flg_register_Dahua=%d,",
				flg_register_NetPose_vehicle, flg_register_Dahua);


		//东方网力平台
		if(flg_register_NetPose_vehicle == 1)
		{
			INFO("vehicle record send to netpose");
			send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(&db_traffic_record,&pic_info);
		}

		//大华平台
		if(flg_register_Dahua == 1)
		{
			INFO("vehicle record send to dahua");
			send_vm_records_motor_vehicle_to_Dahua(&db_traffic_record,&pic_info);
		}
	}while (0);

	printf_with_ms("in %s: after process vm record\n",__func__);

	return 0;
}

/*******************************************************************************
 * 函数名: process_traffic_record_others
 * 功  能: 处理其他交通记录
 * 参  数: result，要处理的结果
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int process_vm_records_others(const SystemVpss_SimcopJpegInfo *info)
{

	printf("In process_vm_records_others\n");

	if (info == NULL)
		return -1;
	const NoVehiclePoint *result =
		    (NoVehiclePoint *) &(info->algResultInfo.AlgResultInfo);

	char partition_path[MOUNT_POINT_LEN];
	memset(partition_path, 0, MOUNT_POINT_LEN);

	int disk_status = get_cur_partition_path(partition_path);

	EP_PicInfo pic_info;
	DB_TrafficRecord db_traffic_record;
	memset(&pic_info,0,sizeof(EP_PicInfo));
	memset(&db_traffic_record,0,sizeof(DB_TrafficRecord));

	int flag_send = 0;

	do /* 处理图片 */
	{
//		AV_DATA av_data;

//		GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data);

//		if (get_pic_info_lock(&av_data, &pic_info, av_data.serial) < 0)
//			return -1;

//		analyze_traffic_record_picture(&pic_info, result);
		analyze_traffic_records_picture(&pic_info, info);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			if (send_traffic_records_picture_buf(&pic_info) < 0)
			{
				flag_send |= 0x02; 	/* 图片上传失败，需要续传 */
				log_debug_storage("Send traffic record image of pedestrian "
				                  "or non-motor vehicle failed !\n");
			}
		}

		if (((flag_send & 0x02) != 0) || (EP_DISK_SAVE_CFG.vehicle == 1))
		{
			if (disk_status == 0)
			{
//				disk_status =
//				    save_traffic_record_image(&pic_info, partition_path);
				disk_status = save_traffic_records_picture_buf(&pic_info, partition_path);
				if (disk_status < 0)
				{
					log_warn_storage("Save traffic record image of pedestrian "
					                 "or non-motor vehicle failed !\n");
				}
			}
			else
			{
				log_warn_storage(
				    "The disk is not available, traffic record image of "
				    "pedestrian or non-motor vehicle is discarded.\n");
			}
		}

//		get_pic_info_unlock(&av_data, av_data.serial);
	}
	while (0);

	do /* 处理信息 */
	{
		analyze_traffic_record_info_others(
		    &db_traffic_record, result, &pic_info, partition_path);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			flag_send |= 0x01; 		/* 先设置为需要续传 */

			if (((flag_send & 0x02) == 0) &&
			        (send_traffic_records_info(&db_traffic_record) == 0))
			{
				flag_send = 0x00; 	/* 如果上传成功，再恢复为无需上传 */
			}
		}

		if (((flag_send & 0x01) != 0) || (EP_DISK_SAVE_CFG.vehicle == 1))
		{
			if (disk_status == 0)
			{
				db_traffic_record.flag_send 	= flag_send;
				db_traffic_record.flag_store 	= EP_DISK_SAVE_CFG.vehicle;

				if (save_traffic_records_info(&db_traffic_record,
				                             &pic_info, partition_path) < 0)
				{
					log_error_storage(
					    "Save traffic record information of "
					    "pedestrian or non-motor vehicle failed !\n");
				}
			}
			else
			{
				log_debug_storage(
				    "The disk is not available, traffic record information of "
				    "pedestrian or non-motor vehicle is discarded.\n");
			}
		}



		//send_vm_records_motor_vehicle_to_NetPose_moter_vehicle shoud be NetPose face!!!!!
		if(flg_register_NetPose_vehicle == 1)
		{
			INFO("others record send to netpose");
			send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(&db_traffic_record,&pic_info);
		}


		//alleyway_senddatas_to_bitcom(&db_traffic_record, &pic_info);

	}
	while (0);

	return 0;
}




/*******************************************************************************
 * 函数名: analyze_traffic_record_picture
 * 功  能: 解析机动车通行记录图片
 * 参  数: pic_info，解析出来的图片信息；lane_num，车道号
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int analyze_traffic_records_picture(
    EP_PicInfo *pic_info, const SystemVpss_SimcopJpegInfo *info)
{
	static int count = 0;
	count++;
	const VD_Time *tm = &(info->algResultInfo.EPtime);
	const TrfcVehiclePlatePoint *result =
	    (TrfcVehiclePlatePoint *) &(info->algResultInfo);

	pic_info->tm.year 		= tm->tm_year;
	pic_info->tm.month 		= tm->tm_mon;
	pic_info->tm.day 		= tm->tm_mday;
	pic_info->tm.hour 		= tm->tm_hour;
	pic_info->tm.minute 	= tm->tm_min;
	pic_info->tm.second 	= tm->tm_sec;
	pic_info->tm.msecond 	= tm->tm_msec;

	/* 路径按照用户的配置进行设置 */
	int len = 0;
	for (int j = 0; j<EP_FTP_URL_LEVEL.levelNum; j++)
	{
		switch (EP_FTP_URL_LEVEL.urlLevel[j])
		{
		case SPORT_ID: 		//地点编号
			len += sprintf(pic_info->path[j], "/%s", EP_POINT_ID);
			break;
		case DEV_ID: 		//设备编号
			len += sprintf(pic_info->path[j], "/%s", EP_DEV_ID);
			break;
		case YEAR_MONTH: 	//年/月
			len += sprintf(pic_info->path[j], "/%04d%02d",
			               pic_info->tm.year, pic_info->tm.month);
			break;
		case DAY: 			//日
			len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.day);
			break;
		case EVENT_NAME: 	//事件类型
			len += sprintf(pic_info->path[j], "/%s",
			               TRAFFIC_RECORDS_FTP_DIR);
			break;
		case HOUR: 			//时
			len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.hour);
			break;
		case FACTORY_NAME: 	//厂商名称
			len += sprintf(pic_info->path[j], "/%s", EP_MANUFACTURER);
			break;
		default:
			break;
		}
		printf("pic_info.path : %s\n",pic_info->path[j]);
	}


	/* 文件名以时间和车道号命名 */
	snprintf(pic_info->name, sizeof(pic_info->name),
	         "%s%04d%02d%02d%02d%02d%02d%03d%02d%d.jpg",
	         EP_EXP_DEV_ID,
	         pic_info->tm.year,
	         pic_info->tm.month,
	         pic_info->tm.day,
	         pic_info->tm.hour,
	         pic_info->tm.minute,
	         pic_info->tm.second,
	         pic_info->tm.msecond,
	         result->laneNum,count);

	pic_info->buf 	= (void *) ((char *)info + SIZE_JPEG_INFO);
	pic_info->size 	= info->jpeg_buf_size;

	printf("pic_info.name is : %s\n",pic_info->name);

	return 0;
}

/*******************************************************************************
 * 函数名: send_traffic_records_image_buf
 * 功  能: 发送缓存中的通行记录图片
 * 参  数: pic_info，图片文件信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_traffic_records_picture_buf(const EP_PicInfo *pic_info)
{
    if (ftp_get_status(FTP_CHANNEL_PASSCAR) < 0)
    {
		return -1;
    }

	if (ftp_send_pic_buf(pic_info, FTP_CHANNEL_PASSCAR) < 0)
	{
		return -1;
	}
	return 0;
}

/*******************************************************************************
 * 函数名: save_traffic_records_image_buf
 * 功  能: 保存缓存中的通行记录图片
 * 参  数: pic_info，图片文件信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_traffic_records_picture_buf(
    const EP_PicInfo *pic_info, const char*partition_path)
{
	char record_path[PATH_MAX_LEN];
	sprintf(record_path, "%s/%s", partition_path, DISK_RECORD_PATH);
	int ret = dir_create(record_path);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", record_path);
		return -1;
	}

	ret = file_save(record_path, pic_info->path, pic_info->name, pic_info->buf,
	                pic_info->size);
	return ret;
}


/*******************************************************************************
 * 函数名: analyze_traffic_record_info_motor_vehicle
 * 功  能: 解析机动车通行记录信息
 * 参  数: db_traffic_record，解析的结果；
 *         image_info，图像信息；pic_info，图片文件信息
 * 返回值: 成功，返回0
*******************************************************************************/
int analyze_traffic_records_info_motor_vehicle(
    DB_TrafficRecord *db_traffic_record, const TrfcVehiclePlatePoint *result,
    const EP_PicInfo *pic_info, const char *partition_path)
{
	memset(db_traffic_record, 0, sizeof(DB_TrafficRecord));

	snprintf(db_traffic_record->plate_num,
	         sizeof(db_traffic_record->plate_num),
	         "%s", result->strResult);

	db_traffic_record->plate_type = result->plateType;

	snprintf(db_traffic_record->point_id,
	         sizeof(db_traffic_record->point_id),
	         "%s", EP_POINT_ID);

	snprintf(db_traffic_record->point_name,
	         sizeof(db_traffic_record->point_name),
	         "%s", EP_POINT_NAME);

	snprintf(db_traffic_record->dev_id,
	         sizeof(db_traffic_record->dev_id),
	         "%s", EP_EXP_DEV_ID);

	db_traffic_record->lane_num = result->laneNum;

	db_traffic_record->speed = result->speed;

	snprintf(db_traffic_record->time,
	         sizeof(db_traffic_record->time),
	         "%04d-%02d-%02d %02d:%02d:%02d",
	         pic_info->tm.year, pic_info->tm.month, pic_info->tm.day,
	         pic_info->tm.hour, pic_info->tm.minute, pic_info->tm.second);

	snprintf(db_traffic_record->collection_agencies,
	         EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);

	db_traffic_record->direction = EP_DIRECTION;

//	snprintf(db_traffic_record->image_path,
//	         sizeof(db_traffic_record->image_path),
//	         "%s/%s",
//	         pic_info->path, pic_info->name);

		get_ftp_path(EP_FTP_URL_LEVEL,
				            pic_info->tm.year,
				            pic_info->tm.month,
				            pic_info->tm.day,
				            pic_info->tm.hour,
				             (char*) TRAFFIC_RECORDS_FTP_DIR,
				             db_traffic_record->image_path);

		strcat(db_traffic_record->image_path, pic_info->name);
		printf("db_park_record->image_path: %s\n",db_traffic_record->image_path);

	snprintf(db_traffic_record->partition_path,
	         sizeof(db_traffic_record->partition_path),
	         "%s", partition_path);

	db_traffic_record->color 			= result->vehicleColor;
	db_traffic_record->vehicle_logo 	= result->vehicleLogo;
	db_traffic_record->objective_type 	= 1; /* 1表示机动车 */
	db_traffic_record->coordinate_x 	= result->xPos;
	db_traffic_record->coordinate_y 	= result->yPos;
	db_traffic_record->width 			= result->width;
	db_traffic_record->height 			= result->height;
	db_traffic_record->pic_flag 		= 1; /* 目前只有1张图片 */
	db_traffic_record->plate_color 		= result->color;
	db_traffic_record->confidence		= result->confidence;
#if (5 == DEV_TYPE)
	db_traffic_record->detect_coil_time = result->triggerCoilTime;
#endif

	snprintf(db_traffic_record->description,
	         sizeof(db_traffic_record->description),
	         "图像编号: %d", result->pic_id);

	snprintf(db_traffic_record->ftp_user,
	         sizeof(db_traffic_record->ftp_user),
	         "%s", EP_TRAFFIC_FTP.user);

	snprintf(db_traffic_record->ftp_passwd,
	         sizeof(db_traffic_record->ftp_passwd),
	         "%s", EP_TRAFFIC_FTP.passwd);

	snprintf(db_traffic_record->ftp_ip,
	         sizeof(db_traffic_record->ftp_ip),
	         "%d.%d.%d.%d",
	         EP_TRAFFIC_FTP.ip[0],
	         EP_TRAFFIC_FTP.ip[1],
	         EP_TRAFFIC_FTP.ip[2],
	         EP_TRAFFIC_FTP.ip[3]);

	db_traffic_record->ftp_port = EP_TRAFFIC_FTP.port;

	db_traffic_record->vehicle_type = result->vehicleType;
	db_traffic_record->obj_state = result->objectState;

	return 0;
}

/*******************************************************************************
 * 函数名: send_traffic_record_info
 * 功  能: 发送通行记录信息
 * 参  数: db_traffic_records，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_traffic_records_info(const DB_TrafficRecord *db_traffic_record)
{
	printf("In send_traffic_record_info\n");
	if (mq_get_status_traffic_record() < 0)
		return -1;

	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);

	format_mq_text_traffic_record(mq_text, db_traffic_record);

	debug_print("text: %s\n", mq_text);

	TRACE_LOG_SYSTEM("mq_text = %s", mq_text);

	mq_send_traffic_record(mq_text);

	return 0;
}

/*******************************************************************************
 * 函数名: save_traffic_record_info
 * 功  能: 保存通行记录信息
 * 参  数: db_traffic_record，通行记录信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_traffic_records_info(
    const DB_TrafficRecord *db_traffic_record,
    const EP_PicInfo *pic_info, const char *partition_path)
{
	resume_print("In save_traffic_record_info!\n");

	char db_traffic_name[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/traffic_records_2013082109.db
	static DB_File db_file;
	int flag_record_in_DB_file=0;//是否需要记录到索引数据库

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/traffic_record");
	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}


	sprintf(db_traffic_name,"%s/%04d%02d%02d%02d.db", dir_temp,pic_info->tm.year,pic_info->tm.month,pic_info->tm.day,pic_info->tm.hour);
	resume_print("db_traffic_name: %s\n",db_traffic_name);

	//判断数据库文件是否已经存在
	//若不存在，需要相应建立一条索引记录
	//续传后，可能没有删记录数据库文件，却删了对应的索引记录。
	if (access(db_traffic_name, F_OK) != 0)
	{
		resume_print("The db_traffic is not exit\n");
		//数据库文件不存在，需要在索引数据库中增加相应的一条记录
		flag_record_in_DB_file=1;
	}
	else
	{
		resume_print("The db_traffic is  exit\n");
		flag_record_in_DB_file=0;
	}



	//检索索引数据库中，是否存在该记录数据库名
	//不能只依赖于是否有续传数据，还有实时存储。
	//if ((flag_upload_complete_traffic==1)||(strcasecmp(db_traffic_name_last, db_traffic_name)))//是新数据库名称

	//若开启实时存储，只依赖于数据库文件是否存在:
	//----若不存在，添加索引记录；若存在，不添加。
	//若不开启实时存储，
	//----若不存在数据库文件，添加索引记录；
	//若存在数据库文件，另外判断是否存在续传数据:
	//       ----有续传数据，不添加；没有续传数据，添加。
	//if((flag_record_in_DB_file==1)||((flag_upload_complete_traffic==1)&&(EP_DISK_SAVE_CFG.vehicle==0)))

	if (flag_record_in_DB_file==1)//保证，索引记录的删除，同时删对应交通数据库
	{

		resume_print("add to DB_files, db_traffic_name is %s\n", db_traffic_name);

		//写一条记录到数据库名管理


		db_file.record_type = 0;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_traffic_name, strlen(db_traffic_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_traffic_record->time, strlen(
		           db_traffic_record->time));
		db_file.flag_send = db_traffic_record->flag_send;
		db_file.flag_store= db_traffic_record->flag_store;

		ret = db_write_DB_file((char*)DB_NAME_VM_RECORD, &db_file,
		                       &mutex_db_files_traffic);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf("db_write_DB_file %s in %s ok.\n", db_traffic_name,
			       (char*)DB_NAME_VM_RECORD);
		}
	}

	else
	{

		// 第一次写索引数据库时，可能是两个条件中任意一个触发，（断网、实时存储）
		// 所以，在另一个条件发生时，需要相应修改索引数据库。
		char sql_cond[1024];
		static int flag_first=1;
		if (flag_first==1)
		{
			//第一次进入，要读取最新的一条索引记录。
			flag_first=0;

			//char sql_cond[1024];
			char history_time_start[64];			//时间起点 示例：2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_traffic_record->time, history_time_start);
			sprintf(sql_cond, "SELECT * FROM DB_files WHERE time>='%s'  limit 1;", history_time_start );
			db_read_DB_file(DB_NAME_VM_RECORD, &db_file, &mutex_db_files_traffic, sql_cond);
		}

		printf("db_file.flag_send=%d,db_traffic_records->flag_send=%d\n",db_file.flag_send,db_traffic_record->flag_send);
		if (( ~(db_file.flag_send) & db_traffic_record->flag_send)!=0)	//若新记录中包含了新的续传标志信息
		{
			db_file.flag_send |=db_traffic_record->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;", db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}

		if ( (db_file.flag_store==0) && (db_traffic_record->flag_store!=0))//需要增加存储信息
		{
			db_file.flag_store=db_traffic_record->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;", db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}



	}
		//memset(db_traffic_name_last, 0, sizeof(db_traffic_name_last));
	//memcpy(db_traffic_name_last, db_traffic_name, sizeof(db_traffic_name));

	ret		= db_write_traffic_records(db_traffic_name,
	                                   (DB_TrafficRecord *) db_traffic_record,
	                                   &mutex_db_records_traffic);
	if (ret != 0)
	{
		printf("db_write_traffic_records failed\n");
		return -1;
	}


	return ret;
}

/*******************************************************************************
 * 函数名: format_mq_text_traffic_record
 * 功  能: 格式化交通通行记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；record，过车记录
 * 返回值: 字符串长度
*******************************************************************************/
int format_mq_text_traffic_record(
    char *mq_text, const DB_TrafficRecord *record)
{
	if (record->objective_type == 1) 	/* 机动车 */
		return format_mq_text_traffic_record_motor_vehicle(mq_text, record);
	else 								/* 行人或非机动车 */
		return format_mq_text_traffic_record_others(mq_text, record);
}

/*******************************************************************************
 * 函数名: format_mq_text_traffic_record_motor_vehicle
 * 功  能: 格式化机动车交通通行记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；record，过车记录
 * 返回值: 字符串长度
*******************************************************************************/
int format_mq_text_traffic_record_motor_vehicle(
    char *mq_text, const DB_TrafficRecord *record)
{
	int len = 0;

	len += sprintf(mq_text + len, "%s", EP_DATA_SOURCE);
	/* 字段1 数据来源 */

	len += sprintf(mq_text + len, ",%s", record->plate_num);
	/* 字段2 车牌号码 */

	len += sprintf(mq_text + len, ",%02d", record->plate_type);
	/* 字段3 号牌类型 */

	len += sprintf(mq_text + len, ",%s", record->point_id);
	/* 字段4 采集点编号 */

	len += sprintf(mq_text + len, ",%s", record->point_name);
	/* 字段5 采集点名称 */

	len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
	/* 字段6 抓拍类型 */

	len += sprintf(mq_text + len, ",%s", record->dev_id);
	/* 字段7 设备编号 */

	len += sprintf(mq_text + len, ",%02d", record->lane_num);
	/* 字段8 车道编号 */

	len += sprintf(mq_text + len, ",%d", record->speed);
	/* 字段9 车辆速度 */

	len += sprintf(mq_text + len, ",%s", record->time);
	/* 字段10 抓拍时间 */

	len += sprintf(mq_text + len, ",%s", record->collection_agencies);
	/* 字段11 采集机关编号 */

	len += sprintf(mq_text + len, ",%d", record->direction);
	/* 字段12 方向编号 */

	len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
	               record->ftp_user, record->ftp_passwd,
	               record->ftp_ip, record->ftp_port, record->image_path);
	/* 字段13 第一张图片地址 */

	len += sprintf(mq_text + len, ",,");
	/* 字段14、15 第二三张图片地址，目前没有 */

	len += sprintf(mq_text + len, ",%d", record->color);
	/* 字段16 车身颜色编号 */

	len += sprintf(mq_text + len, ",%d", record->vehicle_logo);
	/* 字段17 车标编号 */

	len += sprintf(mq_text + len, ",%d", record->objective_type);
	/* 字段18 目标类型 */

	len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d",
	               record->coordinate_x, record->coordinate_y,
	               record->width, record->height, record->pic_flag);
	/* 字段19 车牌定位（X坐标、Y坐标、宽度、高度、第几张图片） */

	len += sprintf(mq_text + len, ",%d", record->plate_color);
	/* 字段20 车牌颜色 */

	len += sprintf(mq_text + len, ",,,,,,,,,,");
	/* 字段21到30 未用 */

	len += sprintf(mq_text + len, ",%d", record->plate_color);
	/* 字段31 车牌颜色 */

	len += sprintf(mq_text + len, ",");
	/* 字段32 未用 */

	len += sprintf(mq_text + len, ",%s", record->description);
	/* 字段33 过车描述 */

	len += sprintf(mq_text + len, ",%d", record->vehicle_type);
	/* 字段34 车型 */

	return len;
}

/*******************************************************************************
 * 函数名: format_mq_text_traffic_record_others
 * 功  能: 格式化行人或非机动车交通通行记录MQ文本信息
 * 参  数: mq_text，MQ文本信息缓冲区；record，过车记录
 * 返回值: 字符串长度
*******************************************************************************/
int format_mq_text_traffic_record_others(
    char *mq_text, const DB_TrafficRecord *record)
{
	int len = 0;

	len += sprintf(mq_text + len, "%s", EP_DATA_SOURCE);
	/* 字段1 数据来源 */

	len += sprintf(mq_text + len, ",");
	/* 字段2 车牌号码，行人和非机动车没有 */

	len += sprintf(mq_text + len, ",");
	/* 字段3 号牌类型，行人和非机动车没有 */

	len += sprintf(mq_text + len, ",%s", record->point_id);
	/* 字段4 采集点编号 */

	len += sprintf(mq_text + len, ",%s", record->point_name);
	/* 字段5 采集点名称 */

	len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
	/* 字段6 抓拍类型 */

	len += sprintf(mq_text + len, ",%s", record->dev_id);
	/* 字段7 设备编号 */

	len += sprintf(mq_text + len, ",%02d", record->lane_num);
	/* 字段8 车道编号 */

	len += sprintf(mq_text + len, ",");
	/* 字段9 车辆速度，行人和非机动车没有 */

	len += sprintf(mq_text + len, ",%s", record->time);
	/* 字段10 抓拍时间 */

	len += sprintf(mq_text + len, ",%s", record->collection_agencies);
	/* 字段11 采集机关编号 */

	len += sprintf(mq_text + len, ",%d", record->direction);
	/* 字段12 方向编号 */

	len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
	               record->ftp_user, record->ftp_passwd,
	               record->ftp_ip, record->ftp_port, record->image_path);
	/* 字段13 第一张图片地址 */

	len += sprintf(mq_text + len, ",,");
	/* 字段14、15 第二三张图片地址，目前没有 */

	len += sprintf(mq_text + len, ",%d", record->color);
	/* 字段16 衣服颜色编号 */

	len += sprintf(mq_text + len, ",");
	/* 字段17 车标编号，行人和非机动车没有 */

	len += sprintf(mq_text + len, ",%d", record->objective_type);
	/* 字段18 目标类型 */

	len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d",
	               record->coordinate_x, record->coordinate_y,
	               record->width, record->height, record->pic_flag);
	/* 字段19 目标定位（X坐标、Y坐标、宽度、高度、第几张图片） */

	len += sprintf(mq_text + len, ",,,,,,,,,,,");
	/* 字段20到30 未用 */

	len += sprintf(mq_text + len, ",");
	/* 字段31 车牌颜色，行人和非机动车没有 */

	len += sprintf(mq_text + len, ",");
	/* 字段32 未用 */

	len += sprintf(mq_text + len, ",%s", record->description);
	/* 字段33 过车描述 */

	len += sprintf(mq_text+len, ",");
	/* 字段34 车型，行人和非机动车没有 */

	return len;
}

/*******************************************************************************
 * 函数名: db_write_traffic_records
 * 功  能: 写交通通行记录数据库
 * 参  数: records，交通通行记录
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_write_traffic_records(char *db_traffic_name, DB_TrafficRecord *records,
                             pthread_mutex_t *mutex_db_records)
{
	char sql[SQL_BUF_SIZE];

	memset(sql, 0, SQL_BUF_SIZE);

	db_format_insert_sql_traffic_records(sql, records);

	pthread_mutex_lock(mutex_db_records);
	int ret = db_write(db_traffic_name, sql, SQL_CREATE_TABLE_TRAFFIC_RECORDS);
	pthread_mutex_unlock(mutex_db_records);

	if (ret != 0)
	{
		printf("db_write_traffic_records failed\n");
		return -1;
	}

	return 0;
}

/*******************************************************************************
 * 函数名: db_format_insert_sql_traffic_records
 * 功  能: 格式化交通通行记录表SQL插入语句
 * 参  数: sql，SQL缓存；records，通行记录
 * 返回值: SQL长度
*******************************************************************************/
int db_format_insert_sql_traffic_records(char *sql, DB_TrafficRecord *records)
{
	int len = 0;

	len += sprintf(sql, "INSERT INTO traffic_records VALUES(NULL");
	len += sprintf(sql + len, ",'%s'", records->plate_num);
	len += sprintf(sql + len, ",'%d'", records->plate_type);
	len += sprintf(sql + len, ",'%s'", records->point_id);
	len += sprintf(sql + len, ",'%s'", records->point_name);
	len += sprintf(sql + len, ",'%s'", records->dev_id);
	len += sprintf(sql + len, ",'%02d'", records->lane_num);
	len += sprintf(sql + len, ",'%d'", records->speed);
	len += sprintf(sql + len, ",'%s'", records->time);
	len += sprintf(sql + len, ",'%s'", records->collection_agencies);
	len += sprintf(sql + len, ",'%d'", records->direction);
	len += sprintf(sql + len, ",'%s'", records->image_path);
	len += sprintf(sql + len, ",'%s'", records->partition_path);
	len += sprintf(sql + len, ",'%d'", records->color);
	len += sprintf(sql + len, ",'%d'", records->vehicle_logo);
	len += sprintf(sql + len, ",'%d'", records->objective_type);
	len += sprintf(sql + len, ",'%d'", records->coordinate_x);
	len += sprintf(sql + len, ",'%d'", records->coordinate_y);
	len += sprintf(sql + len, ",'%d'", records->width);
	len += sprintf(sql + len, ",'%d'", records->height);
	len += sprintf(sql + len, ",'%d'", records->pic_flag);
	len += sprintf(sql + len, ",'%d'", records->plate_color);
	len += sprintf(sql + len, ",'%s'", records->description);
	len += sprintf(sql + len, ",'%s'", records->ftp_user);
	len += sprintf(sql + len, ",'%s'", records->ftp_passwd);
	len += sprintf(sql + len, ",'%s'", records->ftp_ip);
	len += sprintf(sql + len, ",'%d'", records->ftp_port);
	len += sprintf(sql + len, ",'%d'", records->flag_send);
	len += sprintf(sql + len, ",'%d'", records->flag_store);
	len += sprintf(sql + len, ",'%d'", records->vehicle_type);
	len += sprintf(sql + len, ");");

	return len;
}









/*******************************************************************************
 * 函数名: db_unformat_read_sql_traffic_records
 * 功  能: 解析从机动车通行记录表中读出的数据
 * 参  数: azResult，数据库表中读出的数据缓存；buf，通行记录结构体指针
 * 返回值: 0正常，其他为异常
*******************************************************************************/
int db_unformat_read_sql_traffic_records(
    char *azResult[], DB_TrafficRecord *traffic_record)
{
	int ncolumn = 0;

	if (traffic_record == NULL)
	{
		printf("db_unformat_read_sql_traffic_records: traffic_record is NULL\n");
		return -1;
	}

	if (azResult == NULL)
	{
		printf("db_unformat_read_sql_traffic_records: azResult is NULL\n");
		return -1;
	}

	memset(traffic_record, 0, sizeof(DB_TrafficRecord));

	traffic_record->ID= atoi(azResult[ncolumn + 0]);

	sprintf(traffic_record->plate_num, "%s", azResult[ncolumn + 1]);

	traffic_record->plate_type = atoi(azResult[ncolumn + 2]);

	sprintf(traffic_record->point_id, "%s", azResult[ncolumn + 3]);
	sprintf(traffic_record->point_name, "%s", azResult[ncolumn + 4]);
	sprintf(traffic_record->dev_id, "%s", azResult[ncolumn + 5]);

	traffic_record->lane_num = atoi(azResult[ncolumn + 6]);
	traffic_record->speed = atoi(azResult[ncolumn + 7]);

	sprintf(traffic_record->time, "%s", azResult[ncolumn + 8]);
	sprintf(traffic_record->collection_agencies, "%s", azResult[ncolumn + 9]);

	traffic_record->direction = atoi(azResult[ncolumn + 10]);

	sprintf(traffic_record->image_path, "%s", azResult[ncolumn + 11]);
	sprintf(traffic_record->partition_path, "%s", azResult[ncolumn + 12]);

	traffic_record->color 			= atoi(azResult[ncolumn + 13]);
	traffic_record->vehicle_logo 	= atoi(azResult[ncolumn + 14]);
	traffic_record->objective_type 	= atoi(azResult[ncolumn + 15]);
	traffic_record->coordinate_x 	= atoi(azResult[ncolumn + 16]);
	traffic_record->coordinate_y 	= atoi(azResult[ncolumn + 17]);
	traffic_record->width 			= atoi(azResult[ncolumn + 18]);
	traffic_record->height 			= atoi(azResult[ncolumn + 19]);
	traffic_record->pic_flag 		= atoi(azResult[ncolumn + 20]);
	traffic_record->plate_color 	= atoi(azResult[ncolumn + 21]);

	sprintf(traffic_record->description, 	"%s", azResult[ncolumn + 22]);
	sprintf(traffic_record->ftp_user, 		"%s", azResult[ncolumn + 23]);
	sprintf(traffic_record->ftp_passwd, 	"%s", azResult[ncolumn + 24]);
	sprintf(traffic_record->ftp_ip, 		"%s", azResult[ncolumn + 25]);

	traffic_record->ftp_port 		= atoi(azResult[ncolumn + 26]);
	traffic_record->flag_send 		= atoi(azResult[ncolumn + 27]);
	traffic_record->flag_store 		= atoi(azResult[ncolumn + 28]);
	traffic_record->vehicle_type 	= atoi(azResult[ncolumn + 29]);

	return 0;
}


/*******************************************************************************
 * 函数名: db_read_traffic_record
 * 功  能: 读取数据库中的通行记录
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_read_traffic_record(const char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records, char * sql_cond)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
	char **azResult; //二维数组存放结果
	static char plateNum[16]; //判断同一车牌传送的次数，防止删除数据库失败引起重复传送
	int samePlateCnt = 0; //续传时判断同一车牌传送的次数
	int ID_read=0;

	DB_TrafficRecord * traffic_record = (DB_TrafficRecord *) records;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}

	rc = db_create_table(db, SQL_CREATE_TABLE_TRAFFIC_RECORDS);
	if (rc < 0)
	{
		printf("db_create_table failed\n");
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	//查询数据
	//memset(sql, 0, sizeof(sql));
	//sprintf(sql, "SELECT * FROM traffic_records limit 1");
	nrow = 0;
	rc = sqlite3_get_table(db, sql_cond, &azResult, &nrow, &ncolumn, &pzErrMsg);
	if (rc != SQLITE_OK || nrow == 0)
	{
		//		printf("Can't require data, Error Message: %s\n", pzErrMsg);
		//		printf("row:%d column=%d \n", nrow, ncolumn);
		sqlite3_free(pzErrMsg);
		sqlite3_free_table(azResult);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}

	printf(" azResult[%d]=%d,\n", ncolumn, azResult[ncolumn][0]);

	sprintf(traffic_record->plate_num, "%s", azResult[ncolumn + 1]);
	printf("traffic_record->plate_num is %s\n", traffic_record->plate_num);
	if (strcmp(traffic_record->plate_num, plateNum) == 0)//判断车牌号是否重复
	{
		samePlateCnt++;
		if (samePlateCnt == 5)
		{
			printf("Can't del data %s\n", pzErrMsg);
			sqlite3_free(pzErrMsg);
			sqlite3_free_table(azResult);
			sqlite3_close(db);
			pthread_mutex_unlock(mutex_db_records);
			return -1;
		}
	}
	else
	{
		sprintf(plateNum, "%s", traffic_record->plate_num);
		samePlateCnt = 0;
	}

	ID_read= atoi(azResult[ncolumn]);
	db_unformat_read_sql_traffic_records(&(azResult[ncolumn]), traffic_record);

	printf("db_read_traffic_record  finish\n");

	sqlite3_free(pzErrMsg);

	sqlite3_free_table(azResult);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	return ID_read;
}


/*******************************************************************************
 * 函数名: db_add_column_traffic_records
 * 功  能: 在数据库中增加一个新字段－－示例
 * 参  数:
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int db_add_column_traffic_records(char *db_name, void *records,
                                  pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	//int nrow = 0, ncolumn = 0; //查询结果集的行数、列数
	//DB_TrafficRecord * traffic_record = (DB_TrafficRecord *) records;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		printf("Create database %s failed!\n", db_name);
		printf("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		printf("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}


	sqlite3_stmt *stat;//=new(sqlite3_stmt);
	int nret;
	//int count=0;

	//查询符合添加的记录总条数
	memset(sql, 0, sizeof(sql));
	//sprintf(sql, "UPDATE traffic_records SET flag_send=1 WHERE flag_send=0");// 更新某字段值
	sprintf(sql, "Alter TABLE traffic_records ADD COLUMN author_id INTEGER");// 增加字段
	//sprintf(sql, "UPDATE traffic_records SET author_id=26");//设置新字段的值

	nret=sqlite3_prepare_v2(db, sql, -1, &stat, 0);
	if (nret!=SQLITE_OK)
	{
		//sqlite3_free(pzErrMsg);
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return 0;
	}
	if (sqlite3_step(stat)==SQLITE_ROW)
	{
		//count=sqlite3_column_int(stat, 0);
	}
	sqlite3_finalize(stat); //perpare will lock the db, this unlock the db


	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	printf("db_add_column_traffic_records  finish.\n");

	return 0;
}


//删除指定ID 的一条记录
//删除指定ID 的记录－－对于非历史记录
//或清理续传标志--对于历史记录
int db_delete_traffic_records(char *db_name, void *records,
                              pthread_mutex_t *mutex_db_records)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];

	DB_TrafficRecord * traffic_record = (DB_TrafficRecord *) records;

	pthread_mutex_lock(mutex_db_records);
	rc = sqlite3_open(db_name, &db);
	if (rc != SQLITE_OK)
	{
		ERROR("Create database %s failed!\n", db_name);
		ERROR("Result Code: %d, Error Message: %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		pthread_mutex_unlock(mutex_db_records);
		return -1;
	}
	else
	{
		DEBUG("Open database %s succeed, Result Code: %d\n", db_name, rc);
	}
	/*
		rc = db_create_table(db, SQL_CREATE_TABLE_TRAFFIC_RECORDS);
		if (rc < 0)
		{
			printf("db_create_table failed\n");
			sqlite3_close(db);
			pthread_mutex_unlock(mutex_db_records);
			return -1;
		}
	*/
	//查询数据
	memset(sql, 0, sizeof(sql));

	if (traffic_record->flag_store==1)
	{
		//清理续传标志
		sprintf(sql, "UPDATE traffic_records SET flag_send=0 WHERE ID = %d ;", traffic_record->ID);
	}
	else
	{
		//删除指定ID 的记录	//只有在一个数据库全部续传完成时，才能删除这条对应的记录
		sprintf(sql, "DELETE FROM traffic_records WHERE ID = %d ;", traffic_record->ID);
	}

	DEBUG("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		ERROR("Delete data failed, flag_store=%d,  Error Message: %s\n", traffic_record->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	//  表为空?
	//	remove(db_name);

	sqlite3_close(db);
	pthread_mutex_unlock(mutex_db_records);

	if (rc != SQLITE_OK)
	{
		return -1;
	}

	INFO("db_delete_traffic_records  finish\n");

	return 0;
}

/*******************************************************************************
 * 函数名: send_traffic_records_info_history
 * 功  能: 发送通行历史记录信息
 * 参  数: db_traffic_records，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int send_traffic_records_info_history(void *db_traffic_records, int dest_mq, int num_record)
{
	char mq_text[MQ_TEXT_BUF_SIZE];

	memset(mq_text, 0, MQ_TEXT_BUF_SIZE);
	format_mq_text_traffic_record(mq_text,
	                              (DB_TrafficRecord *) db_traffic_records);

	debug_print ("text: %s\n", mq_text);

	mq_send_traffic_record_history(mq_text, dest_mq, num_record);

	return 0;
}

/*******************************************************************************
 * 函数名: send_traffic_records_image_file
 * 功  能: 直接发送过车记录图片文件
 * 参  数: pic_info，图片文件信息；
 * 返回值: 成功，返回0；上传失败，返回-1；文件不存在，返回-2
*******************************************************************************/
int send_traffic_records_image_file(
    DB_TrafficRecord* db_traffic_record, EP_PicInfo *pic_info)
{
	char file_name_pic[NAME_MAX_LEN];
	sprintf(file_name_pic, "%s/%s/%s",
	        db_traffic_record->partition_path,
	        DISK_RECORD_PATH,
	        db_traffic_record->image_path);

	if (access(file_name_pic, F_OK) < 0)
	{
		log_debug_storage("%s does not exist !\n", file_name_pic);
		return -2;
	}

	if (ftp_send_traffic_record_pic_file(file_name_pic, pic_info) < 0)
		return -1;

	if (db_traffic_record->flag_store == 0) /* 如果不是实时存储，转移文件 */
	{
		move_record_to_trash(db_traffic_record->partition_path,
		                     db_traffic_record->image_path);
	}

	return 0;
}

#if 0
/*******************************************************************************
 * 函数名: save_traffic_records_info
 * 功  能: 保存通行记录信息
 * 参  数: db_traffic_records，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_traffic_records_info(
    const DB_TrafficRecord *db_traffic_records,
    const void *image_info, const char *partition_path)
{

	char db_traffic_name[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/traffic_records_2013082109.db
	static DB_File db_file;
	//static char db_traffic_name_last[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/traffic_records_2013082109.db
	int flag_record_in_DB_file=0;//是否需要记录到索引数据库
	passRecordVehicle *traffic_records =
	    (passRecordVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/traffic_records");
	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	sprintf(db_traffic_name, "%s/%04d%02d%02d%02d.db", dir_temp,
	        traffic_records->picInfo.year, traffic_records->picInfo.month,
	        traffic_records->picInfo.day, traffic_records->picInfo.hour);

	//判断数据库文件是否已经存在
	//若不存在，需要相应建立一条索引记录
	//续传后，可能没有删记录数据库文件，却删了对应的索引记录。
	if (access(db_traffic_name, F_OK) != 0)
	{
		//数据库文件不存在，需要在索引数据库中增加相应的一条记录
		flag_record_in_DB_file=1;
	}
	else
	{
		flag_record_in_DB_file=0;
	}




	//检索索引数据库中，是否存在该记录数据库名
	//不能只依赖于是否有续传数据，还有实时存储。
	//if ((flag_upload_complete_traffic==1)||(strcasecmp(db_traffic_name_last, db_traffic_name)))//是新数据库名称

	//若开启实时存储，只依赖于数据库文件是否存在:
	//----若不存在，添加索引记录；若存在，不添加。
	//若不开启实时存储，
	//----若不存在数据库文件，添加索引记录；
	//若存在数据库文件，另外判断是否存在续传数据:
	//       ----有续传数据，不添加；没有续传数据，添加。
	//if((flag_record_in_DB_file==1)||((flag_upload_complete_traffic==1)&&(EP_DISK_SAVE_CFG.vehicle==0)))

	if (flag_record_in_DB_file==1)//保证，索引记录的删除，同时删对应交通数据库
	{
		//printf("db_traffic_name_last is %s, db_traffic_name is %s\n",
		//		db_traffic_name_last, db_traffic_name);
		printf("add to DB_files, db_traffic_name is %s\n", db_traffic_name);

		//写一条记录到数据库名管理


		db_file.record_type = 0;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_traffic_name, strlen(db_traffic_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_traffic_records->time, strlen(
		           db_traffic_records->time));
		db_file.flag_send = db_traffic_records->flag_send;
		db_file.flag_store= db_traffic_records->flag_store;

		ret = db_write_DB_file((char*)DB_NAME_VM_RECORD, &db_file,
		                       &mutex_db_files_traffic);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf("db_write_DB_file %s in %s ok.\n", db_traffic_name,
			       (char*)DB_NAME_VM_RECORD);
		}
	}
	else
	{

		// 第一次写索引数据库时，可能是两个条件中任意一个触发，（断网、实时存储）
		// 所以，在另一个条件发生时，需要相应修改索引数据库。
		char sql_cond[1024];
		static int flag_first=1;
		if (flag_first==1)
		{
			//第一次进入，要读取最新的一条索引记录。
			flag_first=0;

			//char sql_cond[1024];
			char history_time_start[64];			//时间起点 示例：2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_traffic_records->time, history_time_start);
			sprintf(sql_cond, "SELECT * FROM DB_files WHERE time>='%s'  limit 1;", history_time_start );
			db_read_DB_file(DB_NAME_VM_RECORD, &db_file, &mutex_db_files_traffic, sql_cond);
		}

		printf("db_file.flag_send=%d,db_traffic_records->flag_send=%d\n",db_file.flag_send,db_traffic_records->flag_send);
		if (( ~(db_file.flag_send) & db_traffic_records->flag_send)!=0)	//若新记录中包含了新的续传标志信息
		{
			db_file.flag_send |=db_traffic_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;", db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}

		if ( (db_file.flag_store==0) && (db_traffic_records->flag_store!=0))//需要增加存储信息
		{
			db_file.flag_store=db_traffic_records->flag_store;
			sprintf(sql_cond, "UPDATE DB_files SET flag_store=%d WHERE ID=%d;", db_file.flag_store, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}



	}
	//memset(db_traffic_name_last, 0, sizeof(db_traffic_name_last));
	//memcpy(db_traffic_name_last, db_traffic_name, sizeof(db_traffic_name));

	ret		= db_write_traffic_records(db_traffic_name,
	                                   (DB_TrafficRecord *) db_traffic_records,
	                                   &mutex_db_records_traffic);
	if (ret != 0)
	{
		printf("db_write_traffic_records failed\n");
		return -1;
	}


	return ret;
}
#endif

/*******************************************************************************
 * 函数名: save_traffic_records_info_others
 * 功  能: 保存非机动车的通行记录信息
 * 参  数: db_traffic_records，事件报警信息
 * 返回值: 成功，返回0；失败，返回-1
*******************************************************************************/
int save_traffic_records_info_others(
    const DB_TrafficRecord *db_traffic_records,
    const void *image_info, const char *partition_path)
{
	char db_traffic_name[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/traffic_records_2013082109.db
	static char db_traffic_name_last[PATH_MAX_LEN];//  例如: /mnt/sda1/DB/traffic_records_2013082109.db
	passRecordNoVehicle *traffic_records =
	    (passRecordNoVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);//仅次一句不同(对比save_traffic_records_info)

	char dir_temp[PATH_MAX_LEN];
	sprintf(dir_temp, "%s/%s", partition_path, DISK_DB_PATH);
	int ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	strcat(dir_temp, "/traffic_records");
	ret = dir_create(dir_temp);
	if (ret != 0)
	{
		printf("dir_create %s failed\n", dir_temp);
		return -1;
	}

	sprintf(db_traffic_name, "%s/%04d%02d%02d%02d.db", dir_temp,
	        traffic_records->picInfo.year, traffic_records->picInfo.month,
	        traffic_records->picInfo.day, traffic_records->picInfo.hour);

	ret
	    = db_write_traffic_records(db_traffic_name,
	                               (DB_TrafficRecord *) db_traffic_records,
	                               &mutex_db_records_traffic);
	if (ret != 0)
	{
		printf("db_write_traffic_records failed\n");
		return -1;
	}

	//判断新生成文件名，是否与上一次的重复
	//以后添加－－也有可能与多次之前的重复，需要循环判断－－
	//重启后，不记得上一次的名称
	if (strcasecmp(db_traffic_name_last, db_traffic_name))//是新数据库名称
	{
		printf("db_traffic_name_last is %s, db_traffic_name is %s\n",
		       db_traffic_name_last, db_traffic_name);
		//写一条记录到数据库名管理
		DB_File db_file;

		db_file.record_type = 0;
		memset(db_file.record_path, 0, sizeof(db_file.record_path));
		memcpy(db_file.record_path, db_traffic_name, strlen(db_traffic_name));
		memset(db_file.time, 0, sizeof(db_file.time));
		memcpy(db_file.time, db_traffic_records->time, strlen(
		           db_traffic_records->time));
		db_file.flag_send = db_traffic_records->flag_send;
		db_file.flag_store= db_traffic_records->flag_store;

		ret = db_write_DB_file((char*)DB_NAME_VM_RECORD, &db_file,
		                       &mutex_db_files_traffic);
		if (ret != 0)
		{
			printf("db_write_DB_file failed\n");
		}
		else
		{
			printf("db_write_DB_file %s in %s ok.\n", db_traffic_name,
			       DB_NAME_VM_RECORD);
		}
	}
	memset(db_traffic_name_last, 0, sizeof(db_traffic_name_last));
	memcpy(db_traffic_name_last, db_traffic_name, sizeof(db_traffic_name));

	return ret;
}

/*******************************************************************************
 * 函数名: analyze_traffic_record_info_others
 * 功  能: 解析非机动车和行人通行记录信息
 * 参  数: db_traffic_records，解析的结果；
 *         image_info，图像信息；pic_info，图片文件信息
 * 返回值:
*******************************************************************************/
int analyze_traffic_record_info_others(
    DB_TrafficRecord *db_traffic_record, const NoVehiclePoint *result,
    const EP_PicInfo *pic_info, const char *partition_path)
{

	printf("In analyze_traffic_record_info_others\n");
	memset(db_traffic_record, 0, sizeof(DB_TrafficRecord));

	switch (result->targetType)
	{
	case 1:
		snprintf(db_traffic_record->plate_num,
		         sizeof(db_traffic_record->plate_num),
		         "非机动车");
		break;
	case 2:
		snprintf(db_traffic_record->plate_num,
		         sizeof(db_traffic_record->plate_num),
		         "行人");
		break;
	default:
		snprintf(db_traffic_record->plate_num,
		         sizeof(db_traffic_record->plate_num),
		         "非机动车-其他");
		break;
	}

	snprintf(db_traffic_record->point_id,
	         sizeof(db_traffic_record->point_id),
	         "%s", EP_POINT_ID);
	snprintf(db_traffic_record->point_name,
	         sizeof(db_traffic_record->point_name),
	         "%s", EP_POINT_NAME);
	snprintf(db_traffic_record->dev_id,
	         sizeof(db_traffic_record->dev_id),
	         "%s", EP_EXP_DEV_ID);
	db_traffic_record->lane_num = result->laneNum;

	snprintf(db_traffic_record->time,
	         sizeof(db_traffic_record->time),
	         "%04d-%02d-%02d %02d:%02d:%02d",
	         pic_info->tm.year, pic_info->tm.month, pic_info->tm.day,
	         pic_info->tm.hour, pic_info->tm.minute, pic_info->tm.second);

	snprintf(db_traffic_record->collection_agencies,
	         EP_COLLECTION_AGENCIES_SIZE, "%s", EP_EXP_DEV_ID);

	db_traffic_record->direction = EP_DIRECTION;

	//*************************//
//	snprintf(db_traffic_record->image_path,
//	         sizeof(db_traffic_record->image_path),
//	         "%s/%s",
//	         pic_info->path, pic_info->name);
	//*************************//

	get_ftp_path(EP_FTP_URL_LEVEL,
				            pic_info->tm.year,
				            pic_info->tm.month,
				            pic_info->tm.day,
				            pic_info->tm.hour,
				             (char*) TRAFFIC_RECORDS_FTP_DIR,
				             db_traffic_record->image_path);

	strcat(db_traffic_record->image_path, pic_info->name);
	printf("db_park_record->image_path: %s\n",db_traffic_record->image_path);
	//*************************//

	snprintf(db_traffic_record->partition_path,
	         sizeof(db_traffic_record->partition_path),
	         "%s", partition_path);

	db_traffic_record->color 			= result->color;

	/* 目标类型：2表示非机动车，3表示行人。算法输出的要加1 */
	db_traffic_record->objective_type 	= result->targetType + 1;

	db_traffic_record->coordinate_x 	= result->xPos;
	db_traffic_record->coordinate_y 	= result->yPos;
	db_traffic_record->width 			= result->width;
	db_traffic_record->height 			= result->height;
	db_traffic_record->pic_flag 		= 1; /* 目前只有1张图片 */

	snprintf(db_traffic_record->description,
	         sizeof(db_traffic_record->description),
	         "图像编号: %d", result->pic_id);

	snprintf(db_traffic_record->ftp_user,
	         sizeof(db_traffic_record->ftp_user),
	         "%s", EP_TRAFFIC_FTP.user);
	snprintf(db_traffic_record->ftp_passwd,
	         sizeof(db_traffic_record->ftp_passwd),
	         "%s", EP_TRAFFIC_FTP.passwd);
	snprintf(db_traffic_record->ftp_ip,
	         sizeof(db_traffic_record->ftp_ip),
	         "%d.%d.%d.%d",
	         EP_TRAFFIC_FTP.ip[0],
	         EP_TRAFFIC_FTP.ip[1],
	         EP_TRAFFIC_FTP.ip[2],
	         EP_TRAFFIC_FTP.ip[3]);
	db_traffic_record->ftp_port = EP_TRAFFIC_FTP.port;

	return 0;
}

/*******************************************************************************
 * 函数名: bitcom_sendto_dahua
 * 功  能: 发送数据到大华平台
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
void * bitcom_sendto_dahua(void * argv)
{

	//创建套接字并连接服务器
	int sockfd;
	fd_set rset;

	while(1)
	{
		sleep(5);

		struct sockaddr_in client_addr;
		if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
		{
			continue;
		}

		gstr_dahua_data.flag = 0;
		bzero(&client_addr, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort);
		client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp); //IP需要动态赋予

		if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
		 {
			  close(sockfd);

			  continue;
		 }

		while(1)
		{
			sleep(1);

			if(gstr_dahua_data.flag == 1)
			{
				//在发送的时候需要考虑套接字缓冲区中是否有数据，需要先清空缓冲区
				int ret = send(sockfd , gstr_dahua_data.buf, gstr_dahua_data.buf_lens, 0);
				if (ret == -1)
				{
					close(sockfd);
					break;
				}

				TRACE_LOG_PLATFROM_INTERFACE("DaHua send datas : %s", gstr_dahua_data.buf);

				FD_ZERO(&rset);
				FD_SET(sockfd, &rset);

				struct timeval tv;
				tv.tv_sec= 2;
				tv.tv_usec= 0;
				int h= select(sockfd +1, &rset, NULL, NULL, &tv); //过车消息发送完毕之后等待接收服务器的回复消息，如果2s内没有收到回复消息则跳过继续执行

				if (h <= 0)
				{
					TRACE_LOG_PLATFROM_INTERFACE("DaHua send datas ,select failed");
					close(sockfd);

					break;
				}

				if (h > 0)
				{
					char buf[1024];
					memset(buf, 0, 1024);
					int i= read(sockfd, buf, 1023);
					if (i <= 0)
					{
						TRACE_LOG_PLATFROM_INTERFACE("DaHua send datas ,select ,read failed");
						close(sockfd);

						break;
					}

					memset(gstr_dahua_data.buf, 0, sizeof(gstr_dahua_data.buf));
					gstr_dahua_data.buf_lens = 0;
					gstr_dahua_data.flag = 0;
					TRACE_LOG_PLATFROM_INTERFACE("DaHua send datas ,select ,read successful");
				}

			}
		}

	}
    return NULL;
}




int send_vm_records_motor_vehicle_to_Dahua(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{
	//获取时间秒数
	char lt[48];
	memset(lt,0,48);
	long  ltime = time(NULL);
	sprintf(lt,"----------DHMSG0000%x",(unsigned int)ltime); //固定为27个字符

	//bitcom转换大华协议
	TRACE_LOG_PLATFROM_INTERFACE("Bitcom info platecolure=%d，platetype=%d，direction=%d，vehicletype=%d，vehiclecolour=%d", \
								db_traffic_record->plate_color, db_traffic_record->plate_type, db_traffic_record->direction, db_traffic_record->vehicle_type, db_traffic_record->color);
	bitcom_to_dahua_protocol(db_traffic_record);
	TRACE_LOG_PLATFROM_INTERFACE("Dahua  info platecolure=%d，platetype=%d，direction=%d，vehicletype=%d，vehiclecolour=%d", \
								db_traffic_record->plate_color, db_traffic_record->plate_type, db_traffic_record->direction, db_traffic_record->vehicle_type, db_traffic_record->color);

	if(msgbuf_paltform.pf_vtype == DaHua_V)
	{
		//***********过车消息(信息)***********//

		char str_tmp[128];
		memset(str_tmp,0,128);

		sprintf(str_tmp,"%s\r\n"\
						"Content-Type: application/json;charset=UTF-8\r\n\r\n",lt);
		int len_tmp = strlen(str_tmp);

		char PlateColor[12] = "0";
		sprintf(PlateColor,"%d",db_traffic_record->plate_color);

		char PlateNum[64] = "0";
		convert_enc("GBK", "UTF-8", db_traffic_record->plate_num,32,PlateNum,64);


		Json::Value root;
		Json::FastWriter fast_writer;

		root["Longitude"] = 0.0;						            //抓拍点经度(不支持)
		root["Latitude"] = 0.0;							            //抓拍点纬度(不支持)
		root["VehicleInfoState"] = 0;                               //过车信息状态，0:实时过车 1:历史过车 int
		root["IsPicUrl"] = 0;                   			        //图片类型,0:在消息体中包含图片，1:通过图片地址寻找  int
		root["LaneIndex"] = db_traffic_record->lane_num;            //车道号 int
		root["PlateInfo"] = PlateNum;                               //车牌号  字符串
		root["PlateColor"] = PlateColor;                            //车牌颜色  字符串
		root["PlateType"] = db_traffic_record->plate_type;          //车牌类型  int
		root["PassTime"] = db_traffic_record->time;                 //过车时间 格式YYYY-MM-DD HH(24):MI:S  字符串
		root["VehicleSpeed"] = (double)db_traffic_record->speed;    //车辆速度 double
		root["LaneMiniSpeed"] = 0.0;                                //车道低限速，无此功能填0.0  double
		root["LaneMaxSpeed"] = 0.0;                                 //车道高限速，无此功能填0.0  double
		root["VehicleType"] = db_traffic_record->vehicle_type;      //车辆类型 int
		root["VehicleColor"] = db_traffic_record->color;            //车辆颜色 int
		root["VehicleLength"] = 0;                                  //车辆长度，不支持填0   int
		root["VehicleState"] = 1;								    //行车状态  int
		root["PicCount"] = 1;                                       //图片张数 int
		root["PicType"] = "";                                     //这个怎么赋值
		root["PlatePicUrl"] = "";                                 //车牌照片url
		root["VehiclePic1Url"] = "";                              //车辆照片url
		root["VehiclePic2Url"] = "";
		root["VehiclePic3Url"] = "";
		root["CombinedPicUrl"] = "";  			                //图片地址
		root["AlarmAction"] = 1;              			            //违法类型

		std::string  str_vehicalinfo = fast_writer.write(root);

		std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;


		//***********过车消息体(图片)***********//
		char str_vehicalpic_head[72];
		memset(str_vehicalpic_head,0,72);
		sprintf(str_vehicalpic_head,"%s\r\n"\
							   "Content-Type: image/jpeg\r\n",lt);

		int len_str_vehicalpic_head = strlen(str_vehicalpic_head);

		char str_vehicalpic_tail[48];
		memset(str_vehicalpic_tail,0,48);
		sprintf(str_vehicalpic_tail,"%s--\r\n",lt);

		int len_str_vehicalpic_tail = strlen(str_vehicalpic_tail);

		//***********过车消息头***********//
		char str_vehical[1024*1024];
		memset(str_vehical,0,1024*1024);
		sprintf(str_vehical,"POST /DH/Devices/%s$0/Events HTTP/1.1\r\n"\
							"Connection : Keep-alive\r\n"\
							"Content-Type: multipart/form-data; boundary=%s\r\n"\
							"Host: %s:%d\r\n"\
							"Content-Length: %d\r\n\r\n"\
							"%s\r\n"\
						    "Content-Type: application/json;charset=UTF-8\r\n\r\n",
							msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.DeviceId,lt,
							msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp,msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.MsgPort,
							len_tmp + str_vehicalinfo.length() + len_str_vehicalpic_head + pic_info->size + len_str_vehicalpic_tail,lt);

		printf("vehical head:\n");
		printf("%s",str_vehical);

		//***********拼接***********//
		strcat(str_vehical,str_vehicalinfo.c_str()); //消息头 + 过车信息

		strcat(str_vehical,str_vehicalpic_head);     //消息头 + 过车信息 + 过车图片头信息

		int len = strlen(str_vehical);               //消息头 + 过车信息 + 过车图片头信息的字符串长度

		memcpy(str_vehical + len,pic_info->buf,pic_info->size);  //消息头 + 过车信息 + 过车图片头信息 + 图片

		memcpy(str_vehical + len + pic_info->size,str_vehicalpic_tail,len_str_vehicalpic_tail); //消息头 + 过车信息 + 过车图片头信息 + 图片 + 消息尾  总大小


		//bitcom发送大华数据赋值
		if(gstr_dahua_data.flag == 0)
		{
			gstr_dahua_data.buf_lens = len + pic_info->size + len_str_vehicalpic_tail;
			memcpy(gstr_dahua_data.buf, str_vehical, gstr_dahua_data.buf_lens);

			gstr_dahua_data.flag = 1;
		}

	}
    return 0;
}

/*******************************************************************************
 * 函数名: send_vm_records_motor_vehicle_to_NetPose
 * 功  能: 发送过车信息给东方网力平台
 * 参  数: db_traffic_records，解析的结果
 *         pic_info，图片文件信息
 * 返回值:
*******************************************************************************/
int send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{



	fd_set rset;
	NetPoseVehicleProtocol VehicleProtoclo;
	memset(&VehicleProtoclo,0,sizeof(NetPoseVehicleProtocol));

	//获取时间
	char timenow[50];
	memset(timenow,0,50);
	getime(timenow);

	long time_tmp = time(NULL);

	INFO("In send_vm_records_motor_vehicle_to_NetPose : IP: %s Port: %d\n",
		 msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
		 msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);

	if (msgbuf_paltform.pf_vtype != NETPOSA_V) {
		return -1;
	}

	//----------------协议转换----------------//

	protocol_convert_NetPose_moter_vehicle(db_traffic_record,&VehicleProtoclo);


	//----------------消息体(过车信息)----------------//
	/********************
	包含的过车消息有:
	analyseResult
		<vehicaleInfo>:
			vehicleType   :      车辆类型
			vehicleColor  :		 车辆颜色
			vehicleLength :		 车辆轮廓长度
			vehicleNoType :		 车牌类型
			vehicleNo     :		 车牌号码
			vehicleNoColor:      车牌颜色
		<driveInfo>:
			time          :      过车时间
			location	  :      经过地点
			speed         :      行驶速度
	analyseResult

	*********************/

	char str_vehicleinfo[1024];
	memset(str_vehicleinfo, 0, 1024);
	sprintf(str_vehicleinfo,
			"-------------------------------%ld\r\n"
			"Content-Disposition: form-data; name=\"analyseResult\"\r\n\r\n"
			"<analyseResult>"
				"<vehicleInfo "
					"vehicleType=\"%d\" "
					"vehicleColor=\"%s\" "
					"vehicleLogo=\"%d\" "
					"vehicleLength=\"100\" "
					"vehicleNoType=\"%d\" "
					"vehicleNo=\"%s\" "
					"vehicleNoColor=\"%s\">"
					"<driveInfo time=\"%s\" "
						"location=\"%s\" "
						"speed=\"%d\" "
						"status=\"1\""
					"/>"
				"</vehicleInfo>"
			"</analyseResult>\r\n",
			time_tmp,VehicleProtoclo.vehicleType,
			VehicleProtoclo.vehicleColor,
			VehicleProtoclo.vehicleLogo,
			VehicleProtoclo.vehicleNoType,
			VehicleProtoclo.vehicleNo,
			VehicleProtoclo.vehicleNoColor,
			db_traffic_record->time,
			db_traffic_record->point_id,
			db_traffic_record->speed);

	int len_str_vehicleinfo = strlen(str_vehicleinfo);

	printf("str_vehicalinfo:\n");
	printf("%s\n",str_vehicleinfo);

	//----------------消息体(过车图片的信息)----------------//

	char str_vehiclepic_head[312];
	memset(str_vehiclepic_head, 0, 312);

	sprintf(str_vehiclepic_head,
			"-------------------------------%ld\r\n"
			"Content-Disposition: form-data; name=\"vehicleImage\"; filename=\" vehicleImage.jpg\"\r\n"
			"Content-Type: image/jpeg\r\n\r\n",
			time_tmp);

	int len_str_vehiclepic_head = strlen(str_vehiclepic_head);

	char str_vehiclepic_tail[64];
	memset(str_vehiclepic_tail,0,64);
	sprintf(str_vehiclepic_tail,
			"\r\n"
			"-------------------------------%ld\r\n",
			time_tmp);

	int len_str_vehicalpic_tail = strlen(str_vehiclepic_head);

	//-------------------请求行以及消息头-------------------//

	char str_vehicle[1024*1024];
	memset(str_vehicle,0,sizeof(str_vehicle));
	sprintf(str_vehicle,
			"POST /toll-gate/home/upload?deviceId=%s&type=EventSnapshot&time=%s HTTP/1.1\r\n"
			"Content-Type: multipart/form-data; boundary=-----------------------------%ld\r\n"
			"Content-Length: %d\r\n"
			"Cache-Control: no-cache\r\n"
			"Pragma: no-cacher\r\n"
			"Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"
			"Connection: keep-alive\r\n"
			"\r\n",
			msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId, timenow,
			time_tmp,
			len_str_vehicleinfo + len_str_vehiclepic_head + pic_info->size + len_str_vehicalpic_tail);

	DEBUG("str_head:");
	DEBUG("%s",str_vehicle);


	//-------------------拼接-------------------//
	strcat(str_vehicle,str_vehicleinfo);
	strcat(str_vehicle,str_vehiclepic_head);

	int len = strlen(str_vehicle);

	memcpy(str_vehicle + len,pic_info->buf,pic_info->size);

	memcpy(str_vehicle +len + pic_info->size,str_vehiclepic_tail,len_str_vehicalpic_tail);

	DEBUG("http to netpose %s\n", str_vehicle);

	//----------------发送数据----------------//

	//创建套接字并连接服务器
	int sockfd;

	sockfd = tcp_connect_timeout(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
								 msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort,
								 3000);
	if (sockfd < 0) {
		return -1;
	}

	DEBUG("--------Connect NetPose_Platform success!--------");


	int ret = write(sockfd,str_vehicle,len + pic_info->size + len_str_vehicalpic_tail);

	DEBUG("write ret:%d",ret);

	if (ret <= 0)
	{
		close(sockfd);
		ERROR("Post error");
		return -1;
	}


	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	struct timeval tv;
	tv.tv_sec= 1;
	tv.tv_usec= 0;
	int h= select(sockfd +1, &rset, NULL, NULL, &tv);

	if (h <= 0)
	{
		close(sockfd);
		ERROR("Select error");
		return -1;
	}

	if (h > 0)
	{
		char buf[1024];
		memset(buf, 0, 1024);
		int i= read(sockfd, buf, 1023);
		if (i==0)
		{
			close(sockfd);
			ERROR("target closed\n");
			return -1;
		}
		close(sockfd);
		DEBUG("rcvd frome netpose:");
		DEBUG("%s", buf);
	}

	return 0;
}


int send_vm_records_motor_vehicle_to_NetPose_otherstmp(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{

	printf("In send_vm_records_motor_vehicle_to_NetPose : IP: %s Port: %d\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort);

	fd_set rset;
	//NetPoseVehicleProtocol VehicleProtoclo;
	//memset(&VehicleProtoclo,0,sizeof(NetPoseVehicleProtocol));

	//获取时间
	char timenow[50];
	memset(timenow,0,50);
	getime(timenow);

	long time_tmp = time(NULL);

	//----------------协议转换----------------//

	//protocol_convert_NetPose_moter_vehicle(db_traffic_record,&VehicleProtoclo);


	//----------------消息体(过车信息)----------------//
	/********************
	包含的过车消息有:
	analyseResult
		<vehicaleInfo>:
			vehicleType   :      车辆类型
			vehicleColor  :		 车辆颜色
			vehicleLength :		 车辆轮廓长度
			vehicleNoType :		 车牌类型
			vehicleNo     :		 车牌号码
			vehicleNoColor:      车牌颜色
		<driveInfo>:
			time          :      过车时间
			location	  :      经过地点
			speed         :      行驶速度
	analyseResult

	*********************/

	char str_vehicleinfo[1024];
	memset(str_vehicleinfo, 0, 1024);
	sprintf(str_vehicleinfo,"-------------------------------%ld\r\n"\
							"Content-Disposition: form-data; name=\"analyseResult\"\r\n\r\n"\
							"<analyseResult>"\
								"<vehicleInfo vehicleType=\"\" vehicleColor=\"\" vehicleLength=\"100\" vehicleNoType=\"\" vehicleNo=\"\" vehicleNoColor=\"\">"\
									"<driveInfo time=\"%s\" location=\"%s\" speed=\"\" status=\"1\"/>"\
								"</vehicleInfo>"\
							"</analyseResult>\r\n",time_tmp,db_traffic_record->time,db_traffic_record->point_id);

	int len_str_vehicleinfo = strlen(str_vehicleinfo);

	printf("str_vehicalinfo:\n");
	printf("%s\n",str_vehicleinfo);

	//----------------消息体(过车图片的信息)----------------//

	char str_vehiclepic_head[312];
	memset(str_vehiclepic_head, 0, 312);

	sprintf(str_vehiclepic_head,  "-------------------------------%ld\r\n"\
	   						 "Content-Disposition: form-data; name=\"vehicleImage\"; filename=\" vehicleImage.jpg\"\r\n"\
	   						 "Content-Type: image/jpeg\r\n\r\n",time_tmp);

	int len_str_vehiclepic_head = strlen(str_vehiclepic_head);

	char str_vehiclepic_tail[64];
	memset(str_vehiclepic_tail,0,64);
	sprintf(str_vehiclepic_tail,"\r\n"\
					"-------------------------------%ld\r\n",time_tmp);

	int len_str_vehicalpic_tail = strlen(str_vehiclepic_head);

	//-------------------请求行以及消息头-------------------//

	char str_vehicle[1024*1024];
	memset(str_vehicle,0,sizeof(str_vehicle));
	sprintf(str_vehicle,"POST /toll-gate/home/upload?deviceId=%s&type=EventSnapshot&time=%s HTTP/1.1\r\n"\
    					"Content-Type: multipart/form-data; boundary=-----------------------------%ld\r\n"\
    					"Content-Length: %d\r\n"\
    					"Cache-Control: no-cache\r\n"\
    					"Pragma: no-cacher\r\n"\
    					"Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"\
    					"Connection: keep-alive\r\n"\
    					"\r\n",msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.DeviceId,timenow,time_tmp,len_str_vehicleinfo +
    					len_str_vehiclepic_head + pic_info->size + len_str_vehicalpic_tail);

	printf("str_head:\n");
	printf("%s\n",str_vehicle);


	//-------------------拼接-------------------//
	strcat(str_vehicle,str_vehicleinfo);
	strcat(str_vehicle,str_vehiclepic_head);

	int len = strlen(str_vehicle);

	memcpy(str_vehicle + len,pic_info->buf,pic_info->size);

	memcpy(str_vehicle +len + pic_info->size,str_vehiclepic_tail,len_str_vehicalpic_tail);




	//----------------发送数据----------------//

	//创建套接字并连接服务器
	int sockfd;

	sockfd = tcp_connect_timeout(msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.ServerIp,
								 msgbuf_paltform.Msg_buf.NetPosa_Vehicle_Msg.MsgPort,
								 3000);
	if (-1 == sockfd) {
		return -1;
	}

	int ret = write(sockfd,str_vehicle,len + pic_info->size + len_str_vehicalpic_tail);

	printf("ret:%d\n",ret);

	if (ret <= 0)
	{
		perror("Post error");
		close(sockfd);
		return -1;
	}


	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	struct timeval tv;
	tv.tv_sec= 1;
	tv.tv_usec= 0;
	int h= select(sockfd +1, &rset, NULL, NULL, &tv);

	if (h <= 0)
	{
		close(sockfd);
		perror("Select error");
		return -1;
	}

	if (h > 0)
	{
		char buf[1024];
		memset(buf, 0, 1024);
		int i= read(sockfd, buf, 1023);
		if (i==0)
		{
			close(sockfd);
			printf("target closed\n");
			return -1;
		}

		close(sockfd);
		printf("The recieve is ********\n");
		printf("%s\n", buf);
		printf("************************\n");
	}
	//---------------------------------
	return 0;
}

/*******************************************************************************
 * 函数名: send_vm_records_others_to_NetPose
 * 功  能: 发送过车信息给东方网力平台
 * 参  数: db_traffic_records，解析的结果
 *         pic_info，图片文件信息
 * 返回值:
*******************************************************************************/
int send_vm_records_motor_vehicle_to_NetPose_others(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{

	printf("In send_vm_records_others_to_NetPose : IP: %s Port: %d\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp,msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort);

	fd_set rset;
	NetPoseOthersProtocol OthersProtoclo;
	memset(&OthersProtoclo,0,sizeof(NetPoseOthersProtocol));

	//获取时间
	char timenow[50];
	memset(timenow,0,50);
	getime(timenow);

	long time_tmp = time(NULL);

	//----------------协议转换----------------//

	protocol_convert_NetPose_others(db_traffic_record,&OthersProtoclo);

	//----------------消息体(过车信息)----------------//
	/********************
	包含的过车消息有:
	analyseResult

	analyseResult

	*********************/

	char str_faceinfo[1024];
	memset(str_faceinfo, 0, 1024);
	sprintf(str_faceinfo,"-------------------------------%ld\r\n"\
							"Content-Disposition: form-data; name=\"analyseResult\"\r\n\r\n"\
							"<analyseFaceResult>"\
								"<faceInfo faceLeft=\"1\" faceTop=\"1\" faceRight=\"1\" faceBottom=\"1\" ext1 =\"\" ext2 =\"\" ext3 =\"\" ext4 =\"\" ext5 =\"\">"\
									"<throughInfo time=\"%s\" location=\"%s\" speed=\"0\" status=\"1\"/>"\
								"</faceInfo>"\
							"</analyseFaceResult>\r\n",time_tmp,db_traffic_record->time,db_traffic_record->point_id);

	int len_str_faceinfo = strlen(str_faceinfo);

	printf("str_faceinfo:\n");
	printf("%s\n",str_faceinfo);

	//----------------消息体(过车图片的信息)----------------//

	char str_facepic_head[312];
	memset(str_facepic_head, 0, 312);

	sprintf(str_facepic_head,  "-------------------------------%ld\r\n"\
	   						 "Content-Disposition: form-data; name=\"faceImage\"; filename=\" faceImage.jpg\"\r\n"\
	   						 "Content-Type: image/jpeg\r\n\r\n",time_tmp);

	int len_str_facepic_head = strlen(str_facepic_head);

	char str_facepic_tail[64];
	memset(str_facepic_tail,0,64);
	sprintf(str_facepic_tail,"\r\n"\
					"-------------------------------%ld\r\n",time_tmp);

	int len_str_facepic_tail = strlen(str_facepic_tail);

	//-------------------请求行以及消息头-------------------//

	char str_face[1024*1024];
	memset(str_face,0,sizeof(str_face));
	sprintf(str_face,"POST /toll-face/home/upload?deviceId=%s&type=EventSnapshot&time=%s HTTP/1.1\r\n"\
    					"Content-Type: multipart/form-data; boundary=-----------------------------%ld\r\n"\
    					"Content-Length: %d\r\n"\
    					"Cache-Control: no-cache\r\n"\
    					"Pragma: no-cacher\r\n"\
    					"Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"\
    					"Connection: keep-alive\r\n"\
    					"\r\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.DeviceId,timenow,time_tmp,len_str_faceinfo +
    					len_str_facepic_head + pic_info->size + len_str_facepic_tail);

	printf("str_head:\n");
	printf("%s\n",str_face);


	//-------------------拼接-------------------//
	strcat(str_face,str_faceinfo);
	strcat(str_face,str_facepic_head);

	int len = strlen(str_face);

	memcpy(str_face + len,pic_info->buf,pic_info->size);

	memcpy(str_face +len + pic_info->size,str_facepic_tail,len_str_facepic_tail);

	TRACE_LOG_SYSTEM("Netpose people records : %s", str_face);


	//----------------发送数据----------------//

	printf("Start send face info !\n");

	//创建套接字并连接服务器
	int sockfd;

	sockfd = tcp_connect_timeout(msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp,
								 msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort,
								 3000);
	if (-1 == sockfd) {
		return -1;
	}

	TRACE_LOG_SYSTEM("--------Connect NetPose_Platform success!--------\n");


	int ret = write(sockfd,str_face,len + pic_info->size + len_str_facepic_tail);

	printf("ret:%d\n",ret);

	if (ret <= 0)
	{
		close(sockfd);
		perror("Post error");
		return -1;
	}


	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	struct timeval tv;
	tv.tv_sec= 1;
	tv.tv_usec= 0;
	int h= select(sockfd +1, &rset, NULL, NULL, &tv);

	if (h <= 0)
	{
		close(sockfd);
		perror("Select error");
		return -1;
	}

	if (h > 0)
	{
		char buf[1024];
		memset(buf, 0, 1024);
		int i= read(sockfd, buf, 1023);
		if (i==0)
		{
			close(sockfd);
			printf("target closed\n");
			return -1;
		}
		close(sockfd);
		printf("The recieve is ********\n");
		printf("%s\n", buf);
		printf("************************\n");
	}
	//---------------------------------
	return 0;
}



int getime(char *nowtime)
{
    struct tm *pt;
    struct timeval  Tus;
    int cur_sec, cur_min, cur_hour, cur_day, cur_mouth, cur_year, cur_weekday;

    long ust;


	time_t t = time(NULL);
    pt = localtime(&t);
    memset(nowtime, 0, sizeof(*nowtime));

    gettimeofday(&Tus, NULL);
    cur_sec = pt->tm_sec;
    cur_min = pt->tm_min;
    cur_hour = pt->tm_hour;
    cur_day = pt->tm_mday;
    cur_mouth = pt->tm_mon + 1;
    cur_year = pt->tm_year + 1900;
    cur_weekday = pt->tm_wday;
    ust = (long)Tus.tv_usec/1000;

    printf("时间: %d:%d:%d.%ld \n", cur_hour, cur_min, cur_sec,ust);
    printf("日期: %d-%d-%d \n", cur_year, cur_mouth, cur_day);
    sprintf(nowtime,"%d%d%d%d%d%d%ld",cur_year, cur_mouth, cur_day,cur_hour, cur_min, cur_sec,ust);
    printf("Time is:%s\n",nowtime);

    return 0;
}

int32_t netpose_vehicle_logo_convert(int32_t idx)
{
	typedef struct logo{
		int32_t idx;
		const char *name;
	}logo_t;

	const char *bitcom[] = {
		"讴歌", "奥迪", "奔驰", "宝马", "别克", "比亚迪",
		"奇瑞", "雪佛兰", "雪铁龙", "帝豪", "一汽", "福特",
		"长城", "哈飞", "本田", "现代", "起亚", "雷克萨斯",
		"马自达", "尼桑", "标致", "雷诺", "斯柯达", "东南汽车",
		"铃木", "丰田", "夏利", "中华", "大众", "吉利",
		"三菱", "斯巴鲁", "荣威", "沃尔沃", "凌志", "菲亚特",
		"皇冠", "东风", "五十铃", "长安", "金杯", "凯迪拉克",
		"五菱", "江淮汽车", "欧宝" };
	const logo_t netpose[] = {
		{ -1, "无车标" },{ 0, "五菱" }, { 1, "丰田" },
		{ 2, "本田" }, { 3, "现代" }, { 4, "一汽" },
		{ 5, "大众" }, { 6, "别克" }, { 7, "标致" },
		{ 8, "福特" }, { 9, "尼桑" }, { 10, "雪铁龙" },
		{ 11, "奥迪" }, { 12, "雪佛兰" }, { 13, "起亚" },
		{ 14, "红旗" }, { 15, "长安" }, { 16, "长城" },
		{ 17, "东南汽车" }, { 18, "金杯" }, { 19, "马自达" },
		{ 20, "三菱" }, { 21, "夏利" }, { 22, "东风风神" },
		{ 23, "比亚迪" }, { 24, "恒通" }, { 25, "奇瑞" },
		{ 26, "华普" }, { 27, "铃木" }, { 28, "宇通" },
		{ 29, "金龙" }, { 30, "哈飞" }, { 31, "吉利" },
		{ 32, "奔驰" }, { 33, "长城(旧)" }, { 34, "比亚迪(旧)" },
		{ 35, "铃木(字母)" }, { 36, "宝马" }, { 37, "中华" },
		{ 38, "斯柯达" }, { 39, "威麟" }, { 40, "猎豹" },
		{ 41, "众泰" }, { 42, "力帆" }, { 43, "福田" },
		{ 44, "英菲尼迪" }, { 45, "雷克萨斯" }, { 46, "斯巴鲁" },
		{ 47, "雷诺" }, { 48, "沃尔沃" }, { 49, "江淮" },
		{ 50, "海格" }, { 51, "金旅" }
	};

	const char *str;
	const logo_t *p, *end;

	if (((unsigned int)idx >= numberof(bitcom)) || (idx < 0)) {
		str = "无车标";
	} else {
		str = bitcom[idx];
	}

	DEBUG("The vehicle logo idx is %d", idx);
	DEBUG("The vehicle logo name is %s", str);

	p = netpose;
	end = netpose + numberof(netpose);

	for (; p < end; ++p) {
		if (0 == strncmp(p->name, str, strlen(str))) {
			DEBUG("The netpose vehicle logo idx is %d", p->idx);
			DEBUG("The netpose vehicle logo name is %s", p->name);
			return p->idx;
		}
	}

	ERROR("Get netpose vehicle logo idx failed!");
	return -1;
}

static void vm_vehicle_color_to_netpose(char *buf, size_t size, int32_t color)
{
	typedef struct ncolor{
		HSVTRK_COLORS color;
		const char *netpose;
	}ncolor_t;

	const ncolor_t tab[] = {
		{ VCRDEFAULT, "Z" },
		{ VCRWHITE, "A" },
		{ VCRGRAY, "B" },
		{ VCRYELLOW, "C" },
		{ VCRPINK, "D" },
		{ VCRRED, "E" },
		{ VCRPURPLE, "F" },
		{ VCRGREEN, "G" },
		{ VCRBLUE, "H" },
		{ VCRBROWN, "I" },
		{ VCRBLACK, "J" },
		{ VCROTHER, "Z" }
	};

	const ncolor_t *p, *end;

	p = tab;
	end = tab + numberof(tab);

	for (; p < end; ++p) {
		if (p->color == color) {
			strncpy(buf, p->netpose, size);
			return;
		}
	}

	strncpy(buf, "Z", size);
}

static void vm_plate_color_to_netpose(char *buf, size_t size, int32_t color)
{
	typedef struct ncolor{
		PLATE_COLOR color;
		const char *netpose;
	}ncolor_t;

	const ncolor_t tab[] = {
		{ PLATE_WHITE, "White" },
		{ PLATE_YELLOW, "Yellow" },
		{ PLATE_BLUE, "Blue" },
		{ PLATE_BLACK, "Black" },
		{ PLATE_GREEN, "Other" },
		{ PLATE_OTHERS, "Other" },
		{ PLATE_BLUE_R, "Other" }
	};

	const ncolor_t *p, *end;

	p = tab;
	end = tab + numberof(tab);

	for (; p < end; ++p) {
		if (p->color == color) {
			strncpy(buf, p->netpose, size);
			return;
		}
	}

	strncpy(buf, "Other", size);

}

int protocol_convert_NetPose_moter_vehicle(DB_TrafficRecord *db_traffic_record,NetPoseVehicleProtocol *VehicleProtocol)
{

	VehicleProtocol->vehicleLogo = netpose_vehicle_logo_convert(db_traffic_record->vehicle_logo);


	switch(db_traffic_record->vehicle_type)  //车辆类型  按照数字来传给服务器
	{
		case 0:VehicleProtocol->vehicleType = 999;break; //其他
		case 1:VehicleProtocol->vehicleType = 12;break; //小型车
		case 2:VehicleProtocol->vehicleType = 5;break; //中型车
		case 3:VehicleProtocol->vehicleType = 0;break; //大型车
		default:VehicleProtocol->vehicleType = 999;      //其他
	}

	DEBUG("vehicle type: %d, netpose vehicle type: %d",
		  db_traffic_record->vehicle_type, VehicleProtocol->vehicleType);

	vm_vehicle_color_to_netpose(VehicleProtocol->vehicleColor,
								sizeof(VehicleProtocol->vehicleColor),
								db_traffic_record->color);

	DEBUG("vehicle color: %d, netpose vehicle color: %s",
		  db_traffic_record->color, VehicleProtocol->vehicleColor);

	switch(db_traffic_record->plate_type)        //车牌种类   按照数字来传给服务器
	{
		case 1:VehicleProtocol->vehicleNoType = 1;break;    //大型汽车号牌
		case 2:VehicleProtocol->vehicleNoType = 2;break;    //小型汽车号牌
		case 3:VehicleProtocol->vehicleNoType = 3;break;	//使馆汽车号牌
		case 4:VehicleProtocol->vehicleNoType = 4;break;	//领馆汽车号牌
		default:VehicleProtocol->vehicleNoType = 99;         //其他号牌
	}

	DEBUG("vehicle plate type: %d, netpose vehicle plate type: %d",
		  db_traffic_record->plate_type, VehicleProtocol->vehicleNoType);

	strcpy(VehicleProtocol->vehicleNo, db_traffic_record->plate_num);

	DEBUG("vehicle plate no: %s, netpose vehicle plate no: %s",
		  db_traffic_record->plate_num, VehicleProtocol->vehicleNo);

	vm_plate_color_to_netpose(VehicleProtocol->vehicleNoColor,
							  sizeof(VehicleProtocol->vehicleNoColor),
							  db_traffic_record->plate_color);

	DEBUG("vehicle plate color: %d, netpose vehicle plate color: %s",
		  db_traffic_record->plate_color, VehicleProtocol->vehicleNoColor);
    return 0;
}



int	protocol_convert_NetPose_others(DB_TrafficRecord *db_traffic_record,NetPoseOthersProtocol *OthresProtocol)
{

//	OthresProtocol->faceLeft = db_traffic_record->
	return 0;
}

/*******************************************************************************
 * 函数名:process_entrance_control
 * 功  能: 处理出入口道闸控制信息
 * 参  数:
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int process_entrance_control(MSGDATACMEM *msg_alg_result_info)
{
	void *result_addr_vir = get_dsp_result_cmem_pointer();

	if (result_addr_vir == NULL)
	{
		ERROR("Alg result virtual address is NULL\n");
		return -1;
	}

	unsigned long result_addr_phy = get_dsp_result_addr_phy();
	off_t offset = msg_alg_result_info->addr_phy- result_addr_phy;

	EntranceControlOutput entranceControlOutput;
	memcpy(&entranceControlOutput, ((char *)result_addr_vir + offset), sizeof(EntranceControlOutput));
    log_state("vd", "Rcv dsp info roadBrakeControlCmd:%d\n",
    		entranceControlOutput.roadBrakeControlCmd  );

	if (entranceControlOutput.roadBrakeControlCmd == 1) {
		rg_ctrl_t *rg_ctrl = rg_new_ctrl_alloc();

		if (rg_ctrl) {
			rg_ctrl->action = RG_ACTION_UP;
			rg_ctrl->ctrler = RG_CTRLER_LO;
			rg_ctrl->timeout = 1000;
			rg_ctrl->type = RG_CTRL_AUTO;

			roadgate_new_ctrl(rg_ctrl);
		}
	}

	return 0;

}



/**
 * process_fillinlight_smart_control
 *
 * @msg_alg_result_info: alg result information
 *
 * Return:
 *  0 - success, -1 - error
 */
int process_fillinlight_smart_control(MSGDATACMEM *msg_alg_result_info)
{
	int32_t daynight;
	void *result_addr_vir = get_dsp_result_cmem_pointer();
	FILE *fp = NULL;

	if (result_addr_vir == NULL)
	{
		printf("Alg result virtual address is NULL\n");
		return -1;
	}

	unsigned long result_addr_phy = get_dsp_result_addr_phy();
	off_t offset = msg_alg_result_info->addr_phy- result_addr_phy;

	static ISP_Parm_Out lstr_isp_parm_out;
	memcpy(&lstr_isp_parm_out, ((char *)result_addr_vir + offset), sizeof(lstr_isp_parm_out));

	if(lstr_isp_parm_out.flag_day_night == 1)
	{
		gi_filllight_smart = 0; //close the fill-in light, in day
	}
	else
	{
		gi_filllight_smart = 1;//open the fill-in light, in night
	}

	INFO("Recv from vpss info : gi_filllight_smart = %d", gi_filllight_smart);

	fp = fopen("/tmp/fillinlight_smart_control.info", "w");
	fprintf(fp, "%d\n", gi_filllight_smart);
	fclose(fp);

	daynight = (1 == lstr_isp_parm_out.flag_day_night) ? DAYTIME : NIGHTTIME;
	daynight_set(daynight);

	return 0;

}


