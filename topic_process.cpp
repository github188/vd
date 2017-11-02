/*
 * topic_process.cpp
 *
 *  Created on: 2013-5-2
 *      Author: shanhw
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/vfs.h>

#include <iconv.h>
#include <locale.h>

#include "global.h"
#include "messagefile.h"
#include "commonfuncs.h"
#include "xmlCreater.h"

#include "mq.h"
#include "mq_listen.h"
#include "mq_module.h"

#include "ctrl.h"
#include "ftp.h"
#include "arm_config.h"
#include "dsp_config.h"
#include "camera_config.h"
#include "xmlParser.h"
//#include "pcie.h"
#include "interface.h"
#include "Appro_interface.h"
#include "dsp_config.h"

#include "sysctrl.h"
#include "logger/log.h"

#include <mcfw/src_linux/mcfw_api/osdOverlay/include/Macro.h>

#define BATCH_PARAM_FILE_PATH "/config/BulkConfig.xml"
#define MALLOC_SIZE (1024*1024)  //发送/接受文件申请单缓存大小
//char plate_config_buf[500];
//char filename_plate_config[2][200];
//消息内容: 两个文件路径.
//SCapture_Vehicle_Spec file_path;
//int remote_update_finish; //远程升级完成标志 0：未完成，1：完成
char store_name[100];

//extern int flg_encode_pass_car[]; //过车记录编码
//extern int upload_file_flag_ok;
//extern int alg_para_config_flag; //参数配置失败

//extern void changeDSP();
//extern int file_len;
//char mst_all_file[20][100]; //主板所有升级文件（除master_ep,master.X64P）
//int mst_file_index;
//int slv_file_index;
//int update_file_len;

int download_arm_thread_running = 0; //下载arm参数线程运行状态.

//TODO: 上传下载使用不同的路径变量. 避免参数不能同时下载/上传.
NORMAL_FILE_INFO ftp_filePath_down; //normal file
FTP_FILEPATH ftp_filePath; //上传文件
FTP_FILEPATH ftp_arm_filePath; //上传arm文件
FTP_FILEPATH ftp_dsp_filePath; //上传dsp文件
FTP_FILEPATH ftp_camera_filePath; //上传camera文件
FTP_FILEPATH ftp_hide_filePath; //上传 hide 文件


extern Camera_config g_camera_config;

extern int upload_arm_param; //上传arm参数
extern int upload_dsp_param; //上传dsp参数
extern int upload_camera_param; //上传camera参数
extern int upload_hide_param_file; //上传隐性参数
extern int upload_normal_file; //上传一般文件

extern int download_arm_param; //下载arm参数
extern int download_dsp_param; //下载dsp参数
extern int download_camera_param; //下载camera参数
extern int download_hide_param_file; //下载隐性参数
extern int download_normal_file; //下载一般文件

extern int batch_config_arm_param; //批量配置arm参数
extern int batch_config_dsp_param; //批量配置dsp参数
extern int batch_config_camera_param; //批量配置camera参数
extern int batch_config_param;


int capture_pic_upload(void);


#if 0
int update_file_remove(char file[][100], int index)
{
	char del_file[100];

	for (int i = 0; i < index; i++)
	{
		memset(del_file, 0, sizeof(del_file));
		strcpy(del_file, file[i]);
		strcat(del_file, "del");
		debug("尝试删除文件%s, delname is %s", file[i], del_file);
		//尝试3次删除和改名
		while (1)
		{
			if (-1 == remove(file[i]))
			{
				if (-1 == rename(file[i], del_file))
				{
					if (i >= 3)
					{
						return -1;
					}
					i++;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	return 0;

}

int change_file_available(char file[][100], int index)
{
	char right_file[100];
	int j = 0;
	char *p;
	char err[100];
	for (int i = 0; i < index; i++)
	{
		memset(err, 0, sizeof(err));
		memset(right_file, 0, sizeof(right_file));
		strcpy(right_file, file[i]);
		p = strrchr(right_file, '/'); //查找最后一次'/'出现的位置
		p++;
		while (1)
		{
			*p = *(p + 1);
			if (*p == 0)
			{
				break;
			}
			p++;
		}
		//		strcpy(right_file, file[i][1]);
		debug("update file name before changed is %s", file[i]);
		debug("update file name after changed is %s", right_file);
		while (1)
		{
			if (-1 == rename(file[i], right_file))
			{
				if (j >= 3)
				{
					sprintf(
					    err,
					    "update is successful, but change file %s name is failed",
					    file[i]);
					//write_err_log_noflash(err);
					log_error_ftp(err);
					return -1;
				}
				j++;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}
#endif
/**
 * 处理升级
 * 参数：
 * 		filename： 升级文件包对应的文件名称。
 * 返回： SUCCESS 成功；FAILURE 失败
 */
int upgrade_handler(const char *filename)
{
	Grade_New_Header header;

	int i = 0;
	int file_len, len;
	int blk_cnt;
	unsigned char *tmpbuf;
	unsigned char *head_buf;
	int head_len;
	unsigned int file_check_sum;
	char board_type;
	char file_name[256], file_backup[256];
	int path_len;
	int check_sum;
	char *p;
	FILE *file;
	FILE *fp;
	int result = SUCCESS;

	p = (char *) malloc(MALLOC_SIZE);
	if (NULL == p)
	{
		debug("malloc error!\n");
		return FAILURE;
	}

	if (!(fp = fopen(filename, "rb")))
	{
		printf("read %s failed\n", filename);
		free(p);
		return FAILURE;
	}

	debug("start to parse upgrade file %s ...\n", filename);
	while (1)
	{
		len = fread(&header, sizeof(header), 1, fp);
		debug("fread return %d , sizeof(header):%d\n", len,sizeof(header));
		if (len <= 0)
		{
			break;
		}
		debug("正在处理第%d个文件\n", ++i);

		file_len = header.buffer_len;
		head_buf = tmpbuf = (unsigned char *) &header;
		head_len = sizeof(header);
		file_check_sum = header.check_sum;
		board_type = header.board_type;
		strcpy(file_name, header.path);
		path_len = strlen(file_name);
		if (file_name[path_len - 1] != '/')
		{
			strcat(file_name, "/");
		}
		strcat(file_name, header.name);
		sprintf(file_backup, "%s.upgrade", file_name); //暂存为.upgrade

		if (file_len % MALLOC_SIZE == 0)
		{
			blk_cnt = file_len / MALLOC_SIZE;
		}
		else
		{
			blk_cnt = 1 + file_len / MALLOC_SIZE;
		}
		//debug("board_type=%d file_name=%s\n",header.board_type,header.name);
		//debug("buffer_len=%d path=%s\n",header.buffer_len,header.path);
		//debug("check_sum=%d\n",header.check_sum);

		check_sum = 0;
		for (int j = 0; j < (head_len - 4); j++)
		{
			check_sum += *(tmpbuf + j);
			//printf("check_sum=%d, tmpbuf[%d]=%d\n",check_sum,j,*(tmpbuf + j));
		}

		debug("the file name is %s\n", file_name);
		debug("the file_len is %d\n", file_len);
		debug("pack head check_sum is %d headlen=%d\n", check_sum,head_len);

		if (!(file = fopen(file_backup, "wb")))
		{
			printf("open %s failed\n", file_backup);
			fclose(fp);
			free(p);
			return FAILURE;
		}

		for (int j = 0; j < blk_cnt - 1; j++)
		{
			fread(p, MALLOC_SIZE, 1, fp);

			//做文件数据的校验和
			tmpbuf = (unsigned char*) p;
			for (int num = 0; num < MALLOC_SIZE; num++)
			{
				check_sum += *(tmpbuf + num);
			}

			len = fwrite(p, MALLOC_SIZE, 1, file);
			debug("write block len = %d\n", len);
			usleep(100000);
		}

		//The last packet
		fread(p, file_len % MALLOC_SIZE, 1, fp);
		printf("last packet index: %d, len: %d\n", blk_cnt, file_len
		       % MALLOC_SIZE );

		//做文件数据最后一包的校验和
		tmpbuf = (unsigned char*) p;
		for (int j = 0; j < (int) (file_len % MALLOC_SIZE); j++)
		{
			check_sum += *(tmpbuf + j);
		}

		len = fwrite(p, file_len % MALLOC_SIZE, 1, file);
		debug("last block len = %d\n", len);

		fclose(file);

		if (check_sum != (int) (file_check_sum))
		{
			char content[256];
			sprintf(content,
			        "upgrade file name: [%s], check sum is wrong,  %d != %u\n",
			        file_name, check_sum, file_check_sum);
			log_error_ftp(content);
			debug(content);
			result = FAILURE; //若有一次校验和不对，标志置错

			remove(file_backup);
			break;
		}
		debug("start mv file. header.board_type:%d, %s-->%s \n",header.board_type,file_backup,file_name);
		switch (header.board_type)
		{
		case 0: //ARM

			rename(file_backup, file_name);
			remove(file_backup);
			chmod(file_name, S_IRUSR | S_IWUSR | S_IXUSR);
			break;
		case 1: //DSP

			rename(file_backup, DSP_UPGRADE_FILE);
			remove(file_backup);
			break;
//		case 2: //FPGA
//
//			rename(file_backup, FPGA_UPGRADE_FILE);
//			remove(file_backup);
//			break;
//		case 3: //单片机1
//			debug("升级单片机1 :%s", file_backup)
//			;
//			mcu_upgrade(file_backup);
//			remove(file_backup);
//			break;
//		case 4: //单片机2
//			debug("升级单片机2 :%s", file_backup)
//			;
//			mcu_fpga_upgrade(file_backup);
//			remove(file_backup);
//			break;
		default:
			debug("wrong type in %s", header.name)
			;
			break;

		}
		//fseek(fp, header.buffer_len, SEEK_CUR);
	}

	fclose(fp);
	free(p);
	system((char*) "sync");
	debug("finish parse grade.bin...\n");
	return result;
}

void *handle_download_arm_param(void *arg)
{
	int status;
	char bakfile[256];

	CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
	if (ftp_conf_chanel == NULL)
	{
		download_arm_param = -1;
		log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");
		return NULL;
	}

	if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok == 1)*/)
	{
		download_arm_param = -1;
		log_error((char*)"MQ", (char*)"FTP不可用.\n");
		return NULL;
	}
	debug("file: %s, size:%lu \n", ftp_arm_filePath.m_strFileURL, ftp_arm_filePath.file_size);

	sprintf(bakfile, "%s.bak", ARM_PARAM_FILE_PATH);
	status = ftp_conf_chanel->ftp_get_file(ftp_arm_filePath.m_strFileURL,
	                                       bakfile);
	if ((status != SUCCESS) || (get_file_size(bakfile)
	                            != (off_t)ftp_arm_filePath.file_size))
	{
		char text[100];
		sprintf(text, "get %s from ftp server failed. file size is wrong.\n",
		        ARM_PARAM_FILE_PATH);
		log_error((char*)"MQ",text);

		debug(text);
		remove(bakfile);
		//需要回传错误信息
		download_arm_param = -1;
	}
	else
	{
		debug("got arm xml success. !\n");
		ARM_config arm_config;
		int ret=-1;
		memset(&arm_config, 0, sizeof(ARM_config));
		parse_xml_doc(bakfile, NULL, NULL, &arm_config);

#ifndef WITHOUT_SYSSERVE
//		debug("start send param to sysserver...");
//		Serial_config_t serial_cfg_temp[3];
//		for (i = 0; i < 3; i++)
//		{
//			serial_cfg_temp[i].dev_type
//			    = (int) arm_config.interface_param.serial[i].dev_type;
//			serial_cfg_temp[i].baudrate
//			    = (int) arm_config.interface_param.serial[i].bps;
//			serial_cfg_temp[i].parity
//			    = (char) arm_config.interface_param.serial[i].check;
//			serial_cfg_temp[i].databit
//			    = (char) arm_config.interface_param.serial[i].data;
//			serial_cfg_temp[i].stopbit
//			    = (char) arm_config.interface_param.serial[i].stop;
//			debug("bps=%d,check=%d,data=%d,stop=%d\n",serial_cfg_temp[i].baudrate,serial_cfg_temp[i].parity,
//			      serial_cfg_temp[i].databit,serial_cfg_temp[i].stopbit)
//		}
//		ret = set_serial_config(serial_cfg_temp);
//		if (ret != SUCCESS)
//		{
//			remove(bakfile);
//			download_arm_param = -1;
//			log_error((char*)"MQ", (char*)"set_serial_config failed.\n");
//			return NULL;
//		}
//
//		IO_config_t IO_cfg_temp0[8];
//		for (i = 0; i < 8; i++)
//		{
//			IO_cfg_temp0[i].trigger_type
//			    = arm_config.interface_param.io_input_params[i].trigger_type;
//			IO_cfg_temp0[i].mode
//			    = arm_config.interface_param.io_input_params[i].mode;
//			IO_cfg_temp0[i].direction
//			    = arm_config.interface_param.io_input_params[i].io_drt;
//		}
//		ret = set_in_config(IO_cfg_temp0);
//		if (ret != SUCCESS)
//		{
//			remove(bakfile);
//			download_arm_param = -1;
//			log_error((char*)"MQ", (char*)"set_in_config failed.\n");
//			return NULL;
//		}
//
//		IO_config_t IO_cfg_temp1[4];
//		for (i = 0; i < 4; i++)
//		{
//			IO_cfg_temp0[i].trigger_type
//			    = arm_config.interface_param.io_output_params[i].trigger_type;
//			IO_cfg_temp0[i].mode
//			    = arm_config.interface_param.io_output_params[i].mode;
//			IO_cfg_temp0[i].direction
//			    = arm_config.interface_param.io_output_params[i].io_drt;
//		}
//		ret = set_out_config(IO_cfg_temp1);
//		if (ret != SUCCESS)
//		{
//			remove(bakfile);
//			download_arm_param = -1;
//			log_error((char*)"MQ", (char*)"set_out_config failed.\n");
//			return NULL;
//		}
//
//		ret = set_h264_config(&arm_config.h264_config);
//		if (ret != SUCCESS)
//		{
//			char text[100];
//			sprintf(text, "set_h264_config return: %d\n", ret);
//			log_warn((char*)"MQ",text);
//			debug(text);
//			remove(bakfile);
//			download_arm_param = -1;
//			log_error((char*)"MQ", (char*)"set_h264_config failed.\n");
//			return NULL;
//		}
#endif
		remove(ARM_PARAM_FILE_PATH);
		ret = rename(bakfile, ARM_PARAM_FILE_PATH);
		if (ret != 0)
		{
			download_arm_param = -1;
			log_error((char*)"MQ", (char*)"rename arm_config.xml failed.\n");
			return NULL;
		}
		system("sync");

		BYTE IP_tmp[4] = {0};
		WORD MQ_PORT_tmp = 0;
	
		IP_tmp[0] = g_arm_config.basic_param.mq_param.ip[0];
		IP_tmp[1] = g_arm_config.basic_param.mq_param.ip[1];
		IP_tmp[2] = g_arm_config.basic_param.mq_param.ip[2];
		IP_tmp[3] = g_arm_config.basic_param.mq_param.ip[3];
		MQ_PORT_tmp = g_arm_config.basic_param.mq_param.port;

		char ip_org[32];
		memset(ip_org,0,32);
		
		sprintf(ip_org,"%d.%d.%d.%d",IP_tmp[0],IP_tmp[1],IP_tmp[2],IP_tmp[3]);

		printf("ip_org:%s  port:%d\n",ip_org,MQ_PORT_tmp);

		parse_xml_doc(ARM_PARAM_FILE_PATH, NULL, NULL, &g_arm_config);
		send_arm_cfg();

		
		if((IP_tmp[0] != g_arm_config.basic_param.mq_param.ip[0]) || (IP_tmp[1] != g_arm_config.basic_param.mq_param.ip[1])
			||(IP_tmp[2] != g_arm_config.basic_param.mq_param.ip[2])||(IP_tmp[3] != g_arm_config.basic_param.mq_param.ip[3])
			||MQ_PORT_tmp != g_arm_config.basic_param.mq_param.port)
		{
			printf("改动第三方MQ服务器配置信息，重启中.......\n");
			power_down();
		
		}

		//memcpy(&g_arm_config,  &arm_config, sizeof(arm_config));

		get_ftp_chanel(FTP_CHANNEL_ILLEGAL)->set_config(
		    &g_arm_config.basic_param.ftp_param_illegal);
		get_ftp_chanel(FTP_CHANNEL_PASSCAR)->set_config(
		    &g_arm_config.basic_param.ftp_param_pass_car);
		get_ftp_chanel(FTP_CHANNEL_H264)->set_config(
		    &g_arm_config.basic_param.ftp_param_h264);

		get_ftp_chanel(FTP_CHANNEL_ILLEGAL_RESUMING)->set_config(
		    &g_arm_config.basic_param.ftp_param_illegal);
		get_ftp_chanel(FTP_CHANNEL_PASSCAR_RESUMING)->set_config(
		    &g_arm_config.basic_param.ftp_param_pass_car);
		get_ftp_chanel(FTP_CHANNEL_H264_RESUMING)->set_config(
		    &g_arm_config.basic_param.ftp_param_h264);

		set_log_level(g_arm_config.basic_param.log_level);

		download_arm_param = 2;
		debug("xml param handler finish.\n");
	}
	return NULL;
}

/************************************************************
 *******************ftpTransformFxn()函数***************************
 功能: ftp传输文件处理。
 *************************************************************/
void * ftpTransformFxn(void * arg)
{
	char dir[DIR_NUM][DIR_LEN]; //目录数组，用于ftp创建目录
	char filename[FILE_NAME_LEN];
	char filename_full[FILE_NAME_LEN];
	char filename1[FILE_NAME_LEN];
	char bakfile[256];
	int status;
	char type[3];
	char *pSend;

	VDConfigData *p_dsp_cfg = (VDConfigData *)get_dsp_cfg_pointer();

	TYPE_FTP_CONFIG_PARAM *ftp_conf;

	DEBUG(">>>> in  %s\n", __func__);
	set_thread("resultFxn");

	//用于数据从硬盘上传ftp
	//struct timespec abstime;
	//abstime.tv_sec = 300;
	//abstime.tv_nsec = 0;

	while (1)
	{
		usleep(100000);

		if (upgrade == 1)//设备升级，通过ftp获取升级文件，并进行相应升级工作
		{
			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				upgrade = -1;
				log_error((char*)"MQ", (char*)"ftp channe is NULL.\n");
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK))
			{
				upgrade = -1;
				log_error((char*)"MQ", (char*)"ftp status is not ok.\n");
				continue;
			}

			debug("设备升级，通过ftp获取升级文件，并进行相应升级工作\n");
			debug("the grade path of ftp is %s\n", ftp_filePath_upgrade.m_strFileURL);

			for (int i = 0; i < 10; i++)
			{
				status = ftp_conf_chanel->ftp_get_file(
				             ftp_filePath_upgrade.m_strFileURL, UPGRADE_FILE_PATH);
				if (status == SUCCESS)
				{
					break;
				}
				sleep(2);
			}

			if (status != SUCCESS)
			{
				log_error_ftp((char*)"获取ftp升级文件失败!\n");

				upgrade = -1; //ftp 获取升级文件失败

				remove(UPGRADE_FILE_PATH);
				system((char*) "sync");
				log_warn_ftp((char*)"get upgrade file failed\n");

				continue;
			}

			//#########################################
			debug("下载升级文件完毕!!!\n");

			//update_file_len = 0;

			if (SUCCESS == upgrade_handler(UPGRADE_FILE_PATH))
			{
				upgrade = 2;
			}
			else
			{
				upgrade = -1;
			}

			sleep(1); //延时，为了让ftp获取文件成功的消息能上传。

			//abstime.tv_sec = time(NULL) + 60 + update_file_len / 4 / 3000;
			//debug("远程升级超时时间为：%d\n", 10 + update_file_len
			//		/ 4 / 3000);

			//TODO: 升级完成,重启.  脚本会自动完成copy.   //设置设备重启标志，改写脚本//
			system("sync");
			int ret = power_down();
			if (ret != SUCCESS)
			{
				printf("reboot failed!\n");
			}

			//usleep(10000);
		}
		//下载内部文件
		if (1 == download_normal_file)
		{
			debug("Ftp 下载一般内部文件.....%s\n",ftp_filePath_down.m_strDeviceFilePath);

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				download_normal_file = -1;
				log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");
				continue;
			}
			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok != 1)*/)
			{
				download_normal_file = -1;
				log_error((char*)"MQ", (char*)"ftp or mq status is not ok.\n");
				continue;
			}

			//############### 获取文件名 #######################
			pSend = basename(ftp_filePath_down.m_strDeviceFilePath);
			if (pSend == NULL)
			{
				download_normal_file = -1;
				log_debug((char*)"[MQ]", (char*)"文件名为NULL.\n");
				continue;
			}
			mkdir("/home/root/down", 0755);
			sprintf(store_name, "/home/root/down/%s", pSend);

			//############ FTP获取文件  ######################
			status = ftp_conf_chanel->ftp_get_file(
			             ftp_filePath_down.m_cFTPFilePath.m_strFileURL, store_name);

			debug("file size:%lld, src file size:%lu \n", (long long int)get_file_size(store_name), ftp_filePath_down.m_cFTPFilePath.file_size);

			if ((status != SUCCESS) || (get_file_size(store_name)
			                            != (off_t)ftp_filePath_down.m_cFTPFilePath.file_size))
			{
				download_normal_file = -1;
				log_error((char*)"MQ", (char*)"ftp get file fail.\n");
				remove(store_name);
				continue;
			}

			int ret = rename(store_name, ftp_filePath_down.m_strDeviceFilePath);
			if (ret != 0)
			{
				download_normal_file = -1;
				log_error((char*)"MQ", (char*)"ftp path error.\n");
				remove(store_name);
				continue;
			}
			chmod(ftp_filePath_down.m_strDeviceFilePath, S_IRUSR | S_IWUSR
			      | S_IXUSR);
			printf("ftp_filePath_down.m_strDeviceFilePath is %s\n",
			       ftp_filePath_down.m_strDeviceFilePath);
			//remove(store_name);
			system((char*) "sync");
			download_normal_file = 2;
		}
		if (1 == upload_normal_file)//上传内部文件
		{
			debug("FTP 上传内部文件.....%s\n",ftp_filePath_down.m_strDeviceFilePath);

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				upload_normal_file = -1;
				log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok != 1)*/)
			{
				upload_normal_file = -1;
				log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");
				continue;
			}

			memset(filename, 0, sizeof(filename));
			sprintf(filename, "%s", ftp_filePath_up.m_strDeviceFilePath);
			memset(dir, 0, DIR_LEN * DIR_NUM);
			sprintf(dir[0], "%s", g_arm_config.basic_param.spot_id); //地点编号
			sprintf(dir[1], "%s", g_set_net_param.m_NetParam.m_DeviceID); //设备编号
			sprintf(dir[2], "%s", "filetrans");
			sprintf(dir[3], "%s", "upload");

			pSend = basename(ftp_filePath_up.m_strDeviceFilePath);
			if (pSend == NULL)
			{
				upload_normal_file = -1;
				debug("filename error\n");
				continue;
			}

			strcpy(filename1, pSend);
			status = ftp_conf_chanel->ftp_put_file(
			             ftp_filePath_up.m_strDeviceFilePath, dir, filename1);

			sprintf(ftp_filePath.m_strFileURL, "%s/%s/filetrans/upload/%s",
			        g_arm_config.basic_param.spot_id,
			        g_set_net_param.m_NetParam.m_DeviceID, filename1);
			sprintf(type, "%d", MSG_UPLOAD_NORMAL_FILE);

			if (status != SUCCESS)
			{
				debug("send %s failed\n", filename);
				//需要回传错误信息
				upload_normal_file = -1;
			}
			else
			{
				upload_normal_file = 2;
			}
		}
		if (1 == upload_arm_param) //上传arm参数
		{
			debug("FTP 上传arm参数文件.....\n");

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				upload_arm_param = -1;
				log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");

				continue;
			}

			if ((ftp_conf_chanel->get_status() == STA_FTP_OK)/* && (mq_ok == 1)*/)
			{
				sprintf(filename, "%s", ARM_PARAM_FILE);

				memset(dir, 0, DIR_LEN * DIR_NUM);
				sprintf(dir[0], "%s", g_arm_config.basic_param.spot_id); //地点编号
				sprintf(dir[1], "%s", g_set_net_param.m_NetParam.m_DeviceID); //设备编号
				sprintf(dir[2], "%s", "filetrans");
				sprintf(dir[3], "%s", "upload");

				status = ftp_conf_chanel->ftp_put_file(
				             (char *) ARM_PARAM_FILE_PATH, dir, filename);

				ftp_conf = get_ftp_chanel(FTP_CHANNEL_CONFIG)->get_config();

				sprintf(ftp_arm_filePath.m_strFileURL,
				        "ftp://%s:%s@%d.%d.%d.%d:%d"
				        "/%s/%s/filetrans/upload/%s", ftp_conf->user,
				        ftp_conf->passwd, ftp_conf->ip[0], ftp_conf->ip[1],
				        ftp_conf->ip[2], ftp_conf->ip[3], ftp_conf->port,
				        g_arm_config.basic_param.spot_id,
				        g_set_net_param.m_NetParam.m_DeviceID, filename);
				ftp_arm_filePath.file_size = get_file_size(
				                                 (char *) ARM_PARAM_FILE_PATH);

				debug("file: %s, size:%lu \n", ftp_arm_filePath.m_strFileURL, ftp_arm_filePath.file_size);
				sprintf(type, "%d", MSG_UPLOAD_ARM_PARAM);

				if (status != SUCCESS)
				{
					char text[100];
					sprintf(text, "(%s|%d)send %s to ftp failed\n", __FUNCTION__,
					        __LINE__, ARM_PARAM_FILE_PATH);
					log_error((char*)"MQ",text);
					upload_arm_param = -1;//需要回传错误信息
					continue;
				}
				else
				{
					upload_arm_param = 2;
				}
			}
			else
			{
				upload_arm_param = -1;
			}

			printf("##############after ftp upload arm xml :%d\n",upload_arm_param);
			//sleep(1);
		}
		if (1 == download_arm_param) //ftp下载arm参数
		{

			if (download_arm_thread_running == 0)
			{
				download_arm_thread_running = 1;
				pthread_t download_arm_param_Thd;
				pthread_create(&download_arm_param_Thd, NULL,
				               handle_download_arm_param, NULL);
			}
			download_arm_param = 0;
			//sleep(1);
		}
		if (1 == batch_config_param)
		{
			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				batch_config_param = -1;
				log_error((char*)"MQ", (char*)"获取FTP通道为NULL.\n");
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok == 1)*/)
			{
				batch_config_param = -1;
				log_error((char*)"MQ", (char*)"当批量配置参数时，FTP不可用.\n");
				continue;
			}

			status = ftp_conf_chanel->ftp_get_file(ftp_filePath.m_strFileURL,
			                                       BATCH_PARAM_FILE_PATH);
			if (status != SUCCESS)
			{
				char text[100];
				sprintf(text, "get %s from ftp server failed\n",
				        ftp_filePath.m_strFileURL);
				log_error((char*)"MQ",text);

				debug(text);
				//需要回传错误信息
				batch_config_param = -1;
			}
			else
			{

				ARM_config arm_config;
				memcpy(&arm_config, &g_arm_config, sizeof(ARM_config));
//				VDCS_config dsp_config;
				VDConfigData dsp_config;
				memcpy(&dsp_config, p_dsp_cfg, sizeof(VDConfigData));
				Camera_config camera_config;
				memcpy(&camera_config, &g_camera_config, sizeof(Camera_config));

				parse_xml_doc((const char*) BATCH_PARAM_FILE_PATH, &dsp_config,
				              &camera_config, &arm_config);

				if (memcmp(&g_arm_config, &arm_config, sizeof(ARM_config)))
				{
					memcpy(&g_arm_config, &arm_config, sizeof(ARM_config));
					create_xml_file((char *) ARM_PARAM_FILE_PATH, NULL, NULL,
					                &g_arm_config);
				}

				if (memcmp(p_dsp_cfg, &dsp_config, sizeof(VDConfigData)))
				{
					memcpy(p_dsp_cfg, &dsp_config, sizeof(VDConfigData));
					if(4 == DEV_TYPE)
					{
						create_xml_file((char *) DSP_PD_PARAM_FILE_PATH,
											                p_dsp_cfg, NULL, NULL);
					}
					else
					{
						create_xml_file((char *) DSP_PARAM_FILE_PATH,
					        						        p_dsp_cfg, NULL, NULL);
					
					}

				}

				if (memcmp(&g_camera_config, &camera_config,
				           sizeof(Camera_config)))
				{
					memcpy(&g_camera_config, &camera_config,
					       sizeof(Camera_config));
					create_xml_file((char *) CAMERA_PARAM_FILE_PATH, NULL,
					                &g_camera_config, NULL);
				}

				batch_config_param = 2;

			}
			//sleep(1);
		}
		if (1 == upload_dsp_param)
		{
			debug("FTP 上传dsp参数文件.....\n");

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				char text[100];
				sprintf(text, "(%s|%d)获取FTP通道为NULL.\n", __FILE__, __LINE__);
				log_error((char*)"MQ",text);
				upload_dsp_param = -1;

				continue;
			}

			if ((ftp_conf_chanel->get_status() == STA_FTP_OK)/* && (mq_ok == 1)*/)
			{

				sprintf(filename, "%s", DSP_PARAM_FILE);
				sprintf(filename_full, "%s", DSP_PARAM_FILE_PATH);
				memset(dir, 0, DIR_LEN * DIR_NUM);
				sprintf(dir[0], "%s", g_arm_config.basic_param.spot_id); //地点编号
				sprintf(dir[1], "%s", g_set_net_param.m_NetParam.m_DeviceID); //设备编号
				sprintf(dir[2], "%s", "filetrans");
				sprintf(dir[3], "%s", "upload");

				status = ftp_conf_chanel->ftp_put_file(
						           filename_full, dir, filename);

				ftp_conf = get_ftp_chanel(FTP_CHANNEL_CONFIG)->get_config();
				sprintf(ftp_dsp_filePath.m_strFileURL,
				        "ftp://%s:%s@%d.%d.%d.%d:%d"
				        "/%s/%s/filetrans/upload/%s", ftp_conf->user,
				        ftp_conf->passwd, ftp_conf->ip[0], ftp_conf->ip[1],
				        ftp_conf->ip[2], ftp_conf->ip[3], ftp_conf->port,
				        g_arm_config.basic_param.spot_id,
				        g_set_net_param.m_NetParam.m_DeviceID, filename);
				ftp_dsp_filePath.file_size = get_file_size(
				                                 (char *) filename_full);

				debug("file: %s, size:%lu \n", ftp_dsp_filePath.m_strFileURL, ftp_dsp_filePath.file_size);

				sprintf(type, "%d", MSG_UPLOAD_DSP_PARAM);

				if (status != SUCCESS)
				{
					char text[100];
					sprintf(text, "send %s to ftp failed\n",
							filename_full);

					log_error((char*)"MQ",text);
					upload_dsp_param = -1;//需要回传错误信息
					continue;
				}
			}

			upload_dsp_param = 2;
			printf("##############after ftp upload dsp xml : %d\n",upload_dsp_param);

			//sleep(1);
		}
		if (1 == download_dsp_param)
		{

			sprintf(filename, "%s", DSP_PARAM_FILE);
			sprintf(filename_full, "%s", DSP_PARAM_FILE_PATH);
				
			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				download_dsp_param = -1;
				debug_ftp("获取FTP配置通道为NULL.\n");
				log_error((char*)"MQ", (char*)"获取FTP配置通道为NULL.\n");
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok == 1)*/)
			{
				download_dsp_param = -1;
				log_error((char*)"MQ", (char*)"FTP不可用.\n");
				continue;
			}
			debug("file: %s, size:%lu \n", ftp_dsp_filePath.m_strFileURL, ftp_dsp_filePath.file_size);

			sprintf(bakfile, "%s.bak", filename_full);
			status = ftp_conf_chanel->ftp_get_file(
			             ftp_dsp_filePath.m_strFileURL, bakfile);
			if (status != SUCCESS)
			{
				char text[100];
				sprintf(text, "send %s to ftp failed\n", filename_full);
				log_error((char*)"MQ",text);
				remove(bakfile);
				system((char*) "sync");
				//需要回传错误信息
				download_dsp_param = -1;
			}
			else
			{
				remove(filename_full);
				int ret = rename(bakfile, (char*) filename_full);
				if (ret != 0)
				{
					download_dsp_param = -1;
					log_error((char*)"MQ", (char*)"rename dsp_config.xml failed.\n");
					continue;
				}
				sync();

				//发送给dsp
				if (0 == send_config_to_dsp())
				{
					debug("param have sent to dsp success. \n");
					download_dsp_param = 2;
				}
				else
				{
					debug("param have sent to dsp false. \n");
					download_dsp_param = -1;
				}
				//sleep(1);
			}
		}
		if (1 == upload_camera_param)
		{
			debug("FTP 上传camera参数文件.....\n");

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				char text[100];
				sprintf(text, "(%s|%d)获取FTP通道为NULL.\n", __FILE__, __LINE__);
				log_error((char*)"MQ",text);
				upload_camera_param = -1;

				continue;
			}

			if ((ftp_conf_chanel->get_status() == STA_FTP_OK)/* && (mq_ok == 1)*/)
			{
				sprintf(filename, "%s", CAMERA_PARAM_FILE);

				memset(dir, 0, DIR_LEN * DIR_NUM);
				sprintf(dir[0], "%s", g_arm_config.basic_param.spot_id); //地点编号
				sprintf(dir[1], "%s", g_set_net_param.m_NetParam.m_DeviceID); //设备编号
				sprintf(dir[2], "%s", "filetrans");
				sprintf(dir[3], "%s", "upload");

				status = ftp_conf_chanel->ftp_put_file(
				             (char *) CAMERA_PARAM_FILE_PATH, dir, filename);
				ftp_conf = get_ftp_chanel(FTP_CHANNEL_CONFIG)->get_config();
				sprintf(ftp_camera_filePath.m_strFileURL,
				        "ftp://%s:%s@%d.%d.%d.%d:%d"
				        "/%s/%s/filetrans/upload/%s", ftp_conf->user,
				        ftp_conf->passwd, ftp_conf->ip[0], ftp_conf->ip[1],
				        ftp_conf->ip[2], ftp_conf->ip[3], ftp_conf->port,
				        g_arm_config.basic_param.spot_id,
				        g_set_net_param.m_NetParam.m_DeviceID, filename);
				ftp_camera_filePath.file_size = get_file_size(
				                                    (char *) CAMERA_PARAM_FILE_PATH);
				debug("file: %s, size:%lu \n", ftp_camera_filePath.m_strFileURL, ftp_camera_filePath.file_size);

				sprintf(type, "%d", MSG_UPLOAD_CAMERA_PARAM);

				if (status != SUCCESS)
				{
					char text[100];
					sprintf(text, "send %s to ftp failed\n",
					        CAMERA_PARAM_FILE_PATH);

					log_error((char*)"MQ",text);
					upload_camera_param = -1;//需要回传错误信息
					continue;
				}
				upload_camera_param = 2;
			}
			else
			{
				upload_camera_param = -1;
			}
			printf("##############after ftp upload cameral xml : %d\n",upload_camera_param);


			//sleep(1);
		}
		if (1 == download_camera_param)
		{
			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				download_camera_param = -1;
				log_error((char*)"MQ", (char*)"获取FTP配置通道为NULL.\n");
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok == 1)*/)
			{
				download_camera_param = -1;
				log_error((char*)"MQ", (char*)"FTP不可用，或者mq不可用.\n");
				continue;
			}
			debug("file: %s, size:%lu \n", ftp_camera_filePath.m_strFileURL, ftp_camera_filePath.file_size);

			sprintf(bakfile, "%s.bak", CAMERA_PARAM_FILE_PATH);
			status = ftp_conf_chanel->ftp_get_file(
			             ftp_camera_filePath.m_strFileURL, bakfile);
			if (status != SUCCESS)
			{
				char text[100];
				sprintf(text, "send %s to ftp failed\n", CAMERA_PARAM_FILE_PATH);
				log_error((char*)"MQ",text);
				remove(bakfile);
				system((char*) "sync");
				//需要回传错误信息
				download_camera_param = -1;
			}
			else
			{
				Camera_config camera_config;
				memset(&camera_config, 0, sizeof(Camera_config));

				parse_xml_doc(bakfile, NULL, &camera_config, NULL);

				/*设置摄像机参数*/
				int ret = set_camera_config(&camera_config);
				if (SUCCESS != ret)
				{
					remove(bakfile);
					download_camera_param = -1;
					continue;
				}

				remove((char*) CAMERA_PARAM_FILE_PATH);
				ret = rename(bakfile, (char*) CAMERA_PARAM_FILE_PATH);
				if (ret != 0)
				{
					download_camera_param = -1;
					log_error((char*)"MQ", (char*)"rename camera_config.xml failed.\n");
					continue;
				}
				system((char*) "sync");

				//parse_xml_doc((char*) CAMERA_PARAM_FILE_PATH, NULL,
				//		&g_camera_config, NULL);
				memcpy(&g_camera_config, &camera_config, sizeof(camera_config));

				download_camera_param = 2;
			}
		}
		if (1 == download_hide_param_file) //下载隐性参数
		{
			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				char content[60];
				sprintf(content, "(%s|%d)获取FTP通道为NULL.\n", __FUNCTION__, __LINE__);
				log_error((char*)"MQ", (char*)content);
				download_hide_param_file = -1;
				continue;
			}

			if ((ftp_conf_chanel->get_status() != STA_FTP_OK) /*|| (mq_ok == 1)*/)
			{
				char content[60];
				sprintf(content, "(%s|%d)FTP OR mq unavaival\n", __FUNCTION__, __LINE__);
				log_error((char*)"MQ", (char*)content);

				download_hide_param_file = -1;
				continue;
			}

			status = ftp_conf_chanel->ftp_get_file(
			             ftp_hide_filePath.m_strFileURL, HIDE_PARAM_FILE_PATH);
			if (status != SUCCESS)
			{
				char text[100];
				sprintf(text, "send %s to ftp failed.\n", HIDE_PARAM_FILE_PATH);
				log_error((char*)"MQ",text);
				//需要回传错误信息
				download_hide_param_file = -1;
				continue;
			}
			sync();
			send_hidden_param_to_dsp();

			download_hide_param_file = 2;
		}
		if (1 == upload_hide_param_file) //上传隐性参数
		{
			debug("FTP 上传隐性参数文件.....\n");

			CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
			if (ftp_conf_chanel == NULL)
			{
				char content[60];
				sprintf(content, "(%s|%d)获取FTP通道为NULL.\n", __FUNCTION__, __LINE__);
				log_error((char*)"MQ", (char*)content);
				upload_hide_param_file = -1;

				continue;
			}

			if ((ftp_conf_chanel->get_status() == STA_FTP_OK) /*&& (mq_ok == 1)*/)
			{
				sprintf(filename, "%s", HIDE_PARAM_FILE);

				memset(dir, 0, DIR_LEN * DIR_NUM);
				sprintf(dir[0], "%s", g_arm_config.basic_param.spot_id); //地点编号
				sprintf(dir[1], "%s", g_set_net_param.m_NetParam.m_DeviceID); //设备编号
				sprintf(dir[2], "%s", "filetrans");
				sprintf(dir[3], "%s", "upload");

				status = ftp_conf_chanel->ftp_put_file(
				             (char *) HIDE_PARAM_FILE_PATH, dir, filename);
				ftp_conf = get_ftp_chanel(FTP_CHANNEL_CONFIG)->get_config();
				sprintf(ftp_hide_filePath.m_strFileURL,
				        "ftp://%s:%s@%d.%d.%d.%d:%d"
				        "/%s/%s/filetrans/upload/%s", ftp_conf->user,
				        ftp_conf->passwd, ftp_conf->ip[0], ftp_conf->ip[1],
				        ftp_conf->ip[2], ftp_conf->ip[3], ftp_conf->port,
				        g_arm_config.basic_param.spot_id,
				        g_set_net_param.m_NetParam.m_DeviceID, filename);
				sprintf(type, "%d", MSG_UPLOAD_HIDE_PARAM_FILE);

				if (status != SUCCESS)
				{
					char text[100];
					sprintf(text, "send %s to ftp failed\n",
					        HIDE_PARAM_FILE_PATH);

					log_error((char*)"MQ",text);
					upload_hide_param_file = -1;//需要回传错误信息
					continue;
				}
			}

			upload_hide_param_file = 2;

		}
		if (1 == capturePic)
		{
			capturePic = 0;
			capture_pic_upload();
		}
		if (1 == flag_ptz_control)
		{

			if (ControlSystemData(SFIELD_SEND_PTZ_CONTROL,
				                      &ptz_msg, sizeof(PTZ_MSG)) < 0)
			{
				printf("Send ptz control failed !\n");
//				return -1;
			}
			flag_ptz_control = 0;

		}
	}

	pthread_exit(NULL);
}


/*******************************************************************************
 * 函数名: capture_pic_upload
 * 功  能: 抓拍图片并上传
 * 返回值: 成功，返回0；出错，返回-1
*******************************************************************************/
int capture_pic_upload(void)
{
	AV_DATA av_data;
	int ret = -1;

	if (GetAVData(AV_OP_GET_MJPEG_SERIAL, -1, &av_data) != RET_SUCCESS)
	{
		debug("Can't get JPEG image\n");
		return ret;
	}

	debug("serial: %u\n", av_data.serial);

	if (GetAVData(AV_OP_LOCK_MJPEG, av_data.serial, &av_data) != RET_SUCCESS)
	{
		debug("lock jpeg %d failed\n", av_data.serial);
		return ret;
	}

	do
	{
		CLASS_FTP *ftp_conf_chanel = get_ftp_chanel(FTP_CHANNEL_CONFIG);
		if (ftp_conf_chanel == NULL)
		{
			debug("Get FTP_CHANNEL_CONFIG failed !");
			break;
		}

		if ((ftp_conf_chanel->get_status() != STA_FTP_OK))
		{
			debug("FTP_CHANNEL_CONFIG status not ok!\n");
			break;
		}

		char path[DIR_LEN];
		sprintf(path, "/%s/%s/capture",
		        g_arm_config.basic_param.spot_id,
		        g_set_net_param.m_NetParam.m_DeviceID);

		int status = ftp_conf_chanel->ftp_put_data(
		                 path, "capture.jpg", av_data.ptr, av_data.size);
		if (status != SUCCESS)
		{
			debug("抓拍图片ftp发送失败!\n");
			break;
		}

		debug("抓拍图片ftp发送成功!\n");

		memset(&ftp_filePath, 0, sizeof(ftp_filePath));

		sprintf(ftp_filePath.m_strFileURL, "/%s/%s/capture/capture.jpg",
		        g_arm_config.basic_param.spot_id,
		        g_set_net_param.m_NetParam.m_DeviceID);

		printf("capture dir is %s\n", ftp_filePath.m_strFileURL);

		ret = 0;
	}
	while (0);

	GetAVData(AV_OP_UNLOCK_MJPEG, av_data.serial, &av_data);

	if (ret < 0)
	{
		capturePic = -1;
	}
	else
	{
		capturePic = 2;
	}

	return ret;
}
