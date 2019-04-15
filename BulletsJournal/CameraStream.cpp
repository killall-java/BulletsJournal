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
		} else {
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
		} else{
			return false;
		}
	} else {
		return true;
	}
}

//视频流显示函数
bool CameraStream::start(void(CALLBACK *g_RealDataCallBack_V30)(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser), void(CALLBACK *decCBFun)(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2), LONG &nPort, HWND hPlayWnd, char* sDeviceAddress)
{
	// 登录
	if (!login(sDeviceAddress)){
		return false;
	}

	
	//启动预览并设置回调数据流
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = hPlayWnd; //窗口为空，设备SDK不解码只取流
	struPlayInfo.lChannel = 1; //Channel number 设备通道
	struPlayInfo.dwStreamType = 0;// 码流类型，0-主码流，1-子码流，2-码流3，3-码流4, 4-码流5,5-码流6,7-码流7,8-码流8,9-码流9,10-码流10
	struPlayInfo.dwLinkMode = 0;// 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-非阻塞取流, 1-阻塞取流, 如果阻塞SDK内部connect失败将会有5s的超时才能够返回,不适合于轮询取流操作.

	realPlayHandler = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL);
	return true;
}
bool CameraStream::stop(LONG &nPort){
	// TODU
	
	//停止预览
	if ( !NET_DVR_StopRealPlay(realPlayHandler) )
	{
		NET_DVR_GetLastError();
		return false;
	}
	realPlayHandler = -1;
	//停止解码
	//if (Port > -1)
	//{
	//	/*if (!PlayM4_StopSound())
	//	{
	//		PlayM4_GetLastError(Port);
	//	}*/
	//	if (!PlayM4_Stop(Port))
	//	{
	//		PlayM4_GetLastError(Port);
	//	}
	//	if (!PlayM4_CloseStream(Port))
	//	{
	//		PlayM4_GetLastError(Port);
	//	}
	//	PlayM4_FreePort(Port);
	//	Port = -1;
	//	nPort = -1;
	//}
	//if ( !NET_DVR_Logout(lUserID) ){
	//	NET_DVR_GetLastError();
	//	return false;
	//}
	//lUserID = -1;

	//if ( !NET_DVR_Cleanup() ){
	//	NET_DVR_GetLastError();
	//	return false;
	//}
	//nPort = -1;
	return true;
}