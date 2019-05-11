#ifndef _CAMERASTREAM_H_ 
#define _CAMERASTREAM_H_
//#include <stdafx.h>

#include "Windows.h"
#include <stdio.h>
#include <iostream>

#include <time.h>
//#include "PlayM4.h"   //��ͷ�ļ���Ҫ��������ڶ�������Bug�еķ���ȥ���
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
	bool isFirstPreview = true;

    //bool Login(const char* sDeviceAddress,const char* sUserName,const char* sPassword, WORD wPort);            //��½��VS2017�汾��
	int stop(LONG &nPort);
	int start(void(CALLBACK *realDataCallBack_V30)(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser), LONG &nPort, HWND hPlayWnd, char* sDeviceAddress);                  //��ʾͼ��
	

private:
	bool Init();            //��ʼ��
	bool isInit = false;
	bool login(char* sDeviceAddress);            //��½
};

#endif;
 