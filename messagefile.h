/*
 * messagefile.h
 *
 *  Created on: 2013-5-2
 *      Author: shanhw
 */

#ifndef MESSAGEFILE_H_
#define MESSAGEFILE_H_

#define  FACTORY_CONFIG_FILE  "/config/factory_set.txt"
#define  SERVER_CONFIG_FILE  "/config/message.txt"

/****recovery default config of factory device*****/
#define  NET_DEVICE_ID "3702020199"
#define  NET_MQIP1		192
#define  NET_MQIP2		168
#define  NET_MQIP3		0
#define  NET_MQIP4		253
#define  NET_MQPORT		61616

#define  NET_FTPIP1 	192
#define  NET_FTPIP2 	168
#define  NET_FTPIP3 	0
#define  NET_FTPIP4 	253
#define  NET_FTPPORT 	21
#define  NET_FTPUSER 	"sa"
#define  NET_FTPPASSWD 	"sa"
#define  NET_FTPANONYMOUS 0

#define  DEVICE_SERIALNUM 1EP10PEA0001
/*****************end**********************/

typedef enum CFG1_STATUS
{
	GENERAL = 0,	//
	ITEM_NAME,		//
	ITEM_VALUE,
} CFG1_STATUS;

void write_config_file(char * fileName);
int ReadConfigFile(const char * fileName);

void write_factory_file(char * fileName);
int ReadFacFile(const char * fileName);

int32_t parking_lock_read(void);

#endif /* MESSAGEFILE_H_ */
