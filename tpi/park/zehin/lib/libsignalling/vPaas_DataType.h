//vPaas SDK 结构体文件
#ifndef VPAAS_DATA_TYPE_H
#define VPAAS_DATA_TYPE_H

#define BUFSIZE_32 32
#define BUFSIZE_64 64
#define BUFSIZE_128 128
#define BUFSIZE_256 256
#define BUFSIZE_512 512
#define BUFSIZE_1024 1024
#define BUFSIZE_2K	2048
#define BUFSIZE_4K	4096
#define BUFSIZE_10K 10240


//#define  USE_WIN          //windows系统
#define  USE_LINUX      //linux系统
//#define  USE_APPLE      //IOS系统



#ifdef USE_WIN
#define VPAAS_API extern "C" __declspec(dllexport)
/*
回调函数参数
sCmd			：命令关键字
lpStructParam	：自解析结构体参数
iStructLen		：结构体参数长度
lpXMLDataParam	：原始的xml结构
iXMLDataLen		：xml结构长度
sFromIP			：数据包from的ip
iFromPort		：数据包from的port*/
typedef int(__stdcall *CBF_RecvCmd)(char*sCmd,void* lpStructParam,int iStructLen,void* lpXMLDataParam,int iXMLDataLen,char*sFromIP,int iFromPort,void* pCustomDate);
#else
typedef int( *CBF_RecvCmd)(char*sCmd,void* lpStructParam,int iStructLen,void* lpXMLDataParam,int iXMLDataLen,char*sFromIP,int iFromPort,void* pCustomDate);
#endif


#ifdef USE_LINUX
#define VPAAS_API extern "C"
#endif

#ifdef USE_APPLE
#define VPAAS_API extern "C"
#endif





/*
关于整个vPaas的SDK的接口风格的说明
vPaas的SDK总共分2个大部分，播放端(客户端)SDK 和 云终端SDK
其中，
1）播放端SDK主要用于 控件、手机APP、播放客户端的开发使用
播放端SDK接口都以vPaasC_开头
结构体定义以_C_结尾，比如 stZehinRealStart_C_ ，表示播放端使用

2）云终端SDK用于 嵌入式摄像机 或 嵌入器数据采集终端 使用
云终端SDK接口都以vPaasT_开头
结构体定义以_T_结尾，比如 stZehinRealStart_T_，表示云终端使用

*/

typedef struct
{
    int		iRequestType;					//取流请求方式（0-自动   1-P2P取流   2-中转流媒体取流）
    int		iCamID;							//镜头id
	int		iStreamType;					//码流类型（0-主码流  1-子码流）
    char    sClientPublicIP[50];			//对外监听接收的IP
    int		iClientPublicPort;				//对外监听接收的端口
    int		iTerminalLocalPort;				//云终端发送流的端口（通过vPaasC_NAT_TEST可以获得）
	char	sUser[50];						//播放端用户名
    int		iCustomData;					//自定义数据
}stZehinRealStart_C_;						//客户端相关 


typedef struct
{
	int		iRequestType;					//取流请求方式（0-自动   1-P2P取流   2-中转流媒体取流）
	int		iCamID;							//镜头id
	int		iCamIndex;						//镜头序号
	int		iStreamType;					//码流类型（0-主码流  1-子码流）
	int		bStream;						//是否发流到中转服务
	char    sClientPublicIP[50];			//视频流目的地的公网ip
	int		iClientPublicPort;				//视频流目的地的公网端口
	int		iTerminalLocalPort;				//云终端发送流的端口
	char	sUser[50];						//播放端用户名
	int		iCustomData;					//自定义数据
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinRealStart_T_;						// 云终端相关



typedef struct
{
	int		iCamID;							//镜头id
	int		iRetCode;						//返回值（1成功 0云终端不在线 -1 镜头不在线 -2连接镜头错误）
	int		iRealType;						//流类型（0-SDK方式  1-帧方式）
	int		bStream;						//是否发流到中转服务
	char    sStreamPublicIP[50];			//视频流目的地的公网ip
	int		iStreamPublicPort;				//视频流目的地的公网端口
	char	sToUser[50];					//播放端用户名
	int		iCustomData;					//自定义数据
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinRealStartRep_T_,stZehinRealStartRep_C_;// 云终端相关




typedef struct
{
	int		iCamIndex;						//镜头序号
	int		iStreamType;					//码流类型（0-主码流  1-子码流）
	int		bStream;						//是否发流到中转服务
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinRealStop_T_;						// 云终端相关

typedef struct
{
	int		iRequestType;					//RequestType：请求方式  0-自动   1-强制云终端控制   2-强制云台服务控制
	int		iCamID;							//镜头id
	int		iPtzCmd;						//云台控制命令(21上， 22-下， 23-左， 24-右，11-焦距变大，12-焦距变小，13-变倍大，14-变倍小，15-光圈大，16-光圈小)
	int		iParam1;						//开始停止（0-开始 1-停止）
	int		iParam2;						//速度（0-7）
	int     iParam3;						//
	int		iParam4;						//
}stZehinRealPTZ_C_;							// 客户端相关



typedef struct
{
	int		iCamIndex;						//镜头id
	int		iPtzCmd;						//云台控制命令(21上， 22-下， 23-左， 24-右，11-焦距变大，12-焦距变小，13-变倍大，14-变倍小，15-光圈大，16-光圈小)
	int		iParam1;						//开始停止（0-开始 1-停止）
	int		iParam2;						//速度（0-7）
	int     iParam3;						//
	int		iParam4;						//
}stZehinRealPTZ_T_;							// 云终端相关


typedef struct
{
	int		iCamID;							//镜头id
	int		iUploadType;					//0-UDP , 1-http post, 2-ftp
	char	sUpLoadSer[50];					//上传服务器
	int		iUploadPort;					//上传服务端口
	char	sCustomData[100];				//自定义数据
}stZehinRealCapture_C_;						//云终端抓图上传

typedef struct
{
	int		iCamID;							//镜头id
	int		iCamIndex;						//镜头序号
	int		iUploadType;					//0-UDP , 1-http post, 2-ftp
	char	sUpLoadSer[50];					//上传服务器
	int		iUploadPort;					//上传服务端口
	char	sCustomData[100];
}stZehinRealCapture_T_;						//云终端抓图上传

typedef struct
{
	int		PictureType;					//图片类型 (0-请求抓图 1-定时抓图 2-报警联动抓图)
	int		iCamID;							//镜头id
	int		iCamIndex;						//镜头序号
	char	sCapDate[128];					//抓图时间
	char	sFileName[100];					//文件名字
	char	sFileDescript[512];				//文件描述
	char	sCustomData[100];				//自定义数据

}stZehinRealCaptureFinish_T_;				//云终端抓图上传

typedef struct
{
	int		iCamID;							//镜头id
	int		iDate;							//
	int		iBeginTime;						//
	int		iEndTime;						//
	char	sUser[50];						//
}stZehinBackSearch_C_;						// 客户端相关

typedef struct
{
	int		iCamID;							//镜头id
	int		iCamIndex;
	int		iDate;							//
	int		iBeginTime;						//
	int		iEndTime;						//
	char	sToUser[50];					//
}stZehinBackSearch_T_;						// 客户端相关

typedef struct
{
	int		iRetCode;						//返回码（1：成功 小于1：失败  ）
	int		iCamID;							//镜头id
	int		iDate;							//日期（例 20140910
	char    sToUsr[50];						//用户名

	int iResultSize;						//返回结果的条数
	struct
	{
		int iStartTime;						//开始时间点（例 120503）
		int iStopTime;						//停止时间点（例 120503）
	}ResultInfo[1024*10];
}stZehinBackSearchResult_T_,stZehinBackSearchResult_C_;//



typedef struct
{
	int		iRequestType;					//取流请求方式（0-自动   1-P2P取流   2-中转流媒体取流）
	int		iCamID;							//镜头id
	int		iDate;							//日期
	int		iTime;							//时间
	char    sClientPublicIP[50];			//对外监听接收的IP
	int		iClientPublicPort;				//对外监听接收的端口
	int		iTerminalLocalPort;				//云终端发送流的端口（通过vPaasC_NAT_TEST可以获得）
	char	sUser[50];						//播放端用户名
	int		iCustomData;					//自定义数据
}stZehinBackStart_C_;						//客户端相关 

typedef struct
{
	int		iRequestType;					//取流请求方式（0-自动   1-P2P取流   2-中转流媒体取流）
	int		iCamID;							//镜头id
	int		iCamIndex;
	int		iDate;							//日期
	int		iTime;							//时间
	int		bStream;						//是否发流到中转服务
	char    sClientPublicIP[50];			//对外监听接收的IP
	int		iClientPublicPort;				//对外监听接收的端口
	int		iTerminalLocalPort;				//云终端发送流的端口（通过vPaasC_NAT_TEST可以获得）
	char	sToUser[50];					//播放端用户名
	int		iCustomData;					//自定义数据
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinBackStart_T_;						//云终端相关 



typedef struct
{	
	int		iCamID;							//镜头id
	int		iRetCode;						//返回值（1成功 0云终端不在线 -1 镜头不在线 -2连接镜头错误）
	int		iRealType;						//流类型（0-SDK方式  1-帧方式）
	int		bStream;						//是否发流到中转服务
	char    sStreamPublicIP[50];			//视频流目的地的公网ip
	int		iStreamPublicPort;				//视频流目的地的公网端口
	char	sToUser[50];					//播放端用户名
	int		iCustomData;					//自定义数据
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinBackStartRep_T_,stZehinBackStartRep_C_;//云终端相关 


typedef struct
{	
	int		iPause;			//
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinBackPause_T_;						//云终端相关 

typedef struct
{	
	int		iSeek;			//
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinBackSeek_T_;						//云终端相关 

typedef struct
{	
	int		iStreamCustomID;				//每一路视频流的唯一标示（由vPaas分配的唯一号，用来标示流连接）
}stZehinBackStop_T_;						//云终端相关 


typedef struct
{	
	int		iCamID;
	int		iCamIndex;
	char	sToUser[50];
}stZehinGetRecState_T_;						//云终端相关    请求获取镜头的录像状态


typedef struct
{	
	int		iCamID;
	int		iStatus;
	char	sToUser[50];
}stZehinRecState_C_;						//镜头的录像状态返回

typedef struct
{	
	int		iCamID;
	int		iCamIndex;
	int		iStatus;
	char	sToUser[50];
}stZehinSetRecState_T_;						//云终端相关   设置镜头的录像状态

typedef struct
{
    int     iCamID;

    int     iClientLocalPort;
    int		iClientPublicPort;
    char	sClientPublicIP[50];

    int     iTerminalLocalPort;
    int		iTerminalPublicPort;
    char	sTerminalPublicIP[50];
	char	sUser[50];
	int		iCustomData;
}stZehinNatTest;//


typedef struct
{	
	char	sTerminalName[50];				//云终端的名称
	int		iActive;						//标示是否启用
	char	sSerTag[50];					//服务标示
	char	sUsrTag[50];					//用户标示
	char	sLocationTag[50];				//位置标示
}stZehinGetTagInfo_T_;						//云终端相关   获取标示信息

typedef struct
{	
	char	sTerminalSn[50];				//云终端序列号
	char	sTerminalName[50];				//云终端的名称
	int		iActive;						//标示是否启用
	char	sSerTag[50];					//服务标示
	char	sUsrTag[50];					//用户标示
	char	sLocationTag[50];				//位置标示
}stZehinSetTagInfo_T_;						//云终端相关   设置标示信息


typedef struct
{	
	char	sTerminalSn[50];				//云终端序列号
	int		iDevIndex;						//设备序号
	int		iIsCamDev;						//标记是否是镜头设备（0-末端设备  1-镜头设备）
	int		iIsOnline;						//在线状态
}stZehinIsOnlineInfo_T_;					//云终端相关   镜头（或末端设备）在线状态上传

typedef struct
{	
	char	sTerminalSn[50];				//云终端序列号
	int		iDevIndex;						//设备序号（唯一区分，从1开始）
	char	sDevname[50];					//摄像机名字
	char	sDevNo[50];						//摄像机编号
	int		iDevTypeID;						//设备类型ID
	char	sCamDevIP[50];					//镜头的IP
	int		iCamport;						//镜头端口
	char	sCamUsr[50];					//镜头用户名
	char	sCamPass[50];					//镜头密码
	int		iCamChannel;					//镜头通道
	char	sCamDevUrlMain[200];			//主码流URL
	char	sCamDevUrlSub[200];				//子码流URL
}stZehinCamInfo_T_;							//云终端相关   镜头信息

typedef struct
{
	int		iRetCode;						//返回码（1：成功 小于1：失败  ）
	int		iResultSize;					//返回结果的条数
	stZehinCamInfo_T_ CamInfoArray[1024*10];
}stZehinGetCamListInfo_T_;//



typedef struct
{	
	char	sTerminalSn[50];				//云终端序列号
	int		iDevIndex;						//设备序号（唯一区分，从1开始）和stZehinCamInfo_T_设备序号区分
	char	sDevname[50];					//设备名称	
	char	sDevNo[50];						//设备编号
	int		iDevTypeID;						//设备类型id

	int		iConnectMode;					//连接方式（0-ip  1-com）
	char	sDataDevIP[50];					//设备ip（iConnectMode为0时有效）
	int		iDataDevPort;					//设备端口（iConnectMode为0时有效）
	int		iSerialNo;						//设备串口号（iConnectMode为 1 时有效
	int		iSerialMode;					//设备串口模式（0-232 ，1-485）（iConnectMode为 1 时有效
	int		iSerialAddr;					//设备485地址（iConnectMode为 1 时有效
	int		iFunCode;						//功能码（iConnectMode为 1 时有效
	int		iSerialBand;					//设备串口波特率（iConnectMode为 1 时有效
	int		iSerialData;					//设备串口数据位（iConnectMode为 1 时有效
	int		iSerialStop;					//设备串口停止位（iConnectMode为 1 时有效
	int		iSerialCheck;					//设备串口校验位（iConnectMode为 1 时有效
	int		iIsDataMode;					//是否使用数据模板
}stZehinDevInfo_T_;							//云终端相关   末端设备信息

typedef struct
{
	int		iRetCode;						//返回码（1：成功 小于1：失败  ）
	int		iResultSize;					//返回结果的条数
	stZehinDevInfo_T_ DevInfoArray[1024*10];
}stZehinGetDevListInfo_T_;//



typedef struct
{	
	char	sTerminalSn[50];				//云终端序列号
	int		iDevIndex;						//设备序号
	char	sDataName[50];					//数据点名称
	int		iDataIndex;						//数据点序号
	int		iIsDataMode;					//是否使用数据模板

	int		iTemplatePropID;				//模板属性id
	int		iDataSort;						//数据分类（0-模拟量 1-开关量）
	int		iPropParam;						//参数
	char	sPropName[50];					//属性名字

	int		iRegistType;					//寄存器类型
	int		iPropReadType;					//读写类型
	int		iDataFormat;					//数据格式（2进制8进制等）

	char	sAddr[50];						//寄存器地址
	char	sPropCode[50];					//编码（0-ascii ，1-RTU）

	int		iDataType;						//数据类型（寻址类型）字节、位
	char	sBytePos[50];					//字节位置
	int		iBitPos;						//位bit位置
	int		iDataL;							//数据范围下限
	int		iDataH;							//数据范围上限
	int 	iDataPrecision;					//数据精度
	float	fMultFactor;					//乘法系数
	float	fAddFactor;						//加法系数
	char	sDataUnit[50];					//数据单位
	int		iTval;							//采集周期（秒）
	
}stZehinDataPointInfo_T_;					//云终端相关   数据点信息


typedef struct
{
	char	sTerminalSn[50];				//云终端序列号
	char	sPackageVersion[50];			//更新包版本
	char	sPackageURL[50];				//更新包url
}stZehinUpdateVersion_T_;//


typedef struct
{
	char	sTerminalSn[50];				//云终端序列号
	char	sURL[100];						//url
}stZehinConfBackup_T_;//

typedef struct
{
	char	sTerminalSn[50];				//云终端序列号
	char	sURL[100];						//url
}stZehinConfRestore_T_;//


typedef struct
{
	char	sTerminalSn[50];				//云终端序列号
	char	sCmdInfo[500];	//
}stZehinRemoteCmd_T_;//


typedef struct  
{
	int		iRequestType;					//请求方式  0-自动   1-强制P2P   2-强制中转
	int 	iCamID;							//镜头id
	int		iDate;							//日期(格式为20150101)
	int		iBeginTime;						//开始时间点（格式为000000） 
	int		iEndTime;						//结束时间点（格式为000000） 
	int		iClientPublicPort;				//播放端的对外端口
	char	sClientPublicIP[50];			//播放端的对外IP
	int 	iTerminalLocalPort;				//云终端的本地端口（发送视频流）
	char	sUser[50];						//播放端的用户名
	int		iCustomData;					//自定义数据

}stZehinVideoDownLoadStart_C_, stZehinVideoDownLoadStart_T_;


typedef struct  
{
	int		iCamID;							//镜头id
	int		iRetCode;						//返回值（1成功 0云终端不在线 -1 镜头不在线）
	int		iRealType;						//流类型（0-SDK方式   1-帧方式）
	int		iStreamPublicPort;				//视频流目的地的公网端口
	char    sStreamPublicIP[50];			//视频流目的地的公网ip
	char	sToUser[50];					//播放端的用户名
	bool	bStream;						//是否走中转
	int 	iCustomData;					//自定义数据
	int		iStreamCustomID;				//每一路流的唯一标示（由vpaas分配的唯一的号，用来标示流连接）


}stZehinDownLoadStartRep_T_, stZehinDownLoadStartRep_C_;

typedef struct  
{
	int iStreamCustomID;					//每一路流的唯一标示（由vpaas分配的唯一的号，用来标示流连接）
}stZehinDownLoadStopRep_C_, stZehinDownLoadStopRep_T_;


typedef struct  
{
	char sTerminalSn[50];					//云终端序列号
	int iVoucherLength;						//凭单长度
	char sVoucherInfo[1024*10];				//凭单内容
}stZehinRemoteVoucher_T_;

typedef struct  
{
	char	sTerminalSn[50];				//云终端序列号
	int		iDevID;							
	int		iDataPointID;
	char	sDataValue[50];
}stZehinDataRealTimeData_C_;				//实时上传数据

typedef struct  
{
	char	sTerminalSn[50];				//云终端序列号
	int		iDevID;							
	int		iDataPointID;					//数据点ID
	int		iAlarmState;					//报警状态
}stZehinDataRealTimeAlarm_C_;

typedef struct  
{
	int		iDataID;						//数据点id
	int		iGuard;							//布撤防状态 0撤防1布防

}stZehinDataHandGuard_C_;

typedef struct  
{
	int		iDevIndex;						//镜头或者设备序号
	int		iDataIndex;						//数据点序号
	int 	iGuard;							//布撤防状态 0撤防1布防


}stZehinDataHandGuard_T_;


typedef struct  
{
	char	sTerminalSn[BUFSIZE_64];		//设备sn号
	int		iDevIndex;						//镜头或者设备序号
	int		iDataIndex;						//数据点序号
	int		iGuardStatus;					//当前布撤防状态 0撤防1布防
	int		iAutoGuard;						//是否是自动布撤防 1-是  0-否

}stZehinDataRealGuardStatus_T_;


typedef struct  
{
	int		iTerminalID;					//设备ID号
	int		iDevID;							//设备ID
	int		iDataID;						//数据点ID
	int		iGuardStatus;					//当前布撤防状态 0撤防1布防
	int		iAutoGuard;						//是否是自动布撤防 1-是  0-否

}stZehinDataRealGuardStatus_C_;

typedef struct  
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iMsgID;							//通知ID
	int		iMsgTime;						//通知时间
	char	sMsgAuthor[BUFSIZE_64];			//通知作者
	char	sMsgTital[BUFSIZE_64];			//通知标题
	char	sMsgBody[BUFSIZE_1024];			//通知内容
}stZehinDMMsgInfo_T_;

typedef struct  
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iMsgID;							//通知ID
	int		iRepTime;						//回复时间
	char	sRepAuthor[BUFSIZE_64];			//回复作者
	char	sRepBody[BUFSIZE_64];			//回复内容

}stZehinDMMsgRepInfo_T_;

typedef struct  
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iMsgTime;						//通知时间
	char	sMsgAuthor[BUFSIZE_64];			//通知作者
	char	sMsgTital[BUFSIZE_64];			//通知标题
	char	sMsgBody[BUFSIZE_1024];			//通知内容
	char	sUser[BUFSIZE_64];				//客户端用户名 或者vPaasConsole
}stZehinDMMsgInfo_C_;

/*
typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iCurTime;						//当前时间
	char	sCustomData[BUFSIZE_128];				//自定义数据
	int		iSendSize;						//发送的条数
	struct
	{
		int		iDevIndex;					//设备序号
		int		iDataIndex;					//数据点序号
		char	sDataValue[BUFSIZE_64];		//数据点的值
	}DataPointValue[BUFSIZE_128];

}stZehinDataMultiRealTimeData_T_;*/


typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iCurTime;						//当前时间
	char	sCustomData[BUFSIZE_512];       //自定义数据
	int		iSendSize;						//发送的条数
	struct
	{
		int		iDevIndex;					//设备序号
		int		iDataIndex;					//数据点序号
		char	sDataValue[BUFSIZE_64];		//数据点的值
	}DataPointValue[BUFSIZE_128];
	char	sIdentifies[BUFSIZE_128];					//表明当前数据的唯一标识
}stZehinDataMultiRealTimeData_T_;			//批量实时数据上传（非阻塞），发送后对应回调函数


typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iRetCode;						//1成功   0失败
	char	sCustomData[BUFSIZE_128];		//自定义数据
	char	sRecordID[BUFSIZE_32];							//记录ID（相当于订单编号）
	char	sImageURL[BUFSIZE_512];			//图片路径
	int		iActiveURL;						//图片1是否有效   0无效 1有效
	char	sImageURL2[BUFSIZE_512];			//图片路径
	int		iActiveURL2;					//图片2是否有效   0无效 1有效
	int		iCurTime;						//当前时间

}stZehinDataRecordImage_T_;

typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iRetCode;						//1成功   0失败
	char	sCustomData[BUFSIZE_128];		//自定义数据
	char	sRecordID[BUFSIZE_32];			//记录ID（相当于订单编号）

}stZehinDataMultiRealTimeRep_T_;


typedef struct  
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iYear;
	int		iMonth;
	int		iDay;
	int		iHour;
	int		iMinute;
	int		iSecond;
}stZehinGetTimeInfoRep_T_;

typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iDevIndex;
	char	sCustomData[BUFSIZE_128];		//自定义数据
	char	sCurTime[BUFSIZE_64];			//当前时间
	int		iFileType;						//文件类型（1-报警抓图，2-定时抓图，3-手动抓图，4-其他）
	char	sFileUrl[BUFSIZE_256];			//文件URL
	char	sFileDescription[BUFSIZE_256];	//文件描述
}stZehinDataRealFileUpload_T_;



typedef struct  
{
	int		iDataPointID;					//设备点序号
	char	sWriteCMD[BUFSIZE_2K];			//写入的值（0-关 1-开）	
}stZehinDataRevcrsedControl_C_;

typedef struct  
{
	int		iDevIndex;						//设备序号
	int		iDataPointIndex;				//数据点序号
	char	sWriteCmd[BUFSIZE_2K];			//写入的值（0-关 1-开）	
}stZehinDataRevcrsedControl_T_;


typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	char	sCustomData[BUFSIZE_2K];		//自定义数据
}stZehinDataRealCustomDataToCT_T_, stZehinDataRealCustomDataToCT_C_;

typedef struct
{
	char	sUser[BUFSIZE_32];				//在线用户用户名
	char	sCustomData[BUFSIZE_2K];		//自定义数据
}stZehinDataRealCustomDataToUser_C_;


/*typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iDevIndex;						//设备序号
	int		iDataIndex;						//数据点序号
	int		iAlarmState;					//报警状态 1-报警 0-正常
	char	sCurTime[BUFSIZE_64];			//当前时间
	char	sCustomData[BUFSIZE_512];		//自定义数据
	int		iActiveURL;						//ImageURL 是否有效  1有效  0-无效
	char	sImageURL[BUFSIZE_512];			//图片路径
	int		iActiveURL2;					//ImageURL2 是否有效  1有效  0-无效
	char	sImageURL2[BUFSIZE_512];		//图片2路径
}stZehinDataRealTimeAlarmEx_T_;

*/

//特易停添加数据上传标识20170511
typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iDevIndex;						//设备序号
	int		iDataIndex;						//数据点序号
	int		iAlarmState;					//报警状态 1-报警 0-正常
	char	sCurTime[BUFSIZE_64];			//当前时间
	char	sCustomData[BUFSIZE_512];		//自定义数据
	int		iActiveURL;						//ImageURL 是否有效  1-有效  0-无效
	char	sImageURL[BUFSIZE_512];			//图片路径
	int		iActiveURL2;					//ImageURL2 是否有效  1-有效  0-无效
	char	sImageURL2[BUFSIZE_512];		//图片2路径
	char	sIdentifies[BUFSIZE_128];		//表明当前数据的唯一标识
}stZehinDataRealTimeAlarmEx_T_;             //实时报警上传Ex


typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iDevID;							//设备ID
	int		iDataPointID;					//数据点ID
	int		iAlarmState;					//报警状态 1-报警 0-正常
	char	sCurTime[BUFSIZE_64];			//当前时间
	char	sCustomData[BUFSIZE_512];		//自定义数据
	int		iActiveURL;						//ImageURL 是否有效  1有效  0-无效
	char	sImageURL[BUFSIZE_512];			//图片路径
	int		iActiveURL2;					//ImageURL2 是否有效  1有效  0-无效
	char	sImageURL2[BUFSIZE_512];		//图片2路径

}stZehinDataRealTimeAlarmEx_C_;




typedef struct
{
	char	sTerminalSn[BUFSIZE_64];		//云终端序列号
	int		iRetCode;						//1成功  0失败
	char	sCustomData[BUFSIZE_512];		//自定义数据
	char	sRecordID[BUFSIZE_32];			//记录ID，在上传图片的时候带着记录ID（相当于订单编号）
}stZehinDataRealTimeAlarmExRep_T_;


#endif										// VPAAS_DATA_TYPE_H
