/*****************************************************************************
 * debug.c
 *****************************************************************************
 * Copyright (C) 2013, 2015 the EP-HI2013 team

 * Foundation, Inc., qingdao hisense, China.
 *****************************************************************************/


 
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>   
#include <unistd.h>
#include <time.h>


#include <sys/un.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/shm.h>
#include <pthread.h>


#include "debug.h"



char g_thread_name[50]; //保存线程名字;
char g_str[1024];//字符串，写文件前转换使用

pthread_mutex_t mutex_wr_log;//写文件锁




//将缓冲中内容写文件
int file_write(char *file_name, char *addr, int len)
{
	FILE * pfile = NULL;

	if(addr==NULL)
	{
		printf("file_write: addr is NULL\n");
		return -1;
	}
	if(file_name==NULL)
	{
		printf("file_write: file_name is NULL\n");
		return -1;
	}
	if(len<0)
	{
		printf("file_write: len is %d\n",len);
		return -1;
	}	

	pfile = fopen(file_name, "ab");
	if (pfile == NULL)
	{
		printf("open file is failed!");
		return -1;
	}
	
	int i;
	for(i=0;i<10;i++)//查看前10个值是否正确
	{
		printf("file_write: addr[%d]=%d\n",i, addr[i]);
	}
	
	fwrite(addr, 1, len, pfile);
	fclose(pfile);
	return 0;
}

//
////获取系统时间
//char * get_date_time(void)
//{
//	time_t now;
//	static char str_m[100];
//	struct tm *time_now;
//	struct tm tm_read;
//
//	memset(str_m, 0, sizeof(str_m));
//
//	time(&now);
//
//	time_now = localtime_r(&now, &tm_read);
//	strftime(str_m, 28, "%Y-%m-%d %H:%M:%S", time_now);//2013-07-31 10:29:03
//
//	return str_m;
//}


//带时间的打印函数
//int debug_time(char * str)
//{}




#include <map>
#include <vector>
#include <iostream>
#include <string>

#define __NR_gettid 224

#define gettid() syscall(__NR_gettid)

using namespace std;

typedef map<int, string> thread_map;

thread_map map_thread_name; //保存线程名关系;

int init_thread_map()
{
	static int flag_first=1;
	if(flag_first==1)
	{
		map_thread_name.clear();
	}

	flag_first=0;
	
	return 0;
}

int thread_map_set(const char *name)
{
	int thread_id;
	string value = name;

	init_thread_map();
	
	thread_id = gettid();

	map_thread_name.insert(thread_map::value_type(thread_id, value));
	
	return 0;

}
int thread_map_get(char *name)
{
	int map_key;
	string value;

	thread_map::iterator it;
	map_key=gettid();

//	printf("get,thread_name=%s,thread_id=%d\n",name,map_key);

	if (map_thread_name.count(map_key) <= 0)
	{
//		printf("map_thread_name:>> 没有找到 key={%d},count = 0\n", map_key);
		return -1;
	}

	it = map_thread_name.find(map_key);
	if (it == map_thread_name.end()) //没找到
	{
//		printf("map_thread_name:>> 没有找到map,key={%d}\n", map_key);
		return -1;

	} else //找到
	{
		//		printf("map_find:>> 找到map_key= %s,value_str=%s,comment = %s \n", it->first.c_str(), it->second.value.c_str(), it->second.comment.c_str());

		value = it->second;
	}

	sprintf(name,"%s",value.c_str());

	return 0;
}

void set_thread_name(char *thread_name)
{
	prctl(PR_SET_NAME, thread_name);
	thread_map_set(thread_name);
}


// 功能： 获得线程名字;
char* get_thread_name(void)
{
	//prctl(PR_GET_NAME, g_thread_name);
	memset(g_thread_name,0,sizeof(g_thread_name));
	thread_map_get(g_thread_name);
	return g_thread_name;

}




