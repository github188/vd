/*
 * xmlCreater.cpp
 *
 *  Created on: 2013-6-17
 *      Author: shanhw
 */

#include <stdio.h>
#include <stdlib.h>
#include <libxml/tree.h>
#include <string.h>
#include <iconv.h>

#include "commonfuncs.h"
#include "xmlCreater.h"
#include "global.h"

/*
 * 创建xml文件
 */
xmlDocPtr create_xml_doc(void)
{
	return xmlNewDoc((xmlChar *) "1.0");
}

/*
 *  关闭xml文件
 */
void close_xml_doc(xmlDocPtr doc)
{
	xmlFreeDoc(doc);
	return;
}

/**
 * 创建xml的root节点
 */
xmlNodePtr create_root_node(xmlDocPtr doc, char* pRootName)
{
	xmlNodePtr root_node = xmlNewNode(NULL, (xmlChar *) pRootName);
	if (NULL == root_node)
	{
		log_error("XML ", "error when create root node.\n");
		return NULL;
	}

	xmlDocSetRootElement(doc, root_node);

	return root_node;
}

/**
 * 创建VD_date结构体node
 * 参数：
 * 	pDate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_date(VD_date *pDate, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//设置节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "year");
	sprintf(txt, "%d", pDate->year);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "month");
	sprintf(txt, "%d", pDate->month);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "day");
	sprintf(txt, "%d", pDate->day);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	return node;
}

/**
 * 创建VD_daysegment结构体
 * 参数：
 * 	pDaysegment[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_daysegment(VD_daysegment *pDaysegment, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//设置节点中的内容
	child = create_VD_date(&pDaysegment->begin, (char*) "begin");
	xmlAddChild(node, child);

	child = create_VD_date(&pDaysegment->end, (char*) "end");
	xmlAddChild(node, child);

	return 0;
}

/**
 * 解析VD_time结构体
 * 参数：
 * 	pTime[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_time(VD_Time *pTime, char *pNodeName)//未调用
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];
	debug("in create_VD_time:    \n");
	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容

	child = xmlNewNode(NULL, (xmlChar*) "tm_sec");
	sprintf(txt, "%d", pTime->tm_sec);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_min");
	sprintf(txt, "%d", pTime->tm_min);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_hour");
	sprintf(txt, "%d", pTime->tm_hour);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);


	child = xmlNewNode(NULL, (xmlChar*) "tm_mday");
	sprintf(txt, "%d", pTime->tm_mday);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_mon");
	sprintf(txt, "%d", pTime->tm_mon);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_year");
	sprintf(txt, "%d", pTime->tm_year);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_wday");
	sprintf(txt, "%d", pTime->tm_wday);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_yday");
	sprintf(txt, "%d", pTime->tm_yday);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_isdst");
	sprintf(txt, "%d", pTime->tm_isdst);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "tm_msec");
	sprintf(txt, "%d", pTime->tm_msec);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	debug("in create_VD_time:   end \n");
	return node;
}

xmlNodePtr create_VD_time_restriction(VD_Time_restriction *pTime, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];
	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "hour");
	sprintf(txt, "%d", pTime->hour);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "minute");
	sprintf(txt, "%d", pTime->minute);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "second");
	sprintf(txt, "%d", pTime->second);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_timesegment结构体
 * 参数：
 * 	pTimesegment[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_timesegment(VD_Timesegment *pTimesegment, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = create_VD_time_restriction(&pTimesegment->end, (char*) "begin");
	xmlAddChild(node, child);

	child = create_VD_time_restriction(&pTimesegment->end, (char*) "end");
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_rect结构体
 * 参数：
 * 	pRect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_rect(VD_rect *pRect, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "top");
	sprintf(txt, "%d", pRect->top);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "bottom");
	sprintf(txt, "%d", pRect->bottom);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "left");
	sprintf(txt, "%d", pRect->left);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "right");
	sprintf(txt, "%d", pRect->right);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_point结构体
 * 参数：
 * 	pPoint[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_point(VD_point *pPoint, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "x");
	sprintf(txt, "%d", pPoint->x);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "y");
	sprintf(txt, "%d", pPoint->y);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_line结构体
 * 参数：
 * 	pline[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_line(VD_line *pLine, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "a");
	sprintf(txt, "%.3f", pLine->a);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "b");
	sprintf(txt, "%.3f", pLine->b);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_polygon结构体
 * 参数：
 * 	pRect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VD_polygon(VD_polygon *pPolygon, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}


	//节点中的内容

	child = xmlNewNode(NULL, (xmlChar*) "num");
	sprintf(txt, "%d", pPolygon->num);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = create_VD_rect(&pPolygon->rect, (char*) "rect");
	xmlAddChild(node, child);

	int i;
	for (i=0; i < MAX_POINT_POLYGON ; i++)
	{
		child = create_VD_point(&pPolygon->point[i], (char*) "point");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_POINT_POLYGON ; i++)
	{
		child = create_VD_line(&pPolygon->line[i], (char*) "line");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	return node;
}







/**
 * 解析Lane_line结构体
 * 参数：
 * 	pLane_line[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Lane_line(Lane_line *pLane_line, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "num");
	sprintf(txt, "%d", pLane_line->num);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	int i;
	for (i=0; i < MAX_LANE_PART + 1; i++)
	{
		child = create_VD_point(&pLane_line->point[i], (char*) "point");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_PART; i++)
	{
		child = create_VD_line(&pLane_line->line[i], (char*) "line");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析VD_lineSegment结构体
 * 参数：
 * 	leftLane_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_leftLane_info(VD_lineSegment *leftLane_info, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);
	if (node == NULL)
	{
		return NULL;
	}

	child = create_VD_point(&leftLane_info->startPoint, (char*) "startPoint");
	xmlAddChild(node, child);

	child = create_VD_point(&leftLane_info->endPoint, (char*) "endPoint");
	xmlAddChild(node, child);

	child = create_VD_line(&leftLane_info->line, (char*) "line");
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析VD_lineSegment结构体
 * 参数：
 * 	leftLane_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_rightLane_info(VD_lineSegment *rightLane_info,
                                 char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);
	if (node == NULL)
	{
		return NULL;
	}

	child = create_VD_point(&rightLane_info->startPoint, (char*) "startPoint");
	xmlAddChild(node, child);

	child = create_VD_point(&rightLane_info->endPoint, (char*) "endPoint");
	xmlAddChild(node, child);

	child = create_VD_line(&rightLane_info->line, (char*) "line");
	xmlAddChild(node, child);

	return node;
}

/**
 * 解析Leftturn_waiting_zone结构体
 * 参数：
 * 	pLeftturn_waiting_zone[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Leftturn_waiting_zone(
    Leftturn_waiting_zone *pLeftturn_waiting_zone, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "shape");
	sprintf(txt, "%d", pLeftturn_waiting_zone->shape);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	int i;
	for (i=0; i < 4; i++)
	{
		child = create_VD_point(&pLeftturn_waiting_zone->point[i],
		                        (char*) "point");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	for (i=0; i < 4; i++)
	{
		child
		    = create_VD_line(&pLeftturn_waiting_zone->line[i],
		                     (char*) "line");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Lane_info结构体
 * 参数：
 * 	pLane_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Lane_info(Lane_info *pLane_info, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "direction");
	sprintf(txt, "%d", pLane_info->direction);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "up_down");
	sprintf(txt, "%d", pLane_info->up_down);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "nature");
	sprintf(txt, "%d", pLane_info->nature);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = create_Leftturn_waiting_zone(&pLane_info->waiting_zone,
	                                     (char*) "waiting_zone");
	xmlAddChild(node, child);

	int i;
	for (i=0; i < 2; i++)
	{
		child
		    = create_Lane_line(&pLane_info->lane_line[i],
		                       (char*) "lane_line");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Detectarea_info结构体
 * 参数：
 * 	pDetectarea_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Detectarea_info(Detectarea_info *pDetectarea_info,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "lane_num");
	sprintf(txt, "%d", pDetectarea_info->lane_num);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "first_lane_no");
	sprintf(txt, "%d", pDetectarea_info->first_lane_no);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "before_stopline");
	sprintf(txt, "%d", pDetectarea_info->before_stopline);
	content = xmlNewText((xmlChar*) txt);
	xmlAddChild(child, content);
	xmlAddChild(node, child);

	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "stopline");

		if (child)
		{
			sprintf(txt, "%d", pDetectarea_info->stopline[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
		}

		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "after_stopline");
	if (child)
	{
		sprintf(txt, "%d", pDetectarea_info->after_stopline);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "laneLeftNature");
	if (child)
	{
		sprintf(txt, "%d", pDetectarea_info->laneLeftNature);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "laneRightNature");
	if (child)
	{
		sprintf(txt, "%d", pDetectarea_info->laneRightNature);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = create_Lane_info(&pDetectarea_info->lane_info[i],
		                         (char*) "lane_info");

		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = create_VD_rect(&pDetectarea_info->detect_area,
	                       (char*) "detect_area");
	xmlAddChild(node, child);

	child = create_VD_rect(&pDetectarea_info->zebra_area, (char*) "zebra_area");
	xmlAddChild(node, child);

//	for (i=0; i < 3; i++)
//	{
//		child = create_VD_polygon(&pDetectarea_info->safety_area[i],
//		                          (char*) "safety_area");
//
//		if (child)
//		{
//			sprintf(txt, "%d", i + 1);
//			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
//			xmlAddChild(node, child);
//		}
//	}
//
//	for (i=0; i < 4; i++)
//	{
//		child = create_VD_polygon(&pDetectarea_info->congestion_area[i],
//		                          (char*) "congestion_area");
//
//		if (child)
//		{
//			sprintf(txt, "%d", i + 1);
//			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
//			xmlAddChild(node, child);
//		}
//	}
//
//	for (i=0; i < 4; i++)
//	{
//		child = create_VD_polygon(&pDetectarea_info->no_parking_area[i],
//		                          (char*) "no_parking_area");
//
//		if (child)
//		{
//			sprintf(txt, "%d", i + 1);
//			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
//			xmlAddChild(node, child);
//		}
//	}

	child = create_VD_polygon(&pDetectarea_info->safety_area,
							  (char*) "safety_area");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_VD_polygon(&pDetectarea_info->congestion_area,
							  (char*) "congestion_area");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_VD_polygon(&pDetectarea_info->parking_area,
							  (char*) "no_parking_area");
	if (child)
	{
		xmlAddChild(node, child);
	}





	child = create_leftLane_info(&pDetectarea_info->leftLineInfo,
	                             (char*) "leftLineInfo");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_rightLane_info(&pDetectarea_info->rightLineInfo,
	                              (char*) "rightLineInfo");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
	return 0;
}

/**
 * 解析Signal_attribute结构体
 * 参数：
 * 	pSignal_attribute[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Signal_attribute(Signal_attribute *pSignal_attribute,
                                   char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = create_VD_rect(&pSignal_attribute->rect, (char*) "rect");
	xmlAddChild(node, child);

	child = xmlNewNode(NULL, (xmlChar*) "color");
	if (child)
	{
		sprintf(txt, "%d", pSignal_attribute->color);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "type");
	if (child)
	{
		sprintf(txt, "%d", pSignal_attribute->type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "direction");
	if (child)
	{
		sprintf(txt, "%d", pSignal_attribute->direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "group");
	if (child)
	{
		sprintf(txt, "%d", pSignal_attribute->group);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Signal_detect_video结构体
 * 参数：
 * 	pSignal_detect_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Signal_detect_video(
    Signal_detect_video *pSignal_detect_video, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "signal_num");
	if (child)
	{
		sprintf(txt, "%d", pSignal_detect_video->signal_num);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_VD_rect(&pSignal_detect_video->detect_area,
	                       (char*) "detect_area");
	if (child)
	{
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < SIGNAL_MAX_NUM; i++)
	{
		child = create_Signal_attribute(&pSignal_detect_video->signals[i],
		                                (char*) "signals");

		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析Signal_detect结构体
 * 参数：
 * 	pSignal_detect[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Signal_detect(Signal_detect *pSignal_detect, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "mode");
	if (child)
	{
		sprintf(txt, "%d", pSignal_detect->mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_signal_detect");
	if (child)
	{
		sprintf(txt, "%d", pSignal_detect->is_signal_detect);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Signal_detect_video(&pSignal_detect->signal_detect_video,
	                                   (char*) "signal_detect_video");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Plate_size结构体
 * 参数：
 * 	pPlate_size[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Plate_size(Plate_size *pPlate_size, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "max_width");
	if (child)
	{
		sprintf(txt, "%d", pPlate_size->max_width);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "max_height");
	if (child)
	{
		sprintf(txt, "%d", pPlate_size->max_height);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "min_width");
	if (child)
	{
		sprintf(txt, "%d", pPlate_size->min_width);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "min_height");
	if (child)
	{
		sprintf(txt, "%d", pPlate_size->min_height);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Plate结构体
 * 参数：
 * 	pPlate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Plate(Plate *pPlate, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "str_default_prov");
	if (child)
	{
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pPlate->str_default_prov, 4, (char*) txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Plate_size(&pPlate->plate_size, (char*) "plate_size");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析VehicleDet_video结构体
 * 参数：
 * 	pVehicleDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VehicleDet_video(VehicleDet_video *pVehicleDet_video,
                                   char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_out");
	if (child)
	{
		sprintf(txt, "%d", pVehicleDet_video->is_vehicle_out);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "vehiCapture_level");
	if (child)
	{
		sprintf(txt, "%d", pVehicleDet_video->vehiCapture_level);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "hspeed_type");
	if (child)
	{
		sprintf(txt, "%d", pVehicleDet_video->hspeed_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析VehicleDet_loop结构体
 * 参数：
 * 	pVehicleDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_VehicleDet_loop(VehicleDet_loop *pVehicleDet_loop,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops");

		if (child)
		{
			sprintf(txt, "%d", pVehicleDet_loop ->loops[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "plate_holdTime");
	if (child)
	{
		sprintf(txt, "%d", pVehicleDet_loop ->plate_holdTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "signal_holdTime");
	if (child)
	{
		sprintf(txt, "%d", pVehicleDet_loop ->signal_holdTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析_VEHICLE_UNION结构体
 * 参数：
 * 	p_VEHICLE_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create__VEHICLE_UNION(union _VEHICLE_UNION *p_VEHICLE_UNION,
                                 char *pNodeName, int mode)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	if (mode == 0) //视频
	{
		child = create_VehicleDet_video(&p_VEHICLE_UNION->det_video,
		                                (char*) "det_video");
		if (child)
		{
			xmlAddChild(node, child);
		}
	}
	else if (mode == 1) //线圈
	{
		child = create_VehicleDet_loop(&p_VEHICLE_UNION->det_loop,
		                               (char*) "det_loop");
		if (child)
		{
			xmlAddChild(node, child);
		}
	}
	return node;
}

/**
 * 解析Traffic_object_det结构体
 * 参数：
 * 	pTraffic_object_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Traffic_object_det(Traffic_object_det *pTraffic_object_det,
                                     char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容
	child = xmlNewNode(NULL, (xmlChar*) "mode");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_object_det->mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create__VEHICLE_UNION(&pTraffic_object_det->vehicle_union,
	                              (char*) "vehicle_union", pTraffic_object_det->mode);
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Plate(&pTraffic_object_det->plate_det, (char*) "plate_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "vehicle_statis_cycle");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_object_det->vehicle_statis_cycle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "isdetection_novehicle");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_object_det->isdetection_novehicle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "isdetection_pedestrian");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_object_det->isdetection_pedestrian);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "pedestrian_statis_cycle");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_object_det->pedestrian_statis_cycle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析RunredDet_video结构体
 * 参数：
 * 	pRunredDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_RunredDet_video(RunredDet_video *pRunredDet_video,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_run_red");
	if (child)
	{
		sprintf(txt, "%d", pRunredDet_video->is_run_red);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_run_yellow");
	if (child)
	{
		sprintf(txt, "%d", pRunredDet_video->is_run_yellow);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "one_run_red");
		if (child)
		{
			sprintf(txt, "%d", pRunredDet_video->one_run_red[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析RunredDet_loop结构体
 * 参数：
 * 	pRunredDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_RunredDet_loop(RunredDet_loop *pRunredDet_loop,
                                 char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops_first");
		if (child)
		{
			sprintf(txt, "%d", pRunredDet_loop->loops_first[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops_second");
		if (child)
		{
			sprintf(txt, "%d", pRunredDet_loop->loops_second[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析RUNREDDET_UNION结构体
 * 参数：
 * 	pRUNREDDET_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_RUNREDDET_UNION(union RUNREDDET_UNION *pRUNREDDET_UNION,
                                  char *pNodeName, int mode)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}
	if (mode == 0)//视频
	{
		child = create_RunredDet_video(&pRUNREDDET_UNION->video,
		                               (char*) "video");
		if (child)
		{
			xmlAddChild(node, child);
		}
	}
	else if (mode == 1)//线圈
	{
		child = create_RunredDet_loop(&pRUNREDDET_UNION->loop, (char*) "loop");
		if (child)
		{
			xmlAddChild(node, child);
		}
	}
	return node;
}

/**
 * 解析RunredDet结构体
 * 参数：
 * 	pRunredDet[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_RunredDet(RunredDet *pRunredDet, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	//取出节点中的内容

	child = xmlNewNode(NULL, (xmlChar*) "mode");
	if (child)
	{
		sprintf(txt, "%d", pRunredDet->mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_RUNREDDET_UNION(&pRunredDet->runRed_union,
	                               (char*) "runRed_union", pRunredDet->mode);
	if (child)
	{
		xmlAddChild(node, child);
	}

	return 0;
}

/**
 * 解析SpeedDet_loop结构体
 * 参数：
 * 	pSpeedDet_loop[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SpeedDet_loop(SpeedDet_loop *pSpeedDet_loop, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "use_3_loops");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet_loop ->use_3_loops);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops_first");
		if (child)
		{
			sprintf(txt, "%d", pSpeedDet_loop ->loops_first[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops_second");
		if (child)
		{
			sprintf(txt, "%d", pSpeedDet_loop->loops_second[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "loops_third");
		if (child)
		{
			sprintf(txt, "%d", pSpeedDet_loop->loops_third[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析SpeedDet_radio结构体
 * 参数：
 * 	pSpeedDet_radio[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SpeedDet_radio(SpeedDet_radio *pSpeedDet_radio,
                                 char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "can_det_lane");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet_radio->can_det_lane);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "seri_port");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet_radio->seri_port);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "radio_type");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet_radio->radio_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Inter_coeff结构体
 * 参数：
 * 	pInter_coeff[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Inter_coeff(Inter_coeff *pInter_coeff, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "internum");
	if (child)
	{
		sprintf(txt, "%d", pInter_coeff->internum);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 6; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "x");
		if (child)
		{
			sprintf(txt, "%.3f", pInter_coeff->x[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 6; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "y");
		if (child)
		{
			sprintf(txt, "%.3f", pInter_coeff->y[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 6; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "h");
		if (child)
		{
			sprintf(txt, "%.3f", pInter_coeff->h[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 6; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "m");
		if (child)
		{
			sprintf(txt, "%.3f", pInter_coeff->m[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析CaliInfo_point结构体
 * 参数：
 * 	pCaliInfo_point[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_CaliInfo_point(CaliInfo_point *pCaliInfo_point,
                                 char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "x");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->x);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "y");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->y);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "x_distance");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->x_distance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "y_distance");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->y_distance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "xpixel_distance");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->xpixel_distance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "ypixel_distance");
	if (child)
	{
		sprintf(txt, "%d", pCaliInfo_point->ypixel_distance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "rowline_slope");
	if (child)
	{
		sprintf(txt, "%.3f", pCaliInfo_point->rowline_slope);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "colline_slope");
	if (child)
	{
		sprintf(txt, "%.3f", pCaliInfo_point->colline_slope);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析SpeedDet_video结构体
 * 参数：
 * 	pSpeedDet_video[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SpeedDet_video(SpeedDet_video *pSpeedDet_video,
                                 char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < 4; i++)
	{
		child = create_Inter_coeff(&pSpeedDet_video->h_function[i],
		                           (char*) "h_function");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 6; i++)
	{
		child = create_Inter_coeff(&pSpeedDet_video->v_function[i],
		                           (char*) "v_function");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 4; i++)
	{
		int j;
		for (j = 0; j < 6; j++)
		{
			child = create_CaliInfo_point(
			            &pSpeedDet_video->caliinfo_point[i][j],
			            (char*) "caliinfo_point");
			if (child)
			{
				sprintf(txt, "%d", i);
				xmlNewProp(child, (xmlChar*) "x", (xmlChar*) txt);
				sprintf(txt, "%d", j);
				xmlNewProp(child, (xmlChar*) "y", (xmlChar*) txt);
				xmlAddChild(node, child);
			}
		}
	}

	return node;
}

/**
 * 解析SPEED_UNION结构体
 * 参数：
 * 	pSPEED_UNION[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SPEED_UNION(union SPEED_UNION *pSPEED_UNION, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = create_SpeedDet_video(&pSPEED_UNION->speed_video,
	                              (char*) "speed_video");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_SpeedDet_loop(&pSPEED_UNION->speed_loop,
	                             (char*) "speed_loop");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_SpeedDet_radio(&pSPEED_UNION->speed_radio,
	                              (char*) "speed_radio");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析SpeedDet结构体
 * 参数：
 * 	pSpeedDet[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SpeedDet(SpeedDet *pSpeedDet, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "mode");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet->mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_measure_speed");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet->is_measure_speed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_overspeed");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet->is_overspeed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "limitspeed");
	if (child)
	{
		sprintf(txt, "%d", pSpeedDet->limitspeed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_SPEED_UNION(&pSpeedDet->speed_union, (char*) "speed_union");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Time_restriction结构体
 * 参数：
 * 	pTime_restriction[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Time_restriction(Time_restriction *pTime_restriction,
                                   char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "sigle_double_date");
	if (child)
	{
		sprintf(txt, "%d", pTime_restriction->sigle_double_date);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_date_restriction");
	if (child)
	{
		sprintf(txt, "%d", pTime_restriction->is_date_restriction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "week_restriction");
	if (child)
	{
		sprintf(txt, "%d", pTime_restriction->week_restriction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_time_restriction");
	if (child)
	{
		sprintf(txt, "%d", pTime_restriction->is_time_restriction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_VD_daysegment(&pTime_restriction->date_segment,
	                             (char*) "date_segment");
	if (child)
	{
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 4; i++)
	{
		child = create_VD_timesegment(&pTime_restriction->time_segment[i],
		                              (char*) "time_segment");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析LimitTravel_one结构体
 * 参数：
 * 	pLimitTravel_one[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_LimitTravel_one(LimitTravel_one *pLimitTravel_one,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "lane");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->lane);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_time_restrict");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->is_time_restrict);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Time_restriction(&pLimitTravel_one->time_restrict,
	                                (char*) "time_restrict");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_tailnumber");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->is_tailnumber);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 10; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "apointed_tailnumber");
		if (child)
		{
			sprintf(txt, "%d", pLimitTravel_one->apointed_tailnumber[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);

			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "move_direct");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->move_direct);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "turning_direct");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->turning_direct);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "vehicle_type");
	if (child)
	{
		sprintf(txt, "%d", pLimitTravel_one->vehicle_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Limit_travel_det结构体
 * 参数：
 * 	pLimit_travel_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Limit_travel_det(Limit_travel_det *pLimit_travel_det,
                                   char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "restriction_num");
	if (child)
	{
		sprintf(txt, "%d", pLimit_travel_det->restriction_num);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < pLimit_travel_det->restriction_num; i++) //pLimit_travel_det->restriction_num  100
	{
		child = create_LimitTravel_one(&pLimit_travel_det->restriction_info[i],
		                               (char*) "restriction_info");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析Event_det结构体
 * 参数：
 * 	pEvent_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Event_det(Event_det *pEvent_det, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_converse_drive");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_converse_drive);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_cover_line");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_cover_line);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < MAX_LANE_NUM + 1; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "one_cover_line");
		if (child)
		{
			sprintf(txt, "%d", pEvent_det->one_cover_line[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_over_safety_strip");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_over_safety_strip);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_illegal_park");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_illegal_park);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_cross_stop_line");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_cross_stop_line);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "one_cross_stop_line");
		if (child)
		{
			sprintf(txt, "%d", pEvent_det->one_cross_stop_line[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_illegal_lane_run");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_illegal_lane_run);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "one_lane_run");
		if (child)
		{
			sprintf(txt, "%d", pEvent_det->one_lane_run[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_occupy_special");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_occupy_special);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_change_lane");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_change_lane);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_NUM + 1; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "one_change_lane");
		if (child)
		{
			sprintf(txt, "%d", pEvent_det->one_change_lane[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_occupy_nonmotor");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_occupy_nonmotor);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_forced_cross");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_forced_cross);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_left_wait_zone");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_left_wait_zone);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_park_outof_LTWA");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_park_outof_LTWA);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_limit_travel");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_limit_travel);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_notwayto_pedes");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->is_notwayto_pedes);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_park_time");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->illegal_park_time);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "cross_stop_line_time");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->cross_stop_line_time);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "event_alarm_enable");
	if (child)
	{
		sprintf(txt, "%d", pEvent_det->event_alarm_enable);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < 20; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "event_order");
		if (child)
		{
			sprintf(txt, "%d", pEvent_det->event_order[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析Traffic_envent_det结构体
 * 参数：
 * 	pTraffic_envent_det[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Traffic_event_det(Traffic_event_det *pTraffic_envent_det,
                                    char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_event_det");
	if (child)
	{
		sprintf(txt, "%d", pTraffic_envent_det->is_event_det);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_RunredDet(&pTraffic_envent_det->run_red_det,
	                         (char*) "run_red_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_SpeedDet(&pTraffic_envent_det->speed_det,
	                        (char*) "speed_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Limit_travel_det(&pTraffic_envent_det->limit_travel_det,
	                                (char*) "limit_travel_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Event_det(&pTraffic_envent_det->event_det,
	                         (char*) "event_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Camera_arithm结构体
 * 参数：
 * 	pCamera_arithm[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Camera_arithm(Camera_arithm *pCamera_arithm, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_arithm_set");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->is_arithm_set);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "camera_type");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->camera_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "image_width");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->image_width);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "image_height");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->image_height);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "image_type");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->image_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "image_quality");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->image_quality);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "frame_rate");
	if (child)
	{
		sprintf(txt, "%d", pCamera_arithm->frame_rate);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Violate_description结构体
 * 参数：
 * 	pViolate_description[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Violate_description(
    Violate_description *pViolate_description, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_use");
	if (child)
	{
		sprintf(txt, "%d", pViolate_description->is_use);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exceed_peed");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->exceed_peed, strlen(
		                (const char*) pViolate_description->exceed_peed), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "run_red");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->run_red, strlen(
		                (const char*) pViolate_description->run_red), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "converse_drive");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->converse_drive, strlen(
		                (const char*) pViolate_description->converse_drive),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_lane_run");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->illegal_lane_run, strlen(
		                (const char*) pViolate_description->illegal_lane_run),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "cover_line");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->cover_line, strlen(
		                (const char*) pViolate_description->cover_line), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "change_lane");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->change_lane, strlen(
		                (const char*) pViolate_description->change_lane), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "cross_stop_line");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->cross_stop_line, strlen(
		                (const char*) pViolate_description->cross_stop_line),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_park");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->illegal_park, strlen(
		                (const char*) pViolate_description->illegal_park), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "over_safety_strip");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->over_safety_strip, strlen(
		                (const char*) pViolate_description->over_safety_strip),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "occupy_special");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->occupy_special, strlen(
		                (const char*) pViolate_description->occupy_special),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "occupy_no_motor");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->occupy_no_motor, strlen(
		                (const char*) pViolate_description->occupy_no_motor),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "forced_cross");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->forced_cross, strlen(
		                (const char*) pViolate_description->forced_cross), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "limit_travel");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->limit_travel, strlen(
		                (const char*) pViolate_description->limit_travel), txt,
		            sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "nowayto_pedes");
	if (child)
	{
		memset(txt, 0, sizeof(txt));
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pViolate_description->nowayto_pedes, strlen(
		                (const char*) pViolate_description->nowayto_pedes),
		            txt, sizeof(txt));
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析Type_info_color结构体
 * 参数：
 * 	pType_info_color[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Type_info_color(Type_info_color *pType_info_color,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "r");
	if (child)
	{
		sprintf(txt, "%d", pType_info_color->r);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "g");
	if (child)
	{
		sprintf(txt, "%d", pType_info_color->g);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "b");
	if (child)
	{
		sprintf(txt, "%d", pType_info_color->b);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析Overlay_violate结构体
 * 参数：
 * 	pOverlay_violate[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Overlay_violate(Overlay_violate *pOverlay_violate,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "start_x");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->start_x);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "start_y");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->start_y);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_spot_name");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_spot_name);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_device_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_device_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_time");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_time);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_redlamp_starttime");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_redlamp_starttime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_redlamp_keeptime");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_redlamp_keeptime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_direction");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_lane_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_lane_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_lane_direction");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_lane_direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_plate_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_color");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_plate_color);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_type");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_plate_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_speed");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_speed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "pic_num");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->pic_num);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Type_info_color(&pOverlay_violate->color, (char*) "color");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "font_size");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->font_size);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Violate_description(&pOverlay_violate->violate_desc,
	                                   (char*) "violate_desc");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_color");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_vehicle_color);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_logo");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_vehicle_logo);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_type");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_violate->is_vehicle_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Overlay_vehicle结构体
 * 参数：
 * 	pOverlay_vehicle[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Overlay_vehicle(Overlay_vehicle *pOverlay_vehicle,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_overlay_vehicle");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_overlay_vehicle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "start_x");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->start_x);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "start_y");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->start_y);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_spot_name");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_spot_name);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_device_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_device_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_time");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_time);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_direction");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_lane_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_lane_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_lane_direction");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_lane_direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_numble");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_plate_numble);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_color");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_plate_color);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_plate_type");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_plate_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_speed");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_speed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Type_info_color(&pOverlay_vehicle->color, (char*) "color");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "font_size");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->font_size);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_color");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_vehicle_color);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_logo");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_vehicle_logo);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_vehicle_type");
	if (child)
	{
		sprintf(txt, "%d", pOverlay_vehicle->is_vehicle_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}
xmlNodePtr create_Strobe(Strobe *pStrobe, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < 2; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "lane");
		if (child)
		{
			sprintf(txt, "%d", pStrobe->lane[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "address");
	if (child)
	{
		sprintf(txt, "%d", pStrobe->address);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}

xmlNodePtr create_Strobe_param(Strobe_param *pStrobe_param, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "strobe_switch");
		if (child)
		{
			sprintf(txt, "%d", pStrobe_param->strobe_switch[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "strobe_count");
	if (child)
	{
		sprintf(txt, "%d", pStrobe_param->strobe_count);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_LANE_NUM; i++)
	{
		child = create_Strobe(&pStrobe_param->strobe[i], (char*) "strobe");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}
	return node;
}

xmlNodePtr create_signalController_param(SignalControllerParam *pSignal_param, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}
	child = xmlNewNode(NULL, (xmlChar*) "realtime_mode");
	if (child)
	{
		sprintf(txt, "%d", pSignal_param->realtimeMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "car_occupy_flag");
	if (child)
	{
		sprintf(txt, "%d", pSignal_param->m_car_occupy_flag);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "small_car_length");
	if (child)
	{
		sprintf(txt, "%d", pSignal_param->m_small_car_length);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "big_car_length");
	if (child)
	{
		sprintf(txt, "%d", pSignal_param->m_big_car_length);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "motor_occupy_flag");
	if (child)
	{
		sprintf(txt, "%d", pSignal_param->m_motor_occupy_flag);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析DeviceConfig结构体
 * 参数：
 * 	pDeviceConfig[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_DeviceConfig(DeviceConfig *pDeviceConfig, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);
	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "strSpotName");
	if (child)
	{
		convert_enc((char*) "GBK", (char*) "UTF-8",
		            (char*) pDeviceConfig->strSpotName, strlen(
		                pDeviceConfig->strSpotName), txt, sizeof(txt));

		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "spotID");
	if (child)
	{
		sprintf(txt, "%s", pDeviceConfig->spotID);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "deviceID");
	if (child)
	{
		sprintf(txt, "%s", pDeviceConfig->deviceID);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "direction");
	if (child)
	{
		sprintf(txt, "%d", pDeviceConfig->direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "startLane");
	if (child)
	{
		sprintf(txt, "%d", pDeviceConfig->startLane);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "encodeType");
	if (child)
	{
		sprintf(txt, "%d", pDeviceConfig->encodeType);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "dynamicAdjustFlag");
	if (child)
	{
		sprintf(txt, "%d", pDeviceConfig->dynamicAdjustFlag);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "imgSizeUpperLimit");
	if (child)
	{
		sprintf(txt, "%d", pDeviceConfig->imgSizeUpperLimit);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Strobe_param(&pDeviceConfig->strobe_param,
	                            (char*) "strobe_param");
	if (child)
	{
		xmlAddChild(node, child);
	}
	child = create_signalController_param(&pDeviceConfig->signalControllerParam,
	                                      (char*) "signalControllerParam");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;

}
/**
 * 解析DSP_config结构体
 * 参数：
 * 	pDSP_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_DSP_config(VDCS_config *pDSP_config, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "version");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->version);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_DeviceConfig(&pDSP_config->device_config,
	                            (char*) "device_config");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Detectarea_info(&pDSP_config->detectarea_info,
	                               (char*) "detectarea_info");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Signal_detect(&pDSP_config->signal_detect,
	                             (char*) "signal_detect");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Camera_arithm(&pDSP_config->camera_arithm,
	                             (char*) "camera_arithm");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Traffic_object_det(&pDSP_config->traffic_object,
	                                  (char*) "traffic_object_det");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Traffic_event_det(&pDSP_config->traffic_event,
	                                 (char*) "traffic_event");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Overlay_violate(&pDSP_config->overlay_violate,
	                               (char*) "overlay_violate");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_Overlay_vehicle(&pDSP_config->overlay_vehicle,
	                               (char*) "overlay_vehicle");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}



//////////////////////////////////// PD config //////////////////////////////////////////////

xmlNodePtr create_VD_CaptureRule(VD_CaptureRule *pCaptureRule, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "capturePicNum");
	if (child)
	{
		sprintf(txt, "%d", pCaptureRule->capturePicNum);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "minConfirmTime");
	if (child)
	{
		sprintf(txt, "%d", pCaptureRule->minConfirmTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "videoTime");
	if (child)
	{
		sprintf(txt, "%d", pCaptureRule->videoTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < MAX_CAPTURE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "captureTime");
		if (child)
		{
			sprintf(txt, "%d", pCaptureRule->captureTime[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "plateMatchRatio");
	if (child)
	{
		sprintf(txt, "%d", pCaptureRule->plateMatchRatio);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_CAPTURE_NUM; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "captureMode");
		if (child)
		{
			sprintf(txt, "%d", pCaptureRule->captureMode[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "ptzTraceMode");
	if (child)
	{
		sprintf(txt, "%d", pCaptureRule->ptzTraceMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}


xmlNodePtr create_VD_ParkRegion(VD_ParkRegion *pParkRegion, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;//, content;
//	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = create_VD_polygon(&pParkRegion->detectRegions,
	                            (char*) "detectRegions");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_VD_CaptureRule(&pParkRegion->captureRule,
	                            (char*) "captureRule");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}


xmlNodePtr create_VD_PTZinfo(VD_PTZinfo *pPTZinfo, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "PTZid");
	if (child)
	{
		sprintf(txt, "%d", pPTZinfo->PTZid);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "focalDistance");
	if (child)
	{
		sprintf(txt, "%d", pPTZinfo->focalDistance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}


xmlNodePtr create_VD_DetConfig(VD_DetConfig *pDetConfig, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "carNum");
	if (child)
	{
		sprintf(txt, "%d", pDetConfig->carNum);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i=0;
	for (i=0; i < 2; i++)
	{
		child = create_VD_rect(&pDetConfig->carRects[i], (char*) "carRects");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "regionNum");
	if (child)
	{
		sprintf(txt, "%d", pDetConfig->regionNum);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	for (i=0; i < MAX_REGION_NUM; i++)
	{
		child = create_VD_ParkRegion(&pDetConfig->parkRegion[i], (char*) "parkRegion");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}


xmlNodePtr create_VD_PresetPos(VD_PresetPos *pPresetPos, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "presetID");
	if (child)
	{
		sprintf(txt, "%d", pPresetPos->presetID);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "isAdjust");
	if (child)
	{
		sprintf(txt, "%d", pPresetPos->isAdjust);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_VD_PTZinfo(&pPresetPos->pTZinfo,
	                            (char*) "pTZinfo");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_VD_DetConfig(&pPresetPos->detConfig,
	                            (char*) "detConfig");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}


xmlNodePtr create_VD_PresetPosOrder(VD_PresetPosOrder *pPresetPosOrder, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "presetPosID");
	if (child)
	{
		sprintf(txt, "%d", pPresetPosOrder->presetPosID);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "timeInterval");
	if (child)
	{
		sprintf(txt, "%d", pPresetPosOrder->timeInterval);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}


xmlNodePtr create_VD_TimeSchedule(VD_TimeSchedule *pTimeSchedule, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "starTime");
	if (child)
	{
		sprintf(txt, "%d", pTimeSchedule->starTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "endTime");
	if (child)
	{
		sprintf(txt, "%d", pTimeSchedule->endTime);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i=0;
	for (i=0; i < 24; i++)
	{
		child = create_VD_PresetPosOrder(&pTimeSchedule->presetPosOrder[i], (char*) "presetPosOrder");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}


xmlNodePtr create_VD_PresetPosRotation(VD_PresetPosRotation *pPresetPosRotation, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "rotationMode");
	if (child)
	{
		sprintf(txt, "%d", pPresetPosRotation->rotationMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "timeInterval");
	if (child)
	{
		sprintf(txt, "%d", pPresetPosRotation->timeInterval);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i=0;
	for (i=0; i < 24; i++)
	{
		child = create_VD_TimeSchedule(&pPresetPosRotation->timeSchedue[i], (char*) "timeSchedue");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}


xmlNodePtr create_VD_PTZCtrlInfo(VD_PTZCtrlInfo *pPTZCtrlInfo, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "PTZHorSpeed");
	if (child)
	{
		sprintf(txt, "%d", pPTZCtrlInfo->PTZHorSpeed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "PTZVerSpeed");
	if (child)
	{
		sprintf(txt, "%d", pPTZCtrlInfo->PTZVerSpeed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "focusSpeed");
	if (child)
	{
		sprintf(txt, "%d", pPTZCtrlInfo->focusSpeed);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}


/**
 * 解析DSP_config结构体
 * 参数：
 * 	pDSP_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_DSP_PD_config(VD_IllegalParkingConfig *pDSP_config, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	debug("in create_DSP_PD_config\n");

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "version");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->version);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  1\n");
	child = xmlNewNode(NULL, (xmlChar*) "cfgParamChanged");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->cfgParamChanged);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "presetNum");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->presetNum);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  2\n");
	int i=0;
	for (i=0; i < MAX_PRESET_NUM; i++)
	{
		child = create_VD_PresetPos(&pDSP_config->presetPos[i], (char*) "presetPos");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "workMode");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->workMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "camerMode");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->camerMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "detectMode");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->detectMode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "detectprePos");
	if (child)
	{
		sprintf(txt, "%d", pDSP_config->detectprePos);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}



	for (i=0; i < 7; i++)
	{
		child = create_VD_PresetPosRotation(&pDSP_config->presetPosRotation[i], (char*) "presetPosRotation");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}
	debug("in create_DSP_PD_config  3\n");
	child = create_VD_PTZCtrlInfo(&pDSP_config->pTZCtrlInfo,
	                            (char*) "pTZCtrlInfo");
	if (child)
	{
		xmlAddChild(node, child);
	}


	child = create_Camera_arithm(&pDSP_config->camera_arithm,
	                             (char*) "camera_arithm");
	if (child)
	{
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  4\n");
	child = create_Traffic_event_det(&pDSP_config->traffic_event,
	                                 (char*) "traffic_event");
	if (child)
	{
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  4.1\n");
	child = create_Overlay_violate(&pDSP_config->overlay_violate,
	                               (char*) "overlay_violate");
	if (child)
	{
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  4.2\n");
	child = create_DeviceConfig(&pDSP_config->device_config,
	                            (char*) "device_config");
	if (child)
	{
		xmlAddChild(node, child);
	}
	debug("in create_DSP_PD_config  5\n");
	return node;
}

//////////////////////////////////// PD config end //////////////////////////////////////////////






////========================= ISP.h =================================

/**
 * 解析s_ExpMode结构体
 * 参数：
 * 	ps_ExpMode[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_ExpMode(s_ExpMode *ps_ExpMode, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "flag");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpMode->flag);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析s_ExpManu结构体
 * 参数：
 * 	ps_ExpManu[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_ExpManu(s_ExpManu *ps_ExpManu, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpManu->exp);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "gain");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpManu->gain);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 创建s_ExpAuto结构体
 * 参数：
 * 	ps_ExpAuto[out]：创建的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_ExpAuto(s_ExpAuto *ps_ExpAuto, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "light_dt_mode");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->light_dt_mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp_max");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->exp_max);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "gain_max");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->gain_max);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp_mid");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->exp_mid);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "gain_mid");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->gain_mid);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp_min");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->exp_min);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "gain_min");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpAuto->gain_min);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 创建s_ExpWnd结构体
 * 参数：
 * 	ps_ExpWnd[out]： 创建的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_ExpWnd(s_ExpWnd *ps_ExpWnd, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "line1");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line1);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "line2");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line2);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line3");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line3);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line4");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line4);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line5");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line5);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line6");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line6);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line7");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line7);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "line8");
	if (child)
	{
		sprintf(txt, "%d", ps_ExpWnd->line8);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析s_AwbMode结构体
 * 参数：
 * 	ps_AwbMode[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_AwbMode(s_AwbMode *ps_AwbMode, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "flag");
	if (child)
	{
		sprintf(txt, "%d", ps_AwbMode->flag);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析s_AwbManu结构体
 * 参数：
 * 	ps_AwbManu[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_AwbManu(s_AwbManu *ps_AwbManu, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}
	child = xmlNewNode(NULL, (xmlChar*) "gain_r");
	if (child)
	{
		sprintf(txt, "%d", ps_AwbManu->gain_r);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "gain_g");
	if (child)
	{
		sprintf(txt, "%d", ps_AwbManu->gain_g);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "gain_b");
	if (child)
	{
		sprintf(txt, "%d", ps_AwbManu->gain_b);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析s_ColorParam结构体
 * 参数：
 * 	ps_ColorParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_s_ColorParam(s_ColorParam *ps_ColorParam, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "contrast");
	if (child)
	{
		sprintf(txt, "%d", ps_ColorParam->contrast);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "luma");
	if (child)
	{
		sprintf(txt, "%d", ps_ColorParam->luma);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "saturation");
	if (child)
	{
		sprintf(txt, "%d", ps_ColorParam->saturation);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

xmlNodePtr create_Syn_Info(Syn_Info *pSyn_Info, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_syn_open");
	if (child)
	{
		sprintf(txt, "%d", pSyn_Info->is_syn_open);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "phase");
	if (child)
	{
		sprintf(txt, "%d", pSyn_Info->phase);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

xmlNodePtr create_Lamp_Info(Lamp_Info *pLamp_Info, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < LAMP_COUNT; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "nMode");
		if (child)
		{
			sprintf(txt, "%d", pLamp_Info->nMode[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}

		child = xmlNewNode(NULL, (xmlChar*) "nLampType");
		if (child)
		{
			sprintf(txt, "%d", pLamp_Info->nLampType[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析Camera_config结构体
 * 参数：
 * 	pCamera_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Camera_config(Camera_config *pCamera_config, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = create_s_ExpMode(&pCamera_config->exp_mode, (char*) "exp_mode");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_ExpManu(&pCamera_config->exp_manu, (char*) "exp_manu");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_ExpAuto(&pCamera_config->exp_auto, (char*) "exp_auto");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_ExpWnd(&pCamera_config->exp_wnd, (char*) "exp_wnd");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_AwbMode(&pCamera_config->awb_mode, (char*) "awb_mode");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_AwbManu(&pCamera_config->awb_manu, (char*) "awb_manu");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_s_ColorParam(&pCamera_config->color_param,
	                            (char*) "color_param");
	if (child)
	{
		xmlAddChild(node, child);
	}
	child = create_Syn_Info(&pCamera_config->syn_info, (char*) "syn_info");
	if (child)
	{
		xmlAddChild(node, child);
	}
	child = create_Lamp_Info(&pCamera_config->lamp_info, (char*) "lamp_info");
	if (child)
	{
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析TYPE_FTP_CONFIG_PARAM结构体
 * 参数：
 * 	pTYPE_FTP_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_TYPE_FTP_CONFIG_PARAM(
    TYPE_FTP_CONFIG_PARAM *pTYPE_FTP_CONFIG_PARAM, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "user");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_FTP_CONFIG_PARAM->user);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "passwd");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_FTP_CONFIG_PARAM->passwd);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 4; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "ip");
		if (child)
		{
			sprintf(txt, "%d", pTYPE_FTP_CONFIG_PARAM->ip[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "port");
	if (child)
	{
		sprintf(txt, "%d", pTYPE_FTP_CONFIG_PARAM->port);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "allow_anonymous");
	if (child)
	{
		sprintf(txt, "%d", pTYPE_FTP_CONFIG_PARAM->allow_anonymous);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析TYPE_MQ_CONFIG_PARAM结构体
 * 参数：
 * 	pTYPE_MQ_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_TYPE_MQ_CONFIG_PARAM(
    TYPE_MQ_CONFIG_PARAM *pTYPE_MQ_CONFIG_PARAM, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < 4; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "ip");
		if (child)
		{
			sprintf(txt, "%d", pTYPE_MQ_CONFIG_PARAM->ip[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "port");
	if (child)
	{
		sprintf(txt, "%d", pTYPE_MQ_CONFIG_PARAM->port);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析TYPE_MQ_CONFIG_PARAM结构体
 * 参数：
 * 	pTYPE_MQ_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_TYPE_OSS_ALIYUN_CONFIG_PARAM(
	OSS_ALIYUN_PARAM *pTYPE_OSS_ALIYUN_CONFIG_PARAM, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "username");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_OSS_ALIYUN_CONFIG_PARAM->username);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "passwd");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_OSS_ALIYUN_CONFIG_PARAM->passwd);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "url");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_OSS_ALIYUN_CONFIG_PARAM->url);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "bucket_name");
	if (child)
	{
		sprintf(txt, "%s", pTYPE_OSS_ALIYUN_CONFIG_PARAM->bucket_name);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	return node;
}
/**
 * 解析DISK_WRI_DATA结构体
 * 参数：
 * 	pDISK_WRI_DATA[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_DISK_WRI_DATA(DISK_WRI_DATA *pDISK_WRI_DATA, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "remain_disk");
	if (child)
	{
		sprintf(txt, "%ld", pDISK_WRI_DATA->remain_disk);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_picture");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->illegal_picture);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "vehicle");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->vehicle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "event_picture");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->event_picture);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_video");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->illegal_video);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "event_video");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->event_video);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "flow_statistics");
	if (child)
	{
		sprintf(txt, "%d", pDISK_WRI_DATA->flow_statistics);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析FTP_DATA_CONFIG结构体
 * 参数：
 * 	pFTP_DATA_CONFIG[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_FTP_DATA_CONFIG(FTP_DATA_CONFIG *pFTP_DATA_CONFIG,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_picture");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->illegal_picture);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "vehicle");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->vehicle);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "event_picture");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->event_picture);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illegal_video");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->illegal_video);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "event_video");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->event_video);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "flow_statistics");
	if (child)
	{
		sprintf(txt, "%d", pFTP_DATA_CONFIG->flow_statistics);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

xmlNodePtr create_RESUME_UPLOAD_PARAM(RESUME_UPLOAD_PARAM *pResumeUpload_CFG,
                                      char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_resume_passcar");
	if (child)
	{
		sprintf(txt, "%d", pResumeUpload_CFG->is_resume_passcar);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_resume_illegal");
	if (child)
	{
		sprintf(txt, "%d", pResumeUpload_CFG->is_resume_illegal);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_resume_event");
	if (child)
	{
		sprintf(txt, "%d", pResumeUpload_CFG->is_resume_event);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_resume_statistics");
	if (child)
	{
		sprintf(txt, "%d", pResumeUpload_CFG->is_resume_statistics);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析DATA_SAVE结构体
 * 参数：
 * 	pDATA_SAVE[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_DATA_SAVE(DATA_SAVE *pDATA_SAVE, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = create_DISK_WRI_DATA(&pDATA_SAVE->disk_wri_data,
	                             (char *) "disk_wri_data");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_FTP_DATA_CONFIG(&pDATA_SAVE->ftp_data_config,
	                               (char *) "ftp_data_config");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_RESUME_UPLOAD_PARAM(&pDATA_SAVE->resume_upload_data,
	                                   (char *) "resume_upload_data");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析NTP_CONFIG_PARAM结构体
 * 参数：
 * 	pNTP_CONFIG_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_NTP_CONFIG_PARAM(NTP_CONFIG_PARAM *pNTP_CONFIG_PARAM,
                                   char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "useNTP");
	if (child)
	{
		sprintf(txt, "%d", pNTP_CONFIG_PARAM->useNTP);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	debug("doc useNTP ok\n");
	int i;
	for (i=0; i < 4; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "NTP_server_ip");
		if (child)
		{
			sprintf(txt, "%d", pNTP_CONFIG_PARAM->NTP_server_ip[i]);
			content = xmlNewText((xmlChar*) txt);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
		debug("doc NTP_server_ip[%d] ok\n", i);
	}

	child = xmlNewNode(NULL, (xmlChar*) "NTP_distance");
	if (child)
	{
		sprintf(txt, "%d", pNTP_CONFIG_PARAM->NTP_distance);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析BASIC_PARAM结构体
 * 参数：
 * 	pBASIC_PARAM[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_BASIC_PARAM(BASIC_PARAM *pBASIC_PARAM, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "monitor_type");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->monitor_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	debug("doc monitor_type ok\n");

	child = xmlNewNode(NULL, (xmlChar*) "spot_id");
	if (child)
	{
		sprintf(txt, "%s", pBASIC_PARAM->spot_id);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	debug("doc spot_id=%s ok\n",pBASIC_PARAM->spot_id);
	child = xmlNewNode(NULL, (xmlChar*) "road_id");
	if (child)
	{
		sprintf(txt, "%s", pBASIC_PARAM->road_id);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "spot");
	if (child)
	{
		convert_enc((char*) "GBK", (char*) "UTF-8", (char*) pBASIC_PARAM->spot,
		            strlen(pBASIC_PARAM->spot), (char*) txt, sizeof(txt));

		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "direction");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->direction);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	debug("child: %x, doc direction ok\n", *(int *)child);

	child = create_NTP_CONFIG_PARAM(&pBASIC_PARAM->ntp_config_param,
	                                (char*) "ntp_config_param");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp_type");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->exp_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "exp_device_id");
	if (child)
	{
		sprintf(txt, "%s", pBASIC_PARAM->exp_device_id);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "collect_actor_size");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->collect_actor_size);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}
	child = xmlNewNode(NULL, (xmlChar*) "log_level");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->log_level);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_DATA_SAVE(&pBASIC_PARAM->data_save, (char*) "data_save");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "h264_record");
	if (child)
	{
		sprintf(txt, "%d", pBASIC_PARAM->h264_record);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_pass_car,
	                                     (char*) "ftp_param_pass_car");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_illegal,
	                                     (char*) "ftp_param_illegal");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_TYPE_FTP_CONFIG_PARAM(&pBASIC_PARAM->ftp_param_h264,
	                                     (char*) "ftp_param_h264");
	if (child)
	{
		xmlAddChild(node, child);
	}

	// added by durd 2016-05-10
	child = create_TYPE_MQ_CONFIG_PARAM(&pBASIC_PARAM->mq_param,
			                             (char*) "mq_param");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child = create_TYPE_OSS_ALIYUN_CONFIG_PARAM(&pBASIC_PARAM->oss_aliyun_param,
			                             (char*) "oss_aliyun_param");
	if (child)
	{
		xmlAddChild(node, child);
	}
	return node;
}

/**
 * 解析SerialParam结构体
 * 参数：
 * 	pSerialParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_SerialParam(SerialParam *pSerialParam, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "dev_type");
	if (child)
	{
		sprintf(txt, "%d", pSerialParam->dev_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "bps");
	if (child)
	{
		sprintf(txt, "%ld", pSerialParam->bps);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "check");
	if (child)
	{
		sprintf(txt, "%d", pSerialParam->check);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "data");
	if (child)
	{
		sprintf(txt, "%d", pSerialParam->data);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "stop");
	if (child)
	{
		sprintf(txt, "%d", pSerialParam->stop);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析IOParam结构体
 * 参数：
 * 	pIOParam[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_IOParam(IO_cfg *pIOParam, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "trigger_type");
	if (child)
	{
		sprintf(txt, "%d", pIOParam->trigger_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "mode");
	if (child)
	{
		sprintf(txt, "%d", pIOParam->mode);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "io_drt");
	if (child)
	{
		sprintf(txt, "%d", pIOParam->io_drt);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Interface_Param结构体
 * 参数：
 * 	pInterface_Param[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Interface_Param(Interface_Param *pInterface_Param,
                                  char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < 3; i++)
	{
		child = create_SerialParam(&pInterface_Param->serial[i],
		                           (char*) "serial");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 8; i++)
	{
		child = create_IOParam(&pInterface_Param->io_input_params[i],
		                       (char*) "io_input_params");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	for (i=0; i < 4; i++)
	{
		child = create_IOParam(&pInterface_Param->io_output_params[i],
		                       (char*) "io_output_params");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析Osd_item结构体
 * 参数：
 * 	pOsd_item[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Osd_item(Osd_item *pOsd_item, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "switch_on");
	if (child)
	{
		sprintf(txt, "%d", pOsd_item->switch_on);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "x");
	if (child)
	{
		sprintf(txt, "%d", pOsd_item->x);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "y");
	if (child)
	{
		sprintf(txt, "%d", pOsd_item->y);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "is_time");
	if (child)
	{
		sprintf(txt, "%d", pOsd_item->is_time);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "content");
	if (child)
	{
		convert_enc((char*) "GBK", (char*) "UTF-8", (char*) pOsd_item->content,
		            strlen((char*) pOsd_item->content), txt, sizeof(txt));

		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析Osd_info结构体
 * 参数：
 * 	pOsd_info[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_Osd_info(Osd_info *pOsd_info, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = create_Type_info_color(&pOsd_info->color, (char *) "color");
	if (child)
	{
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 8; i++)
	{
		child = create_Osd_item(&pOsd_info->osd_item[i], (char *) "osd_item");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析H264_chanel结构体
 * 参数：
 * 	pH264_chanel[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_H264_chanel(H264_chanel *pH264_chanel, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "h264_on");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->h264_on);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "cast");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->cast);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < 4; i++)
	{
		child = xmlNewNode(NULL, (xmlChar*) "ip");
		if (child)
		{
			sprintf(txt, "%d", pH264_chanel->ip[i]);
			content = xmlNewText((xmlChar*) txt);
			xmlAddChild(child, content);
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	child = xmlNewNode(NULL, (xmlChar*) "port");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->port);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "fps");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->fps);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "rate");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->rate);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "width");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->width);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "height");
	if (child)
	{
		sprintf(txt, "%d", pH264_chanel->height);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = create_Osd_info(&pH264_chanel->osd_info, (char*) "osd_info");
	if (child)
	{
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析H264_config结构体
 * 参数：
 * 	pH264_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_H264_config(H264_config *pH264_config, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	int i;
	for (i=0; i < 2; i++)
	{
		child = create_H264_chanel(&pH264_config->h264_channel[i],
		                           (char*) "h264_channel");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 创建ILLEGAL_CODE_INFO结构体
 * 参数：
 * 	pInterface_Param[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_ILLEGAL_CODE_INFO(ILLEGAL_CODE_INFO *pillegal_code_info,
                                    char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child, content;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		return NULL;
	}

	child = xmlNewNode(NULL, (xmlChar*) "illeagal_type");
	if (child)
	{
		sprintf(txt, "%d", pillegal_code_info->illeagal_type);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	child = xmlNewNode(NULL, (xmlChar*) "illeagal_num");
	if (child)
	{
		sprintf(txt, "%d", pillegal_code_info->illeagal_num);
		content = xmlNewText((xmlChar*) txt);
		xmlAddChild(child, content);
		xmlAddChild(node, child);
	}

	return node;
}

/**
 * 解析ARM_config结构体
 * 参数：
 * 	pARM_config[out]： 解析的变量输出指针
 *  node［in］： 变量对应到节点
 */
xmlNodePtr create_ARM_config(ARM_config *pARM_config, char *pNodeName)
{
	xmlNodePtr node;
	xmlNodePtr child;
	char txt[256];

	node = xmlNewNode(NULL, (xmlChar *) pNodeName);

	if (node == NULL)
	{
		debug("node null\n");
		return NULL;
	}

	child = create_BASIC_PARAM(&pARM_config->basic_param,
	                           (char *) "basic_param");
	if (child)
	{
		xmlAddChild(node, child);
	}

	debug("basic_param ok\n");

	child = create_Interface_Param(&pARM_config->interface_param,
	                               (char*) "interface_param");
	if (child)
	{
		xmlAddChild(node, child);
	}

	child
	    = create_H264_config(&pARM_config->h264_config,
	                         (char*) "h264_config");
	if (child)
	{
		xmlAddChild(node, child);
	}

	int i;
	for (i=0; i < ILLEGAL_CODE_COUNT; i++)
	{
		child = create_ILLEGAL_CODE_INFO(&pARM_config->illegal_code_info[i],
		                                 (char*) "illegal_code_info");
		if (child)
		{
			sprintf(txt, "%d", i + 1);
			xmlNewProp(child, (xmlChar*) "id", (xmlChar*) txt);
			xmlAddChild(node, child);
		}
	}

	return node;
}

/**
 * 解析VDCS_config的xml文件
 * 参数：
 * 		filename： 要创建的xml文件
 * 		pVDCS_config： 的结构体
 * 返回：
 * 		0:成功， -1:失败
 */
int create_xml_file(const char *filename, VDConfigData *pDSP_config,
                    Camera_config* pCamera_config, ARM_config *pARM_config)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr child;

	debug("in create_xml_file\n");

	if (pDSP_config)
	{
		if(1 == pDSP_config->flag_func  )                 //  1:泊车
		
		{
			doc = create_xml_doc();
			
			if (NULL == doc)
			{
				log_error("XML", "create xml doc error. \n");
				return -1;
			}
			debug("DSP doc ok\n");
			
			root = create_root_node(doc, (char*) "vdcs_protocol");

			if (NULL == root)
			{				
        			log_error("XML", "error when create root node. \n");
				close_xml_doc(doc);
				return -1;
			}

			child = create_DSP_config(&pDSP_config->vdConfig.vdCS_config, (char*) "park_config");
			xmlAddChild(root, child);

			//xmlSaveFile(filename, doc);
			xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
			close_xml_doc(doc);
		}
	
		else if(2 == pDSP_config->flag_func  )       //  2 : 枪击卡口
		{
			doc = create_xml_doc();
			
			if (NULL == doc)
			{
				log_error("XML", "create xml doc error. \n");
				return -1;
			}
			debug("DSP doc ok\n");
			
			root = create_root_node(doc, (char*) "vdcs_protocol");
			
			if (NULL == root)
			{
				log_error("XML", "error when create root node. \n");
				close_xml_doc(doc);
				return -1;
			}

			child = create_DSP_config(&pDSP_config->vdConfig.vdCS_config, (char*) "VM_config");
			xmlAddChild(root, child);

			//xmlSaveFile(filename, doc);
			xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
			close_xml_doc(doc);
		}
		else if(3 == pDSP_config->flag_func  )      // 3 :  云台卡口
		{
			doc = create_xml_doc();
			
			if (NULL == doc)
			{
				log_error("XML", "create xml doc error. \n");
				return -1;
			}
			debug("DSP doc ok\n");
			
			root = create_root_node(doc, (char*) "vdcs_protocol");
			
			if (NULL == root)
			{
				log_error("XML", "error when create root node. \n");
				close_xml_doc(doc);
				return -1;
			}

			child = create_DSP_config(&pDSP_config->vdConfig.vdCS_config, (char*) "VM_P_config");
			xmlAddChild(root, child);

			//xmlSaveFile(filename, doc);
			xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
			close_xml_doc(doc);
		}
		else if(4 == pDSP_config->flag_func  )     //  4  :  违停
		{
			debug("in create_xml_file: to deal with illpk_config\n");
			doc = create_xml_doc();
			if (NULL == doc)
			{
				log_error("XML", "create xml doc error. \n");
				return -1;
			}
			debug("DSP PD doc ok\n");
			root = create_root_node(doc, (char*) "vdcs_protocol");
			if (NULL == root)
			{
				log_error("XML", "error when create root node. \n");
				close_xml_doc(doc);
				return -1;
			}

			child = create_DSP_PD_config(&pDSP_config->vdConfig.pdCS_config, (char*) "VP_config");
			xmlAddChild(root, child);

			//xmlSaveFile(filename, doc);
			xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
			close_xml_doc(doc);
		}
		else if(5 == pDSP_config->flag_func  )       //  VE
		{
			doc = create_xml_doc();
			
			if (NULL == doc)
			{
				log_error("XML", "create xml doc error. \n");
				return -1;
			}
			debug("DSP doc ok\n");
			
			root = create_root_node(doc, (char*) "vdcs_protocol");
			
			if (NULL == root)
			{
				log_error("XML", "error when create root node. \n");
				close_xml_doc(doc);
				return -1;
			}

			child = create_DSP_config(&pDSP_config->vdConfig.vdCS_config, (char*) "VE_config");
			xmlAddChild(root, child);

			//xmlSaveFile(filename, doc);
			xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
			close_xml_doc(doc);
		}
	}

	if (pCamera_config)
	{
		doc = create_xml_doc();
		if (NULL == doc)
		{
			log_error("XML", "create xml doc error. \n");
			return -1;
		}
		debug("camera doc ok\n");
		root = create_root_node(doc, (char*) "epcs_protocol");
		if (NULL == root)
		{
			log_error("XML", "error when create root node. \n");
			close_xml_doc(doc);
			return -1;
		}

		child = create_Camera_config(pCamera_config, (char*) "camera_config");
		xmlAddChild(root, child);

		//xmlSaveFile(filename, doc);
		xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
		close_xml_doc(doc);
	}

	if (pARM_config)
	{

		doc = create_xml_doc();
		if (NULL == doc)
		{
			log_error("XML", "create xml doc error. \n");
			return -1;
		}
		debug("doc ok\n");
		root = create_root_node(doc, (char*) "vdcs_protocol");
		if (NULL == root)
		{
			log_error("XML", "error when create root node. \n");
			close_xml_doc(doc);
			return -1;
		}
		debug("root ok\n");
		child = create_ARM_config(pARM_config, (char*) "arm_config");
		xmlAddChild(root, child);

		debug("start save...\n");
		//xmlSaveFile("arm_config.xml", doc);

		xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

		debug("finish save\n");
		close_xml_doc(doc);

	}

	return 0;
}
