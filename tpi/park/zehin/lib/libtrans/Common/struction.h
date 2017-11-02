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
    int                                 iCamID;                               //镜头id
    int                                 iFileType;                            //图片类型 (0-请求抓图 1-定时抓图 2-报警联动抓图)
    int								    iCamIndex;                            //镜头序号
    stCreateFileDate					stCreateDate;						  //创建文件日期
    char								sFileName[260];					      //文件名字(可以包含路径，路径分隔符请使用"/")
    char								sFileDescript[512];				      //文件描述
    int									iFileLen;                             //图片长度 不用自己赋值,赋值0即可
    char								sCustomData[100];				      //自定义数据
	int									iVersion;						      //版本号,不用赋值
};

struct stRepInfo
{
	int iRepCode;
	char sUrl[256];
};
