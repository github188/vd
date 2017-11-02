/*****************************************************************************
 * debug.h
 *****************************************************************************
 * Copyright (C) 2013, 2015 the EP-HI2013 team

 * Foundation, Inc., qingdao hisense, China.
 *****************************************************************************/

  
#ifndef _DEBUG_H
#define _DEBUG_H


#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "commonfuncs.h"



//�������Դ�ӡ
#define _DEBUG_



extern char g_thread_name[]; //�����߳�����;
extern char g_str[];

extern pthread_mutex_t mutex_wr_log;



//������������д�ļ�
int file_write(char *file_name, char *addr, int len);

//ȡϵͳʱ��
//char * get_date_time(void);

//�����߳���
void set_thread_name(char *thread_name);
	
//ȡ�߳���
char* get_thread_name(void);




#ifdef _DEBUG_
	//��ʱ��Ĵ�ӡ����
	#define print_time(msg...) printf("[%s]: <<thread: %s>> : ",get_date_time(),get_thread_name()),printf(msg)
	
	//д�̶���־�ļ���������ʱ���ӡ������Ҫ����д������Ϣ
	#define write_log_time(msg...) printf("[%s]: <<thread: %s>> : ",get_date_time(),get_thread_name()),printf(msg),pthread_mutex_lock(&mutex_wr_log),sprintf(g_str,msg),file_write("h264.log",g_str,strlen(g_str)),pthread_mutex_unlock(&mutex_wr_log)

#else
	#define print_time(msg...)
	#define write_log_time(msg...) 

#endif





#endif



