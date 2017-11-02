#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "logger/log.h"
#include "Msg_Def.h"
#include "xmlCreater.h"
#include "xmlParser.h"
#include "commonfuncs.h"
#include "tvpuniview_records.h"
#include "tvpuniview_protocol.h"

extern PLATFORM_SET msgbuf_paltform;

//protocol translate
int tvp_bitcomstr_to_univiewstr(DB_ViolationRecordsMotorVehicle *as_input, str_tvpuniview_xml *os_output)
{
	sprintf(os_output->szDevCode, "%s", msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.DeviceId);
	sprintf(os_output->szRecordID, "%d", as_input->ID);
	DEBUG("______as_input->dev_id=%s______", msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.DeviceId);
	sprintf(os_output->szTollgateID, "%s", msgbuf_paltform.Msg_buf.Uniview_Illegally_Park_Msg.DeviceId);
	sprintf(os_output->szPassTime, "%s", as_input->time);
	sprintf(os_output->szPlaceCode, "%s", as_input->point_id);
	DEBUG("______as_input->lane_num=%d______", as_input->lane_num);
	if(as_input->lane_num <= 0)
	{
		os_output->ulLaneID = 1;
	}
	else
	{
		os_output->ulLaneID = as_input->lane_num;
	}
	sprintf(os_output->szCarPlate, "%s", as_input->plate_num);
	os_output->ulPlateColor = as_input->plate_color;
	if(as_input->plate_color == 4) os_output->ulPlateColor = 5;
	if(as_input->plate_color == 5) os_output->ulPlateColor = 4;
	os_output->ulPlateNum = 1;
	os_output->ulVehSpeed = 0; //无测速
	os_output->ulLimitedSpeed = 0;
	os_output->ulIdentifyStatus = 0;
	os_output->ulIdentifyTime = 0;
    if(as_input->plate_type <= 24)
        sprintf(os_output->szPlateType, "%02d", as_input->plate_type);
    else
        sprintf(os_output->szPlateType, "%02d", 99);
	os_output->szVehBodyColor = as_input->color+65;
	if(as_input->color >= 11) os_output->szVehBodyColor = 'Z';
	os_output->ulVehColorDepth = 0;
	os_output->ulVehDirection = 1;
	os_output->ulVehLength = 0;
	os_output->ulVehtype = as_input->vehicle_type;
	if(as_input->vehicle_type >= 4) os_output->ulVehtype = 4;
	os_output->ulLaneType = 1;    //TODO
	sprintf(os_output->szVehBrand, "%d", 99); //TODO MAP
	os_output->ulDressColor = 0;
	os_output->ulTransgressType = as_input->violation_type;
	os_output->ulRedLightTime = as_input->red_lamp_keep_time;
	os_output->ulPicNum = 3;
	sprintf(os_output->szEquipmentCode, "%s", as_input->point_id);
	os_output->ulApplicationType = 0;
	os_output->ulGlobalComposeFlag = 1;  //TODO self-adaption 1

	return 0;
}


#define SNPRINTF_XML(fmt, values) \
	li_ret_lens = snprintf((char *)oc_data+li_current_lens, (MAX_XML_LENS_1M-li_current_lens), (const char *)(fmt), (values)); \
	if((MAX_XML_LENS_1M-li_current_lens) <= li_ret_lens) \
	{ \
		return -1; \
	} \
	li_current_lens += li_ret_lens


//tvpuniview_xmlcreate 
int tvpuniview_xmlcreate(str_tvpuniview_protocol *as_tvpuniview_protocol, char *oc_data)
{
	int li_current_lens = 0;
	int li_ret_lens = 0;
	char lc_time[64] = {0};
	char lc_tmp[64] = {0};
	int  li_i = 0;
	static int lsi_recordid = 1;
	str_tvpuniview_xml *lstr_tvpuniview_xml = &as_tvpuniview_protocol->data.xml;


	/*获取系统时间，按YYYYMMDDHHMMSS格式，时间按24小时制*/
	struct tm t = {0};
	sscanf(lstr_tvpuniview_xml->szPassTime, "%04d-%02d-%02d %02d:%02d:%02d", &(t.tm_year), &(t.tm_mon), &(t.tm_mday), &(t.tm_hour), &(t.tm_min), &(t.tm_sec));
	snprintf(lc_time, sizeof(lc_time), "%d%02d%02d%02d%02d%02d", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour,t.tm_min,t.tm_sec);

	SNPRINTF_XML("<?xml version=\"1.0\" ?>%s\r\n", "");
	SNPRINTF_XML("<Vehicle>\r\n%s", "");
	SNPRINTF_XML("<CamID>%s</CamID>\r\n", "987654321");    //TODO
	SNPRINTF_XML("<RecordID>%d</RecordID>\r\n", lsi_recordid++); //TODO 可能断电或者重启之后保存
	SNPRINTF_XML("<TollgateID>%s</TollgateID>\r\n", lstr_tvpuniview_xml->szTollgateID);
	SNPRINTF_XML("<PassTime>%s</PassTime>\r\n", lc_time);
	SNPRINTF_XML("<LaneID>%d</LaneID>\r\n", lstr_tvpuniview_xml->ulLaneID);
	convert_enc("GBK", "UTF-8", lstr_tvpuniview_xml->szCarPlate, 32, lc_tmp, 64);
	SNPRINTF_XML("<CarPlate>%s</CarPlate>\r\n", lc_tmp);
	SNPRINTF_XML("<PlateColor>%d</PlateColor>\r\n", lstr_tvpuniview_xml->ulPlateColor);
	SNPRINTF_XML("<PlateNumber>%d</PlateNumber>\r\n", lstr_tvpuniview_xml->ulPlateNum);
	SNPRINTF_XML("<VehicleSpeed>%d</VehicleSpeed>\r\n", lstr_tvpuniview_xml->ulVehSpeed);
	SNPRINTF_XML("<LimitedSpeed>%d</LimitedSpeed>\r\n", lstr_tvpuniview_xml->ulLimitedSpeed);
	SNPRINTF_XML("<IdentifyStatus>%d</IdentifyStatus>\r\n", lstr_tvpuniview_xml->ulIdentifyStatus);
	SNPRINTF_XML("<IdentifyTime>%d</IdentifyTime>\r\n", lstr_tvpuniview_xml->ulIdentifyTime);
	SNPRINTF_XML("<PlateType>%s</PlateType>\r\n", lstr_tvpuniview_xml->szPlateType);
	SNPRINTF_XML("<VehicleColor>%c</VehicleColor>\r\n", lstr_tvpuniview_xml->szVehBodyColor);
	SNPRINTF_XML("<VehicleColorDept>%d</VehicleColorDept>\r\n", lstr_tvpuniview_xml->ulVehColorDepth);
	SNPRINTF_XML("<Direction>%d</Direction>\r\n", lstr_tvpuniview_xml->ulVehDirection);
	SNPRINTF_XML("<VehicleLength>%d</VehicleLength>\r\n", lstr_tvpuniview_xml->ulVehLength);
	SNPRINTF_XML("<VehicleType>%d</VehicleType>\r\n", lstr_tvpuniview_xml->ulVehtype);
	SNPRINTF_XML("<LaneType>%d</LaneType>\r\n", lstr_tvpuniview_xml->ulLaneType);
	SNPRINTF_XML("<VehicleBrand>%s</VehicleBrand>\r\n", lstr_tvpuniview_xml->szVehBrand);
	SNPRINTF_XML("<DressColor>%d</DressColor>\r\n", lstr_tvpuniview_xml->ulDressColor);
	SNPRINTF_XML("<DriveStatus>%d</DriveStatus>\r\n", lstr_tvpuniview_xml->ulTransgressType);
	SNPRINTF_XML("<RedLightTime>%d</RedLightTime>\r\n", lstr_tvpuniview_xml->ulRedLightTime);
	SNPRINTF_XML("<ApplicationType>%d</ApplicationType>\r\n", lstr_tvpuniview_xml->ulApplicationType);
	SNPRINTF_XML("<GlobalComposeFlag>%d</GlobalComposeFlag>\r\n", lstr_tvpuniview_xml->ulGlobalComposeFlag);

	/*图片信息*/
	SNPRINTF_XML("<PicNumber>%d</PicNumber>\r\n", lstr_tvpuniview_xml->ulPicNum);

	/*车牌图片*/
	for(li_i=0; li_i<lstr_tvpuniview_xml->ulPicNum; li_i++)
	{
		SNPRINTF_XML("<Image>%s\r\n", "");
		SNPRINTF_XML("<ImageIndex>%d</ImageIndex>\r\n", li_i);
		SNPRINTF_XML("<ImageURL>%s</ImageURL>\r\n", "");
		SNPRINTF_XML("<ImageType>%d</ImageType>\r\n", 1);
		SNPRINTF_XML("</Image>%s\r\n", "");
	}

	/*一些为空的信息*/
#if 1
	SNPRINTF_XML("<EquipmentType/>%s\r\n", "");
	SNPRINTF_XML("<PanoramaFlag/>%s\r\n", "");
	SNPRINTF_XML("<TollgateName/>%s\r\n", "");
	SNPRINTF_XML("<PlaceCode/>%s\r\n", "");
	SNPRINTF_XML("<PlaceName/>%s\r\n", "");
	SNPRINTF_XML("<DirectionName/>%s\r\n", "");
	SNPRINTF_XML("<PlateConfidence/>%s\r\n", "");
	SNPRINTF_XML("<PlateCoincide/>%s\r\n", "");
	SNPRINTF_XML("<RearVehiclePlateID/>%s\r\n", "");
	SNPRINTF_XML("<RearPlateConfidence/>%s\r\n", "");
	SNPRINTF_XML("<RearPlateColor/>%s\r\n", "");
	SNPRINTF_XML("<MarkedSpeed/>%s\r\n", "");
	SNPRINTF_XML("<VehicleBody/>%s\r\n", "");
	SNPRINTF_XML("<ImageURL2/>%s\r\n", "");
	SNPRINTF_XML("<ImageURL3/>%s\r\n", "");
	SNPRINTF_XML("<ImageURL4/>%s\r\n", "");
	SNPRINTF_XML("<DealTag/>%s\r\n", "");
#endif
	SNPRINTF_XML("</Vehicle>%s\r\n", "");

    return li_current_lens;
}


//tvpuniview protocol encode
int tvpuniview_encode(str_tvpuniview_protocol *as_tvpuniview_protocol, char *oc_data)
{
	int  li_lens = 0;
	int  li_i = 0;
	int  li_tmp = 0;
	char lc_xml_data[MAX_XML_LENS_1M] = {0};
	
	//head
	as_tvpuniview_protocol->frame_head = htonl(0x77aa77aa);
	memcpy(oc_data+li_lens, &as_tvpuniview_protocol->frame_head, 4);
	li_lens += 4;

	//frame lens
	as_tvpuniview_protocol->frame_lens = htonl(8) + htonl(as_tvpuniview_protocol->frame_lens);
	memcpy(oc_data+li_lens, &as_tvpuniview_protocol->frame_lens, 4);
	li_lens += 4;

	//protocol version
	as_tvpuniview_protocol->version = htonl(2);
	memcpy(oc_data+li_lens, &as_tvpuniview_protocol->version, 4);
	li_lens += 4;

	//protocol cmd
	as_tvpuniview_protocol->frame_cmd = htonl(as_tvpuniview_protocol->frame_cmd);
	memcpy(oc_data+li_lens, &as_tvpuniview_protocol->frame_cmd, 4);
	li_lens += 4;
	
	//protocol cmd
	switch(htonl(as_tvpuniview_protocol->frame_cmd))
	{
    	case HEART_BEATE_CMD:
			memcpy(oc_data+li_lens, &as_tvpuniview_protocol->heart_beate.dev_code, 32);
			li_lens += 32;
		break;
		
    	case RT_RECORDS_CMD:
			tvp_bitcomstr_to_univiewstr(&gstr_tvpuniview_records.field, &as_tvpuniview_protocol->data.xml);
			as_tvpuniview_protocol->data.xml.ulPicNum = as_tvpuniview_protocol->data.pic_num;
			as_tvpuniview_protocol->data.xml_lens = tvpuniview_xmlcreate(as_tvpuniview_protocol, lc_xml_data);
			if(as_tvpuniview_protocol->data.xml_lens == -1)
			{
				DEBUG("Uniview xml create failed!!");
			}

			DEBUG("Uniview xml data: %s", lc_xml_data);
			
			//xml length
			li_tmp = htonl(as_tvpuniview_protocol->data.xml_lens);
			memcpy(oc_data+li_lens, &li_tmp, 4);
			li_lens += 4;

			//xml data
			DEBUG("Uniview xml data_lens: %d, li_lens=%d", as_tvpuniview_protocol->data.xml_lens, li_lens);
			memcpy(oc_data+li_lens, lc_xml_data, as_tvpuniview_protocol->data.xml_lens);
			li_lens += as_tvpuniview_protocol->data.xml_lens;

			//pic num
			li_tmp = htonl(as_tvpuniview_protocol->data.pic_num);
			memcpy(oc_data+li_lens, &li_tmp, 4);
			li_lens += 4;

			//picture info
			for(li_i=0; li_i<as_tvpuniview_protocol->data.pic_num; li_i++)
			{
				DEBUG("as_tvpuniview_protocol->data.pic_lens[%d]=%d", li_i, gstr_tvpuniview_records.picture_size[li_i]);
				li_tmp = htonl(gstr_tvpuniview_records.picture_size[li_i]);
				memcpy(oc_data+li_lens, &li_tmp, 4);
				li_lens += 4;
				memcpy(oc_data+li_lens, gstr_tvpuniview_records.picture[li_i], gstr_tvpuniview_records.picture_size[li_i]);
				li_lens += gstr_tvpuniview_records.picture_size[li_i];
			}

			//frame lens
			as_tvpuniview_protocol->frame_lens = 4 + 4 + (4 + as_tvpuniview_protocol->data.xml_lens)+4+4*as_tvpuniview_protocol->data.pic_num;
			for(li_i=0; li_i<as_tvpuniview_protocol->data.pic_num; li_i++)
			{
				as_tvpuniview_protocol->frame_lens += gstr_tvpuniview_records.picture_size[li_i];
			}
			as_tvpuniview_protocol->frame_lens = htonl(as_tvpuniview_protocol->frame_lens);
			memcpy(oc_data+4, &as_tvpuniview_protocol->frame_lens, 4);

		break;		
	}
	
	//protocol tail
	as_tvpuniview_protocol->frame_tail = htonl(0x77ab77ab);
	memcpy(oc_data+li_lens, &as_tvpuniview_protocol->frame_tail, 4);
	li_lens += 4;

	return li_lens;
}


 
