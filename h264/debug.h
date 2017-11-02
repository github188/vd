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



//开启调试打印
#define _DEBUG_



extern char g_thread_name[]; //保存线程名字;
extern char g_str[];

extern pthread_mutex_t mutex_wr_log;



//将缓冲中内容写文件
int file_write(char *file_name, char *addr, int len);

//取系统时间
//char * get_date_time(void);

//设置线程名
void set_thread_name(char *thread_name);
	
//取线程名
char* get_thread_name(void);




#ifdef _DEBUG_
	//带时间的打印函数
	#define print_time(msg...) printf("[%s]: <<thread: %s>> : ",get_date_time(),get_thread_name()),printf(msg)
	
	//写固定日志文件，并附带时间打印－－主要用于写调试信息
	#define write_log_time(msg...) printf("[%s]: <<thread: %s>> : ",get_date_time(),get_thread_name()),printf(msg),pthread_mutex_lock(&mutex_wr_log),sprintf(g_str,msg),file_write("h264.log",g_str,strlen(g_str)),pthread_mutex_unlock(&mutex_wr_log)

#else
	#define print_time(msg...)
	#define write_log_time(msg...) 

#endif





#endif



