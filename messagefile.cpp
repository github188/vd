/*
 * messagefile.cpp
 *
 *  Created on: 2013-5-2
 *      Author: shanhw
 */

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>

#include "ctrl.h"
#include "commonfuncs.h"
#include "ep_type.h"
#include "messagefile.h"
#include "mq_listen.h"
#include "mq_module.h"
#include "global.h"
#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>

#include "profile.h"
#include "commontypes.h"
#include "logger/log.h"


char enter[] = "\n";
char semicolon[] = ";";
char HWUUID[15];
char HWVersion[12];
char serial_add_flag[] = "serial_add_flag";
int serialadd_flag = 0;

////////////////////////网络参数////////////////////////////////////////

char Net_DeviceID[] = "Net_DeviceID";
char Net_MQIP1[] = "Net_MQIP1";
char Net_MQIP2[] = "Net_MQIP2";
char Net_MQIP3[] = "Net_MQIP3";
char Net_MQIP4[] = "Net_MQIP4";
char Net_MQPort[] = "Net_MQPort";

char Net_FTPIP1[] = "Net_FTPIP1";
char Net_FTPIP2[] = "Net_FTPIP2";
char Net_FTPIP3[] = "Net_FTPIP3";
char Net_FTPIP4[] = "Net_FTPIP4";
char Net_FTPPort[] = "Net_FTPPort";
char Net_FTPuser[] = "Net_FTPuser";
char Net_FTPpasswd[] = "Net_FTPpasswd";
char Net_FTPallow_anonymous[] = "Net_FTPallow_anonymous";
char FTP_PATH[] = "FTP_path";
char FTP_PATH_COUNT[] = "FTP_path_count";
/******************************************************************************/

extern Device_Information device_info; //ctrl.cpp 中定义。

//************************************************************
//*******write_config_file()函数，message文件。 设定设备的硬件信息，如IP地址、MAC地址等
//*************************************************************
void write_config_file(char *filename)
{
	char path[64];

	debug(">>>>in %s\n", __func__);
	FILE *pfile = NULL;
	while (1)
	{
		if ((pfile = fopen(filename, "w")))
			break;
		else
		{
			static int open_failed_num = 0;
			open_failed_num++;
			if (open_failed_num >= 3)
			{
				return;
			}

			sleep(1);
		}
	}

	fputs(Net_DeviceID, pfile);
	fprintf(pfile, "=%s", g_set_net_param.m_NetParam.m_DeviceID);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_MQIP1, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.m_MQ_IP[0]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_MQIP2, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.m_MQ_IP[1]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_MQIP3, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.m_MQ_IP[2]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_MQIP4, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.m_MQ_IP[3]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_MQPort, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.m_MQ_PORT);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPIP1, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_param_conf.ip[0]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPIP2, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_param_conf.ip[1]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPIP3, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_param_conf.ip[2]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPIP4, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_param_conf.ip[3]);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPPort, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_param_conf.port);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPuser, pfile);
	fprintf(pfile, "=%s", g_set_net_param.m_NetParam.ftp_param_conf.user);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPpasswd, pfile);
	fprintf(pfile, "=%s", g_set_net_param.m_NetParam.ftp_param_conf.passwd);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(Net_FTPallow_anonymous, pfile);
	fprintf(pfile, "=%d",
	        g_set_net_param.m_NetParam.ftp_param_conf.allow_anonymous);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(FTP_PATH_COUNT, pfile);
	fprintf(pfile, "=%d", g_set_net_param.m_NetParam.ftp_url_level.levelNum);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	for (int i = 0; i < g_set_net_param.m_NetParam.ftp_url_level.levelNum; i++)
	{
		sprintf(path, "%s%d", FTP_PATH, i);
		fputs(path, pfile);

		fprintf(pfile, "=%d",
		        g_set_net_param.m_NetParam.ftp_url_level.urlLevel[i]);

		fputs(semicolon, pfile);
		fputs(enter, pfile);
	}
	fflush(pfile);
	fsync(fileno(pfile));
	fclose(pfile);
	system("sync");
}


//************************************************************
//*******ReadConfigFile()函数，message文件.设定设备的硬件信息，如MQ IP地址、MAC地址等
//*************************************************************
int ReadConfigFile(const char * fileName)
{
	FILE *pfile = NULL;
	CFG1_STATUS cfg_status = ITEM_NAME;
	char c;
	int digit = 0;
	char itemname[256] = "";
	char itemvalue[256] = "";
	int itemindex = 0;
	int itemvalueindex = 0;
	int open_failed_num = 0;

	while (1)
	{
		if ((pfile = fopen(fileName, "rt")))
			break;
		else
			printf("open %s failed\n", fileName);
		open_failed_num++;
		if (open_failed_num >= 3)
		{
			return -1;
		}
		sleep(1);
	}

	while (!feof(pfile))
	{
		fread(&c, 1, 1, pfile);
		if (c == ' ')
		{
			continue;
		}
		else if (c == '=')
		{
			itemname[itemindex] = 0;
			cfg_status = ITEM_VALUE;
		}
		else if (c == ';')
		{
			digit = atoi(itemvalue);

			if (!strcasecmp(itemname, Net_DeviceID))
			{
				memset(g_set_net_param.m_NetParam.m_DeviceID, 0,
				       sizeof(g_set_net_param.m_NetParam.m_DeviceID));
				strncpy(g_set_net_param.m_NetParam.m_DeviceID, itemvalue,
				        sizeof(g_set_net_param.m_NetParam.m_DeviceID));
			}
			else if (!strcasecmp(itemname, Net_MQIP1))
			{
				g_set_net_param.m_NetParam.m_MQ_IP[0] = digit;
			}
			else if (!strcasecmp(itemname, Net_MQIP2))
			{
				g_set_net_param.m_NetParam.m_MQ_IP[1] = digit;
			}
			else if (!strcasecmp(itemname, Net_MQIP3))
			{
				g_set_net_param.m_NetParam.m_MQ_IP[2] = digit;
			}
			else if (!strcasecmp(itemname, Net_MQIP4))
			{
				g_set_net_param.m_NetParam.m_MQ_IP[3] = digit;
			}
			else if (!strcasecmp(itemname, Net_MQPort))
			{
				g_set_net_param.m_NetParam.m_MQ_PORT = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPIP1))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.ip[0] = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPIP2))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.ip[1] = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPIP3))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.ip[2] = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPIP4))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.ip[3] = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPPort))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.port = digit;
			}
			else if (!strcasecmp(itemname, Net_FTPuser))
			{
				strncpy(g_set_net_param.m_NetParam.ftp_param_conf.user,
				        itemvalue,
				        sizeof(g_set_net_param.m_NetParam.ftp_param_conf.user));
			}
			else if (!strcasecmp(itemname, Net_FTPpasswd))
			{
				strncpy(
				    g_set_net_param.m_NetParam.ftp_param_conf.passwd,
				    itemvalue,
				    sizeof(g_set_net_param.m_NetParam.ftp_param_conf.passwd));
			}
			else if (!strcasecmp(itemname, Net_FTPallow_anonymous))
			{
				g_set_net_param.m_NetParam.ftp_param_conf.allow_anonymous
				    = digit;
			}
			else if (!strcasecmp(itemname, FTP_PATH_COUNT))
			{
				g_set_net_param.m_NetParam.ftp_url_level.levelNum = digit;
			}
			else if (!strncasecmp(itemname, FTP_PATH, strlen(FTP_PATH)))
			{
				char temp = itemname[strlen(FTP_PATH)];

				if (temp < '0' || temp > '9')
				{
					continue;
				}

				g_set_net_param.m_NetParam.ftp_url_level.urlLevel[temp - '0']
				    = digit;
				//printf("i= %d digit= %d\n", i,digit);
			}

			digit = 0;
			itemname[0] = 0;
			itemindex = 0;
			memset(itemname, 0, sizeof(itemname));
			memset(itemvalue, 0, sizeof(itemvalue));
			itemvalueindex = 0;
			cfg_status = ITEM_NAME;
		}
		//else if ( (c=='_') ||((c>='A')&& (c<='z'))||((c>="1")&&(c<="9")) )
		else if ((c >= '(') && (c <= 'z'))
			//else if ( c!='\n' )
		{
			if (cfg_status == ITEM_VALUE)
			{
				itemvalue[itemvalueindex] = c;
				itemvalueindex++;
			}
			else if (cfg_status == ITEM_NAME)
			{
				itemname[itemindex] = c;
				itemindex++;
			}
		}
	}

	fclose(pfile);
	return 0;
}
#if 1

/*************************************************************
函数名: ReadFacFile
功能:  读取/config/factory_set.txt ，文件内容如下:
Device_serialnum=;
serial_add_flag=0;
hw_version=;
device_type=1;
返回值: 

*************************************************************/

int ReadFacFile(const char * fileName)
{
	FILE *pfile = NULL;
	CFG1_STATUS cfg_status = ITEM_NAME;
	char c;
	int digit = 0;
	char itemname[256] = "";
	char itemvalue[256] = "";
	int itemindex = 0;//char
	int itemvalueindex = 0;//char
	int open_failed_num = 0;

	while (1)
	{
		if ((pfile = fopen(fileName, "rt")))
			break;
		else
			printf("open %s failed\n", fileName);
		open_failed_num++;
		if (open_failed_num >= 3)
		{
			return -1;
		}
		sleep(1);

	}
	while (!feof(pfile))
	{
		fread(&c, 1, 1, pfile);
		if (c == ' ')
		{
			continue;
		}

		else if (c == '=')
		{
			itemname[itemindex] = 0;
			cfg_status = ITEM_VALUE;
		}
		else if (c == ';')
		{
			digit = atoi(itemvalue);
			if (!strcasecmp(itemname, serial_add_flag))
			{
				serialadd_flag = digit;
			}
			else if (!strcasecmp(itemname, "Device_serialnum"))
			{
				memset(device_info.hw_uuid, 0, sizeof(device_info.hw_uuid));
				memcpy(device_info.hw_uuid, itemvalue,
				       sizeof(device_info.hw_uuid));
				memset(HWUUID, 0, 15);
				memcpy(HWUUID, device_info.hw_uuid, 15);
			}
			else if (!strcasecmp(itemname, "hw_version"))
			{
				memset(device_info.hw_ver, 0, sizeof(device_info.hw_ver));
				memcpy(device_info.hw_ver, itemvalue,
				       sizeof(device_info.hw_ver));
				memset(HWVersion, 0, 12);
				memcpy(HWVersion, device_info.hw_ver, 12);
			}
			else if (!strcasecmp(itemname, "device_type"))
			{
//				memset(device_info.hw_ver, 0, sizeof(device_info.hw_ver));
//				memcpy(device_info.hw_ver, itemvalue,
//				       sizeof(device_info.hw_ver));			}

			digit = 0;
			itemname[0] = 0;
			itemindex = 0;
			memset(itemname, 0, sizeof(itemname));
			memset(itemvalue, 0, sizeof(itemvalue));
			itemvalueindex = 0;
			cfg_status = ITEM_NAME;
		}
		else if ((c >= '(') && (c <= 'z'))
		{
			if (cfg_status == ITEM_VALUE)
			{
				itemvalue[itemvalueindex] = c;
				itemvalueindex++;
			}
			else if (cfg_status == ITEM_NAME)
			{
				itemname[itemindex] = c;
				itemindex++;
			}
		}
	}

	fclose(pfile);
	return 0;
	}

	return -1;
}

void  write_factory_file(char *filename)
{
	FILE *pfile = NULL;

	debug(" >>> in %s\n", __func__)
	while (1)
	{
		if ((pfile = fopen(filename, "w")))
			break;
		else
		{
			static int open_failed_num = 0;
			open_failed_num++;
			if (open_failed_num >= 3)
			{
				return;
			}

			sleep(1);
		}
	}
	debug("start write factory file:%s \n",filename);

	fputs("Device_serialnum", pfile);
	fprintf(pfile, "=%s", HWUUID);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(serial_add_flag, pfile);
	fprintf(pfile, "=%d", serialadd_flag);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs("hw_version", pfile);
	fprintf(pfile, "=%s", HWVersion);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs("device_type", pfile);
	fprintf(pfile, "=%d", 1);
	fputs(semicolon, pfile);
	fputs(enter, pfile);

	fputs(enter, pfile);
	fputs(enter, pfile);
	fclose(pfile);
	system("sync");
}

#endif



int32_t parking_lock_read(void)
{
	int32_t val_len=-1;
	char val[128];
	int32_t mode = 0;//default is not lock.
	profile_t profile;

	if (0 == profile_open(&profile, "/mnt/nand/park.cfg", "r"))
	{
		val_len = profile_rd_val(&profile, "parking_lock", val);
		profile_close(&profile);
	}
	if ((val_len > 0))// && (0 == strncmp(val, "1", strlen("1"))))
	{
		mode = atoi(val);
	}

	INFO("parking_lock: %d.", mode);

	return mode;
}



