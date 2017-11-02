/*
 * xmlParser.cpp
 *
 *  Created on: 2013-5-30
 *      Author: shanhw
 */

#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "commonfuncs.h"
#include "xmlParser.h"
#include "global.h"
#include "stdio.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>


static int parse_VD_point(VD_point *pPoint, xmlNodePtr node);
static int parse_VD_line(VD_line *pLine, xmlNodePtr node);
static int parse_TYPE_OSS_ALIYUN_CONFIG_PARAM(OSS_ALIYUN_PARAM *as_oss_aliyun_param,
											  xmlNodePtr node);
static int parse_DeviceConfig(DeviceConfig *pDeviceConfig, xmlNodePtr node);
static int parse_Camera_arithm(Camera_arithm *pCamera_arithm, xmlNodePtr node);
static int parse_Traffic_event_det(Traffic_event_det *pTraffic_envent_det,
								   xmlNodePtr node);
static int parse_Overlay_violate(Overlay_violate *pOverlay_violate, xmlNodePtr node);

/*
 * 打开xml文件
 */
int open_xml_doc(const char *xml_file, xmlDocPtr* p_doc)
{
	xmlDocPtr doc;

	doc = xmlReadFile(xml_file, "UTF-8", XML_PARSE_RECOVER); //解析文件

	//检查解析文档是否成功，如果不成功，libxml将指一个注册的错误并停止。
	//一个常见错误是不适当的编码。XML标准文档除了用UTF-8或UTF-16外还可用其它编码保存。
	//如果文档是这样，libxml将自动地为你转换到UTF-8。更多关于XML编码信息包含在XML标准中.

	if (NULL == doc)
	{
		log_error("XML", "XML document parsed error.\n");
		return -1;
	}

	*p_doc = doc;
	return 0;
}

/*
 *  关闭xml文件
 */
static void close_xml_doc(xmlDocPtr doc)
{
	xmlFreeDoc(doc);
	return;
}

/**
 * 获取xml的root节点
 *
 */
int get_root_node(xmlDocPtr doc, xmlNodePtr *proot)
{
	xmlNodePtr curNode; //定义结点指针(你需要它为了在各个结点间移动)

	curNode = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (NULL == curNode)
	{
		log_error("XML", "empty document\n");
		return -1;
	}

	/*在这个例子中，我们需要确认文档是正确的类型。“root”是在这个示例中使用文档的根类型。*/
	#if 0
	if (xmlStrcmp(curNode->name, (const xmlChar *) "epcs_protocol"))
	{
		log_error("XML", "document of the wrong type, root node != epcs_protocol\n");
		return -1;
	}
	#endif
	
	*proot = curNode;
	return 0;
}

/**
 * 解析VD_date结构体
 * 参数：
 * 	pDate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_date(VD_date *pDate, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		printf("name: %s\n", curNode->name);
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "year")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDate->year = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "month")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDate->month = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "day")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDate->day = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VD_daysegment结构体
 * 参数：
 * 	pDaysegment[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_daysegment(VD_daysegment *pDaysegment, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "begin")))
		{
			parse_VD_date(&pDaysegment->begin, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "end")))
		{
			parse_VD_date(&pDaysegment->end, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VD_Time结构体
 * 参数：
 * 	pTime[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
//int parse_VD_time(VD_Time *pTime, xmlNodePtr node)
int parse_VD_time_restriction(VD_Time_restriction *pTime, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "hour")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime->hour = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "minute")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime->minute = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "second")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime->second = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}



/**
 * 解析VD_timesegment结构体
 * 参数：
 * 	pTimesegment[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_timesegment(VD_Timesegment *pTimesegment, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "begin")))
		{
			parse_VD_time_restriction(&pTimesegment->begin, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "end")))
		{
			parse_VD_time_restriction(&pTimesegment->end, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VD_rect结构体
 * 参数：
 * 	pRect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_rect(VD_rect *pRect, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "top")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRect->top = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "bottom")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRect->bottom = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "left")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRect->left = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "right")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRect->right = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

#if 0

/**
 * 解析VD_polygon结构体
 * 参数：
 * 	pRect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_polygon(VD_polygon *pPolygon, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "point")))
		{
			parse_VD_point(&pPolygon->point, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
			parse_VD_line(&pPolygon->line, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}
#endif
#if 1
int parse_VD_polygon(VD_polygon *pPolygon, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;
	printf("curNode->name is %s\n",curNode->name);

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPolygon->num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rect")))
		{
			parse_VD_rect(&pPolygon->rect, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "point")))
		{

//			parse_VD_point(&pPolygon->point, curNode);

			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;
			printf("%s\n",attrPtr->name);

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_point(&pPolygon->point[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}

		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
//			parse_VD_line(&pPolygon->line, curNode);

			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_line(&pPolygon->line[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}
#endif

int parse_VD_rightLineInfo(VD_lineSegment *prightLineInfo, xmlNodePtr node)
{
	xmlNodePtr curNode;
	//xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "startPoint")))
		{
			parse_VD_point(&prightLineInfo->startPoint, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "endPoint")))
		{
			parse_VD_point(&prightLineInfo->endPoint, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
			parse_VD_line(&prightLineInfo->line, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}




/**
 * 解析Signal_attribute结构体
 * 参数：
 * 	pSignal_attribute[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Signal_attribute(Signal_attribute *pSignal_attribute, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "color")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_attribute->color = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_attribute->type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_attribute->direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "group")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_attribute->group = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rect")))
		{
			parse_VD_rect(&pSignal_attribute->rect, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Signal_detect_video结构体
 * 参数：
 * 	pSignal_detect_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Signal_detect_video(Signal_detect_video *pSignal_detect_video,
                              xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "signal_num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_detect_video->signal_num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detect_area")))
		{
			parse_VD_rect(&pSignal_detect_video->detect_area, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "signals")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				parse_Signal_attribute(&pSignal_detect_video->signals[atoi(
				                           (const char*) szAttr) - 1], curNode);
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析Plate_size结构体
 * 参数：
 * 	pPlate_size[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Plate_size(Plate_size *pPlate_size, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "max_width")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPlate_size->max_width = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "max_height")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPlate_size->max_height = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "min_width")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPlate_size->min_width = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "min_height")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPlate_size->min_height = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Plate结构体
 * 参数：
 * 	pPlate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Plate(Plate *pPlate, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "str_default_prov")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey), (char*) pPlate->str_default_prov, 4);

			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "plate_size")))
		{
			parse_Plate_size(&pPlate->plate_size, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VehicleDet_video结构体
 * 参数：
 * 	pVehicleDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VehicleDet_video(VehicleDet_video *pVehicleDet_video, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_vehicle_out")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVehicleDet_video->is_vehicle_out = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "vehiCapture_level")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVehicleDet_video->vehiCapture_level = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "hspeed_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVehicleDet_video->hspeed_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VehicleDet_loop结构体
 * 参数：
 * 	pVehicleDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VehicleDet_loop(VehicleDet_loop *pVehicleDet_loop, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pVehicleDet_loop ->loops[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "plate_holdTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVehicleDet_loop->plate_holdTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "signal_holdTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVehicleDet_loop->signal_holdTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析_VEHICLE_UNION结构体
 * 参数：
 * 	p_VEHICLE_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse__VEHICLE_UNION(union _VEHICLE_UNION *p_VEHICLE_UNION, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "det_video")))
		{
			parse_VehicleDet_video(&p_VEHICLE_UNION->det_video, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "det_loop")))
		{
			parse_VehicleDet_loop(&p_VEHICLE_UNION->det_loop, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析RunredDet_video结构体
 * 参数：
 * 	pRunredDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_RunredDet_video(RunredDet_video *pRunredDet_video, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_run_red")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRunredDet_video->is_run_red = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_run_yellow")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRunredDet_video->is_run_yellow = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "one_run_red")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pRunredDet_video ->one_run_red[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析RunredDet_loop结构体
 * 参数：
 * 	pRunredDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_RunredDet_loop(RunredDet_loop *pRunredDet_loop, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops_first")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pRunredDet_loop ->loops_first[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops_second")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pRunredDet_loop ->loops_second[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析RUNREDDET_UNION结构体
 * 参数：
 * 	pRUNREDDET_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_RUNREDDET_UNION(union RUNREDDET_UNION *pRUNREDDET_UNION, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "video")))
		{
			parse_RunredDet_video(&pRUNREDDET_UNION->video, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loop")))
		{
			parse_RunredDet_loop(&pRUNREDDET_UNION->loop, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析RunredDet结构体
 * 参数：
 * 	pRunredDet[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_RunredDet(RunredDet *pRunredDet, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRunredDet->mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "runRed_union")))
		{
			parse_RUNREDDET_UNION(&pRunredDet->runRed_union, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析SpeedDet_loop结构体
 * 参数：
 * 	pSpeedDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SpeedDet_loop(SpeedDet_loop *pSpeedDet_loop, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "use_3_loops")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet_loop ->use_3_loops = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops_first")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pSpeedDet_loop ->loops_first[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops_second")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pSpeedDet_loop ->loops_second[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "loops_third")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pSpeedDet_loop ->loops_third[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析SpeedDet_radio结构体
 * 参数：
 * 	pSpeedDet_radio[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SpeedDet_radio(SpeedDet_radio *pSpeedDet_radio, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "can_det_lane")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet_radio->can_det_lane = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "seri_port")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet_radio->seri_port = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "radio_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet_radio->radio_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析Inter_coeff结构体
 * 参数：
 * 	pInter_coeff[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Inter_coeff(Inter_coeff *pInter_coeff, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "internum")))
		{
			szKey = xmlNodeGetContent(curNode);
			pInter_coeff->internum = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "x")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pInter_coeff->x[atoi((const char*) szAttr) - 1] = atof(
				            (const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "y")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pInter_coeff->y[atoi((const char*) szAttr) - 1] = atof(
				            (const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pInter_coeff->h[atoi((const char*) szAttr) - 1] = atof(
				            (const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "m")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pInter_coeff->m[atoi((const char*) szAttr) - 1] = atof(
				            (const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析CaliInfo_point结构体
 * 参数：
 * 	pCaliInfo_point[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_CaliInfo_point(CaliInfo_point *pCaliInfo_point, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "x")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->x = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "y")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->y = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "x_distance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->x_distance = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "y_distance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->y_distance = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "xpixel_distance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->xpixel_distance = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ypixel_distance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->ypixel_distance = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rowline_slope")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->rowline_slope = atof((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "colline_slope")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaliInfo_point->colline_slope = atof((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析SpeedDet_video结构体
 * 参数：
 * 	pSpeedDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SpeedDet_video(SpeedDet_video *pSpeedDet_video, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h_function")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				parse_Inter_coeff(&pSpeedDet_video->h_function[atoi(
				                      (const char*) szAttr) - 1], curNode);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "v_function")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				parse_Inter_coeff(&pSpeedDet_video->v_function[atoi(
				                      (const char*) szAttr) - 1], curNode);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "caliinfo_point")))
		{
			int x = 0, y = 0;

			xmlAttrPtr attrPtr = curNode->properties;
			while (attrPtr != NULL)
			{
				if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "x"))
				{
					xmlChar* szAttr =
					    xmlGetProp(curNode, (const xmlChar *) "x");
					x = atoi((const char*) szAttr);
					xmlFree(szAttr);
				}
				else if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "y"))
				{
					xmlChar* szAttr =
					    xmlGetProp(curNode, (const xmlChar *) "y");
					y = atoi((const char*) szAttr);
					xmlFree(szAttr);
				}
				attrPtr = attrPtr->next;
			}

			parse_CaliInfo_point(&pSpeedDet_video->caliinfo_point[x][y],
			                     curNode);
		}

		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析SPEED_UNION结构体
 * 参数：
 * 	pSPEED_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SPEED_UNION(union SPEED_UNION *pSPEED_UNION, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "speed_video")))
		{
			parse_SpeedDet_video(&pSPEED_UNION->speed_video, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "speed_loop")))
		{
			parse_SpeedDet_loop(&pSPEED_UNION->speed_loop, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "speed_radio")))
		{
			parse_SpeedDet_radio(&pSPEED_UNION->speed_radio, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析SpeedDet结构体
 * 参数：
 * 	pSpeedDet[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SpeedDet(SpeedDet *pSpeedDet, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet->mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_measure_speed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet->is_measure_speed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_overspeed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet->is_overspeed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "limitspeed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSpeedDet->limitspeed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "speed_union")))
		{
			parse_SPEED_UNION(&pSpeedDet->speed_union, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Time_restriction结构体
 * 参数：
 * 	pTime_restriction[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Time_restriction(Time_restriction *pTime_restriction, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "sigle_double_date")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime_restriction->sigle_double_date = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_date_restriction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime_restriction->is_date_restriction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "week_restriction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime_restriction->week_restriction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_time_restriction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTime_restriction->is_time_restriction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "date_segment")))
		{
			parse_VD_daysegment(&pTime_restriction->date_segment, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "time_segment")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				parse_VD_timesegment(&pTime_restriction->time_segment[atoi(
				                         (const char*) szAttr) - 1], curNode);
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析LimitTravel_one结构体
 * 参数：
 * 	pLimitTravel_one[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_LimitTravel_one(LimitTravel_one *pLimitTravel_one, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "lane")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->lane = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_time_restrict")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->is_time_restrict = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "time_restrict")))
		{
			parse_Time_restriction(&pLimitTravel_one->time_restrict, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_tailnumber")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->is_tailnumber = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "apointed_tailnumber")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pLimitTravel_one->apointed_tailnumber[atoi(
					        (const char*) szAttr) - 1] = atoi(
					                (const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "move_direct")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->move_direct = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "turning_direct")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->turning_direct = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vehicle_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimitTravel_one->vehicle_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Limit_travel_det结构体
 * 参数：
 * 	pLimit_travel_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Limit_travel_det(Limit_travel_det *pLimit_travel_det, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "restriction_num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLimit_travel_det->restriction_num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "restriction_info")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_LimitTravel_one(
					    &pLimit_travel_det->restriction_info[atoi(
					                (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析Event_det结构体
 * 参数：
 * 	pEvent_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Event_det(Event_det *pEvent_det, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_converse_drive")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_converse_drive = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_cover_line")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_cover_line = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "one_cover_line")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pEvent_det->one_cover_line[atoi((const char*) szAttr) - 1]
					    = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_over_safety_strip")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_over_safety_strip = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_illegal_park")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_illegal_park = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_cross_stop_line")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_cross_stop_line = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "one_cross_stop_line")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pEvent_det->one_cross_stop_line[atoi((const char*) szAttr)
					                                - 1] = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_illegal_lane_run")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_illegal_lane_run = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "one_lane_run")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pEvent_det->one_lane_run[atoi((const char*) szAttr) - 1]
					    = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_occupy_special")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_occupy_special = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_change_lane")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_change_lane = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "one_change_lane")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pEvent_det->one_change_lane[atoi((const char*) szAttr) - 1]
					    = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_occupy_nonmotor")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_occupy_nonmotor = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_forced_cross")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_forced_cross = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_left_wait_zone")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_left_wait_zone = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_park_outof_LTWA")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_park_outof_LTWA = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_limit_travel")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_limit_travel = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_notwayto_pedes")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->is_notwayto_pedes = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "illegal_park_time")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->illegal_park_time = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "cross_stop_line_time")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->cross_stop_line_time = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "event_alarm_enable")))
		{
			szKey = xmlNodeGetContent(curNode);
			pEvent_det->event_alarm_enable = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_order")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pEvent_det->event_order[atoi((const char*) szAttr) - 1]
					    = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}
	return 0;
}


/**
 * 解析Violate_description结构体
 * 参数：
 * 	pViolate_description[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Violate_description(Violate_description *pViolate_description,
                              xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_use")))
		{
			szKey = xmlNodeGetContent(curNode);
			pViolate_description->is_use = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exceed_peed")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->exceed_peed,
			            sizeof(pViolate_description->exceed_peed));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "run_red")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->run_red,
			            sizeof(pViolate_description->run_red));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "converse_drive")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->converse_drive,
			            sizeof(pViolate_description->converse_drive));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "illegal_lane_run")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->illegal_lane_run,
			            sizeof(pViolate_description->illegal_lane_run));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "cover_line")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->cover_line,
			            sizeof(pViolate_description->cover_line));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "change_lane")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->change_lane,
			            sizeof(pViolate_description->change_lane));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "cross_stop_line")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->cross_stop_line,
			            sizeof(pViolate_description->cross_stop_line));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illegal_park")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->illegal_park,
			            sizeof(pViolate_description->illegal_park));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "over_safety_strip")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->over_safety_strip,
			            sizeof(pViolate_description->over_safety_strip));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "occupy_special")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->occupy_special,
			            sizeof(pViolate_description->occupy_special));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "occupy_no_motor")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->occupy_no_motor,
			            sizeof(pViolate_description->occupy_no_motor));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "forced_cross")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->forced_cross,
			            sizeof(pViolate_description->forced_cross));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "limit_travel")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->limit_travel,
			            sizeof(pViolate_description->limit_travel));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "nowayto_pedes")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey),
			            (char*) pViolate_description->nowayto_pedes,
			            sizeof(pViolate_description->nowayto_pedes));
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析Type_info_color结构体
 * 参数：
 * 	pType_info_color[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Type_info_color(Type_info_color *pType_info_color, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "r")))
		{
			szKey = xmlNodeGetContent(curNode);
			pType_info_color->r = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "g")))
		{
			szKey = xmlNodeGetContent(curNode);
			pType_info_color->g = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "b")))
		{
			szKey = xmlNodeGetContent(curNode);
			pType_info_color->b = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}




int parse_Strobe(Strobe *pStrobe, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if (!xmlStrcmp(curNode->name, (const xmlChar *) "lane"))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pStrobe->lane[atoi((const char*) szAttr) - 1] = atoi(
					            (char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if (!xmlStrcmp(curNode->name, (const xmlChar *) "address"))
		{
			szKey = xmlNodeGetContent(curNode);
			pStrobe->address = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}
	return 0;
}








//////////////////////////////////// PD config //////////////////////////////////////////////

int parse_VD_CaptureRule(VD_CaptureRule *pCaptureRule, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "capturePicNum")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaptureRule->capturePicNum = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "minConfirmTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaptureRule->minConfirmTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "videoTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaptureRule->videoTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "captureTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pCaptureRule->captureTime[atoi((const char*) szAttr) - 1] = atoi(
								(char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "plateMatchRatio")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaptureRule->plateMatchRatio = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "captureMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pCaptureRule->captureMode[atoi((const char*) szAttr) - 1] = atoi(
								(char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ptzTraceMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCaptureRule->ptzTraceMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;

}


int parse_VD_ParkRegion(VD_ParkRegion *pParkRegion, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detectRegions")))
		{
			parse_VD_polygon(&pParkRegion->detectRegions, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "captureRule")))
		{
			parse_VD_CaptureRule(&pParkRegion->captureRule, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析VD_PTZinfo结构体
 * 参数：
 * 	pPTZinfo[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_VD_PTZinfo(VD_PTZinfo *pPTZinfo, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "PTZid")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPTZinfo->PTZid = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "focalDistance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPTZinfo->focalDistance = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}


int parse_VD_DetConfig(VD_DetConfig *pDetConfig, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "carNum")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetConfig->carNum = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "carRects")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_rect(&pDetConfig->carRects[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}

		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "regionNum")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetConfig->regionNum = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "parkRegion")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_ParkRegion(&pDetConfig->parkRegion[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}




int parse_VD_PresetPosOrder(VD_PresetPosOrder *pPresetPosOrder, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetPosID")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPosOrder->presetPosID = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "timeInterval")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPosOrder->timeInterval = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "algswitch")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPosOrder->algswitch = atoi((const char*) szKey);
//			TRACE_LOG_SYSTEM("algswitch=%d\n",pPresetPosOrder->algswitch)
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;

}


int parse_VD_TimeSchedule(VD_TimeSchedule *pTimeSchedule, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "starTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTimeSchedule->starTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "endTime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTimeSchedule->endTime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetPosOrder")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_PresetPosOrder(&pTimeSchedule->presetPosOrder[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;

}







//////////////////////////////////// PD config end //////////////////////////////////////////////





//========================= ISP.h =================================

/**
 * 解析s_ExpMode结构体
 * 参数：
 * 	ps_ExpMode[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_ExpMode(s_ExpMode *ps_ExpMode, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "flag")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpMode->flag = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_ExpManu结构体
 * 参数：
 * 	ps_ExpManu[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_ExpManu(s_ExpManu *ps_ExpManu, xmlNodePtr node)
{
	xmlChar *szKey;
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpManu->exp = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpManu->gain = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_ExpAuto结构体
 * 参数：
 * 	ps_ExpAuto[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_ExpAuto(s_ExpAuto *ps_ExpAuto, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "light_dt_mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->light_dt_mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_max")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->exp_max = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_max")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->gain_max = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_mid")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->exp_mid = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_mid")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->gain_mid = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_min")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->exp_min = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_min")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpAuto->gain_min = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_ExpWnd结构体
 * 参数：
 * 	ps_ExpWnd[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_ExpWnd(s_ExpWnd *ps_ExpWnd, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line1")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line1 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line2")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line2 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line3")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line3 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line4")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line4 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line5")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line5 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line6")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line6 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line7")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line7 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line8")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ExpWnd->line8 = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_AwbMode结构体
 * 参数：
 * 	ps_AwbMode[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_AwbMode(s_AwbMode *ps_AwbMode, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "flag")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_AwbMode->flag = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_AwbManu结构体
 * 参数：
 * 	ps_AwbManu[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_AwbManu(s_AwbManu *ps_AwbManu, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_r")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_AwbManu->gain_r = atoi((const char*) szKey);
			xmlFree(szKey);
			debug("ps_AwbManu->gain_r = %d \n", ps_AwbManu->gain_r);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_g")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_AwbManu->gain_g = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "gain_b")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_AwbManu->gain_b = atoi((const char*) szKey);
			xmlFree(szKey);
			debug("ps_AwbManu->gain_b = %d \n", ps_AwbManu->gain_b);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析s_ColorParam结构体
 * 参数：
 * 	ps_ColorParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_s_ColorParam(s_ColorParam *ps_ColorParam, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "contrast")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ColorParam->contrast = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "luma")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ColorParam->luma = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "saturation")))
		{
			szKey = xmlNodeGetContent(curNode);
			ps_ColorParam->saturation = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

int parse_Syn_Info(Syn_Info *pSyn_Info, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_syn_open")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSyn_Info->is_syn_open = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "phase")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSyn_Info->phase = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

int parse_Lamp_Info(Lamp_Info *pLamp_Info, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "nMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pLamp_Info->nMode[atoi((const char*) szAttr) - 1] = atoi(
					            (char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "nLampType")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pLamp_Info->nLampType[atoi((const char*) szAttr) - 1]
					    = atoi((char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析DISK_WRI_DATA结构体
 * 参数：
 * 	pDISK_WRI_DATA[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_DISK_WRI_DATA(DISK_WRI_DATA *pDISK_WRI_DATA, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "remain_disk")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->remain_disk = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illegal_picture")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->illegal_picture = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vehicle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->vehicle = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_picture")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->event_picture = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illegal_video")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->illegal_video = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_video")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->event_video = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "flow_statistics")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDISK_WRI_DATA->flow_statistics = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析FTP_DATA_CONFIG结构体
 * 参数：
 * 	pFTP_DATA_CONFIG[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_FTP_DATA_CONFIG(FTP_DATA_CONFIG *pFTP_DATA_CONFIG, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illegal_picture")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->illegal_picture = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vehicle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->vehicle = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_picture")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->event_picture = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illegal_video")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->illegal_video = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_video")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->event_video = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "flow_statistics")))
		{
			szKey = xmlNodeGetContent(curNode);
			pFTP_DATA_CONFIG->flow_statistics = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

int parse_RESUME_UPLOAD_PARAM(RESUME_UPLOAD_PARAM *pRESUME_UPLOAD_PARAM,
                              xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_resume_passcar")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRESUME_UPLOAD_PARAM->is_resume_passcar = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_resume_illegal")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRESUME_UPLOAD_PARAM->is_resume_illegal = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_resume_event")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRESUME_UPLOAD_PARAM->is_resume_event = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_resume_statistics")))
		{
			szKey = xmlNodeGetContent(curNode);
			pRESUME_UPLOAD_PARAM->is_resume_statistics = atoi(
			            (const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}





/**
 * 解析SerialParam结构体
 * 参数：
 * 	pSerialParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_SerialParam(SerialParam *pSerialParam, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "dev_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSerialParam->dev_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "bps")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSerialParam->bps = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "check")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSerialParam->check = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "data")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSerialParam->data = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "stop")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSerialParam->stop = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析IOParam结构体
 * 参数：
 * 	pIOParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_IOParam(IO_cfg *pIOParam, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "trigger_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pIOParam->trigger_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pIOParam->mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "io_drt")))
		{
			szKey = xmlNodeGetContent(curNode);
			pIOParam->io_drt = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}



/**
 * 解析Osd_item结构体
 * 参数：
 * 	pOsd_item[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Osd_item(Osd_item *pOsd_item, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "switch_on")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOsd_item->switch_on = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "x")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOsd_item->x = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "y")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOsd_item->y = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_time")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOsd_item->is_time = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "content")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey), (char*) pOsd_item->content,
			            sizeof(pOsd_item->content));
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Osd_info结构体
 * 参数：
 * 	pOsd_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Osd_info(Osd_info *pOsd_info, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "color")))
		{
			parse_Type_info_color(&pOsd_info->color, curNode);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "osd_item")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_Osd_item(&pOsd_info->osd_item[atoi(
					                                        (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析H264_chanel结构体
 * 参数：
 * 	pH264_chanel[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_H264_chanel(H264_chanel *pH264_chanel, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h264_on")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->h264_on = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "cast")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->cast = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pH264_chanel->ip[atoi((const char*) szAttr) - 1] = atoi(
					            (char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "port")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->port = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "fps")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->fps = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rate")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->rate = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "width")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->width = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "height")))
		{
			szKey = xmlNodeGetContent(curNode);
			pH264_chanel->height = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "osd_info")))
		{
			parse_Osd_info(&pH264_chanel->osd_info, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}



#if 1
/**
 * 解析VD_line结构体
 * 参数：
 * 	pline[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_VD_line(VD_line *pLine, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "a")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLine->a = atof((const char*) szKey);

			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "b")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLine->b = atof((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析VD_point结构体
 * 参数：
 * 	pPoint[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_VD_point(VD_point *pPoint, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;
	
	printf("curNode->name is %s\n",curNode->name);


	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "x")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPoint->x = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "y")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPoint->y = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

int parse_VD_leftLineInfo(VD_lineSegment *pleftLineInfo, xmlNodePtr node)
{
	xmlNodePtr curNode;
	//xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "startPoint")))
		{
			parse_VD_point(&pleftLineInfo->startPoint, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "endPoint")))
		{
			parse_VD_point(&pleftLineInfo->endPoint, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
			parse_VD_line(&pleftLineInfo->line, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析Lane_line结构体
 * 参数：
 * 	pLane_line[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Lane_line(Lane_line *pLane_line, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLane_line->num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "point")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_point(&pLane_line->point[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_line(
				    &pLane_line->line[atoi((const char*) szAttr) - 1],
				    curNode);

				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Leftturn_waiting_zone结构体
 * 参数：
 * 	pLeftturn_waiting_zone[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Leftturn_waiting_zone(Leftturn_waiting_zone *pLeftturn_waiting_zone,
                                xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "shape")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLeftturn_waiting_zone->shape = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "point")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_point(&pLeftturn_waiting_zone->point[atoi(
				                   (const char*) szAttr) - 1], curNode);

				xmlFree(szAttr);
			}
		}

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "line")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_line(&pLeftturn_waiting_zone->line[atoi(
				                  (const char*) szAttr) - 1], curNode);

				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Lane_info结构体
 * 参数：
 * 	pLane_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Lane_info(Lane_info *pLane_info, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLane_info->direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "up_down")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLane_info->up_down = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "nature")))
		{
			szKey = xmlNodeGetContent(curNode);
			pLane_info->nature = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "waiting_zone")))
		{
			parse_Leftturn_waiting_zone(&pLane_info->waiting_zone, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "lane_line")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_Lane_line(&pLane_info->lane_line[atoi(
				        (const char*) szAttr) - 1], curNode);

				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}

int parse_signalController_param(SignalControllerParam *pSignal_param, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "realtime_mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_param->realtimeMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "car_occupy_flag")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_param->m_car_occupy_flag = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "small_car_length")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_param->m_small_car_length = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "big_car_length")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_param->m_big_car_length = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "motor_occupy_flag")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_param->m_motor_occupy_flag = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}
	return 0;
}


int parse_Strobe_param(Strobe_param *pStrobe_param, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "strobe_switch")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					pStrobe_param->strobe_switch[atoi((const char*) szAttr) - 1]
					    = atoi((char*) szKey);
				}
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "strobe_count")))
		{
			szKey = xmlNodeGetContent(curNode);
			pStrobe_param->strobe_count = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "strobe")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_Strobe(&pStrobe_param->strobe[atoi(
					                                        (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}
	return 0;
}
#endif

#if 1
/**
 * 解析Camera_config结构体
 * 参数：
 * 	pCamera_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Camera_config(Camera_config *pCamera_config, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_mode")))
		{
			parse_s_ExpMode(&pCamera_config->exp_mode, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_manu")))
		{
			parse_s_ExpManu(&pCamera_config->exp_manu, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_auto")))
		{
			parse_s_ExpAuto(&pCamera_config->exp_auto, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_wnd")))
		{
			parse_s_ExpWnd(&pCamera_config->exp_wnd, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "awb_mode")))
		{
			parse_s_AwbMode(&pCamera_config->awb_mode, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "awb_manu")))
		{
			parse_s_AwbManu(&pCamera_config->awb_manu, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "color_param")))
		{
			parse_s_ColorParam(&pCamera_config->color_param, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "syn_info")))
		{
			parse_Syn_Info(&pCamera_config->syn_info, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "lamp_info")))
		{
			parse_Lamp_Info(&pCamera_config->lamp_info, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}
#endif

#if 1

/************************************************
*函数名: parse_IP_BERTH_FRONT
*参数:       
*功能:      解析前边泊位的IP地址参数
*返回值:
************************************************/


static int parse_IP_BERTH_FRONT(char* ip_berth_back, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性 id 
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				ip_berth_back[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}


int parse_TYPE_MQ_CONFIG_PARAM(TYPE_MQ_CONFIG_PARAM *pTYPE_MQ_CONFIG_PARAM,
                                xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{		
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pTYPE_MQ_CONFIG_PARAM->ip[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "port")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTYPE_MQ_CONFIG_PARAM->port = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		
		curNode = curNode->next;
	}

	return 0;
}
////////============================================================
////////////  parse arm_config structure ///////////////////////////
/**
 * 解析TYPE_FTP_CONFIG_PARAM结构体
 * 参数：
 * 	pTYPE_FTP_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_TYPE_FTP_CONFIG_PARAM(TYPE_FTP_CONFIG_PARAM *pTYPE_FTP_CONFIG_PARAM,
                                xmlNodePtr node)

{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "user")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pTYPE_FTP_CONFIG_PARAM->user, (const char*) szKey,
			        sizeof(pTYPE_FTP_CONFIG_PARAM->user));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "passwd")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pTYPE_FTP_CONFIG_PARAM->passwd, (const char*) szKey,
			        sizeof(pTYPE_FTP_CONFIG_PARAM->passwd));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				szKey = xmlNodeGetContent(curNode);
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				pTYPE_FTP_CONFIG_PARAM->ip[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "port")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTYPE_FTP_CONFIG_PARAM->port = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "allow_anonymous")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTYPE_FTP_CONFIG_PARAM->allow_anonymous = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析DATA_SAVE结构体
 * 参数：
 * 	pDATA_SAVE[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_DATA_SAVE(DATA_SAVE *pDATA_SAVE, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "disk_wri_data")))
		{
			parse_DISK_WRI_DATA(&pDATA_SAVE->disk_wri_data, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ftp_data_config")))
		{
			parse_FTP_DATA_CONFIG(&pDATA_SAVE->ftp_data_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "resume_upload_data")))
		{
			parse_RESUME_UPLOAD_PARAM(&pDATA_SAVE->resume_upload_data, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析NTP_CONFIG_PARAM结构体
 * 参数：
 * 	pNTP_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_NTP_CONFIG_PARAM(NTP_CONFIG_PARAM *pNTP_CONFIG_PARAM, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "useNTP")))
		{
			szKey = xmlNodeGetContent(curNode);
			pNTP_CONFIG_PARAM->useNTP = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "NTP_server_ip")))
		{
			szKey = xmlNodeGetContent(curNode);
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
					pNTP_CONFIG_PARAM->NTP_server_ip[atoi((const char*) szAttr)
					                                 - 1] = atoi((const char*) szKey);
				xmlFree(szAttr);
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "NTP_distance")))
		{
			szKey = xmlNodeGetContent(curNode);
			pNTP_CONFIG_PARAM->NTP_distance = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}
	return 0;
}

/**
 * 创建ILLEGAL_CODE_INFO结构体
 * 参数：
 * 	pInterface_Param[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_ILLEGAL_CODE_INFO(ILLEGAL_CODE_INFO *pillegal_code_info,
                            xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illeagal_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pillegal_code_info->illeagal_type = atoi((const char*) szKey);
			xmlFree(szKey);

		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "illeagal_num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pillegal_code_info->illeagal_num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析H264_config结构体
 * 参数：
 * 	pH264_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_H264_config(H264_config *pH264_config, xmlNodePtr node)
{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h264_channel")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_H264_chanel(&pH264_config->h264_channel[atoi(
					                      (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}
		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Interface_Param结构体
 * 参数：
 * 	pInterface_Param[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Interface_Param(Interface_Param *pInterface_Param, xmlNodePtr node)

{
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "serial")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_SerialParam(&pInterface_Param->serial[atoi(
					                      (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "io_input_params")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_IOParam(&pInterface_Param->io_input_params[atoi(
					                  (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "io_output_params")))
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (*(char*) szAttr >= '1')
				{
					parse_IOParam(&pInterface_Param->io_output_params[atoi(
					                  (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}
/**
 * 解析BASIC_PARAM结构体
 * 参数：
 * 	pBASIC_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_BASIC_PARAM(BASIC_PARAM *pBASIC_PARAM, xmlNodePtr node)

{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		debug("node: %s\n", curNode->name);
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "monitor_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->monitor_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "spot_id")))
		{
			szKey = xmlNodeGetContent(curNode);

			strncpy(pBASIC_PARAM->spot_id, (const char*) szKey,
			        sizeof(pBASIC_PARAM->spot_id));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "road_id")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pBASIC_PARAM->road_id, (const char*) szKey,
			        sizeof(pBASIC_PARAM->road_id));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "spot")))
		{
			szKey = xmlNodeGetContent(curNode);

			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey), (char*) pBASIC_PARAM->spot,
			            sizeof(pBASIC_PARAM->spot));
			debug("spot: %s\n", pBASIC_PARAM->spot);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "ntp_config_param")))
		{
			parse_NTP_CONFIG_PARAM(&pBASIC_PARAM->ntp_config_param, curNode);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->exp_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "exp_device_id")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pBASIC_PARAM->exp_device_id, (const char*) szKey,
			        sizeof(pBASIC_PARAM->exp_device_id));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "collect_actor_size")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->collect_actor_size = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "log_level")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->log_level = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "data_save")))
		{
			parse_DATA_SAVE(&pBASIC_PARAM->data_save, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h264_record")))
		{
			szKey = xmlNodeGetContent(curNode);
			pBASIC_PARAM->h264_record = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "ftp_param_pass_car")))
		{
			parse_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_pass_car,
			                            curNode);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "ftp_param_illegal")))
		{
			parse_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_illegal,
			                            curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ftp_param_h264")))
		{
			parse_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_h264, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ftp_param_h264")))
		{
			parse_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_h264, curNode);
		}

		//第三方
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mq_param")))
		{
			parse_TYPE_MQ_CONFIG_PARAM(&pBASIC_PARAM->mq_param, curNode);

		}
		//////////////////
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip_berth_front")))    //仅泊车使用
		{
			parse_IP_BERTH_FRONT((char *)&pBASIC_PARAM->ip_berth_front, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip_berth_back")))     //仅泊车使用
		{
			parse_IP_BERTH_FRONT((char *)&pBASIC_PARAM->ip_berth_back, curNode);
		}

		
		//by shp 2015/04/28
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "oss_aliyun_param")))     //仅泊车使用
		{
			parse_TYPE_OSS_ALIYUN_CONFIG_PARAM(&pBASIC_PARAM->oss_aliyun_param, curNode);
		}
//		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "ip_berth_back")))     //仅泊车使用
//		{
//			parse_IP_BERTH_FRONT(&pBASIC_PARAM->ip_berth_back, curNode);
//		}

		curNode = curNode->next;
	}

	return 0;
}
/**
 * 解析ARM_config结构体
 * 参数：
 * 	pARM_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_ARM_config(ARM_config *pARM_config, xmlNodePtr node)
{
	printf("In parse_ARM_config\n");
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "basic_param")))
		{
			parse_BASIC_PARAM(&pARM_config->basic_param, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "interface_param")))
		{
			parse_Interface_Param(&pARM_config->interface_param, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "h264_config")))
		{
			parse_H264_config(&pARM_config->h264_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "illegal_code_info")))                                                                                                   
		{
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				if (atoi((const char *) szAttr) >= 1)
				{
					parse_ILLEGAL_CODE_INFO(
					    &pARM_config->illegal_code_info[atoi(
					                                        (const char*) szAttr) - 1], curNode);
				}
				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;
}
#endif

#if 1
int parse_VD_PresetPos(VD_PresetPos *pPresetPos, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetID")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPos->presetID = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "isAdjust")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPos->isAdjust = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "pTZinfo")))
		{
			parse_VD_PTZinfo(&pPresetPos->pTZinfo, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detConfig")))
		{
			parse_VD_DetConfig(&pPresetPos->detConfig, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "algtype")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPos->algtype = atoi((const char*) szKey);
//			TRACE_LOG_SYSTEM("algtype = %d\n",pPresetPos->algtype);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "preset_describe")))
		{
			szKey = xmlNodeGetContent(curNode);
//			pPresetPos->algtype = atoi((const char*) szKey);
			memcpy(pPresetPos->preset_describe,(const char*) szKey,strlen((const char*) szKey));
//			TRACE_LOG_SYSTEM("preset_describe = %s\n",pPresetPos->preset_describe);
			xmlFree(szKey);
		}
		curNode = curNode->next;
	}

	return 0;
}

int parse_VD_PresetPosRotation(VD_PresetPosRotation* pPresetPosRotation, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rotationMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPosRotation->rotationMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "timeInterval")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPresetPosRotation->timeInterval = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "timeSchedue")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_TimeSchedule(&pPresetPosRotation->timeSchedue[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}
		}

		curNode = curNode->next;
	}

	return 0;

}


int parse_VD_PTZCtrlInfo(VD_PTZCtrlInfo* pPTZCtrlInfo, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "PTZHorSpeed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPTZCtrlInfo->PTZHorSpeed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "PTZVerSpeed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPTZCtrlInfo->PTZVerSpeed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "focusSpeed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pPTZCtrlInfo->focusSpeed = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;

}


/**
 * 解析VD_IllegalParkingConfig结构体
 * 参数：
 * 	pVD_IllegalParkingConfig[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_DSP_PD_config(VD_IllegalParkingConfig *pVDCS_config, xmlNodePtr node)
{
	xmlChar *szKey; //临时字符串变量
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;
	while (curNode != NULL)
	{
		printf("parse_DSP_PD_config: curNode->name is %s\n",curNode->name);

		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "version")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->version = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "cfgParamChanged")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->cfgParamChanged = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetNum")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->presetNum = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "workMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->workMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camerMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->camerMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detectMode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->detectMode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detectprePos")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->detectprePos = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "device_config")))
		{
			parse_DeviceConfig(&pVDCS_config->device_config, curNode);
		}	
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_arithm")))
		{
			parse_Camera_arithm(&pVDCS_config->camera_arithm, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "traffic_event")))
		{
			parse_Traffic_event_det(&pVDCS_config->traffic_event, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "overlay_violate")))
		{
			parse_Overlay_violate(&pVDCS_config->overlay_violate, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "pTZCtrlInfo")))
		{
			parse_VD_PTZCtrlInfo(&pVDCS_config->pTZCtrlInfo, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetPosRotation")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_PresetPosRotation(&pVDCS_config->presetPosRotation[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}

//			parse_Signal_detect(&pVDCS_config->presetPosRotation, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "presetPos")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");

				parse_VD_PresetPos(&pVDCS_config->presetPos[atoi((const char*) szAttr)
				                                  - 1], curNode);

				xmlFree(szAttr);
			}

//			parse_Detectarea_info(&pVDCS_config->presetPos, curNode);
		}

		curNode = curNode->next;
	}

	return 0;
}
#endif

#if 1
/**
 * 解析Overlay_vehicle结构体
 * 参数：
 * 	pOverlay_vehicle[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Overlay_vehicle(Overlay_vehicle *pOverlay_vehicle, xmlNodePtr node)

{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_overlay_vehicle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_overlay_vehicle = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "start_x")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->start_x = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "start_y")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->start_y = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_spot_name")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_spot_name = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_device_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_device_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_time")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_time = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_lane_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_lane_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_lane_direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_lane_direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_plate_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_color")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_plate_color = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_plate_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_speed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_speed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "color")))
		{
			parse_Type_info_color(&pOverlay_vehicle->color, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "font_size")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->font_size = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_vehicle_color")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_vehicle_color = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_vehicle_logo")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_vehicle_logo = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_vehicle_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_vehicle_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_check_code")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_vehicle->is_check_code = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}
/**
 * 解析Overlay_violate结构体
 * 参数：
 * 	pOverlay_violate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_Overlay_violate(Overlay_violate *pOverlay_violate, xmlNodePtr node)

{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	printf("parse_Overlay_violate: pOverlay_violate\n");

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "start_x")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->start_x = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "start_y")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->start_y = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_spot_name")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_spot_name = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_device_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_device_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_time")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_time = atoi((const char*) szKey);
			printf("pOverlay_violate->is_time: str: %s, value: %d\n",(const char*)szKey, pOverlay_violate->is_time);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_redlamp_starttime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_redlamp_starttime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_redlamp_keeptime")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_redlamp_keeptime = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_lane_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_lane_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_lane_direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_lane_direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_numble")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_plate_numble = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_color")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_plate_color = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_plate_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_plate_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_speed")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_speed = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "pic_num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->pic_num = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "color")))
		{
			parse_Type_info_color(&pOverlay_violate->color, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "violate_desc")))
		{
			parse_Violate_description(&pOverlay_violate->violate_desc, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "font_size")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->font_size = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_vehicle_color")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_vehicle_color = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_vehicle_logo")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_vehicle_logo = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_vehicle_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_vehicle_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_check_code")))
		{
			szKey = xmlNodeGetContent(curNode);
			pOverlay_violate->is_check_code = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Traffic_envent_det结构体
 * 参数：
 * 	pTraffic_envent_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_Traffic_event_det(Traffic_event_det *pTraffic_envent_det,
								   xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_event_det")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_envent_det->is_event_det = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "run_red_det")))
		{
			parse_RunredDet(&pTraffic_envent_det->run_red_det, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "speed_det")))
		{
			parse_SpeedDet(&pTraffic_envent_det->speed_det, curNode);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "limit_travel_det")))
		{
			parse_Limit_travel_det(&pTraffic_envent_det->limit_travel_det,
			                       curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "event_det")))
		{
			parse_Event_det(&pTraffic_envent_det->event_det, curNode);
		}

		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析Traffic_object_det结构体
 * 参数：
 * 	pTraffic_object_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */

int parse_Traffic_object_det(Traffic_object_det *pTraffic_object_det,
                             xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vehicle_union")))
		{
			parse__VEHICLE_UNION(&pTraffic_object_det->vehicle_union, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_object_det->mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "plate_det")))
		{
			parse_Plate(&pTraffic_object_det->plate_det, curNode);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "vehicle_statis_cycle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_object_det->vehicle_statis_cycle = atoi(
			            (const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "isdetection_novehicle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_object_det->isdetection_novehicle = atoi(
			            (const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "isdetection_pedestrian")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_object_det->isdetection_pedestrian = atoi(
			            (const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "pedestrian_statis_cycle")))
		{
			szKey = xmlNodeGetContent(curNode);
			pTraffic_object_det->pedestrian_statis_cycle = atoi(
			            (const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}

	return 0;
}

/**
 * 解析Camera_arithm结构体
 * 参数：
 * 	pCamera_arithm[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_Camera_arithm(Camera_arithm *pCamera_arithm, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "is_arithm_set")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->is_arithm_set = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->camera_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "image_width")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->image_width = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "image_height")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->image_height = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "image_type")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->image_type = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "image_quality")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->image_quality = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "frame_rate")))
		{
			szKey = xmlNodeGetContent(curNode);
			pCamera_arithm->frame_rate = atoi((const char*) szKey);
			xmlFree(szKey);
		}

		curNode = curNode->next;
	}
	return 0;
}

/**
 * 解析Signal_detect结构体
 * 参数：
 * 	pSignal_detect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Signal_detect(Signal_detect *pSignal_detect, xmlNodePtr node)
{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "mode")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_detect->mode = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "is_signal_detect")))
		{
			szKey = xmlNodeGetContent(curNode);
			pSignal_detect->is_signal_detect = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "signal_detect_video")))
		{
			parse_Signal_detect_video(&pSignal_detect->signal_detect_video,
			                          curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}


/**
 * 解析Detectarea_info结构体
 * 参数：
 * 	pDetectarea_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_Detectarea_info(Detectarea_info *pDetectarea_info, xmlNodePtr node)         

{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;
#if 1
	while (curNode != NULL)
	{
		printf("parse_Detectarea_info: curNode->name is %s\n",curNode->name);
		//取出节点中的内容
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "lane_num")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->lane_num = atoi((const char*) szKey);
			printf("pDetectarea_info->lane_num:%d\n",pDetectarea_info->lane_num);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "first_lane_no")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->first_lane_no = atoi((const char*) szKey);
			printf("pDetectarea_info->first_lane_no:%d\n",pDetectarea_info->first_lane_no);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "before_stopline")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->before_stopline = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "stopline")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				szKey = xmlNodeGetContent(curNode);
				pDetectarea_info->stopline[atoi((const char*) szAttr) - 1]
				    = atoi((const char*) szKey);

				xmlFree(szKey);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "after_stopline")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->after_stopline = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "lane_info")))
		{
			//查找属性
			xmlAttrPtr attrPtr = curNode->properties;

			//只是考虑仅有一个属性
			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
			{
				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
				parse_Lane_info(&pDetectarea_info->lane_info[atoi(
				                    (const char*) szAttr) - 1], curNode);
				xmlFree(szAttr);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "leftLineInfo")))
		{
			parse_VD_leftLineInfo(&pDetectarea_info->leftLineInfo, curNode);
		}

		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "rightLineInfo")))
		{
			parse_VD_rightLineInfo(&pDetectarea_info->rightLineInfo, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "laneLeftNature")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->laneLeftNature = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "laneRightNature")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDetectarea_info->laneRightNature = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detect_area")))
		{
			parse_VD_rect(&pDetectarea_info->detect_area, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "zebra_area")))
		{
			parse_VD_rect(&pDetectarea_info->zebra_area, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "safety_area")))
		{

			//查找属性
	//		xmlAttrPtr attrPtr = curNode->properties;
			

			//只是考虑仅有一个属性
	//		if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))

	//	//	{
		//		xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
	//		
				parse_VD_polygon(&pDetectarea_info->safety_area, curNode);
		
		//		xmlFree(szAttr);
	         //      }
//		//	parse_VD_polygon(&pDetectarea_info->safety_area, curNode);

		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "congestion_area")))
		{
//			//查找属性
//			xmlAttrPtr attrPtr = curNode->properties;
//
//			//只是考虑仅有一个属性
//			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
//			{
//				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
//				parse_VD_polygon(&pDetectarea_info->congestion_area[atoi(
//				                     (const char*) szAttr) - 1], curNode);
//				xmlFree(szAttr);
//			}
			parse_VD_polygon(&pDetectarea_info->congestion_area, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "no_parking_area")))
		{
//			//查找属性
//			xmlAttrPtr attrPtr = curNode->properties;
//
//			//只是考虑仅有一个属性
//			if (!xmlStrcmp(attrPtr->name, (const xmlChar *) "id"))
//			{
//				xmlChar* szAttr = xmlGetProp(curNode, (const xmlChar *) "id");
//				parse_VD_polygon(&pDetectarea_info->no_parking_area[atoi(
//				                     (const char*) szAttr) - 1], curNode);
//				xmlFree(szAttr);
//			}
			parse_VD_polygon(&pDetectarea_info->parking_area, curNode);
		}

	

		curNode = curNode->next;
	}
	printf("pDetectarea_info->lane_num=%d\n", pDetectarea_info->lane_num);
#endif
	return 0;
}

/**
 * 解析DeviceConfig结构体
 * 参数：
 * 	pDeviceConfig[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
static int parse_DeviceConfig(DeviceConfig *pDeviceConfig, xmlNodePtr node)  // 卡口使用过

{
	xmlNodePtr curNode;
	xmlChar *szKey; //临时字符串变量

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "strSpotName")))
		{
			szKey = xmlNodeGetContent(curNode);
			convert_enc((char*) "UTF-8", (char*) "GBK", (char*) szKey, strlen(
			                (const char*) szKey), (char*) pDeviceConfig->strSpotName,
			            sizeof(pDeviceConfig->strSpotName));
			xmlFree(szKey);

			debug("违法地点: %s\n",pDeviceConfig->strSpotName );
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "spotID")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pDeviceConfig->spotID, (const char*) szKey,
			        sizeof(pDeviceConfig->spotID));


			printf("DeviceConfig->spotID:%s\n",pDeviceConfig->spotID);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "deviceID")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(pDeviceConfig->deviceID, (const char*) szKey,
			        sizeof(pDeviceConfig->deviceID));

			//if deviceID is diff, use message.txt, rather than dsp_config.xml
			if (strcasecmp(pDeviceConfig->deviceID, g_set_net_param.m_NetParam.m_DeviceID))
			{
				memset(pDeviceConfig->deviceID, 0, sizeof(pDeviceConfig->deviceID));
				strncpy(pDeviceConfig->deviceID, (const char*) g_set_net_param.m_NetParam.m_DeviceID,
							        strlen(g_set_net_param.m_NetParam.m_DeviceID));
			}
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "direction")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDeviceConfig->direction = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "startLane")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDeviceConfig->startLane = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "encodeType")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDeviceConfig->encodeType = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "dynamicAdjustFlag")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDeviceConfig->dynamicAdjustFlag = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name,
		                     (const xmlChar *) "imgSizeUpperLimit")))
		{
			szKey = xmlNodeGetContent(curNode);
			pDeviceConfig->imgSizeUpperLimit = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "strobe_param")))
		{
			parse_Strobe_param(&pDeviceConfig->strobe_param, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "signalControllerParam")))
		{
			parse_signalController_param(&pDeviceConfig->signalControllerParam, curNode);
		}
		curNode = curNode->next;
	}

	return 0;
}
/**
 * 解析VDCS_config结构体
 * 参数：
 * 	pVDCS_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
int parse_DSP_config(VDCS_config *pVDCS_config, xmlNodePtr node)
{
	xmlChar *szKey; //临时字符串变量
	xmlNodePtr curNode;

	curNode = node->xmlChildrenNode;
	while (curNode != NULL)
	{
		printf("parse_DSP_config: curNode->name is %s\n",curNode->name);
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "version")))
		{
			szKey = xmlNodeGetContent(curNode);
			pVDCS_config->version = atoi((const char*) szKey);
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "device_config")))
		{
			parse_DeviceConfig(&pVDCS_config->device_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "detectarea_info")))
		{
			parse_Detectarea_info(&pVDCS_config->detectarea_info, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "signal_detect")))
		{
			parse_Signal_detect(&pVDCS_config->signal_detect, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_arithm")))
		{
			parse_Camera_arithm(&pVDCS_config->camera_arithm, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "traffic_object_det")))
		{
			parse_Traffic_object_det(&pVDCS_config->traffic_object, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "traffic_event")))//traffic_event_det
		{
			parse_Traffic_event_det(&pVDCS_config->traffic_event, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "overlay_violate")))
		{
			parse_Overlay_violate(&pVDCS_config->overlay_violate, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "overlay_vehicle")))
		{
			parse_Overlay_vehicle(&pVDCS_config->overlay_vehicle, curNode);
		}
		
		curNode = curNode->next;
	}

	return 0;
}

#endif



/**
 * 解析所有3个xml文件
 * 参数：
 * 		filename： 要解析的xml文件
 * 		pVDCS_config： 解析出的算法参数结构体
 * 		pCamera_config：解析出的相机参数结构体
 * 		pARM_config：解析出的嵌入式参数结构体
 * 返回：
 * 		0:成功， -1:失败
 */
int parse_xml_doc(const char *filename, //VDCS_config *pVDCS_config,
				  VDConfigData *pVDConfigData,
                  Camera_config *pCamera_config, ARM_config *pARM_config
                  //,                  VD_IllegalParkingConfig *pVD_IllegalParkingConfig
                  )
{
	printf("In parse_xml_doc\n");
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr curNode;

	if (-1 == open_xml_doc(filename, &doc))
	{
		log_error("XML", "open xml file error. \n");
		return -1;
	}

	if (-1 == get_root_node(doc, &root))
	{
		log_error("XML", "error when get root node. \n");
		close_xml_doc(doc);
		return -1;
	}
	curNode = root->xmlChildrenNode;
	printf("1\n");
	printf("parse_xml_doc: curNode->name: %s\n",curNode->name);
	printf("2\n");
	while (curNode != NULL)
	{
//		printf("parse_xml_doc: curNode->name: %s\n",curNode->name);
		
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "VM_P_config")))                      //云台卡口dsp_config.xml
		{
			if (pVDConfigData)
			{
				pVDConfigData->flag_func = DEV_TYPE;
				parse_DSP_config(&pVDConfigData->vdConfig.vdCS_config, curNode);
			}
		}
		else if((!xmlStrcmp(curNode->name, (const xmlChar *) "VM_config")))                    //枪击卡口dsp_config.xml
		{
			if (pVDConfigData)
			{
				pVDConfigData->flag_func = DEV_TYPE;     
				parse_DSP_config(&pVDConfigData->vdConfig.vdCS_config, curNode);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "VP_config")))                  //违停dsp_config.xml
		{
			if (pVDConfigData)
			{
				pVDConfigData->flag_func = DEV_TYPE;     
				parse_DSP_PD_config(&pVDConfigData->vdConfig.pdCS_config, curNode);
			}
		}	
		else if((!xmlStrcmp(curNode->name, (const xmlChar *) "park_config")))                    //泊车dsp_config.xml
		{
			if (pVDConfigData)
			{
				pVDConfigData->flag_func = DEV_TYPE;     
				parse_DSP_config(&pVDConfigData->vdConfig.vdCS_config, curNode);
			}
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_config")))
		{
			if (pCamera_config)
				parse_Camera_config(pCamera_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "arm_config")))
		{
			if (pARM_config)
				parse_ARM_config(pARM_config, curNode);

		}
		else if((!xmlStrcmp(curNode->name, (const xmlChar *) "VE_config")))                    //出入口dsp_config.xml
		{
			if (pVDConfigData)
			{
				pVDConfigData->flag_func = DEV_TYPE;
				parse_DSP_config(&pVDConfigData->vdConfig.vdCS_config, curNode);
			}
		}

		curNode = curNode->next;
	}

	close_xml_doc(doc);

	return 0;
}



/**
 * 解析违停系统的3个xml文件
 * 参数：
 * 		filename： 要解析的xml文件
 * 		pVD_IllegalParkingConfig： 解析出的算法参数结构体
 * 		pCamera_config：解析出的相机参数结构体
 * 		pARM_config：解析出的嵌入式参数结构体
 * 返回：
 * 		0:成功， -1:失败
 */
//int parse_PD_xml_doc(const char *filename, VD_IllegalParkingConfig *pVD_IllegalParkingConfig,
//                  Camera_config *pCamera_config, ARM_config *pARM_config)
//{
//	xmlDocPtr doc;
//	xmlNodePtr root;
//	xmlNodePtr curNode;
//
//	if (-1 == open_xml_doc(filename, &doc))
//	{
//		log_error("XML", "open xml file error. \n");
//		return -1;
//	}
//
//	if (-1 == get_root_node(doc, &root))
//	{
//		log_error("XML", "error when get root node. \n");
//		close_xml_doc(doc);
//		return -1;
//	}
//
//	curNode = root->xmlChildrenNode;
//	while (curNode != NULL)
//	{
//		printf("parse_xml_doc: curNode->name: %s\n",curNode->name);
//		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vdcs_config")))
//		{
//			if (pVD_IllegalParkingConfig)
//			{
//				parse_DSP_PD_config(pVD_IllegalParkingConfig, curNode);
//			}
//		}
//		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_config")))
//		{
//			if (pCamera_config)
//				parse_Camera_config(pCamera_config, curNode);
//		}
//		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "arm_config")))
//		{
//			if (pARM_config)
//				parse_ARM_config(pARM_config, curNode);
//
//		}
//
//		curNode = curNode->next;
//	}
//
//	close_xml_doc(doc);
//
//	return 0;
//}


int parse_xml_string(const char* buffer, int size, VDCS_config *pVDCS_config,
                     Camera_config *pCamera_config, ARM_config *pARM_config)
{
	xmlDocPtr doc;
	doc = xmlParseMemory(buffer, size);

	if (NULL == doc)
	{
		log_error("XML", "error when parse memory!");
		return -1;
	}

	xmlNodePtr root;
	if (-1 == get_root_node(doc, &root))
	{
		log_error("XML", "error when get root!");
		xmlFreeDoc(doc);
		return -1;
	}

	xmlNodePtr curNode;
	curNode = root->xmlChildrenNode;
	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "vdcs_config")))
		{
			if (pVDCS_config)
				parse_DSP_config(pVDCS_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "camera_config")))
		{
			if (pCamera_config)
				parse_Camera_config(pCamera_config, curNode);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "arm_config")))
		{
			if (pARM_config)
				parse_ARM_config(pARM_config, curNode);

		}
		curNode = curNode->next;
	}

	xmlFreeDoc(doc);

	return 0;
}




/************************************************
*函数名: parse_TYPE_OSS_ALIYUN_CONFIG_PARAM
*参数:       
*功能:      解析aliyun OSS配置参数
*返回值:
*作者:   shp 2015/04/28
************************************************/
static int parse_TYPE_OSS_ALIYUN_CONFIG_PARAM(OSS_ALIYUN_PARAM *as_oss_aliyun_param,
											  xmlNodePtr node)

{
	xmlNodePtr curNode;
	xmlChar *szKey;

	curNode = node->xmlChildrenNode;

	while (curNode != NULL)
	{
		if ((!xmlStrcmp(curNode->name, (const xmlChar *) "username")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(as_oss_aliyun_param->username, (const char*) szKey,
			        sizeof(as_oss_aliyun_param->username));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "passwd")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(as_oss_aliyun_param->passwd, (const char*) szKey,
			        sizeof(as_oss_aliyun_param->passwd));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "url")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(as_oss_aliyun_param->url, (const char*) szKey,
			        sizeof(as_oss_aliyun_param->url));
			xmlFree(szKey);
		}
		else if ((!xmlStrcmp(curNode->name, (const xmlChar *) "bucket_name")))
		{
			szKey = xmlNodeGetContent(curNode);
			strncpy(as_oss_aliyun_param->bucket_name, (const char*) szKey,
			        sizeof(as_oss_aliyun_param->bucket_name));
			xmlFree(szKey);
		}
		
		curNode = curNode->next;
	}

	return 0;
}























