/*
 * xmlParser.h
 *
 *  Created on: 2013-5-30
 *      Author: shanhw
 */

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

#include <libxml/parser.h>

#include "arm_config.h"
#include "dsp_config.h"
#include "camera_config.h"


#ifdef  __cplusplus
extern "C"
{
#endif

int parse_xml_doc(const char *filename, VDConfigData *pVDConfigData,
                  Camera_config *pCamera_config, ARM_config *pARM_config);

int parse_xml_string(const char* buffer, int size, VDCS_config *pVDCS_config,
                     Camera_config *pCamera_config, ARM_config *pARM_config);

#ifdef  __cplusplus
}
#endif


#endif /* XMLPARSER_H_ */
