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
	SF_SUCCESS			= 1,			//�ɹ�
	SF_TIMEDOUT			= 0,			//��ʱ
	SF_PATHCREATEERR	= -1,			//�ļ�·������ʧ��
	SF_FILEEXIST		= -2,			//�ļ��Ѵ���
	SF_FILEWRITEERR		= -3,			//�ļ�д��ʧ��
	SF_FILECREATEERR	= -4,			//�ļ�����ʧ��
	SF_SOCKETINVALID	= -5,			//Socket��Ч
	SF_FILEISNULL		= -6,			//�ļ���Ϊ��
	SF_IOCTRLERROR		= -7,			//IO���ƴ���
	SF_SELECTERROR		= -8,			//SELECT����
	SF_SENDDATAERROR	= -9,			//�������ݴ���
	SF_RECVDATAERROR	= -10,			//�������ݴ���
	SF_PARAMERROR		= -12,			//��������
	SF_UNKNOWNERROR		= -11

};

//����:��ȡURL ֻ�з���ֵΪ�ɹ����ļ�����ʱ������ȷ��URL
//����:
//@szUrl			post��url
//@szFileName		�ļ���ŵ�·��
//szRepUrl			�������ظ�URL
//����ֵ��			0:�ɹ� ����:ʧ��
int SF_SendForPOSTFile(const char *szUrl, const char *szFileName, char *szRepUrl);
int SF_SendForPOSTData(const char *szUrl, const char *filename, const char *data, size_t size, char *szRepUrl);

#define SF_API extern "C"
#define sf_send(a, b, c, d, e, f, g) \
            SF_SendForTCP(a, b, c, d, e, f, g);

//����:ͨ��TCP��ʽ�ϴ�ͼƬ����ȡURL
//����ֵ:
//@RetCode:			�����enum RetCode
//������
//@szIp				������IP
//@iPort			�������˿�
//@pFileBuf			ͼƬ������
//@iFileLen			ͼƬ������
//@FileInfo			ͼƬ��Ϣ  ������Ϣʱ�ṹ����picLen��ֵΪ0����
//@iTimedOutSec		��ʱʱ��
//@szRepUrl			�������ظ�URL
SF_API RetCode SF_SendForTCP(const char szIp[16], int iPort, char *pFileBuf, int iFileLen, stFileInfo fileInfo, int iTimedOutSec, char *szRepUrl);
