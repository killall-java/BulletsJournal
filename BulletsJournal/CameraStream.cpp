#include "stdafx.h"
#include "CameraStream.h"
#include <iostream>

// https://blog.csdn.net/xiaoyao_SDU/article/details/80412803


//构造函数
CameraStream::CameraStream(void)
{

}
//析构函数
CameraStream::~CameraStream(void)
{
}
//初始化函数，用作初始化状态检测
bool CameraStream::Init()
{
	if (NET_DVR_Init())
	{
		this->isInit = true;
		return true;
	}
	else
	{
		this->isInit = false;
		return false;
	}
}

//登录函数，用作摄像头id以及密码输入登录
bool CameraStream::login(char* sDeviceAddress)
//bool HK_camera::Login(const char* sDeviceAddress,const char* sUserName,const char* sPassword, WORD wPort);        //登陆（VS2017版本）
{
	// 初始化sdk
	if (!this->isInit){
		if (this->Init()){
			cout << "------------ 初始化成功" << endl;
		}
		else {
			cout << "------------ 初始化失败" << endl;
			return false;
		}
	}

	if (lUserID < 0)
	{
		NET_DVR_DEVICEINFO_V30 struDeviceInfo;
		lUserID = NET_DVR_Login_V30(sDeviceAddress, 8000, "admin", "qazwsx12!@", &struDeviceInfo);
		if (lUserID >= 0){
			return true;
		}
		else{
			return false;
		}
	}
	else {
		return true;
	}
}

//视频流显示函数
int CameraStream::start(void(CALLBACK *g_RealDataCallBack_V30)(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser), LONG &nPort, HWND hPlayWnd, char* sDeviceAddress)
{
	// 登录
	if (!login(sDeviceAddress)){
		return -1;
	}
	// https://blog.csdn.net/hust_bochu_xuchao/article/details/53610588


	//启动预览并设置回调数据流
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = hPlayWnd; //窗口为空，设备SDK不解码只取流
	struPlayInfo.lChannel = 1; //Channel number 设备通道
	struPlayInfo.dwStreamType = 0;// 码流类型，0-主码流，1-子码流，2-码流3，3-码流4, 4-码流5,5-码流6,7-码流7,8-码流8,9-码流9,10-码流10
	struPlayInfo.dwLinkMode = 0;// 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-非阻塞取流, 1-阻塞取流, 如果阻塞SDK内部connect失败将会有5s的超时才能够返回,不适合于轮询取流操作.

	// 第一次预览设置帧率
	if (isFirstPreview){
		bool configCameraFlag = false;
		int Ret;
		/**
		*设置图像压缩参数
		*/
		NET_DVR_COMPRESSIONCFG_V30 compressionCfgV30 = { 0 };
		DWORD dwReturnLen;
		Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_COMPRESSCFG_V30, struPlayInfo.lChannel, &compressionCfgV30, sizeof(NET_DVR_COMPRESSIONCFG_V30), &dwReturnLen);
		compressionCfgV30.struNormHighRecordPara.dwVideoFrameRate = 5; // 设置帧率
		configCameraFlag = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_COMPRESSCFG_V30, struPlayInfo.lChannel, &compressionCfgV30, sizeof(NET_DVR_COMPRESSIONCFG_V30));

		/**
		*设置图像前端参数
		*/
		NET_DVR_CAMERAPARAMCFG_EX  cameraParamCfg_ex = { 0 };
		DWORD dwCameraParamCfgReturnLen;
		Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_CCDPARAMCFG_EX, struPlayInfo.lChannel, &cameraParamCfg_ex, sizeof(NET_DVR_CAMERAPARAMCFG_EX), &dwCameraParamCfgReturnLen);

		cameraParamCfg_ex.struWdr.byWDREnabled = 1;// 宽动态：0 dsibale  1 enable 2 auto 
		cameraParamCfg_ex.struWdr.byWDRContrastLevel = 90; // 0-100
		cameraParamCfg_ex.struWdr.byWDRLevel1 = 0xF;
		cameraParamCfg_ex.struWdr.byWDRLevel2 = 0xF;

		cameraParamCfg_ex.struNoiseRemove.byDigitalNoiseRemoveEnable = 2; // 0-不启用，1-普通模式数字降噪，2-专家模式数字降噪
		cameraParamCfg_ex.struNoiseRemove.bySpectralLevel = 100;       //专家模式下空域强度：0-100
		cameraParamCfg_ex.struNoiseRemove.byTemporalLevel = 100;   //专家模式下时域强度：0-100

		/*0-手动白平衡（MWB）,1-自动白平衡1（AWB1）,2-自动白平衡2 (AWB2),3-自动控制改名为锁定白平衡(Locked WB)，
		4-室外(Indoor)，5-室内(Outdoor)6-日光灯(Fluorescent Lamp)，7-钠灯(Sodium Lamp)，
		8-自动跟踪(Auto-Track)9-一次白平衡(One Push)，10-室外自动(Auto-Outdoor)，
		11-钠灯自动 (Auto-Sodiumlight)，12-水银灯(Mercury Lamp)，13-自动白平衡(Auto)，*/
		cameraParamCfg_ex.struWhiteBalance.byWhiteBalanceMode = 1;

		cameraParamCfg_ex.struExposure.dwVideoExposureSet = 20000; //自定义视频曝光时间（单位us）*//*注:自动曝光时该值为曝光最慢值 新增20-1s(1000000us)

		//configCameraFlag = false;
		//configCameraFlag = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_CCDPARAMCFG_EX, struPlayInfo.lChannel, &cameraParamCfg_ex, sizeof(NET_DVR_CAMERAPARAMCFG_EX));
		/*调整5秒后切换成锁定白平衡*/
		//Sleep(5 * 1000);
		cameraParamCfg_ex.struWhiteBalance.byWhiteBalanceMode = 3;
		//configCameraFlag = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_CCDPARAMCFG_EX, struPlayInfo.lChannel, &cameraParamCfg_ex, sizeof(NET_DVR_CAMERAPARAMCFG_EX));

		if (!configCameraFlag) {
			TRACE(" ----------------------------------------------SetCamera error! \n");
			return -3;
		}

		isFirstPreview = false;
	}

	if (realPlayHandler < 0){
		realPlayHandler = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL);
	}
	else {
		return -3;
	}

	return 0;
}
int CameraStream::stop(LONG &nPort){
	// TODU

	//停止预览
	if (!NET_DVR_StopRealPlay(realPlayHandler))
	{
		NET_DVR_GetLastError();
		return -4;
	}
	realPlayHandler = -1;
	//停止解码
	if (Port > -1)
	{
		/*if (!PlayM4_StopSound())
		{
		PlayM4_GetLastError(Port);
		}*/
		if (!PlayM4_Stop(nPort))
		{
			PlayM4_GetLastError(nPort);
			return -5;
		}
		if (!PlayM4_CloseStream(nPort))
		{
			PlayM4_GetLastError(nPort);
			return -6;
		}
		if (!PlayM4_FreePort(nPort)){
			PlayM4_GetLastError(nPort);
			return -7;
		}
		Port = -1;
		nPort = -1;
	}
	if (!NET_DVR_Logout(lUserID)){
		NET_DVR_GetLastError();
		return -8;
	}
	lUserID = -1;
	isInit = false;
	//if ( !NET_DVR_Cleanup() ){
	//	NET_DVR_GetLastError();
	//	return false;
	//}
	//nPort = -1;
	return 0;
}