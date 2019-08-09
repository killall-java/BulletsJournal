#include "stdafx.h"
#include "CameraStream.h"
#include <iostream>

// https://blog.csdn.net/xiaoyao_SDU/article/details/80412803


//���캯��
CameraStream::CameraStream(void)
{

}
//��������
CameraStream::~CameraStream(void)
{
}
//��ʼ��������������ʼ��״̬���
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

//��¼��������������ͷid�Լ����������¼
bool CameraStream::login(char* sDeviceAddress)
//bool HK_camera::Login(const char* sDeviceAddress,const char* sUserName,const char* sPassword, WORD wPort);        //��½��VS2017�汾��
{
	// ��ʼ��sdk
	if (!this->isInit){
		if (this->Init()){
			cout << "------------ ��ʼ���ɹ�" << endl;
		}
		else {
			cout << "------------ ��ʼ��ʧ��" << endl;
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

//��Ƶ����ʾ����
int CameraStream::start(void(CALLBACK *g_RealDataCallBack_V30)(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser), LONG &nPort, HWND hPlayWnd, char* sDeviceAddress)
{
	// ��¼
	if (!login(sDeviceAddress)){
		return -1;
	}
	// https://blog.csdn.net/hust_bochu_xuchao/article/details/53610588


	//����Ԥ�������ûص�������
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = hPlayWnd; //����Ϊ�գ��豸SDK������ֻȡ��
	struPlayInfo.lChannel = 1; //Channel number �豸ͨ��
	struPlayInfo.dwStreamType = 0;// �������ͣ�0-��������1-��������2-����3��3-����4, 4-����5,5-����6,7-����7,8-����8,9-����9,10-����10
	struPlayInfo.dwLinkMode = 0;// 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-������ȡ��, 1-����ȡ��, �������SDK�ڲ�connectʧ�ܽ�����5s�ĳ�ʱ���ܹ�����,���ʺ�����ѯȡ������.

	// ��һ��Ԥ������֡��
	if (isFirstPreview){
		bool configCameraFlag = false;
		int Ret;
		/**
		*����ͼ��ѹ������
		*/
		NET_DVR_COMPRESSIONCFG_V30 compressionCfgV30 = { 0 };
		DWORD dwReturnLen;
		Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_COMPRESSCFG_V30, struPlayInfo.lChannel, &compressionCfgV30, sizeof(NET_DVR_COMPRESSIONCFG_V30), &dwReturnLen);
		compressionCfgV30.struNormHighRecordPara.dwVideoFrameRate = 5; // ����֡��
		configCameraFlag = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_COMPRESSCFG_V30, struPlayInfo.lChannel, &compressionCfgV30, sizeof(NET_DVR_COMPRESSIONCFG_V30));

		/**
		*����ͼ��ǰ�˲���
		*/
		NET_DVR_CAMERAPARAMCFG_EX  cameraParamCfg_ex = { 0 };
		DWORD dwCameraParamCfgReturnLen;
		Ret = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_CCDPARAMCFG_EX, struPlayInfo.lChannel, &cameraParamCfg_ex, sizeof(NET_DVR_CAMERAPARAMCFG_EX), &dwCameraParamCfgReturnLen);

		cameraParamCfg_ex.struWdr.byWDREnabled = 1;// ��̬��0 dsibale  1 enable 2 auto 
		cameraParamCfg_ex.struWdr.byWDRContrastLevel = 90; // 0-100
		cameraParamCfg_ex.struWdr.byWDRLevel1 = 0xF;
		cameraParamCfg_ex.struWdr.byWDRLevel2 = 0xF;

		cameraParamCfg_ex.struNoiseRemove.byDigitalNoiseRemoveEnable = 2; // 0-�����ã�1-��ͨģʽ���ֽ��룬2-ר��ģʽ���ֽ���
		cameraParamCfg_ex.struNoiseRemove.bySpectralLevel = 100;       //ר��ģʽ�¿���ǿ�ȣ�0-100
		cameraParamCfg_ex.struNoiseRemove.byTemporalLevel = 100;   //ר��ģʽ��ʱ��ǿ�ȣ�0-100

		/*0-�ֶ���ƽ�⣨MWB��,1-�Զ���ƽ��1��AWB1��,2-�Զ���ƽ��2 (AWB2),3-�Զ����Ƹ���Ϊ������ƽ��(Locked WB)��
		4-����(Indoor)��5-����(Outdoor)6-�չ��(Fluorescent Lamp)��7-�Ƶ�(Sodium Lamp)��
		8-�Զ�����(Auto-Track)9-һ�ΰ�ƽ��(One Push)��10-�����Զ�(Auto-Outdoor)��
		11-�Ƶ��Զ� (Auto-Sodiumlight)��12-ˮ����(Mercury Lamp)��13-�Զ���ƽ��(Auto)��*/
		cameraParamCfg_ex.struWhiteBalance.byWhiteBalanceMode = 1;

		cameraParamCfg_ex.struExposure.dwVideoExposureSet = 20000; //�Զ�����Ƶ�ع�ʱ�䣨��λus��*//*ע:�Զ��ع�ʱ��ֵΪ�ع�����ֵ ����20-1s(1000000us)

		//configCameraFlag = false;
		//configCameraFlag = NET_DVR_SetDVRConfig(lUserID, NET_DVR_SET_CCDPARAMCFG_EX, struPlayInfo.lChannel, &cameraParamCfg_ex, sizeof(NET_DVR_CAMERAPARAMCFG_EX));
		/*����5����л���������ƽ��*/
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

	//ֹͣԤ��
	if (!NET_DVR_StopRealPlay(realPlayHandler))
	{
		NET_DVR_GetLastError();
		return -4;
	}
	realPlayHandler = -1;
	//ֹͣ����
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