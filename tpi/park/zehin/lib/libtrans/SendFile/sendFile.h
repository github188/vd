#pragma once
#include "../Common/struction.h"

#ifdef _WIN32
	#include <WinSock.h>
	#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

#define SF_UDP			1
#define SF_TCP			2
#define SF_HTTP			3
enum RetCode
{
	SF_SUCCESS			= 1,			//成功
	SF_TIMEDOUT			= 0,			//超时
	SF_PATHCREATEERR	= -1,			//文件路径创建失败
	SF_FILEEXIST		= -2,			//文件已存在
	SF_FILEWRITEERR		= -3,			//文件写入失败
	SF_FILECREATEERR	= -4,			//文件创建失败
	SF_SOCKETINVALID	= -5,			//Socket无效
	SF_FILEISNULL		= -6,			//文件流为空
	SF_IOCTRLERROR		= -7,			//IO控制错误
	SF_SELECTERROR		= -8,			//SELECT错误
	SF_SENDDATAERROR	= -9,			//发送数据错误
	SF_RECVDATAERROR	= -10,			//接收数据错误
	SF_PARAMERROR		= -12,			//参数错误
	SF_UNKNOWNERROR		= -11

};

//功能:获取URL 只有返回值为成功或文件存在时返回正确的URL
//参数:
//@szUrl			post的url
//@szFileName		文件存放的路径
//szRepUrl			服务器回复URL
//返回值：			0:成功 其它:失败
int SF_SendForPOSTFile(const char *szUrl, const char *szFileName, char *szRepUrl);
int SF_SendForPOSTData(const char *szUrl, const char *filename, const char *data, size_t size, char *szRepUrl);

#define SF_API extern "C"
#define sf_send(a, b, c, d, e, f, g) \
            SF_SendForTCP(a, b, c, d, e, f, g);

//功能:通过TCP方式上传图片并获取URL
//返回值:
//@RetCode:			请参照enum RetCode
//参数：
//@szIp				服务器IP
//@iPort			服务器端口
//@pFileBuf			图片缓冲流
//@iFileLen			图片流长度
//@FileInfo			图片信息  传入信息时结构体中picLen赋值为0即可
//@iTimedOutSec		超时时间
//@szRepUrl			服务器回复URL
SF_API RetCode SF_SendForTCP(const char szIp[16], int iPort, char *pFileBuf, int iFileLen, stFileInfo fileInfo, int iTimedOutSec, char *szRepUrl);
