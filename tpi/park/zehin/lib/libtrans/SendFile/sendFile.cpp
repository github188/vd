#include <string.h>
#include "../Http/httpclient.h"
#include <pthread.h>
#include <errno.h>
#include "sendFile.h"

#ifdef WIN32
#pragma comment(lib, "./lib/pthread.lib")
#endif

//stRepInfo *g_stRetInfo = NULL;


#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year     = wtm.wYear - 1900;
	tm.tm_mon     = wtm.wMonth - 1;
	tm.tm_mday     = wtm.wDay;
	tm.tm_hour     = wtm.wHour;
	tm.tm_min     = wtm.wMinute;
	tm.tm_sec     = wtm.wSecond;
	tm. tm_isdst    = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

static pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

int SF_SendForPOSTFile(const char *szUrl, const char *szFileName, char *szRepUrl)
{
    pthread_mutex_lock(&fastmutex);
	std::string strResponse(szRepUrl);
	CHttpClient hHttpClient;
	hHttpClient.Init();
	int iRet = hHttpClient.http_post_file(szUrl, szFileName,strResponse);

	strncpy(szRepUrl, strResponse.c_str(), strResponse.length());
	hHttpClient.UnInit();
    pthread_mutex_unlock(&fastmutex);
	return iRet;
}

int SF_SendForPOSTData(const char *szUrl,const char* filename, const char *data, size_t size, char *szRepUrl)
{
    pthread_mutex_lock(&fastmutex);
	std::string strResponse(szRepUrl);
	CHttpClient hHttpClient;
	hHttpClient.Init();
	int iRet = hHttpClient.http_post_data(szUrl,filename, data,size,strResponse);

	strncpy(szRepUrl, strResponse.c_str(), strResponse.length());
	hHttpClient.UnInit();
    pthread_mutex_unlock(&fastmutex);
	return iRet;
}
