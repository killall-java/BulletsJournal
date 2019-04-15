#ifndef _CAMERASTREAM_H_ 
#define _CAMERASTREAM_H_
//#include <stdafx.h>

#include "Windows.h"
#include <stdio.h>
#include <iostream>

#include <time.h>
//#include "PlayM4.h"   //此头文件需要按照下面第二步调试Bug中的方法去添加
#include "HCNetSDK.h"
#include "plaympeg4.h"

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
 
using namespace cv;
using namespace std;
 
//int realframe_count;
//CRITICAL_SECTION g_csThreadCode1;

class CameraStream
{
public:
    CameraStream(void);
    ~CameraStream(void);
 
public:
	LONG realPlayHandler = -1;
	LONG Port = -1;
	LONG lUserID = -1;
	
    //bool Login(const char* sDeviceAddress,const char* sUserName,const char* sPassword, WORD wPort);            //登陆（VS2017版本）
	bool stop(LONG &nPort);
	bool start(void(CALLBACK *realDataCallBack_V30)(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser), void(CALLBACK *decCBFun)(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2), LONG &nPort, HWND hPlayWnd, char* sDeviceAddress);                  //显示图像
	

private:
	bool Init();            //初始化
	bool isInit = false;
	bool login(char* sDeviceAddress);            //登陆
};

#endif;
 