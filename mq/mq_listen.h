/***********************************************************************
 * mq_listen.h
 *
 *  Created on: 2013-4-9 *
 ***********************************************************************/

#ifndef MQ_LISTEN_H_
#define MQ_LISTEN_H_
#include "mq.h"

#define UPGRADE_FILE_PATH "/grade.bin"
#define FPGA_UPGRADE_FILE "/opt/ipnc/isp_ccd8m_420p.bin"  //FPGA升级程序
#define DSP_UPGRADE_FILE "/opt/ipnc/Ep2013-C6678-le.bin"    //DSP升级程序
#define MCU1_UPGRADE_FILE "/mcu1_upgrade.bin"  //单片机1 升级程序
#define MCU2_UPGRADE_FILE "/mcu2_upgrade.bin"  //单片机2 升级程序

typedef struct _New_Grade_Header
{
	char board_type;    	//0：ARM板; 1：DSP; 2:FPGA; 3:单片机1; 4:单片机2
	char name[50];			//文件名称
	unsigned int buffer_len;//文件长度
	char path[256];			//文件在arm板上的路径
	unsigned int check_sum; //校验和
}Grade_New_Header;

extern SET_NET_PARAM g_set_net_param;
extern Online_Device online_device;

extern FTP_FILEPATH ftp_filePath_upgrade;


//#########################################//#########################################//
void recv_msg_broadcast(const Message *msg) ;
void recv_msg_down(const Message *msg);

//#########################################//#########################################//

#endif /* MQ_LISTEN_H_ */
