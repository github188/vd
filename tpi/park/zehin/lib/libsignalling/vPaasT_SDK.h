//vPaas 终端连接SDK
#ifndef VPAAST_SDK_H
#define VPAAST_SDK_H

#include "vPaas_DataType.h"


/*初始化*/
VPAAS_API bool	vPaasT_Init();

/*设置和获取STUN地址*/
VPAAS_API void	vPaasT_SetStunIP(char*pStunIP);
VPAAS_API char*	vPaasT_GetStunIP();

/*设置和获取中心地址*/
VPAAS_API void	vPaasT_SetCenterIP(char*pCenterIP);
VPAAS_API char*	vPaasT_GetCenterIP();

/*设置和获取中心信令端口*/
VPAAS_API void	vPaasT_SetCenterCmdPort(int iCenterCmdPort);
VPAAS_API int	vPaasT_GetCenterCmdPort();

/*设置和获取中心心跳端口*/
VPAAS_API void	vPaasT_SetCenterHeartPort(int iCenterHeartPort);
VPAAS_API int	vPaasT_GetCenterHeartPort();

/*设置和获取本地端口*/
VPAAS_API void	vPaasT_SetClientPort(int iClientPort);
VPAAS_API int	vPaasT_GetClientPort();

/*设置心跳周期 单位毫秒
imSec 单位毫秒  默认为5000 ,最大不超过20000 */
VPAAS_API void	vPaasT_SetHeartTime(int imSec);

/*设置登录是否自动掉线重连*/
VPAAS_API void 	vPaasT_SetAutoReLogin(bool bAutoReLogin);
VPAAS_API bool	vPaasT_GetAutoReLogin();

/*连接STUN服务*/
VPAAS_API bool	vPaasT_ConnetStunSer();

/*关闭*/
VPAAS_API bool	vPaasT_Close();

/*得到对外的公网ip和端口（调用vPaasC_ConnetStunSer后就能获取）*/
VPAAS_API char*	vPaasT_GetClientPublicIP();
VPAAS_API int	vPaasT_GetClientPublicPort();

/*得到vPaasC_ConnetStunSer的连接日志信息*/
VPAAS_API char*	vPaasT_GetStunText();

/*得到登录的用户名*/
VPAAS_API char*	vPaasT_GetUser();

/*验证登录端是否在线*/
VPAAS_API bool	vPaasT_GetIsOnLine();

/*设置命令接收的回调函数（回调函数收不到登录和心跳命令，心跳已经做在库内部实现，掉线的时候会收到LostHeart命令）*/
/*
----------------------------|-----------------------------------|---------------------------------------------------|
sCmd命令关键字				|	lpStructParam结构体参数			|				说明								|
----------------------------|-----------------------------------|---------------------------------------------------|
LostHeart					|									|		失去心跳（掉线）							|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_REAL_START			|	stZehinRealStart_T_				|		实时视频流播放请求							|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_REAL_STOP				|	stZehinRealStop_T_				|		实时视频流停止								|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_REAL_PTZ				|	stZehinRealPTZ_T_				|		实时云台控制								|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_REAL_CAPTURE			|	stZehinRealCapture_T_			|		实时抓图上传								|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_BACK_SEARCH			|	stZehinBackSearch_T_			|		回放录像查询请求							|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_BACK_START			|	stZehinBackStart_T_				|		录像回放流开始播放请求						|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_BACK_PAUSE			|	stZehinBackPause_T_				|		录像回放暂停（恢复）						|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_BACK_SEEK				|	stZehinBackSeek_T_				|		录像回放跳转								|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_BACK_STOP				|	stZehinBackStop_T_				|		录像回放停止								|
----------------------------|---------------------------------------------------------------------------------------|
VIDEO_GET_REC_STATUS		|	stZehinGetRecState_T_			|		请求镜头的录像状态							|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_SET_REC_STATUS		|	stZehinSetRecState_T_			|		用户请求控制镜头的录像状态					|
----------------------------|-----------------------------------|---------------------------------------------------|
VIDEO_REC_DOWNLOAD_START	|	stZehinVideoDownLoadStart_T_	|		录像下载开始								|
----------------------------|-------------------————|-----------|---------------------------------------------------|
VIDEO_REC_DOWNLOAD_STOP		|	stZehinDownLoadStopRep_T_		|		录像下载停止								|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_UPLOAD_TAG_INFO_REQ		|	无								|		vPaas要求云终端上传tag信息					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_UPLOAD_ONLINE_INFO_REQ	|	无								|		vPaas要求云终端上传设备（或镜头）的在线信息	|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_CAMINFO_UPLOAD_REQ		|	无								|		vPaas要求云终端上传镜头信息					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_DEVINFO_UPLOAD_REQ		|	无								|		vPaas要求云终端上传设备信息					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_DATAPOINT_UPLOAD_REQ		|	无								|		vPaas要求云终端上传数据点信息				|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_RESTART_REQ				|	无								|		远程重启									|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_T_UPDATE_VERSION			|	stZehinUpdateVersion_T_			|		vPaas要求云终端升级							|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_CONF_BACKUP_REQ			|	stZehinConfBackup_T_			|		vPaas要求云终端上传配置						|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_CONF_RESTORE_REQ			|	stZehinConfRestore_T_			|		vPaas要求云终端回复配置						|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_RESET_REQ				|	无								|		vPaas要求云终端恢复出厂设置					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_REMOTE_COMMAND			|	stZehinRemoteCmd_T_				|		vPaas要求云终端执行shell命令				|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_REMOTE_VOUCHER			|	stZehinRemoteVoucher_T_			|		操作vPaas要求发送给云终端凭单信息			|
----------------------------|-----------------------------------|---------------------------------------------------|
DATA_HAND_GUARD				|	stZehinDataHandGuard_T_			|		vPaas发送布撤防命令给云终端					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_MSG_INFO					|	stZehinDMMsgInfo_T_				|		Web操作vPaas要求发送云终端通知信息			|
----------------------------|-----------------------------------|---------------------------------------------------|
DATA_MULTI_REAL_TIME_REP	|	stZehinDataMultiRealTimeRep_T_	|		批量实时数据上传后vPaas回复					|
----------------------------|-----------------------------------|---------------------------------------------------|
DM_GET_TIME_INFO_REP		|	stZehinGetTimeInfoRep_T_		|		获取时间信息回复							|
----------------------------|-----------------------------------|---------------------------------------------------|
DATA_REVCRSED_CONTROL		|	stZehinDataRevcrsedControl_T_	|		数据反向写入								|
----------------------------|-----------------------------------|---------------------------------------------------|
DATA_REAL_CUSTOMDATA_TO_CT	|	stZehinDataRealCustomDataToCT_T_|		云终端接收自定义数据						|
----------------------------|-----------------------------------|---------------------------------------------------|


*/
VPAAS_API void	vPaasT_SetCmdCallBack(CBF_RecvCmd pCallBack,void* pCustomDate);

/*登录函数，阻塞函数
参数：	iState		- 状态（0-忙碌，1-隐身，2-在线，3-下线）
		iTerminalType - 云终端类型
返回值：1000： 登录成功
        1001： 用户不存在
        1002： 用户已经存在
        1003： 用户密码不正确
        1004： 服务器错误

        0：	登录超时
        -1： 连接中心失败
        -2：用户名参数不对
        -3：发送函数失败
*/
VPAAS_API int	vPaasT_Login(char* pSN,int iState,int iTerminalType=1);

VPAAS_API int	vPaasT_Logout();


/*发送自定义命令*/
VPAAS_API bool	vPaasT_SendCmd(char*pCmd,int iCmdLen);
VPAAS_API bool	vPaasT_SendCmdTo(char*pCmd,int iCmdLen ,char*sToIP,int iToPort);

/*实时数据上传*/
VPAAS_API bool  vPaasT_RealDataUp(int iDevIndex,int iDataPointIndex,float fValue);

/*实时报警上传*/
VPAAS_API bool  vPaasT_RealAlarmUp(int iDevIndex,int iDataPointIndex,int iAlarmStatus);

/*实时数据订阅*/
VPAAS_API bool  vPaasT_RealTimeDataSubscribe(char*pName,int iDataPointID);
VPAAS_API bool  vPaasT_RealTimeDataUnSubscribe(char*pName,int iDataPointID);

/*实时流播放开始回复*/
VPAAS_API bool  vPaasT_RealStartRep(stZehinRealStartRep_T_ struRealRepInfo);

/*录像查询结果返回*/
VPAAS_API bool  vPaasT_BackSearchResult(stZehinBackSearchResult_T_ struResultInfo);

/*录像回放播放开始回复*/
VPAAS_API bool  vPaasT_BackStartRep(stZehinBackStartRep_T_ struBackRepInfo);

/*获取录像状态回复*/
VPAAS_API bool  vPaasT_RecStateRep(int iCamID,int iStatus,char*pToUser);

/*设备管理相关*/
VPAAS_API bool  vPaasT_GetTagInfo();
VPAAS_API bool  vPaasT_SetTagInfo(stZehinSetTagInfo_T_ struTagInfo);


/*上传镜头（或末端设备）的在线状态*/
VPAAS_API bool  vPaasT_UploadDeviceIsOnline(stZehinIsOnlineInfo_T_ struOnlineInfo);

/*上传镜头信息*/
VPAAS_API bool  vPaasT_UploadCamInfo(stZehinCamInfo_T_ struCamInfo);
VPAAS_API bool  vPaasT_DeleteCamInfo(int iDevIndex);
VPAAS_API bool  vPaasT_GetCamList();

/*上传末端设备信息*/
VPAAS_API bool  vPaasT_UploaDevInfo(stZehinDevInfo_T_ struDevInfo);
VPAAS_API bool  vPaasT_DeleteDevInfo(int iDevIndex);
VPAAS_API bool  vPaasT_GetDevList();

/*上传数据点信息*/
VPAAS_API bool  vPaasT_UploaDataPointInfo(stZehinDataPointInfo_T_ struDataPointInfo);
VPAAS_API bool  vPaasT_DeleteDataPointInfo(int iDevIndex,int iDataPointIndex);
VPAAS_API bool  vPaasT_GetDataPointList(int iDevIndex);

/*抓图完成后上传图片信息*/
VPAAS_API bool  vPaasT_RealCaptureFinish(stZehinRealCaptureFinish_T_ struRealCaptureInfo);

/*开始下载流*/
VPAAS_API bool  vPaasT_RecDownloadStrarRep(stZehinDownLoadStartRep_T_ struStartRepInfo);

/*布撤防相关*/
VPAAS_API bool  vPaasT_DataRealGuardStatus(stZehinDataRealGuardStatus_T_ struDataRealGuardStatusInfo);

/*会议通知相关*/
VPAAS_API bool  vPaasT_DMMsgRepInfo(stZehinDMMsgRepInfo_T_ struDMMsgRepInfo);

/*批量实时数据上传（非阻塞），发送后 对应回调函数 DATA_MULTI_REAL_TIME_REP 命令*/
VPAAS_API bool vPaasT_DataMultiRealTimeData(stZehinDataMultiRealTimeData_T_ struDataMultiRealTimeData);

/*批量实时数据上传（for 特易停）
阻塞式
返回值：NULL : 失败  not NULL ：recordID
*/
VPAAS_API char* vPaasT_DataMultiRealTimeDataEx(stZehinDataMultiRealTimeData_T_ struDataMultiRealTimeData,int iTimedOutSec=5);

/*图片上传成功后告诉vPaas图片信息*/
VPAAS_API bool vPaasT_DataRecordImage(stZehinDataRecordImage_T_ struDataRecordImage);

//获取服务器时间信息
VPAAS_API bool vPaasT_GetTimeInfoReq();

//实时文件内容上传
VPAAS_API bool vPaasT_DataRealFileUpload(stZehinDataRealFileUpload_T_ struDataRealFileUpload);

//实时报警上传（特易停）
//返回值:RecordID
//阻塞式
VPAAS_API char* vPaasT_DataRealTimeAlarmEx(stZehinDataRealTimeAlarmEx_T_ struDataRealTimeAlarm,int iTimedOutSec);

//云终端给所在域的应用发送自定义数据
//发送失败或sCustomData为NULL返回false 否则返回true
VPAAS_API bool vPaasT_DataRealCustomDataToAPP(char *sCustomData);
#endif // VPAAST_SDK_H
