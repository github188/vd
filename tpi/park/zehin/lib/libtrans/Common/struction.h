#pragma once

struct stCreateFileDate
{
    int		iYear;
    int		iMonth;
    int		iDay;
    int		iHour;
    int		iMinutes;
    int		iSecond;
};

struct  stFileInfo
{
    int                                 iCamID;                               //��ͷid
    int                                 iFileType;                            //ͼƬ���� (0-����ץͼ 1-��ʱץͼ 2-��������ץͼ)
    int								    iCamIndex;                            //��ͷ���
    stCreateFileDate					stCreateDate;						  //�����ļ�����
    char								sFileName[260];					      //�ļ�����(���԰���·����·���ָ�����ʹ��"/")
    char								sFileDescript[512];				      //�ļ�����
    int									iFileLen;                             //ͼƬ���� �����Լ���ֵ,��ֵ0����
    char								sCustomData[100];				      //�Զ�������
	int									iVersion;						      //�汾��,���ø�ֵ
};

struct stRepInfo
{
	int iRepCode;
	char sUrl[256];
};
