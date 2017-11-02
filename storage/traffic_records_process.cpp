
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
 * ������: alleyway_sendstatus_to_bitcom
 * ��  ��: ����� ����bitcomЭ��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int alleyway_sendstatus_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{
	Json::Value root;
	Json::StyledWriter style_writer;

	root["FaultState"] = gstr_alleyway_status.FaultState;						            //�豸����״̬
	root["SenseCoilState"] = gstr_alleyway_status.SenseCoilState;							      //��Ȧ״̬
	root["FlashlLightState"] = gstr_alleyway_status.FlashlLightState;                 //�����״̬
	root["IndicatoLightState"] = gstr_alleyway_status.IndicatoLightState;               //ָʾ��״̬

	std::string  str_vehicalinfo = style_writer.write(root);

	std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;

	//Э����Ϣ
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

	//***********ƴ��***********//
	strcat(str_vehical,str_vehicalinfo.c_str()); //��Ϣͷ + ������Ϣ

	TRACE_LOG_PLATFROM_INTERFACE("alleyway bitcom status = \n%s", str_vehical);

	//----------------��������----------------//

	//�����׽��ֲ����ӷ�����
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
	client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp); //IP��Ҫ��̬����


	if(connect(sockfd,(struct sockaddr *)(&client_addr),sizeof(struct sockaddr))==-1)
	 {
		  close(sockfd);
		  return -1;
	 }


	while(1)
	{
		//�ڷ��͵�ʱ����Ҫ�����׽��ֻ��������Ƿ������ݣ���Ҫ����ջ�����
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
		int h= select(sockfd +1, &rset, NULL, NULL, &tv); //������Ϣ�������֮��ȴ����շ������Ļظ���Ϣ�����2s��û���յ��ظ���Ϣ����������ִ��

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
 * ������: alleyway_sendto_bitcom_thread
 * ��  ��: �������ݵ���ƽ̨
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
void * alleyway_sendto_bitcom_thread(void * argv)
{

	//�����׽��ֲ����ӷ�����
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
		client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.Bitcom_Crk_Msg.ServerIp); //IP��Ҫ��̬����

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
				//�ڷ��͵�ʱ����Ҫ�����׽��ֻ��������Ƿ������ݣ���Ҫ����ջ�����
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
				int h= select(sockfd +1, &rset, NULL, NULL, &tv); //������Ϣ�������֮��ȴ����շ������Ļظ���Ϣ�����2s��û���յ��ظ���Ϣ����������ִ��

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
 * ������: alleyway_senddatas_to_bitcom
 * ��  ��: ����� ����bitcomЭ��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int alleyway_senddatas_to_bitcom(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{

	//��ȡʱ������
	char lt[48];
	char lc_position[20] = {0};
	memset(lt,0,48);
	long  ltime = time(NULL);
//	sprintf(lt,"----------BITCOMMSG0000%x",ltime); //�̶�Ϊ27���ַ�
	sprintf(lt,"----------COMPANYMSG%012x",(unsigned int)ltime); //�̶�Ϊ32���ַ�
	printf("%s:lt is %s\n",__func__,lt);
	//***********������Ϣ(��Ϣ)***********//

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

	root["Longitude"] = 0.0;						                        //ץ�ĵ㾭��(��֧��)
	root["Latitude"] = 0.0;							                        //ץ�ĵ�γ��(��֧��)
	root["VehicleInfoState"] = 0;                               //������Ϣ״̬��0:ʵʱ���� 1:��ʷ���� int
	root["IsPicUrl"] = 0;                   			              //ͼƬ����,0:����Ϣ���а���ͼƬ��1:ͨ��ͼƬ��ַѰ�� �����Ϊ0
	root["LaneIndex"] = db_traffic_record->lane_num;            //������ int

	//	root["position"] = lc_position;                                 //�����ڵ�һ��ͼƬ��λ��
	root["position"][0u] = (db_traffic_record->coordinate_x);                                 //�����ڵ�һ��ͼƬ��λ��
	root["position"][1] = (uint)db_traffic_record->coordinate_y;
	root["position"][2] = db_traffic_record->width;
	root["position"][3] = db_traffic_record->height;
	root["direction"] = db_traffic_record->direction;                               //���ƺ�  �ַ���
	root["PlateInfo1"] = PlateNum;                               //�����ƺ�  �ַ���
	root["PlateInfo2"] = "³B88888";             				//�����ƺ�  �ַ���  //��ʱδ��ֵ

	root["PlateColor"] = db_traffic_record->plate_color;//PlateColor;                            //������ɫ  �ַ���
	root["PlateType"] = db_traffic_record->plate_type;          //��������  int
	root["PassTime"] = db_traffic_record->time;                 //����ʱ�� ��ʽYYYY-MM-DD HH(24):MI:S  �ַ���
	root["VehicleSpeed"] = (double)db_traffic_record->speed;    //�����ٶ� double
	root["LaneMiniSpeed"] = 0.0;                                //���������٣��޴˹�����0.0  double
	root["LaneMaxSpeed"] = 0.0;                                 //���������٣��޴˹�����0.0  double
	root["VehicleType"] = db_traffic_record->vehicle_type;      //�������� int
	root["VehicleSubType"] = 0;                                 //����������
	root["VehicleColor"] = db_traffic_record->color;            //������ɫ int
	root["VehicleColorDepth"] = 0;        //������ɫ��ǳ
	root["VehicleLength"] = 0;                                  //�������ȣ���֧����0   int
	root["VehicleState"] = 1;								    //�г�״̬  int
	root["PicCount"] = 1;                                       //ͼƬ���� int

//	root["PicType"] = "[1]";                                        //�����ô��ֵ
	root["PicType"][0u] = 1;                                        //�������鸳ֵ

	root["PlatePicUrl"] = "";                                 //������Ƭurl
	root["VehiclePic1Url"] = "";                              //������Ƭurl
	root["VehiclePic2Url"] = "";
	root["VehiclePic3Url"] = "";
	root["CombinedPicUrl"] = "";  			                      //ͼƬ��ַ
	root["AlarmAction"] = 1;              			                //Υ������

	std::string  str_vehicalinfo = style_writer.write(root);
	std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;


	//***********������Ϣ��(ͼƬ)***********//
	char str_vehicalpic_head[72];
	memset(str_vehicalpic_head,0,72);
	sprintf(str_vehicalpic_head,"--%s\r\n"\
						   "Content-Type: image/jpeg\r\n",lt);

	int len_str_vehicalpic_head = strlen(str_vehicalpic_head);

	char str_vehicalpic_tail[48];
	memset(str_vehicalpic_tail,0,48);
	sprintf(str_vehicalpic_tail,"--%s--\r\n",lt);

	int len_str_vehicalpic_tail = strlen(str_vehicalpic_tail);

	//***********������Ϣͷ***********//
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

	//***********ƴ��***********//
	strcat(str_vehical,str_vehicalinfo.c_str()); //��Ϣͷ + ������Ϣ

	strcat(str_vehical,str_vehicalpic_head);     //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ

	int len = strlen(str_vehical);               //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ���ַ�������

	memcpy(str_vehical + len,pic_info->buf,pic_info->size);  //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ + ͼƬ

	memcpy(str_vehical + len + pic_info->size,str_vehicalpic_tail,len_str_vehicalpic_tail); //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ + ͼƬ + ��Ϣβ  �ܴ�С


	TRACE_LOG_PLATFROM_INTERFACE("alleyway bitcom datas = \n%s", str_vehical);


	//bitcom���ʹ����ݸ�ֵ
	if(gstr_bitcom_data.flag == 0)
	{
		gstr_bitcom_data.buf_lens = len + pic_info->size + len_str_vehicalpic_tail;
		memcpy(gstr_bitcom_data.buf, str_vehical, gstr_bitcom_data.buf_lens);

		gstr_bitcom_data.flag = 1;
	}
    return 0;
}

/*******************************************************************************
 * ������: bitcom_to_dahua_illegal
 * ��  ��: bitcom to dahua Υ������
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_illegal(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->color)
	{
		//Υ��ͣ��
		case BITCOM_ILLEGAL_PARKING_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_PARKING_VEHICLE;
		break;

		//ѹ��
		case BITCOM_ILLEGAL_LINEBALL_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_LINEBALL_VEHICLE;
		break;

		//����������ʻ
		case BITCOM_ILLEGAL_UNLANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_UNLANE_VEHICLE;
		break;

		//ѹ����
		case BITCOM_ILLEGAL_PRESSYELLOW_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_PRESSYELLOW_VEHICLE;
		break;

		//Υ�±��
		case BITCOM_ILLEGAL_LANECHANG_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_LANECHANG_VEHICLE;
		break;

		//����
		case BITCOM_ILLEGAL_RETROGRADE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_RETROGRADE_VEHICLE;
		break;

		//�����
		case BITCOM_ILLEGAL_JAYWALK_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_JAYWALK_VEHICLE;
		break;

		//����
		case BITCOM_ILLEGAL_OVERSPEED_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_OVERSPEED_VEHICLE;
		break;

		//�г�ռ��
		case BITCOM_ILLEGAL_CARLANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_CARLANE_VEHICLE;
		break;

		//�ǻ�������
		case BITCOM_ILLEGAL_BICYCLELANE_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_BICYCLELANE_VEHICLE;
		break;

		//������Υ�������־ָʾ
		case DAHUA_ILLEGAL_FLAG1_VEHICLE:
		case DAHUA_ILLEGAL_FLAG2_VEHICLE:
		case DAHUA_ILLEGAL_FLAG3_VEHICLE:
		case DAHUA_ILLEGAL_FLAG4_VEHICLE:
		case DAHUA_ILLEGAL_FLAG5_VEHICLE:
			as_traffic_record->color = DAHUA_ILLEGAL_FLAG_VEHICLE;
		break;

		//����
		default:
			as_traffic_record->color = DAHUA_ILLEGAL_OTHERS_VEHICLE;
		break;

	}

	return 0;
}


/*******************************************************************************
 * ������: bitcom_to_dahua_vehiclecolour
 * ��  ��: bitcom to dahua ������ɫת��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_vehiclecolour(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->color)
	{
		//��ɫ
		case BITCOM_COLOUR_WHITE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_WHITE_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_BLACK_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_BLACK_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_RED_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_RED_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_YELLOW_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_YELLOW_VEHICLE;
		break;

		//����ɫ
		case BITCOM_COLOUR_SILVERGREY_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_SILVERGREY_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_BLUE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_BLUE_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_GREEN_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_GREEN_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_PURPLE_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_PURPLE_VEHICLE;
		break;

		//��ɫ
		case BITCOM_COLOUR_BROWN_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_OTHERS_VEHICLE;
		break;

		//����
		case BITCOM_COLOUR_OTHERS_VEHICLE:
			as_traffic_record->color = DAHUA_COLOUR_OTHERS_VEHICLE;
		break;

		//δʶ��
		default:
			as_traffic_record->color = DAHUA_COLOUR_UNKNOW_VEHICLE;
		break;

	}
	return 0;
}



/*******************************************************************************
 * ������: bitcom_to_dahua_vehicletype
 * ��  ��: bitcom to dahua ��������ת��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_vehicletype(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->plate_type)
	{
		//С�ͳ�
		case BITCOM_SMALL_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_MINIATURE_VEHICLE;
		break;

		//���ͳ�
		case BITCOM_LARGE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_OVERSIZE_VEHICLE;
		break;

		//ʹ������
		case BITCOM_EMBASSY_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_EMBASSY_VEHICLE;
		break;

		//�������
		case BITCOM_CONSULATE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_CONSULATE_VEHICLE;
		break;

		//��������
		case BITCOM_FOREIGN_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_OVERSEAS_VEHICLE;
		break;

		//�⼮����
		case BITCOM_FOREIGNNATION_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_FOREIGN_VEHICLE;
		break;

		//��������
		case BITCOM_LOWSPEED_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_LOWSPEED_VEHICLE;
		break;

		//������
		case BITCOM_TRACTOR_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TRACTOR_VEHICLE;
		break;

		//�ҳ�
		case BITCOM_GUA_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TRAILER_VEHICLE;
		break;

		//������
		case BITCOM_XUE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_COACH_VEHICLE;
		break;

		//��ʱ��ʻ��
		case BITCOM_TEMPORARY_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_TEMP_VEHICLE;
		break;

		//��������
		case BITCOM_POLICE_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_POLICE_VEHICLE;
		break;

		//����Ħ�г�
		case BITCOM_POLICE_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_POLICEMOTOR_VEHICLE;
		break;

		//��ͨĦ��
		case BITCOM_TRIWHEEL_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_MOTOR_VEHICLE;
		break;

		//���Ħ�г�
		case BITCOM_LIGHT_MOTOBIKE:
			as_traffic_record->vehicle_type = DAHUA_TYPE_LIGHTMOTOR_VEHICLE;
		break;

		//δʶ��
		case BITCOM_UNKNOWN_CAR:
			as_traffic_record->vehicle_type = DAHUA_TYPE_UNKNOW_VEHICLE;
		break;

		default:
		break;
	}

	return 0;
}



/*******************************************************************************
 * ������: bitcom_to_dahua_platetype
 * ��  ��: bitcom to dahua ��������
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_platetype(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->plate_type)
	{
		//С�ͳ�
		case BITCOM_SMALL_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_MINIATURE_VEHICLE;
		break;

		//���ͳ�
		case BITCOM_LARGE_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_OVERSIZE_VEHICLE;
		break;

		//���⳵
		case BITCOM_FOREIGNNATION_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_FOREIGN_VEHICLE;
		break;

		//δʶ��
		case BITCOM_UNKNOWN_CAR:
			as_traffic_record->plate_type = DAHUA_PLATE_UNKONW_VEHICLE;
		break;

		//�����ͳ���
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
 * ������: bitcom_to_dahua_direction
 * ��  ��: bitcom to dahua ������ʻ����ת��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_direction(DB_TrafficRecord *as_traffic_record)
{
	switch(as_traffic_record->direction)
	{
		//������
		case BITCOM_EAST_TO_WEST:
			as_traffic_record->direction = DAHUA_EAST_TO_WEST;
		break;

		//����
		case BITCOM_WEST_TO_EAST:
			as_traffic_record->direction = DAHUA_WEST_TO_EAST;
		break;

		//����
		case BITCOM_NORTH_TO_SOUTH:
			as_traffic_record->direction = DAHUA_NORTH_TO_SOUTH;
		break;

		//������
		case BITCOM_SOUTH_TO_NORTH:
			as_traffic_record->direction = DAHUA_SOUTH_TO_NORTH;
		break;

		//����������
		case BITCOM_SOUTHEAST_TO_NORTHWEST:
			as_traffic_record->direction = DAHUA_SOUTHEAST_TO_NORTHWEST;
		break;

		//��������
		case BITCOM_NORTHWEST_TO_SOUTHEAST:
			as_traffic_record->direction = DAHUA_NORTHWEST_TO_SOUTHEAST;
		break;

		//����������
		case BITCOM_NORTHEAST_TO_SOUTHWEST:
			as_traffic_record->direction = DAHUA_NORTHEAST_TO_SOUTHWEST;
		break;

		//�����򶫱�
		case BITCOM_SOUTHWEST_TO_NORTHEAST:
			as_traffic_record->direction = DAHUA_SOUTHWEST_TO_NORTHEAST;
		break;

	}
	return 0;
}

/*******************************************************************************
 * ������: bitcom_to_dahua_platecolour
 * ��  ��: bitcom to dahua ������ɫת��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
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
 * ������: bitcom_to_dahua_protocol
 * ��  ��: bitcom to dahua Э��ת��
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
int bitcom_to_dahua_protocol(DB_TrafficRecord *as_traffic_record)
{
	//��������ת��
	bitcom_to_dahua_platetype(as_traffic_record);

	//������ɫת��
	bitcom_to_dahua_platecolour(as_traffic_record);

	//��ʻ����ת��
	bitcom_to_dahua_direction(as_traffic_record);

	//��������ת��
	bitcom_to_dahua_vehicletype(as_traffic_record);

	//������ɫת��
	bitcom_to_dahua_vehiclecolour(as_traffic_record);

	return 0;
}



/*******************************************************************************
 * ������: process_VM_record
 * ��  ��: ������������ͨ�м�¼
 * ��  ��: result��Ҫ����Ľ��
 * ����ֵ: �ɹ�������0����������-1
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

	do /* ����ͼƬ */
	{
		analyze_traffic_records_picture(&pic_info, info);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			if (send_traffic_records_picture_buf(&pic_info) < 0)
			{
				flag_send |= 0x02; 	/* ͼƬ�ϴ�ʧ�ܣ���Ҫ���� */
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

	do /* ������Ϣ */
	{
		analyze_traffic_records_info_motor_vehicle(
		    &db_traffic_record, result, &pic_info, partition_path);

		TRACE_LOG_SYSTEM("picture1=%s", pic_info.name);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			flag_send |= 0x01; 		/* ������Ϊ��Ҫ���� */

			if (((flag_send & 0x02) == 0) &&
			        (send_traffic_records_info(&db_traffic_record) == 0))
			{
				flag_send = 0x00; 	/* ����ϴ��ɹ����ٻָ�Ϊ�����ϴ� */
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


		//��������ƽ̨
		if(flg_register_NetPose_vehicle == 1)
		{
			INFO("vehicle record send to netpose");
			send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(&db_traffic_record,&pic_info);
		}

		//��ƽ̨
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
 * ������: process_traffic_record_others
 * ��  ��: ����������ͨ��¼
 * ��  ��: result��Ҫ����Ľ��
 * ����ֵ: �ɹ�������0����������-1
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

	do /* ����ͼƬ */
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
				flag_send |= 0x02; 	/* ͼƬ�ϴ�ʧ�ܣ���Ҫ���� */
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

	do /* ������Ϣ */
	{
		analyze_traffic_record_info_others(
		    &db_traffic_record, result, &pic_info, partition_path);

		if (EP_UPLOAD_CFG.vehicle == 1)
		{
			flag_send |= 0x01; 		/* ������Ϊ��Ҫ���� */

			if (((flag_send & 0x02) == 0) &&
			        (send_traffic_records_info(&db_traffic_record) == 0))
			{
				flag_send = 0x00; 	/* ����ϴ��ɹ����ٻָ�Ϊ�����ϴ� */
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
 * ������: analyze_traffic_record_picture
 * ��  ��: ����������ͨ�м�¼ͼƬ
 * ��  ��: pic_info������������ͼƬ��Ϣ��lane_num��������
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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

	/* ·�������û������ý������� */
	int len = 0;
	for (int j = 0; j<EP_FTP_URL_LEVEL.levelNum; j++)
	{
		switch (EP_FTP_URL_LEVEL.urlLevel[j])
		{
		case SPORT_ID: 		//�ص���
			len += sprintf(pic_info->path[j], "/%s", EP_POINT_ID);
			break;
		case DEV_ID: 		//�豸���
			len += sprintf(pic_info->path[j], "/%s", EP_DEV_ID);
			break;
		case YEAR_MONTH: 	//��/��
			len += sprintf(pic_info->path[j], "/%04d%02d",
			               pic_info->tm.year, pic_info->tm.month);
			break;
		case DAY: 			//��
			len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.day);
			break;
		case EVENT_NAME: 	//�¼�����
			len += sprintf(pic_info->path[j], "/%s",
			               TRAFFIC_RECORDS_FTP_DIR);
			break;
		case HOUR: 			//ʱ
			len += sprintf(pic_info->path[j], "/%02d", pic_info->tm.hour);
			break;
		case FACTORY_NAME: 	//��������
			len += sprintf(pic_info->path[j], "/%s", EP_MANUFACTURER);
			break;
		default:
			break;
		}
		printf("pic_info.path : %s\n",pic_info->path[j]);
	}


	/* �ļ�����ʱ��ͳ��������� */
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
 * ������: send_traffic_records_image_buf
 * ��  ��: ���ͻ����е�ͨ�м�¼ͼƬ
 * ��  ��: pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: save_traffic_records_image_buf
 * ��  ��: ���滺���е�ͨ�м�¼ͼƬ
 * ��  ��: pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: analyze_traffic_record_info_motor_vehicle
 * ��  ��: ����������ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_record�������Ľ����
 *         image_info��ͼ����Ϣ��pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ: �ɹ�������0
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
	db_traffic_record->objective_type 	= 1; /* 1��ʾ������ */
	db_traffic_record->coordinate_x 	= result->xPos;
	db_traffic_record->coordinate_y 	= result->yPos;
	db_traffic_record->width 			= result->width;
	db_traffic_record->height 			= result->height;
	db_traffic_record->pic_flag 		= 1; /* Ŀǰֻ��1��ͼƬ */
	db_traffic_record->plate_color 		= result->color;
	db_traffic_record->confidence		= result->confidence;
#if (5 == DEV_TYPE)
	db_traffic_record->detect_coil_time = result->triggerCoilTime;
#endif

	snprintf(db_traffic_record->description,
	         sizeof(db_traffic_record->description),
	         "ͼ����: %d", result->pic_id);

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
 * ������: send_traffic_record_info
 * ��  ��: ����ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_records���¼�������Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: save_traffic_record_info
 * ��  ��: ����ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_record��ͨ�м�¼��Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int save_traffic_records_info(
    const DB_TrafficRecord *db_traffic_record,
    const EP_PicInfo *pic_info, const char *partition_path)
{
	resume_print("In save_traffic_record_info!\n");

	char db_traffic_name[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	static DB_File db_file;
	int flag_record_in_DB_file=0;//�Ƿ���Ҫ��¼���������ݿ�

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

	//�ж����ݿ��ļ��Ƿ��Ѿ�����
	//�������ڣ���Ҫ��Ӧ����һ��������¼
	//�����󣬿���û��ɾ��¼���ݿ��ļ���ȴɾ�˶�Ӧ��������¼��
	if (access(db_traffic_name, F_OK) != 0)
	{
		resume_print("The db_traffic is not exit\n");
		//���ݿ��ļ������ڣ���Ҫ���������ݿ���������Ӧ��һ����¼
		flag_record_in_DB_file=1;
	}
	else
	{
		resume_print("The db_traffic is  exit\n");
		flag_record_in_DB_file=0;
	}



	//�����������ݿ��У��Ƿ���ڸü�¼���ݿ���
	//����ֻ�������Ƿ����������ݣ�����ʵʱ�洢��
	//if ((flag_upload_complete_traffic==1)||(strcasecmp(db_traffic_name_last, db_traffic_name)))//�������ݿ�����

	//������ʵʱ�洢��ֻ���������ݿ��ļ��Ƿ����:
	//----�������ڣ����������¼�������ڣ�����ӡ�
	//��������ʵʱ�洢��
	//----�����������ݿ��ļ������������¼��
	//���������ݿ��ļ��������ж��Ƿ������������:
	//       ----���������ݣ�����ӣ�û���������ݣ���ӡ�
	//if((flag_record_in_DB_file==1)||((flag_upload_complete_traffic==1)&&(EP_DISK_SAVE_CFG.vehicle==0)))

	if (flag_record_in_DB_file==1)//��֤��������¼��ɾ����ͬʱɾ��Ӧ��ͨ���ݿ�
	{

		resume_print("add to DB_files, db_traffic_name is %s\n", db_traffic_name);

		//дһ����¼�����ݿ�������


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

		// ��һ��д�������ݿ�ʱ����������������������һ����������������ʵʱ�洢��
		// ���ԣ�����һ����������ʱ����Ҫ��Ӧ�޸��������ݿ⡣
		char sql_cond[1024];
		static int flag_first=1;
		if (flag_first==1)
		{
			//��һ�ν��룬Ҫ��ȡ���µ�һ��������¼��
			flag_first=0;

			//char sql_cond[1024];
			char history_time_start[64];			//ʱ����� ʾ����2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_traffic_record->time, history_time_start);
			sprintf(sql_cond, "SELECT * FROM DB_files WHERE time>='%s'  limit 1;", history_time_start );
			db_read_DB_file(DB_NAME_VM_RECORD, &db_file, &mutex_db_files_traffic, sql_cond);
		}

		printf("db_file.flag_send=%d,db_traffic_records->flag_send=%d\n",db_file.flag_send,db_traffic_record->flag_send);
		if (( ~(db_file.flag_send) & db_traffic_record->flag_send)!=0)	//���¼�¼�а������µ�������־��Ϣ
		{
			db_file.flag_send |=db_traffic_record->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;", db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}

		if ( (db_file.flag_store==0) && (db_traffic_record->flag_store!=0))//��Ҫ���Ӵ洢��Ϣ
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
 * ������: format_mq_text_traffic_record
 * ��  ��: ��ʽ����ͨͨ�м�¼MQ�ı���Ϣ
 * ��  ��: mq_text��MQ�ı���Ϣ��������record��������¼
 * ����ֵ: �ַ�������
*******************************************************************************/
int format_mq_text_traffic_record(
    char *mq_text, const DB_TrafficRecord *record)
{
	if (record->objective_type == 1) 	/* ������ */
		return format_mq_text_traffic_record_motor_vehicle(mq_text, record);
	else 								/* ���˻�ǻ����� */
		return format_mq_text_traffic_record_others(mq_text, record);
}

/*******************************************************************************
 * ������: format_mq_text_traffic_record_motor_vehicle
 * ��  ��: ��ʽ����������ͨͨ�м�¼MQ�ı���Ϣ
 * ��  ��: mq_text��MQ�ı���Ϣ��������record��������¼
 * ����ֵ: �ַ�������
*******************************************************************************/
int format_mq_text_traffic_record_motor_vehicle(
    char *mq_text, const DB_TrafficRecord *record)
{
	int len = 0;

	len += sprintf(mq_text + len, "%s", EP_DATA_SOURCE);
	/* �ֶ�1 ������Դ */

	len += sprintf(mq_text + len, ",%s", record->plate_num);
	/* �ֶ�2 ���ƺ��� */

	len += sprintf(mq_text + len, ",%02d", record->plate_type);
	/* �ֶ�3 �������� */

	len += sprintf(mq_text + len, ",%s", record->point_id);
	/* �ֶ�4 �ɼ����� */

	len += sprintf(mq_text + len, ",%s", record->point_name);
	/* �ֶ�5 �ɼ������� */

	len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
	/* �ֶ�6 ץ������ */

	len += sprintf(mq_text + len, ",%s", record->dev_id);
	/* �ֶ�7 �豸��� */

	len += sprintf(mq_text + len, ",%02d", record->lane_num);
	/* �ֶ�8 ������� */

	len += sprintf(mq_text + len, ",%d", record->speed);
	/* �ֶ�9 �����ٶ� */

	len += sprintf(mq_text + len, ",%s", record->time);
	/* �ֶ�10 ץ��ʱ�� */

	len += sprintf(mq_text + len, ",%s", record->collection_agencies);
	/* �ֶ�11 �ɼ����ر�� */

	len += sprintf(mq_text + len, ",%d", record->direction);
	/* �ֶ�12 ������ */

	len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
	               record->ftp_user, record->ftp_passwd,
	               record->ftp_ip, record->ftp_port, record->image_path);
	/* �ֶ�13 ��һ��ͼƬ��ַ */

	len += sprintf(mq_text + len, ",,");
	/* �ֶ�14��15 �ڶ�����ͼƬ��ַ��Ŀǰû�� */

	len += sprintf(mq_text + len, ",%d", record->color);
	/* �ֶ�16 ������ɫ��� */

	len += sprintf(mq_text + len, ",%d", record->vehicle_logo);
	/* �ֶ�17 ������ */

	len += sprintf(mq_text + len, ",%d", record->objective_type);
	/* �ֶ�18 Ŀ������ */

	len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d",
	               record->coordinate_x, record->coordinate_y,
	               record->width, record->height, record->pic_flag);
	/* �ֶ�19 ���ƶ�λ��X���ꡢY���ꡢ��ȡ��߶ȡ��ڼ���ͼƬ�� */

	len += sprintf(mq_text + len, ",%d", record->plate_color);
	/* �ֶ�20 ������ɫ */

	len += sprintf(mq_text + len, ",,,,,,,,,,");
	/* �ֶ�21��30 δ�� */

	len += sprintf(mq_text + len, ",%d", record->plate_color);
	/* �ֶ�31 ������ɫ */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�32 δ�� */

	len += sprintf(mq_text + len, ",%s", record->description);
	/* �ֶ�33 �������� */

	len += sprintf(mq_text + len, ",%d", record->vehicle_type);
	/* �ֶ�34 ���� */

	return len;
}

/*******************************************************************************
 * ������: format_mq_text_traffic_record_others
 * ��  ��: ��ʽ�����˻�ǻ�������ͨͨ�м�¼MQ�ı���Ϣ
 * ��  ��: mq_text��MQ�ı���Ϣ��������record��������¼
 * ����ֵ: �ַ�������
*******************************************************************************/
int format_mq_text_traffic_record_others(
    char *mq_text, const DB_TrafficRecord *record)
{
	int len = 0;

	len += sprintf(mq_text + len, "%s", EP_DATA_SOURCE);
	/* �ֶ�1 ������Դ */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�2 ���ƺ��룬���˺ͷǻ�����û�� */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�3 �������ͣ����˺ͷǻ�����û�� */

	len += sprintf(mq_text + len, ",%s", record->point_id);
	/* �ֶ�4 �ɼ����� */

	len += sprintf(mq_text + len, ",%s", record->point_name);
	/* �ֶ�5 �ɼ������� */

	len += sprintf(mq_text + len, ",%s", EP_SNAP_TYPE);
	/* �ֶ�6 ץ������ */

	len += sprintf(mq_text + len, ",%s", record->dev_id);
	/* �ֶ�7 �豸��� */

	len += sprintf(mq_text + len, ",%02d", record->lane_num);
	/* �ֶ�8 ������� */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�9 �����ٶȣ����˺ͷǻ�����û�� */

	len += sprintf(mq_text + len, ",%s", record->time);
	/* �ֶ�10 ץ��ʱ�� */

	len += sprintf(mq_text + len, ",%s", record->collection_agencies);
	/* �ֶ�11 �ɼ����ر�� */

	len += sprintf(mq_text + len, ",%d", record->direction);
	/* �ֶ�12 ������ */

	len += sprintf(mq_text + len, ",ftp://%s:%s@%s:%d/%s",
	               record->ftp_user, record->ftp_passwd,
	               record->ftp_ip, record->ftp_port, record->image_path);
	/* �ֶ�13 ��һ��ͼƬ��ַ */

	len += sprintf(mq_text + len, ",,");
	/* �ֶ�14��15 �ڶ�����ͼƬ��ַ��Ŀǰû�� */

	len += sprintf(mq_text + len, ",%d", record->color);
	/* �ֶ�16 �·���ɫ��� */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�17 �����ţ����˺ͷǻ�����û�� */

	len += sprintf(mq_text + len, ",%d", record->objective_type);
	/* �ֶ�18 Ŀ������ */

	len += sprintf(mq_text + len, ",%d/%d/%d/%d/%d",
	               record->coordinate_x, record->coordinate_y,
	               record->width, record->height, record->pic_flag);
	/* �ֶ�19 Ŀ�궨λ��X���ꡢY���ꡢ��ȡ��߶ȡ��ڼ���ͼƬ�� */

	len += sprintf(mq_text + len, ",,,,,,,,,,,");
	/* �ֶ�20��30 δ�� */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�31 ������ɫ�����˺ͷǻ�����û�� */

	len += sprintf(mq_text + len, ",");
	/* �ֶ�32 δ�� */

	len += sprintf(mq_text + len, ",%s", record->description);
	/* �ֶ�33 �������� */

	len += sprintf(mq_text+len, ",");
	/* �ֶ�34 ���ͣ����˺ͷǻ�����û�� */

	return len;
}

/*******************************************************************************
 * ������: db_write_traffic_records
 * ��  ��: д��ͨͨ�м�¼���ݿ�
 * ��  ��: records����ͨͨ�м�¼
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: db_format_insert_sql_traffic_records
 * ��  ��: ��ʽ����ͨͨ�м�¼��SQL�������
 * ��  ��: sql��SQL���棻records��ͨ�м�¼
 * ����ֵ: SQL����
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
 * ������: db_unformat_read_sql_traffic_records
 * ��  ��: �����ӻ�����ͨ�м�¼���ж���������
 * ��  ��: azResult�����ݿ���ж��������ݻ��棻buf��ͨ�м�¼�ṹ��ָ��
 * ����ֵ: 0����������Ϊ�쳣
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
 * ������: db_read_traffic_record
 * ��  ��: ��ȡ���ݿ��е�ͨ�м�¼
 * ��  ��:
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_read_traffic_record(const char *db_name, void *records,
                           pthread_mutex_t *mutex_db_records, char * sql_cond)
{
	char *pzErrMsg = NULL;
	int rc;
	sqlite3 *db = NULL;
	//char sql[1024];
	int nrow = 0, ncolumn = 0; //��ѯ�����������������
	char **azResult; //��ά�����Ž��
	static char plateNum[16]; //�ж�ͬһ���ƴ��͵Ĵ�������ֹɾ�����ݿ�ʧ�������ظ�����
	int samePlateCnt = 0; //����ʱ�ж�ͬһ���ƴ��͵Ĵ���
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

	//��ѯ����
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
	if (strcmp(traffic_record->plate_num, plateNum) == 0)//�жϳ��ƺ��Ƿ��ظ�
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
 * ������: db_add_column_traffic_records
 * ��  ��: �����ݿ�������һ�����ֶΣ���ʾ��
 * ��  ��:
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int db_add_column_traffic_records(char *db_name, void *records,
                                  pthread_mutex_t *mutex_db_records)
{
	int rc;
	sqlite3 *db = NULL;
	char sql[1024];
	//int nrow = 0, ncolumn = 0; //��ѯ�����������������
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

	//��ѯ������ӵļ�¼������
	memset(sql, 0, sizeof(sql));
	//sprintf(sql, "UPDATE traffic_records SET flag_send=1 WHERE flag_send=0");// ����ĳ�ֶ�ֵ
	sprintf(sql, "Alter TABLE traffic_records ADD COLUMN author_id INTEGER");// �����ֶ�
	//sprintf(sql, "UPDATE traffic_records SET author_id=26");//�������ֶε�ֵ

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


//ɾ��ָ��ID ��һ����¼
//ɾ��ָ��ID �ļ�¼�������ڷ���ʷ��¼
//������������־--������ʷ��¼
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
	//��ѯ����
	memset(sql, 0, sizeof(sql));

	if (traffic_record->flag_store==1)
	{
		//����������־
		sprintf(sql, "UPDATE traffic_records SET flag_send=0 WHERE ID = %d ;", traffic_record->ID);
	}
	else
	{
		//ɾ��ָ��ID �ļ�¼	//ֻ����һ�����ݿ�ȫ���������ʱ������ɾ��������Ӧ�ļ�¼
		sprintf(sql, "DELETE FROM traffic_records WHERE ID = %d ;", traffic_record->ID);
	}

	DEBUG("the sql is %s\n", sql);
	rc = sqlite3_exec(db, sql, 0, 0, &pzErrMsg);
	if (rc != SQLITE_OK)
	{
		ERROR("Delete data failed, flag_store=%d,  Error Message: %s\n", traffic_record->flag_store, pzErrMsg);
	}

	sqlite3_free(pzErrMsg);

	//  ��Ϊ��?
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
 * ������: send_traffic_records_info_history
 * ��  ��: ����ͨ����ʷ��¼��Ϣ
 * ��  ��: db_traffic_records���¼�������Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
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
 * ������: send_traffic_records_image_file
 * ��  ��: ֱ�ӷ��͹�����¼ͼƬ�ļ�
 * ��  ��: pic_info��ͼƬ�ļ���Ϣ��
 * ����ֵ: �ɹ�������0���ϴ�ʧ�ܣ�����-1���ļ������ڣ�����-2
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

	if (db_traffic_record->flag_store == 0) /* �������ʵʱ�洢��ת���ļ� */
	{
		move_record_to_trash(db_traffic_record->partition_path,
		                     db_traffic_record->image_path);
	}

	return 0;
}

#if 0
/*******************************************************************************
 * ������: save_traffic_records_info
 * ��  ��: ����ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_records���¼�������Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int save_traffic_records_info(
    const DB_TrafficRecord *db_traffic_records,
    const void *image_info, const char *partition_path)
{

	char db_traffic_name[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	static DB_File db_file;
	//static char db_traffic_name_last[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	int flag_record_in_DB_file=0;//�Ƿ���Ҫ��¼���������ݿ�
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

	//�ж����ݿ��ļ��Ƿ��Ѿ�����
	//�������ڣ���Ҫ��Ӧ����һ��������¼
	//�����󣬿���û��ɾ��¼���ݿ��ļ���ȴɾ�˶�Ӧ��������¼��
	if (access(db_traffic_name, F_OK) != 0)
	{
		//���ݿ��ļ������ڣ���Ҫ���������ݿ���������Ӧ��һ����¼
		flag_record_in_DB_file=1;
	}
	else
	{
		flag_record_in_DB_file=0;
	}




	//�����������ݿ��У��Ƿ���ڸü�¼���ݿ���
	//����ֻ�������Ƿ����������ݣ�����ʵʱ�洢��
	//if ((flag_upload_complete_traffic==1)||(strcasecmp(db_traffic_name_last, db_traffic_name)))//�������ݿ�����

	//������ʵʱ�洢��ֻ���������ݿ��ļ��Ƿ����:
	//----�������ڣ����������¼�������ڣ�����ӡ�
	//��������ʵʱ�洢��
	//----�����������ݿ��ļ������������¼��
	//���������ݿ��ļ��������ж��Ƿ������������:
	//       ----���������ݣ�����ӣ�û���������ݣ���ӡ�
	//if((flag_record_in_DB_file==1)||((flag_upload_complete_traffic==1)&&(EP_DISK_SAVE_CFG.vehicle==0)))

	if (flag_record_in_DB_file==1)//��֤��������¼��ɾ����ͬʱɾ��Ӧ��ͨ���ݿ�
	{
		//printf("db_traffic_name_last is %s, db_traffic_name is %s\n",
		//		db_traffic_name_last, db_traffic_name);
		printf("add to DB_files, db_traffic_name is %s\n", db_traffic_name);

		//дһ����¼�����ݿ�������


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

		// ��һ��д�������ݿ�ʱ����������������������һ����������������ʵʱ�洢��
		// ���ԣ�����һ����������ʱ����Ҫ��Ӧ�޸��������ݿ⡣
		char sql_cond[1024];
		static int flag_first=1;
		if (flag_first==1)
		{
			//��һ�ν��룬Ҫ��ȡ���µ�һ��������¼��
			flag_first=0;

			//char sql_cond[1024];
			char history_time_start[64];			//ʱ����� ʾ����2013-06-01 12:01:02

			memset(history_time_start, 0, sizeof(history_time_start));
			get_hour_start(db_traffic_records->time, history_time_start);
			sprintf(sql_cond, "SELECT * FROM DB_files WHERE time>='%s'  limit 1;", history_time_start );
			db_read_DB_file(DB_NAME_VM_RECORD, &db_file, &mutex_db_files_traffic, sql_cond);
		}

		printf("db_file.flag_send=%d,db_traffic_records->flag_send=%d\n",db_file.flag_send,db_traffic_records->flag_send);
		if (( ~(db_file.flag_send) & db_traffic_records->flag_send)!=0)	//���¼�¼�а������µ�������־��Ϣ
		{
			db_file.flag_send |=db_traffic_records->flag_send;
			sprintf(sql_cond, "UPDATE DB_files SET flag_send=%d WHERE ID=%d;", db_file.flag_send, db_file.ID);
			db_update_records(DB_NAME_VM_RECORD, sql_cond, &mutex_db_files_traffic);
		}

		if ( (db_file.flag_store==0) && (db_traffic_records->flag_store!=0))//��Ҫ���Ӵ洢��Ϣ
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
 * ������: save_traffic_records_info_others
 * ��  ��: ����ǻ�������ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_records���¼�������Ϣ
 * ����ֵ: �ɹ�������0��ʧ�ܣ�����-1
*******************************************************************************/
int save_traffic_records_info_others(
    const DB_TrafficRecord *db_traffic_records,
    const void *image_info, const char *partition_path)
{
	char db_traffic_name[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	static char db_traffic_name_last[PATH_MAX_LEN];//  ����: /mnt/sda1/DB/traffic_records_2013082109.db
	passRecordNoVehicle *traffic_records =
	    (passRecordNoVehicle *) ((char *) image_info + IMAGE_INFO_SIZE);//����һ�䲻ͬ(�Ա�save_traffic_records_info)

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

	//�ж��������ļ������Ƿ�����һ�ε��ظ�
	//�Ժ���ӣ���Ҳ�п�������֮ǰ���ظ�����Ҫѭ���жϣ���
	//�����󣬲��ǵ���һ�ε�����
	if (strcasecmp(db_traffic_name_last, db_traffic_name))//�������ݿ�����
	{
		printf("db_traffic_name_last is %s, db_traffic_name is %s\n",
		       db_traffic_name_last, db_traffic_name);
		//дһ����¼�����ݿ�������
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
 * ������: analyze_traffic_record_info_others
 * ��  ��: �����ǻ�����������ͨ�м�¼��Ϣ
 * ��  ��: db_traffic_records�������Ľ����
 *         image_info��ͼ����Ϣ��pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ:
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
		         "�ǻ�����");
		break;
	case 2:
		snprintf(db_traffic_record->plate_num,
		         sizeof(db_traffic_record->plate_num),
		         "����");
		break;
	default:
		snprintf(db_traffic_record->plate_num,
		         sizeof(db_traffic_record->plate_num),
		         "�ǻ�����-����");
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

	/* Ŀ�����ͣ�2��ʾ�ǻ�������3��ʾ���ˡ��㷨�����Ҫ��1 */
	db_traffic_record->objective_type 	= result->targetType + 1;

	db_traffic_record->coordinate_x 	= result->xPos;
	db_traffic_record->coordinate_y 	= result->yPos;
	db_traffic_record->width 			= result->width;
	db_traffic_record->height 			= result->height;
	db_traffic_record->pic_flag 		= 1; /* Ŀǰֻ��1��ͼƬ */

	snprintf(db_traffic_record->description,
	         sizeof(db_traffic_record->description),
	         "ͼ����: %d", result->pic_id);

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
 * ������: bitcom_sendto_dahua
 * ��  ��: �������ݵ���ƽ̨
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
*******************************************************************************/
void * bitcom_sendto_dahua(void * argv)
{

	//�����׽��ֲ����ӷ�����
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
		client_addr.sin_addr.s_addr = inet_addr(msgbuf_paltform.Msg_buf.DaHua_Vehicle_Msg.ServerIp); //IP��Ҫ��̬����

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
				//�ڷ��͵�ʱ����Ҫ�����׽��ֻ��������Ƿ������ݣ���Ҫ����ջ�����
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
				int h= select(sockfd +1, &rset, NULL, NULL, &tv); //������Ϣ�������֮��ȴ����շ������Ļظ���Ϣ�����2s��û���յ��ظ���Ϣ����������ִ��

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
	//��ȡʱ������
	char lt[48];
	memset(lt,0,48);
	long  ltime = time(NULL);
	sprintf(lt,"----------DHMSG0000%x",(unsigned int)ltime); //�̶�Ϊ27���ַ�

	//bitcomת����Э��
	TRACE_LOG_PLATFROM_INTERFACE("Bitcom info platecolure=%d��platetype=%d��direction=%d��vehicletype=%d��vehiclecolour=%d", \
								db_traffic_record->plate_color, db_traffic_record->plate_type, db_traffic_record->direction, db_traffic_record->vehicle_type, db_traffic_record->color);
	bitcom_to_dahua_protocol(db_traffic_record);
	TRACE_LOG_PLATFROM_INTERFACE("Dahua  info platecolure=%d��platetype=%d��direction=%d��vehicletype=%d��vehiclecolour=%d", \
								db_traffic_record->plate_color, db_traffic_record->plate_type, db_traffic_record->direction, db_traffic_record->vehicle_type, db_traffic_record->color);

	if(msgbuf_paltform.pf_vtype == DaHua_V)
	{
		//***********������Ϣ(��Ϣ)***********//

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

		root["Longitude"] = 0.0;						            //ץ�ĵ㾭��(��֧��)
		root["Latitude"] = 0.0;							            //ץ�ĵ�γ��(��֧��)
		root["VehicleInfoState"] = 0;                               //������Ϣ״̬��0:ʵʱ���� 1:��ʷ���� int
		root["IsPicUrl"] = 0;                   			        //ͼƬ����,0:����Ϣ���а���ͼƬ��1:ͨ��ͼƬ��ַѰ��  int
		root["LaneIndex"] = db_traffic_record->lane_num;            //������ int
		root["PlateInfo"] = PlateNum;                               //���ƺ�  �ַ���
		root["PlateColor"] = PlateColor;                            //������ɫ  �ַ���
		root["PlateType"] = db_traffic_record->plate_type;          //��������  int
		root["PassTime"] = db_traffic_record->time;                 //����ʱ�� ��ʽYYYY-MM-DD HH(24):MI:S  �ַ���
		root["VehicleSpeed"] = (double)db_traffic_record->speed;    //�����ٶ� double
		root["LaneMiniSpeed"] = 0.0;                                //���������٣��޴˹�����0.0  double
		root["LaneMaxSpeed"] = 0.0;                                 //���������٣��޴˹�����0.0  double
		root["VehicleType"] = db_traffic_record->vehicle_type;      //�������� int
		root["VehicleColor"] = db_traffic_record->color;            //������ɫ int
		root["VehicleLength"] = 0;                                  //�������ȣ���֧����0   int
		root["VehicleState"] = 1;								    //�г�״̬  int
		root["PicCount"] = 1;                                       //ͼƬ���� int
		root["PicType"] = "";                                     //�����ô��ֵ
		root["PlatePicUrl"] = "";                                 //������Ƭurl
		root["VehiclePic1Url"] = "";                              //������Ƭurl
		root["VehiclePic2Url"] = "";
		root["VehiclePic3Url"] = "";
		root["CombinedPicUrl"] = "";  			                //ͼƬ��ַ
		root["AlarmAction"] = 1;              			            //Υ������

		std::string  str_vehicalinfo = fast_writer.write(root);

		std::cout << "str_vehicalinfo json:" <<str_vehicalinfo << std::endl;


		//***********������Ϣ��(ͼƬ)***********//
		char str_vehicalpic_head[72];
		memset(str_vehicalpic_head,0,72);
		sprintf(str_vehicalpic_head,"%s\r\n"\
							   "Content-Type: image/jpeg\r\n",lt);

		int len_str_vehicalpic_head = strlen(str_vehicalpic_head);

		char str_vehicalpic_tail[48];
		memset(str_vehicalpic_tail,0,48);
		sprintf(str_vehicalpic_tail,"%s--\r\n",lt);

		int len_str_vehicalpic_tail = strlen(str_vehicalpic_tail);

		//***********������Ϣͷ***********//
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

		//***********ƴ��***********//
		strcat(str_vehical,str_vehicalinfo.c_str()); //��Ϣͷ + ������Ϣ

		strcat(str_vehical,str_vehicalpic_head);     //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ

		int len = strlen(str_vehical);               //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ���ַ�������

		memcpy(str_vehical + len,pic_info->buf,pic_info->size);  //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ + ͼƬ

		memcpy(str_vehical + len + pic_info->size,str_vehicalpic_tail,len_str_vehicalpic_tail); //��Ϣͷ + ������Ϣ + ����ͼƬͷ��Ϣ + ͼƬ + ��Ϣβ  �ܴ�С


		//bitcom���ʹ����ݸ�ֵ
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
 * ������: send_vm_records_motor_vehicle_to_NetPose
 * ��  ��: ���͹�����Ϣ����������ƽ̨
 * ��  ��: db_traffic_records�������Ľ��
 *         pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ:
*******************************************************************************/
int send_vm_records_motor_vehicle_to_NetPose_moter_vehicle(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{



	fd_set rset;
	NetPoseVehicleProtocol VehicleProtoclo;
	memset(&VehicleProtoclo,0,sizeof(NetPoseVehicleProtocol));

	//��ȡʱ��
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

	//----------------Э��ת��----------------//

	protocol_convert_NetPose_moter_vehicle(db_traffic_record,&VehicleProtoclo);


	//----------------��Ϣ��(������Ϣ)----------------//
	/********************
	�����Ĺ�����Ϣ��:
	analyseResult
		<vehicaleInfo>:
			vehicleType   :      ��������
			vehicleColor  :		 ������ɫ
			vehicleLength :		 ������������
			vehicleNoType :		 ��������
			vehicleNo     :		 ���ƺ���
			vehicleNoColor:      ������ɫ
		<driveInfo>:
			time          :      ����ʱ��
			location	  :      �����ص�
			speed         :      ��ʻ�ٶ�
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

	//----------------��Ϣ��(����ͼƬ����Ϣ)----------------//

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

	//-------------------�������Լ���Ϣͷ-------------------//

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


	//-------------------ƴ��-------------------//
	strcat(str_vehicle,str_vehicleinfo);
	strcat(str_vehicle,str_vehiclepic_head);

	int len = strlen(str_vehicle);

	memcpy(str_vehicle + len,pic_info->buf,pic_info->size);

	memcpy(str_vehicle +len + pic_info->size,str_vehiclepic_tail,len_str_vehicalpic_tail);

	DEBUG("http to netpose %s\n", str_vehicle);

	//----------------��������----------------//

	//�����׽��ֲ����ӷ�����
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

	//��ȡʱ��
	char timenow[50];
	memset(timenow,0,50);
	getime(timenow);

	long time_tmp = time(NULL);

	//----------------Э��ת��----------------//

	//protocol_convert_NetPose_moter_vehicle(db_traffic_record,&VehicleProtoclo);


	//----------------��Ϣ��(������Ϣ)----------------//
	/********************
	�����Ĺ�����Ϣ��:
	analyseResult
		<vehicaleInfo>:
			vehicleType   :      ��������
			vehicleColor  :		 ������ɫ
			vehicleLength :		 ������������
			vehicleNoType :		 ��������
			vehicleNo     :		 ���ƺ���
			vehicleNoColor:      ������ɫ
		<driveInfo>:
			time          :      ����ʱ��
			location	  :      �����ص�
			speed         :      ��ʻ�ٶ�
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

	//----------------��Ϣ��(����ͼƬ����Ϣ)----------------//

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

	//-------------------�������Լ���Ϣͷ-------------------//

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


	//-------------------ƴ��-------------------//
	strcat(str_vehicle,str_vehicleinfo);
	strcat(str_vehicle,str_vehiclepic_head);

	int len = strlen(str_vehicle);

	memcpy(str_vehicle + len,pic_info->buf,pic_info->size);

	memcpy(str_vehicle +len + pic_info->size,str_vehiclepic_tail,len_str_vehicalpic_tail);




	//----------------��������----------------//

	//�����׽��ֲ����ӷ�����
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
 * ������: send_vm_records_others_to_NetPose
 * ��  ��: ���͹�����Ϣ����������ƽ̨
 * ��  ��: db_traffic_records�������Ľ��
 *         pic_info��ͼƬ�ļ���Ϣ
 * ����ֵ:
*******************************************************************************/
int send_vm_records_motor_vehicle_to_NetPose_others(DB_TrafficRecord *db_traffic_record,EP_PicInfo *pic_info)
{

	printf("In send_vm_records_others_to_NetPose : IP: %s Port: %d\n",msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.ServerIp,msgbuf_paltform.Msg_buf.NetPosa_Face_Msg.MsgPort);

	fd_set rset;
	NetPoseOthersProtocol OthersProtoclo;
	memset(&OthersProtoclo,0,sizeof(NetPoseOthersProtocol));

	//��ȡʱ��
	char timenow[50];
	memset(timenow,0,50);
	getime(timenow);

	long time_tmp = time(NULL);

	//----------------Э��ת��----------------//

	protocol_convert_NetPose_others(db_traffic_record,&OthersProtoclo);

	//----------------��Ϣ��(������Ϣ)----------------//
	/********************
	�����Ĺ�����Ϣ��:
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

	//----------------��Ϣ��(����ͼƬ����Ϣ)----------------//

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

	//-------------------�������Լ���Ϣͷ-------------------//

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


	//-------------------ƴ��-------------------//
	strcat(str_face,str_faceinfo);
	strcat(str_face,str_facepic_head);

	int len = strlen(str_face);

	memcpy(str_face + len,pic_info->buf,pic_info->size);

	memcpy(str_face +len + pic_info->size,str_facepic_tail,len_str_facepic_tail);

	TRACE_LOG_SYSTEM("Netpose people records : %s", str_face);


	//----------------��������----------------//

	printf("Start send face info !\n");

	//�����׽��ֲ����ӷ�����
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

    printf("ʱ��: %d:%d:%d.%ld \n", cur_hour, cur_min, cur_sec,ust);
    printf("����: %d-%d-%d \n", cur_year, cur_mouth, cur_day);
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
		"ک��", "�µ�", "����", "����", "���", "���ǵ�",
		"����", "ѩ����", "ѩ����", "�ۺ�", "һ��", "����",
		"����", "����", "����", "�ִ�", "����", "�׿���˹",
		"���Դ�", "��ɣ", "����", "��ŵ", "˹�´�", "��������",
		"��ľ", "����", "����", "�л�", "����", "����",
		"����", "˹��³", "����", "�ֶ���", "��־", "������",
		"�ʹ�", "����", "��ʮ��", "����", "��", "��������",
		"����", "��������", "ŷ��" };
	const logo_t netpose[] = {
		{ -1, "�޳���" },{ 0, "����" }, { 1, "����" },
		{ 2, "����" }, { 3, "�ִ�" }, { 4, "һ��" },
		{ 5, "����" }, { 6, "���" }, { 7, "����" },
		{ 8, "����" }, { 9, "��ɣ" }, { 10, "ѩ����" },
		{ 11, "�µ�" }, { 12, "ѩ����" }, { 13, "����" },
		{ 14, "����" }, { 15, "����" }, { 16, "����" },
		{ 17, "��������" }, { 18, "��" }, { 19, "���Դ�" },
		{ 20, "����" }, { 21, "����" }, { 22, "�������" },
		{ 23, "���ǵ�" }, { 24, "��ͨ" }, { 25, "����" },
		{ 26, "����" }, { 27, "��ľ" }, { 28, "��ͨ" },
		{ 29, "����" }, { 30, "����" }, { 31, "����" },
		{ 32, "����" }, { 33, "����(��)" }, { 34, "���ǵ�(��)" },
		{ 35, "��ľ(��ĸ)" }, { 36, "����" }, { 37, "�л�" },
		{ 38, "˹�´�" }, { 39, "����" }, { 40, "�Ա�" },
		{ 41, "��̩" }, { 42, "����" }, { 43, "����" },
		{ 44, "Ӣ�����" }, { 45, "�׿���˹" }, { 46, "˹��³" },
		{ 47, "��ŵ" }, { 48, "�ֶ���" }, { 49, "����" },
		{ 50, "����" }, { 51, "����" }
	};

	const char *str;
	const logo_t *p, *end;

	if (((unsigned int)idx >= numberof(bitcom)) || (idx < 0)) {
		str = "�޳���";
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


	switch(db_traffic_record->vehicle_type)  //��������  ��������������������
	{
		case 0:VehicleProtocol->vehicleType = 999;break; //����
		case 1:VehicleProtocol->vehicleType = 12;break; //С�ͳ�
		case 2:VehicleProtocol->vehicleType = 5;break; //���ͳ�
		case 3:VehicleProtocol->vehicleType = 0;break; //���ͳ�
		default:VehicleProtocol->vehicleType = 999;      //����
	}

	DEBUG("vehicle type: %d, netpose vehicle type: %d",
		  db_traffic_record->vehicle_type, VehicleProtocol->vehicleType);

	vm_vehicle_color_to_netpose(VehicleProtocol->vehicleColor,
								sizeof(VehicleProtocol->vehicleColor),
								db_traffic_record->color);

	DEBUG("vehicle color: %d, netpose vehicle color: %s",
		  db_traffic_record->color, VehicleProtocol->vehicleColor);

	switch(db_traffic_record->plate_type)        //��������   ��������������������
	{
		case 1:VehicleProtocol->vehicleNoType = 1;break;    //������������
		case 2:VehicleProtocol->vehicleNoType = 2;break;    //С����������
		case 3:VehicleProtocol->vehicleNoType = 3;break;	//ʹ����������
		case 4:VehicleProtocol->vehicleNoType = 4;break;	//�����������
		default:VehicleProtocol->vehicleNoType = 99;         //��������
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
 * ������:process_entrance_control
 * ��  ��: �������ڵ�բ������Ϣ
 * ��  ��:
 * ����ֵ: �ɹ�������0����������-1
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


