/*
 * xmlCreater.h
 *
 *  Created on: 2013-6-17
 *      Author: shanhw
 */

#ifndef XMLCREATER_H_
#define XMLCREATER_H_


#include <libxml/parser.h>

#include "arm_config.h"
#include "dsp_config.h"
#include "camera_config.h"


#ifdef  __cplusplus
extern "C"
{
#endif

int create_xml_file(const char *filename, VDConfigData *pVDCS_config,
                    Camera_config* pCamera_config, ARM_config *pARM_config);

#ifdef  __cplusplus
}
#endif


#endif /* XMLCREATER_H_ */
