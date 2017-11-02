/***********************************************************************
 * mq_listen.h
 *
 *  Created on: 2013-4-9 *
 ***********************************************************************/

#ifndef MQ_LISTEN_H_
#define MQ_LISTEN_H_
#include "mq.h"

#define UPGRADE_FILE_PATH "/grade.bin"
#define FPGA_UPGRADE_FILE "/opt/ipnc/isp_ccd8m_420p.bin"  //FPGA��������
#define DSP_UPGRADE_FILE "/opt/ipnc/Ep2013-C6678-le.bin"    //DSP��������
#define MCU1_UPGRADE_FILE "/mcu1_upgrade.bin"  //��Ƭ��1 ��������
#define MCU2_UPGRADE_FILE "/mcu2_upgrade.bin"  //��Ƭ��2 ��������

typedef struct _New_Grade_Header
{
	char board_type;    	//0��ARM��; 1��DSP; 2:FPGA; 3:��Ƭ��1; 4:��Ƭ��2
	char name[50];			//�ļ�����
	unsigned int buffer_len;//�ļ�����
	char path[256];			//�ļ���arm���ϵ�·��
	unsigned int check_sum; //У���
}Grade_New_Header;

extern SET_NET_PARAM g_set_net_param;
extern Online_Device online_device;

extern FTP_FILEPATH ftp_filePath_upgrade;


//#########################################//#########################################//
void recv_msg_broadcast(const Message *msg) ;
void recv_msg_down(const Message *msg);

//#########################################//#########################################//

#endif /* MQ_LISTEN_H_ */
