
// BulletsJournalDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "BulletsJournal.h"
#include "BulletsJournalDlg.h"
#include "afxdialogex.h"
#include "CameraStream.h"
#include <atomic>
#include "curl.h"
#include "getgun.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum CODE_NUM {
	CONNECTED = 0,
	INIT_FAILED = -1,
	LOGIN_FAILED = -2,
	REALPLAY_FAILED = -3,
	STOP_REALPLAY_FAILED = -4,
	STOP_PLAYM4_FAILED = -5,
	PLAYM4_CLOSESTREAM_FAILED = -6,
	PLAYM4_FREEPORT_FAILED = -7,
	NET_DVR_LOGOUT_FAILED = -8,
	UNCONNECTED = -9,
	UNKNOWN = -10
} codeNum;

struct ChannelInfo{
	// 窗口句柄（弃用，改用imshow）
	HWND hPlayWnd;
	// 伪队列
	list<Mat> matQueque; // 全局
	// 线程锁
	std::mutex mutexLock;
	// 相机播放库通道号
	LONG nPort = -1;
	// 相机IP
	char deviceIp[16];
	// 相机SDK异常返回
	char des[20]; 
	// 是否正在进行预览
	bool enableRealPlay = TRUE;
	// 是否已经登录
	bool isLogin = false;
	// 是否可进行图像处理
	bool isPlayingCV = true;
	// 相机成功连接标志位
	bool isReady = false;
	// 当前状态
	string status = "";

	int samplingCounter = 1;

	int quequePushInterval = 3;

	int COUNTER_MAX = 10;

} ChannelInfo1, ChannelInfo2, ChannelInfo3, ChannelInfo4, ChannelInfo5;

CameraStream camera1, camera2, camera3, camera4, camera5;

//std::atomic<bool> forcedReturn(false);

//Mat g_BGRImage;
//int g_count = 0;
clock_t g_start=0, g_ends=0;
static int onCurlDebug(CURL *, curl_infotype itype, char* pData, size_t size, void*){
	if (itype == CURLINFO_TEXT)
	{
		TRACE("[TEXT]%s\n", pData);
	}
	else if (itype == CURLINFO_HEADER_IN)
	{
		TRACE("[HEADER_IN]%s\n", pData);
	}
	else if (itype == CURLINFO_HEADER_OUT)
	{
		TRACE("[HEADER_OUT]%s\n", pData);
	}
	else if (itype == CURLINFO_DATA_IN)
	{
		TRACE("[DATA_IN]%s\n", pData);
	}
	else if (itype == CURLINFO_DATA_OUT)
	{
		TRACE("[DATA_OUT]%s\n", pData);
	}
	return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
	string data((const char*)ptr, (size_t)size * nmemb);
	*((stringstream*)stream) << data << endl;
	return size * nmemb;
}

static bool g_postData(cv::Mat mat, std::string ip, int goal, int cnt, std::string cameraId) {
	// https://blog.csdn.net/qq_32435729/article/details/77095903
	if (mat.empty()) {
		return false;
	}

	std::vector<uchar> data_encode;
	try {
		std::vector<int> param = std::vector<int>(2);
		param[0] = CV_IMWRITE_JPEG_QUALITY;
		param[1] = 95;
		cv::imencode(".jpg", mat, data_encode, param);

	}
	catch (Exception& e) {
		const char * s_ERROR = e.what();
		std::string a(s_ERROR);
		int c = 0;
		c++;
	}

	long imgLength = (long)data_encode.size();
	// 获取图片数据指针
	unsigned char* imgPtr = &*data_encode.begin();
	// 构造请求头
	struct curl_slist* headers = NULL;
	/*headers = curl_slist_append(headers, "Content-Type:multipart/form-data; charset=UTF-8");*/
	headers = curl_slist_append(headers, "Accept:text/html,text/plain,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
	headers = curl_slist_append(headers, "Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3");
	headers = curl_slist_append(headers, "Accept-Encoding:gzip, deflate");
	headers = curl_slist_append(headers, "User-Agent:Mozilla/5.0 (Windows NT 6.1; WOW64; rv:22.0) Gecko/20100101 Firefox/22.0");
	headers = curl_slist_append(headers, "Referer:http://minki.com/");



	//初始化easy interface，想使用easy interface的api函数就必须首先初始化easy interface
	CURL *easy_handle = curl_easy_init();
	//设置easy handle的属性和操作

	std::string url = "http://" + ip + ":80/app/upload.action";
	curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 3);
	//curl_easy_setopt(easy_handle, CURLOPT_NOSIGNAL, 1);
	std::stringstream out;
	//curl_easy_setopt(easy_handle, CURLOPT_READFUNCTION, NULL);
	//curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 1);// 调试用
	//curl_easy_setopt(easy_handle, CURLOPT_DEBUGFUNCTION, OnDebug);// 调试用
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &out);
	curl_easy_setopt(easy_handle, CURLOPT_CONNECTTIMEOUT, 3); // 超时时间，单位：秒
	curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, headers);// 设置请求头
	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;
	

	srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子

	/*std::string cameraId = std::to_string(rand() % 20);*/

	std::string value = "{\"camaraId\":\"" + cameraId + "\",\"score\":" + std::to_string(cnt) + ",\"goal\":" + std::to_string(goal) + ",\"username\":\"JoyChou\"}";
	//std::string value = "{\"camaraId\":\"" + cameraId + "\",\"score\":" + std::to_string(rand() % 100) + ",\"username\":\"JoyChou\"}";

	std::string param = "param";
	curl_formadd(&post, &last,
		CURLFORM_COPYNAME, param.c_str(), //json字符串的参数名
		CURLFORM_COPYCONTENTS, value.c_str(), //json字符串
		CURLFORM_END
		);
	curl_formadd(&post, &last,
		CURLFORM_COPYNAME, "image", //图片的参数名
		CURLFORM_BUFFER, "image.jpg", //图片名称,这里随便起的,如果不传会出错
		CURLFORM_BUFFERPTR, imgPtr, //图片存放的数组
		CURLFORM_BUFFERLENGTH, imgLength, //存放图片数组长度
		CURLFORM_CONTENTTYPE, "application/x-jpg", // application/x-jpg
		CURLFORM_END);

	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, post);// 设置为post请求，注意此步骤不能移到前面
	CURLcode code = curl_easy_perform(easy_handle);//连接到远程主机，发送请求，并接收响应
	/*string str_json = out.str();*/
	TRACE("############################################################ code: %d \n", code);
	curl_easy_cleanup(easy_handle);//释放资源
	if (CURLE_OK != code){
		return false;
	}
	return true;
}

//数据解码回调函数，
//功能：将YV_12格式的视频数据流转码为可供opencv处理的BGR类型的图片数据，并实时显示。
void CALLBACK g_decCBFun_1(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo1.samplingCounter > ChannelInfo1.COUNTER_MAX){
		ChannelInfo1.samplingCounter = 1;
	} else if (ChannelInfo1.samplingCounter % ChannelInfo1.quequePushInterval == 0){
		ChannelInfo1.samplingCounter++;
		if (pFrameInfo->nType == T_YV12)
		{
			Mat YUVImage(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

			Rect rect(600, 200, 1000, 800);
			Mat ROI = YUVImage(rect);

			ChannelInfo1.mutexLock.lock();
			ChannelInfo1.matQueque.push_back(ROI.clone());
			ChannelInfo1.mutexLock.unlock();

			TRACE("----------------------------------------------通道【一】入队，队中数量：：%d \n", ChannelInfo1.matQueque.size());

			if (ChannelInfo1.enableRealPlay){
				imshow("IPCamera1", ROI);
			}
			YUVImage.~Mat();
			ROI.~Mat();
		}

	} else{
		ChannelInfo1.samplingCounter++;
	}
};

// 回调相机视频流
void CALLBACK g_RealDataCallBack_V30_1(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //系统头
		if (!PlayM4_GetPort(&ChannelInfo1.nPort)) //获取播放库未使用的通道号
		{
			break;
		}
		//m_iPort = lPort; 
		//第一次回调的是系统头，将获取的播放库 port 号赋值给全局 port，下次回调数据时即使用此 port 号播放
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo1.nPort, STREAME_REALTIME)) //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo1.nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo1.nPort, g_decCBFun_1, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo1.nPort, NULL)) //播放开始
			{
				break;
			}
		} 
		break; 
	case NET_DVR_STREAMDATA: //码流数据
		if (dwBufSize > 0 && ChannelInfo1.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo1.nPort, pBuffer, dwBufSize)){
				break;
			} 
		} 
		break; 
	default: //其他数据
		if (dwBufSize > 0 && ChannelInfo1.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo1.nPort, pBuffer, dwBufSize)){
				break;
			} 
		} 
		break; 
	}
}


void g_popList_1(std::string ip, ChannelInfo &channelInfo) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (channelInfo.isPlayingCV){

		if (channelInfo.matQueque.size() > 0){

			TRACE("\n---------------------------------------------- 通道【一】元素首部出队，队中剩余: %d \n", channelInfo.matQueque.size());
			channelInfo.mutexLock.lock();
			Mat pop = channelInfo.matQueque.front();

			channelInfo.matQueque.pop_front();
			channelInfo.mutexLock.unlock();
			// TODO 图像处理在这里调用
			//uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			// post到web服务器
			if (goal > 0){
				g_postData(pop, ip, goal, cnt, channelInfo.deviceIp);
			}
			pop.~Mat();

		}
		else {
			// 队列中没有图片则阻塞200ms，以免线程一直占用cpu资源
			srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			int RandomNumber = rand() % 10;//生成100以内的随机数
			Sleep(200 + RandomNumber);
		}
	}
	TRACE("\n---------------------------------------------- popList finidhed \n");
	return;
}

void CALLBACK g_decCBFun_2(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo2.samplingCounter > ChannelInfo2.COUNTER_MAX){
		ChannelInfo2.samplingCounter = 1;
	} else if (ChannelInfo2.samplingCounter % ChannelInfo2.quequePushInterval == 0){
		ChannelInfo2.samplingCounter++;
		if (pFrameInfo->nType == T_YV12)
		{
			/*if (g_count == 0){
			g_start = clock();
			cout << "g_start = clock()" << endl;
			}*/
			//std::cout << "the frame infomation is T_YV12" << std::endl;
			/*if (g_BGRImage.empty())
			{
			g_BGRImage.create(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
			}*/

			Mat YUVImage(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

			////cvtColor(YUVImage, g_BGRImage, COLOR_YUV2BGR_YV12);
			Rect rect(600, 200, 1000, 800);
			Mat ROI = YUVImage(rect);

			ChannelInfo2.mutexLock.lock();
			ChannelInfo2.matQueque.push_back(ROI.clone());
			ChannelInfo2.mutexLock.unlock();
			//srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			//int RandomNumber = rand() % 10;//生成100以内的随机数
			//Sleep(50 + RandomNumber);
			///*Mat dst;
			//cvtColor(ROI, dst, CV_BGR2GRAY);
			//namedWindow("FHUT", WINDOW_AUTOSIZE);
			//imshow("IPCamera1", ROI);
			//waitKey(1);
			TRACE("----------------------------------------------通道【二】入队，队中数量：：%d \n", ChannelInfo2.matQueque.size());
			//char image_name[25];
			//sprintf(image_name, "%s%d%s", "", start, ".jpg");//保存的图片名 
			//imwrite(image_name, ROI); //保存一帧图片 
			//Sleep(1 * 1000);
			//delete[] image_name;
			if (ChannelInfo2.enableRealPlay){
				imshow("IPCamera2", ROI);
			}
			YUVImage.~Mat();
			ROI.~Mat();
		}

		/*if (g_count > 24){
		g_count = 1;
		g_start = clock();

		}else if (g_count==24){
		g_ends = clock();
		cout << "---------------- every 25 frames time escape: " << (double)(g_ends - g_start) / CLOCKS_PER_SEC << endl;

		}
		g_count++;*/
	} else {
		ChannelInfo2.samplingCounter++;
	}
};

// 回调相机视频流
void CALLBACK g_RealDataCallBack_V30_2(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //系统头
		if (!PlayM4_GetPort(&ChannelInfo2.nPort)) //获取播放库未使用的通道号
		{
			break;
		}
		//m_iPort = lPort; 
		//第一次回调的是系统头，将获取的播放库 port 号赋值给全局 port，下次回调数据时即使用此 port 号播放
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo2.nPort, STREAME_REALTIME)) //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo2.nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo2.nPort, g_decCBFun_2, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo2.nPort, NULL)) //播放开始
			{
				break;
			}
		}
		break;
	case NET_DVR_STREAMDATA: //码流数据
		if (dwBufSize > 0 && ChannelInfo2.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo2.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	default: //其他数据
		if (dwBufSize > 0 && ChannelInfo2.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo2.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	}
}


void g_popList_2(std::string ip, ChannelInfo &channelInfo) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (channelInfo.isPlayingCV){

		if (channelInfo.matQueque.size() > 0){

			TRACE("\n 通道【二】元素出队，队中剩余: %d \n", channelInfo.matQueque.size());
			channelInfo.mutexLock.lock();
			Mat pop = channelInfo.matQueque.front();
			channelInfo.matQueque.pop_front();
			channelInfo.mutexLock.unlock();
			// TODO 图像处理在这里调用
			g_start = clock();
			uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			g_ends = clock();
			TRACE("---------------------------------------------------------------------- uploadRslt time escape: %f\n", (double)(g_ends - g_start) / CLOCKS_PER_SEC );
			// post到web服务器
			if (goal > 0){
				g_postData(pop, ip, goal, cnt, channelInfo.deviceIp);
			}
			pop.~Mat();
			//srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			//int RandomNumber = rand() % 10;//生成100以内的随机数
			//Sleep(200 + RandomNumber);
		}
		else {
			// 队列中没有图片则阻塞200ms，以免线程一直占用cpu资源
			srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			int RandomNumber = rand() % 10;//生成100以内的随机数
			Sleep(200 + RandomNumber);
		}
	}
	TRACE("\n---------------------------------------------- popList finidhed \n");
	return;
}

void CALLBACK g_decCBFun_3(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo3.samplingCounter > ChannelInfo3.COUNTER_MAX){
		ChannelInfo3.samplingCounter = 1;
	} else if (ChannelInfo3.samplingCounter % ChannelInfo3.quequePushInterval == 0){
		ChannelInfo3.samplingCounter++;
		if (pFrameInfo->nType == T_YV12)
		{
			Mat YUVImage(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

			Rect rect(600, 200, 1000, 800);
			Mat ROI = YUVImage(rect);

			ChannelInfo3.mutexLock.lock();
			ChannelInfo3.matQueque.push_back(ROI.clone());
			ChannelInfo3.mutexLock.unlock();
			
			if (ChannelInfo3.enableRealPlay){
				imshow("IPCamera3", ROI);
			}
			TRACE("----------------------------------------------通道【三】入队，队中数量：：%d \n", ChannelInfo3.matQueque.size());
			YUVImage.~Mat();
			ROI.~Mat();
		}

	}
	else{
		ChannelInfo3.samplingCounter++;
	}
};

// 回调相机视频流
void CALLBACK g_RealDataCallBack_V30_3(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //系统头
		if (!PlayM4_GetPort(&ChannelInfo3.nPort)) //获取播放库未使用的通道号
		{
			break;
		}
		//m_iPort = lPort; 
		//第一次回调的是系统头，将获取的播放库 port 号赋值给全局 port，下次回调数据时即使用此 port 号播放
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo3.nPort, STREAME_REALTIME)) //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo3.nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo3.nPort, g_decCBFun_3, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo3.nPort, NULL)) //播放开始
			{
				break;
			}
		}
		break;
	case NET_DVR_STREAMDATA: //码流数据
		if (dwBufSize > 0 && ChannelInfo3.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo3.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	default: //其他数据
		if (dwBufSize > 0 && ChannelInfo3.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo3.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	}
}


void g_popList_3(std::string ip, ChannelInfo &channelInfo) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (channelInfo.isPlayingCV){

		if (channelInfo.matQueque.size() > 0){

			TRACE("\n---------------------------------------------- 通道【三】元素出队，队中剩余: %d \n", channelInfo.matQueque.size());
			channelInfo.mutexLock.lock();
			Mat pop = channelInfo.matQueque.front();
			channelInfo.matQueque.pop_front();
			channelInfo.mutexLock.unlock();
			// TODO 图像处理在这里调用
			//uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			// post到web服务器
			if (goal > 0){
				g_postData(pop, ip, goal, cnt, channelInfo.deviceIp);
			}
			
			pop.~Mat();

		}
		else {
			// 队列中没有图片则阻塞200ms，以免线程一直占用cpu资源
			srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			int RandomNumber = rand() % 10;//生成100以内的随机数
			Sleep(200 + RandomNumber);
		}
	}
	TRACE("\n---------------------------------------------- popList finidhed \n");
	return;
}
void CALLBACK g_decCBFun_4(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo4.samplingCounter > ChannelInfo4.COUNTER_MAX){
		ChannelInfo4.samplingCounter = 1;
	} else if (ChannelInfo4.samplingCounter % ChannelInfo4.quequePushInterval == 0){
		ChannelInfo4.samplingCounter++;
		if (pFrameInfo->nType == T_YV12)
		{
			Mat YUVImage(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

			Rect rect(600, 200, 1000, 800);
			Mat ROI = YUVImage(rect);

			ChannelInfo4.mutexLock.lock();
			ChannelInfo4.matQueque.push_back(ROI.clone());
			ChannelInfo4.mutexLock.unlock();

			if (ChannelInfo4.enableRealPlay){
				imshow("IPCamera4", ROI);
			}
			TRACE("----------------------------------------------通道【四】入队，队中数量：%d \n", ChannelInfo4.matQueque.size());

			YUVImage.~Mat();
			ROI.~Mat();
		}
	}
	else{
		ChannelInfo4.samplingCounter++;
	}
};

// 回调相机视频流
void CALLBACK g_RealDataCallBack_V30_4(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //系统头
		if (!PlayM4_GetPort(&ChannelInfo4.nPort)) //获取播放库未使用的通道号
		{
			break;
		}
		//m_iPort = lPort; 
		//第一次回调的是系统头，将获取的播放库 port 号赋值给全局 port，下次回调数据时即使用此 port 号播放
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo4.nPort, STREAME_REALTIME)) //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo4.nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo4.nPort, g_decCBFun_4, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo4.nPort, NULL)) //播放开始
			{
				break;
			}
		}
		break;
	case NET_DVR_STREAMDATA: //码流数据
		if (dwBufSize > 0 && ChannelInfo4.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo4.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	default: //其他数据
		if (dwBufSize > 0 && ChannelInfo4.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo4.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	}
}


void g_popList_4(std::string ip, ChannelInfo &channelInfo) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (channelInfo.isPlayingCV){

		if (channelInfo.matQueque.size() > 0){

			TRACE("\n---------------------------------------------- 通道【四】元素首部出队，队中剩余: %d \n", channelInfo.matQueque.size());
			channelInfo.mutexLock.lock();
			Mat pop = channelInfo.matQueque.front();

			channelInfo.matQueque.pop_front();
			channelInfo.mutexLock.unlock();
			// TODO 图像处理在这里调用
			//uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			// post到web服务器
			if (goal > 0){
				g_postData(pop, ip, goal, cnt, channelInfo.deviceIp);
			}
			pop.~Mat();

		}
		else {
			// 队列中没有图片则阻塞200ms，以免线程一直占用cpu资源
			srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			int RandomNumber = rand() % 10;//生成100以内的随机数
			Sleep(200 + RandomNumber);
		}
	}
	TRACE("\n---------------------------------------------- popList finidhed \n");
	return;
}

void CALLBACK g_decCBFun_5(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo5.samplingCounter > ChannelInfo5.COUNTER_MAX){
		ChannelInfo5.samplingCounter = 1;
	} else if (ChannelInfo5.samplingCounter % ChannelInfo5.quequePushInterval == 0){
		ChannelInfo5.samplingCounter++;
		if (pFrameInfo->nType == T_YV12)
		{

			Mat YUVImage(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

			Rect rect(600, 200, 1000, 800);
			Mat ROI = YUVImage(rect);

			ChannelInfo5.mutexLock.lock();
			ChannelInfo5.matQueque.push_back(ROI.clone());
			ChannelInfo5.mutexLock.unlock();
			
			if (ChannelInfo5.enableRealPlay){
				imshow("IPCamera5", ROI);
			}
			TRACE("----------------------------------------------通道【五】入队，队中数量：：%d \n", ChannelInfo5.matQueque.size());

			YUVImage.~Mat();
			ROI.~Mat();
		}
	}
	else{
		ChannelInfo5.samplingCounter++;
	}
};

// 回调相机视频流
void CALLBACK g_RealDataCallBack_V30_5(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //系统头
		if (!PlayM4_GetPort(&ChannelInfo5.nPort)) //获取播放库未使用的通道号
		{
			break;
		}
		//m_iPort = lPort; 
		//第一次回调的是系统头，将获取的播放库 port 号赋值给全局 port，下次回调数据时即使用此 port 号播放
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo5.nPort, STREAME_REALTIME)) //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo5.nPort, pBuffer, dwBufSize, 1024 * 1024)) //打开流接口
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo5.nPort, g_decCBFun_5, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo5.nPort, NULL)) //播放开始
			{
				break;
			}
		}
		break;
	case NET_DVR_STREAMDATA: //码流数据
		if (dwBufSize > 0 && ChannelInfo5.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo5.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	default: //其他数据
		if (dwBufSize > 0 && ChannelInfo5.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo5.nPort, pBuffer, dwBufSize)){
				break;
			}
		}
		break;
	}
}


void g_popList_5(std::string ip, ChannelInfo &channelInfo) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (channelInfo.isPlayingCV){

		if (channelInfo.matQueque.size() > 0){

			TRACE("\n---------------------------------------------- 通道【五】元素首部出队，队中剩余: %d \n", channelInfo.matQueque.size());
			channelInfo.mutexLock.lock();
			Mat pop = channelInfo.matQueque.front();

			channelInfo.matQueque.pop_front();
			channelInfo.mutexLock.unlock();
			// TODO 图像处理在这里调用
			//uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			// post到web服务器
			if (goal > 0){
				g_postData(pop, ip, goal, cnt, channelInfo.deviceIp);
			}
			pop.~Mat();

		}
		else {
			// 队列中没有图片则阻塞200ms，以免线程一直占用cpu资源
			srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			int RandomNumber = rand() % 10;//生成100以内的随机数
			Sleep(200 + RandomNumber);
		}
	}
	TRACE("\n---------------------------------------------- popList finidhed \n");
	return;
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CBulletsJournalDlg 对话框



CBulletsJournalDlg::CBulletsJournalDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBulletsJournalDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBulletsJournalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_deviceIp1);
	DDX_Control(pDX, IDC_IPADDRESS2, m_deviceIp2);
	DDX_Control(pDX, IDC_IPADDRESS3, m_deviceIp3);
	DDX_Control(pDX, IDC_IPADDRESS4, m_deviceIp4);
	DDX_Control(pDX, IDC_IPADDRESS5, m_deviceIp5);
	DDX_Control(pDX, IDC_IPADDRESS6, m_WebServerIp);
	DDX_Control(pDX, IDC_STATIC_STATUS1, g_status_1);
	DDX_Control(pDX, IDC_STATIC_STATUS2, g_status_2);
	DDX_Control(pDX, IDC_STATIC_STATUS3, g_status_3);
	DDX_Control(pDX, IDC_STATIC_STATUS4, g_status_4);
	DDX_Control(pDX, IDC_STATIC_STATUS5, g_status_5);

	DDX_Control(pDX, IDC_PREVIEW_CHECK1, g_previewCheck_1);
	DDX_Control(pDX, IDC_PREVIEW_CHECK2, g_previewCheck_2);
	DDX_Control(pDX, IDC_PREVIEW_CHECK3, g_previewCheck_3);
	DDX_Control(pDX, IDC_PREVIEW_CHECK4, g_previewCheck_4);
	DDX_Control(pDX, IDC_PREVIEW_CHECK5, g_previewCheck_5);
}

BEGIN_MESSAGE_MAP(CBulletsJournalDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGIN_BUTTON1, &CBulletsJournalDlg::OnBnClickedLoginButton1)
	ON_BN_CLICKED(IDC_ONEKEY_BUTTON, &CBulletsJournalDlg::OnBnClickedOnekeyButton)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON1, &CBulletsJournalDlg::OnBnClickedConfigButton1)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON2, &CBulletsJournalDlg::OnBnClickedLoginButton2)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON2, &CBulletsJournalDlg::OnBnClickedConfigButton2)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON3, &CBulletsJournalDlg::OnBnClickedLoginButton3)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON3, &CBulletsJournalDlg::OnBnClickedConfigButton3)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON4, &CBulletsJournalDlg::OnBnClickedLoginButton4)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON4, &CBulletsJournalDlg::OnBnClickedConfigButton4)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON5, &CBulletsJournalDlg::OnBnClickedLoginButton5)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON5, &CBulletsJournalDlg::OnBnClickedConfigButton5)
	ON_BN_CLICKED(IDC_PREVIEW_CHECK1, &CBulletsJournalDlg::OnBnClickedPreviewCheck1)
	ON_BN_CLICKED(IDC_PREVIEW_CHECK2, &CBulletsJournalDlg::OnBnClickedPreviewCheck2)
	ON_BN_CLICKED(IDC_PREVIEW_CHECK3, &CBulletsJournalDlg::OnBnClickedPreviewCheck3)
	ON_BN_CLICKED(IDC_PREVIEW_CHECK4, &CBulletsJournalDlg::OnBnClickedPreviewCheck4)
	ON_BN_CLICKED(IDC_PREVIEW_CHECK5, &CBulletsJournalDlg::OnBnClickedPreviewCheck5)
	ON_BN_CLICKED(IDC_TEST_WEB_BUTTON, &CBulletsJournalDlg::OnBnClickedTestWebButton)
END_MESSAGE_MAP()


// CBulletsJournalDlg 消息处理程序

BOOL CBulletsJournalDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	ShowWindow(SW_MINIMIZE);
	//hPlayWnd = GetDlgItem(IDC_STATIC_SCREEN1)->m_hWnd; 

	// 初始化SDK
	NET_DVR_Init();

	//初始化libcurl通信库，想用libcurl库的函数就必须首先初始化libcurl
	CURLcode code;
	code = curl_global_init(CURL_GLOBAL_ALL);
	if (CURLE_OK != code) {
		TRACE("\n ---------------------------------------------- 初始化通信库失败 \n");
		AfxMessageBox("初始化通信库失败，请联系管理员！");
		return false;
	}
	// 设置初始预览状态和风格
	g_previewCheck_1.SetCheck(1);
	g_previewCheck_2.SetCheck(1);
	g_previewCheck_3.SetCheck(1);
	g_previewCheck_4.SetCheck(1);
	g_previewCheck_5.SetCheck(1);
	g_previewCheck_1.SetButtonStyle(BS_AUTOCHECKBOX);
	g_previewCheck_2.SetButtonStyle(BS_AUTOCHECKBOX);
	g_previewCheck_3.SetButtonStyle(BS_AUTOCHECKBOX);
	g_previewCheck_4.SetButtonStyle(BS_AUTOCHECKBOX);
	g_previewCheck_5.SetButtonStyle(BS_AUTOCHECKBOX);

	// 设置默认IP
	m_deviceIp1.SetAddress(192, 168, 1, 101);
	m_deviceIp2.SetAddress(192, 168, 1, 102);
	m_deviceIp3.SetAddress(192, 168, 1, 101);
	m_deviceIp4.SetAddress(192, 168, 1, 102);
	m_deviceIp5.SetAddress(192, 168, 1, 64);
	m_WebServerIp.SetAddress(192, 168, 1, 3);

	// 将opencv imshow绑定到MFC pictrue control控件
	namedWindow("IPCamera1", 0);
	CRect screen1;
	CWnd *pWnd1 = GetDlgItem(IDC_STATIC_SCREEN1);//IDC_PICTURE为控件ID号
	pWnd1->GetClientRect(&screen1);
	resizeWindow("IPCamera1", screen1.Width(), screen1.Height());
	HWND hWnd_CAM1 = (HWND)cvGetWindowHandle("IPCamera1");
	HWND hParent_CAM1 = ::GetParent(hWnd_CAM1);
	::SetParent(hWnd_CAM1, GetDlgItem(IDC_STATIC_SCREEN1)->m_hWnd);
	::ShowWindow(hParent_CAM1, SW_HIDE); //隐藏运行程序框
	//GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(0);//创建时不显示播放控件

	namedWindow("IPCamera2", 0);
	CRect screen2;
	CWnd *pWnd2 = GetDlgItem(IDC_STATIC_SCREEN2);//IDC_PICTURE为控件ID号
	pWnd2->GetClientRect(&screen2);
	resizeWindow("IPCamera2", screen2.Width(), screen2.Height());
	HWND hWnd_CAM2 = (HWND)cvGetWindowHandle("IPCamera2");
	HWND hParent_CAM2 = ::GetParent(hWnd_CAM2);
	::SetParent(hWnd_CAM2, GetDlgItem(IDC_STATIC_SCREEN2)->m_hWnd);
	::ShowWindow(hParent_CAM2, SW_HIDE); //隐藏运行程序框

	namedWindow("IPCamera3", 0);
	CRect screen3;
	CWnd *pWnd3 = GetDlgItem(IDC_STATIC_SCREEN3);//IDC_PICTURE为控件ID号
	pWnd3->GetClientRect(&screen3);
	resizeWindow("IPCamera3", screen3.Width(), screen3.Height());
	HWND hWnd_CAM3 = (HWND)cvGetWindowHandle("IPCamera3");
	HWND hParent_CAM3 = ::GetParent(hWnd_CAM3);
	::SetParent(hWnd_CAM3, GetDlgItem(IDC_STATIC_SCREEN3)->m_hWnd);
	::ShowWindow(hParent_CAM3, SW_HIDE); //隐藏运行程序框

	namedWindow("IPCamera4", 0);
	CRect screen4;
	CWnd *pWnd4 = GetDlgItem(IDC_STATIC_SCREEN4);//IDC_PICTURE为控件ID号
	pWnd4->GetClientRect(&screen4);
	resizeWindow("IPCamera4", screen4.Width(), screen4.Height());
	HWND hWnd_CAM4 = (HWND)cvGetWindowHandle("IPCamera4");
	HWND hParent_CAM4 = ::GetParent(hWnd_CAM4);
	::SetParent(hWnd_CAM4, GetDlgItem(IDC_STATIC_SCREEN4)->m_hWnd);
	::ShowWindow(hParent_CAM4, SW_HIDE); //隐藏运行程序框

	namedWindow("IPCamera5", 0);
	CRect screen5;
	CWnd *pWnd5 = GetDlgItem(IDC_STATIC_SCREEN5);//IDC_PICTURE为控件ID号
	pWnd5->GetClientRect(&screen5);
	resizeWindow("IPCamera5", screen5.Width(), screen5.Height());
	HWND hWnd_CAM5 = (HWND)cvGetWindowHandle("IPCamera5");
	HWND hParent_CAM5 = ::GetParent(hWnd_CAM5);
	::SetParent(hWnd_CAM5, GetDlgItem(IDC_STATIC_SCREEN5)->m_hWnd);
	::ShowWindow(hParent_CAM5, SW_HIDE); //隐藏运行程序框
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CBulletsJournalDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else if (nID == SC_CLOSE){
		if (AfxMessageBox("您确定要退出系统吗?", MB_OKCANCEL) == IDCANCEL){
			return;
		}
		// 释放curllib通信库资源
		curl_global_cleanup();
		// 释放相机SDK资源
		NET_DVR_Cleanup();
	} else if (nID == SC_MINIMIZE){
		// 当选择最小化窗口时，关闭预览
		g_previewCheck_1.SetCheck(0);
		g_previewCheck_2.SetCheck(0);
		g_previewCheck_3.SetCheck(0);
		g_previewCheck_4.SetCheck(0);
		g_previewCheck_5.SetCheck(0);
		ChannelInfo1.enableRealPlay = false;
		ChannelInfo2.enableRealPlay = false;
		ChannelInfo3.enableRealPlay = false;
		ChannelInfo4.enableRealPlay = false;
		ChannelInfo5.enableRealPlay = false;
	} else{
		
	}
	

	CDialogEx::OnSysCommand(nID, lParam);
	
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CBulletsJournalDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CBulletsJournalDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CBulletsJournalDlg::OnBnClickedLoginButton1()
{
	// 定义错误
	map<int, string> errorCodeMap;
	errorCodeMap.insert(pair<int, string>(CONNECTED, "成功，检测程序正在运行。"));
	errorCodeMap.insert(pair<int, string>(INIT_FAILED, "异常，初始化失败。"));
	errorCodeMap.insert(pair<int, string>(LOGIN_FAILED, "异常，连接相机失败。"));
	errorCodeMap.insert(pair<int, string>(REALPLAY_FAILED, "异常，预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_REALPLAY_FAILED, "异常，停止预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_PLAYM4_FAILED, "异常，停止播放失败。"));
	errorCodeMap.insert(pair<int, string>(PLAYM4_CLOSESTREAM_FAILED, "异常，关闭数据流失败。"));
	errorCodeMap.insert(pair<int, string>(NET_DVR_LOGOUT_FAILED, "异常，退出登录失败。"));
	errorCodeMap.insert(pair<int, string>(UNCONNECTED, "已停止。"));
	errorCodeMap.insert(pair<int, string>(UNKNOWN, "异常，未知错误。"));

	// 每次启动预览，先清一次
	ChannelInfo1.mutexLock.lock();
	if (ChannelInfo1.matQueque.size() > 0){
		ChannelInfo1.matQueque.clear();
	}
	ChannelInfo1.mutexLock.unlock();

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON1)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		ChannelInfo1.isLogin = false;
		ChannelInfo1.isPlayingCV = false;
		
		int status = camera1.stop(std::ref(ChannelInfo1.nPort));
		if (status != 0){
			GetDlgItem(IDC_STATIC_STATUS1)->SetWindowText(errorCodeMap[status].c_str());
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}

		Sleep(1500);
		
		GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("开始");
		GetDlgItem(IDC_STATIC_STATUS1)->SetWindowText(errorCodeMap[UNCONNECTED].c_str());
		return;
	}

	ChannelInfo1.isPlayingCV = true;
	//显示播放控件
	GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(1);

	// 获取相机ip, web服务器ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp1.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));

	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);

	// 连接摄像机
	int status = camera1.start(g_RealDataCallBack_V30_1, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, ChannelInfo1.deviceIp);
	if (status != 0){
		AfxMessageBox("连接失败，请联系管理员！");
		GetDlgItem(IDC_STATIC_STATUS1)->SetWindowText(errorCodeMap[status].c_str());
		return;
	}
	ChannelInfo1.isLogin = true;
	
	GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("停止");
	GetDlgItem(IDC_STATIC_STATUS1)->SetWindowText(errorCodeMap[CONNECTED].c_str());
	
	// 开启图像处理线程
	std::thread popListThread(g_popList_1, webServerIp, std::ref(ChannelInfo1));
	popListThread.detach();
	// 预览
	//while (ChannelInfo1.isReady){
	//	ChannelInfo1.mutexLock.lock();
	//	if (ChannelInfo1.matQueque.size() > 0){

	//		TRACE("############################################################ 元素首部出栈: %d \n", ChannelInfo1.matQueque.size());

	//		Mat pop = ChannelInfo1.matQueque.front();
	//		ChannelInfo1.matQueque.pop_front();

	//		imshow("IPCamera", pop);
	//		GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(1); //显示播放控件
	//		pop.~Mat();
	//		waitKey(1);
	//	}

	//	ChannelInfo1.mutexLock.unlock();
	//	srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
	//	int RandomNumber = rand() % 10;//生成100以内的随机数
	//	Sleep(200 + RandomNumber);
	//}
	return;
}

void CBulletsJournalDlg::OnBnClickedConfigButton1()
{
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp1.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(FALSE);
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo1.deviceIp), NULL, SW_SHOW);
	
	
}


void CBulletsJournalDlg::OnBnClickedLoginButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	// 定义错误
	map<int, string> errorCodeMap;
	errorCodeMap.insert(pair<int, string>(CONNECTED, "成功，检测程序正在运行。"));
	errorCodeMap.insert(pair<int, string>(INIT_FAILED, "异常，初始化失败。"));
	errorCodeMap.insert(pair<int, string>(LOGIN_FAILED, "异常，连接相机失败。"));
	errorCodeMap.insert(pair<int, string>(REALPLAY_FAILED, "异常，预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_REALPLAY_FAILED, "异常，停止预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_PLAYM4_FAILED, "异常，停止播放失败。"));
	errorCodeMap.insert(pair<int, string>(PLAYM4_CLOSESTREAM_FAILED, "异常，关闭数据流失败。"));
	errorCodeMap.insert(pair<int, string>(NET_DVR_LOGOUT_FAILED, "异常，退出登录失败。"));
	errorCodeMap.insert(pair<int, string>(UNCONNECTED, "已停止。"));
	errorCodeMap.insert(pair<int, string>(UNKNOWN, "异常，未知错误。"));

	// 每次启动预览，先清一次
	ChannelInfo2.mutexLock.lock();
	if (ChannelInfo2.matQueque.size() > 0){
		ChannelInfo2.matQueque.clear();
	}
	ChannelInfo2.mutexLock.unlock();

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON2)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		ChannelInfo2.isLogin = false;
		ChannelInfo2.isPlayingCV = false;

		int status = camera2.stop(std::ref(ChannelInfo2.nPort));
		if (status != 0){
			GetDlgItem(IDC_STATIC_STATUS2)->SetWindowText(errorCodeMap[status].c_str());
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}

		Sleep(1500);

		GetDlgItem(IDC_LOGIN_BUTTON2)->SetWindowText("开始");
		GetDlgItem(IDC_STATIC_STATUS2)->SetWindowText(errorCodeMap[UNCONNECTED].c_str());
		return;
	}

	ChannelInfo2.isPlayingCV = true;
	//显示播放控件
	GetDlgItem(IDC_STATIC_SCREEN2)->ShowWindow(1);

	// 获取相机ip, web服务器ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp2.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo2.deviceIp, 16, "%s", csTemp.GetBuffer(0));

	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);

	// 连接摄像机
	int status = camera2.start(g_RealDataCallBack_V30_2, std::ref(ChannelInfo2.nPort), ChannelInfo2.hPlayWnd, ChannelInfo2.deviceIp);
	if (status != 0){
		AfxMessageBox("连接失败，请联系管理员！");
		GetDlgItem(IDC_STATIC_STATUS2)->SetWindowText(errorCodeMap[status].c_str());
		return;
	}
	ChannelInfo2.isLogin = true;

	GetDlgItem(IDC_LOGIN_BUTTON2)->SetWindowText("停止");
	GetDlgItem(IDC_STATIC_STATUS2)->SetWindowText(errorCodeMap[CONNECTED].c_str());

	// 开启图像处理线程
	std::thread popListThread(g_popList_2, webServerIp, std::ref(ChannelInfo2));
	popListThread.detach();
	return;
}


void CBulletsJournalDlg::OnBnClickedConfigButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp2.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo2.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(FALSE);
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo2.deviceIp), NULL, SW_SHOW);
}


void CBulletsJournalDlg::OnBnClickedLoginButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	// 定义错误
	map<int, string> errorCodeMap;
	errorCodeMap.insert(pair<int, string>(CONNECTED, "成功，检测程序正在运行。"));
	errorCodeMap.insert(pair<int, string>(INIT_FAILED, "异常，初始化失败。"));
	errorCodeMap.insert(pair<int, string>(LOGIN_FAILED, "异常，连接相机失败。"));
	errorCodeMap.insert(pair<int, string>(REALPLAY_FAILED, "异常，预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_REALPLAY_FAILED, "异常，停止预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_PLAYM4_FAILED, "异常，停止播放失败。"));
	errorCodeMap.insert(pair<int, string>(PLAYM4_CLOSESTREAM_FAILED, "异常，关闭数据流失败。"));
	errorCodeMap.insert(pair<int, string>(NET_DVR_LOGOUT_FAILED, "异常，退出登录失败。"));
	errorCodeMap.insert(pair<int, string>(UNCONNECTED, "已停止。"));
	errorCodeMap.insert(pair<int, string>(UNKNOWN, "异常，未知错误。"));

	// 每次启动预览，先清一次
	ChannelInfo3.mutexLock.lock();
	if (ChannelInfo3.matQueque.size() > 0){
		ChannelInfo3.matQueque.clear();
	}
	ChannelInfo3.mutexLock.unlock();

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON3)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		ChannelInfo3.isLogin = false;
		ChannelInfo3.isPlayingCV = false;

		int status = camera3.stop(std::ref(ChannelInfo3.nPort));
		if (status != 0){
			GetDlgItem(IDC_STATIC_STATUS3)->SetWindowText(errorCodeMap[status].c_str());
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}

		Sleep(1500);

		GetDlgItem(IDC_LOGIN_BUTTON3)->SetWindowText("开始");
		GetDlgItem(IDC_STATIC_STATUS3)->SetWindowText(errorCodeMap[UNCONNECTED].c_str());
		return;
	}

	ChannelInfo3.isPlayingCV = true;
	//显示播放控件
	GetDlgItem(IDC_STATIC_SCREEN3)->ShowWindow(1);

	// 获取相机ip, web服务器ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp3.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo3.deviceIp, 16, "%s", csTemp.GetBuffer(0));

	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);

	// 连接摄像机
	int status = camera3.start(g_RealDataCallBack_V30_3, std::ref(ChannelInfo3.nPort), ChannelInfo3.hPlayWnd, ChannelInfo3.deviceIp);
	if (status != 0){
		AfxMessageBox("连接失败，请联系管理员！");
		GetDlgItem(IDC_STATIC_STATUS3)->SetWindowText(errorCodeMap[status].c_str());
		return;
	}
	ChannelInfo3.isLogin = true;

	GetDlgItem(IDC_LOGIN_BUTTON3)->SetWindowText("停止");
	GetDlgItem(IDC_STATIC_STATUS3)->SetWindowText(errorCodeMap[CONNECTED].c_str());

	// 开启图像处理线程
	std::thread popListThread(g_popList_3, webServerIp, std::ref(ChannelInfo3));
	popListThread.detach();
	return;
}


void CBulletsJournalDlg::OnBnClickedConfigButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp3.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo3.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(FALSE);
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo3.deviceIp), NULL, SW_SHOW);
}


void CBulletsJournalDlg::OnBnClickedLoginButton4()
{
	// TODO:  在此添加控件通知处理程序代码
	// 定义错误
	map<int, string> errorCodeMap;
	errorCodeMap.insert(pair<int, string>(CONNECTED, "成功，检测程序正在运行。"));
	errorCodeMap.insert(pair<int, string>(INIT_FAILED, "异常，初始化失败。"));
	errorCodeMap.insert(pair<int, string>(LOGIN_FAILED, "异常，连接相机失败。"));
	errorCodeMap.insert(pair<int, string>(REALPLAY_FAILED, "异常，预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_REALPLAY_FAILED, "异常，停止预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_PLAYM4_FAILED, "异常，停止播放失败。"));
	errorCodeMap.insert(pair<int, string>(PLAYM4_CLOSESTREAM_FAILED, "异常，关闭数据流失败。"));
	errorCodeMap.insert(pair<int, string>(NET_DVR_LOGOUT_FAILED, "异常，退出登录失败。"));
	errorCodeMap.insert(pair<int, string>(UNCONNECTED, "已停止。"));
	errorCodeMap.insert(pair<int, string>(UNKNOWN, "异常，未知错误。"));

	// 每次启动预览，先清一次
	ChannelInfo4.mutexLock.lock();
	if (ChannelInfo4.matQueque.size() > 0){
		ChannelInfo4.matQueque.clear();
	}
	ChannelInfo4.mutexLock.unlock();

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON4)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		ChannelInfo4.isLogin = false;
		ChannelInfo4.isPlayingCV = false;

		int status = camera4.stop(std::ref(ChannelInfo4.nPort));
		if (status != 0){
			GetDlgItem(IDC_STATIC_STATUS4)->SetWindowText(errorCodeMap[status].c_str());
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}

		Sleep(1500);

		GetDlgItem(IDC_LOGIN_BUTTON4)->SetWindowText("开始");
		GetDlgItem(IDC_STATIC_STATUS4)->SetWindowText(errorCodeMap[UNCONNECTED].c_str());
		return;
	}

	ChannelInfo4.isPlayingCV = true;
	//显示播放控件
	GetDlgItem(IDC_STATIC_SCREEN4)->ShowWindow(1);

	// 获取相机ip, web服务器ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp4.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo4.deviceIp, 16, "%s", csTemp.GetBuffer(0));

	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);

	// 连接摄像机
	int status = camera4.start(g_RealDataCallBack_V30_4, std::ref(ChannelInfo4.nPort), ChannelInfo4.hPlayWnd, ChannelInfo4.deviceIp);
	if (status != 0){
		AfxMessageBox("连接失败，请联系管理员！");
		GetDlgItem(IDC_STATIC_STATUS4)->SetWindowText(errorCodeMap[status].c_str());
		return;
	}
	ChannelInfo4.isLogin = true;

	GetDlgItem(IDC_LOGIN_BUTTON4)->SetWindowText("停止");
	GetDlgItem(IDC_STATIC_STATUS4)->SetWindowText(errorCodeMap[CONNECTED].c_str());

	// 开启图像处理线程
	std::thread popListThread(g_popList_4, webServerIp, std::ref(ChannelInfo4));
	popListThread.detach();
	return;
}


void CBulletsJournalDlg::OnBnClickedConfigButton4()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp4.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo4.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(FALSE);
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo4.deviceIp), NULL, SW_SHOW);
}


void CBulletsJournalDlg::OnBnClickedLoginButton5()
{
	// TODO:  在此添加控件通知处理程序代码
	// 定义错误
	map<int, string> errorCodeMap;
	errorCodeMap.insert(pair<int, string>(CONNECTED, "成功，检测程序正在运行。"));
	errorCodeMap.insert(pair<int, string>(INIT_FAILED, "异常，初始化失败。"));
	errorCodeMap.insert(pair<int, string>(LOGIN_FAILED, "异常，连接相机失败。"));
	errorCodeMap.insert(pair<int, string>(REALPLAY_FAILED, "异常，预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_REALPLAY_FAILED, "异常，停止预览失败。"));
	errorCodeMap.insert(pair<int, string>(STOP_PLAYM4_FAILED, "异常，停止播放失败。"));
	errorCodeMap.insert(pair<int, string>(PLAYM4_CLOSESTREAM_FAILED, "异常，关闭数据流失败。"));
	errorCodeMap.insert(pair<int, string>(NET_DVR_LOGOUT_FAILED, "异常，退出登录失败。"));
	errorCodeMap.insert(pair<int, string>(UNCONNECTED, "已停止。"));
	errorCodeMap.insert(pair<int, string>(UNKNOWN, "异常，未知错误。"));

	// 每次启动预览，先清一次
	ChannelInfo5.mutexLock.lock();
	if (ChannelInfo5.matQueque.size() > 0){
		ChannelInfo5.matQueque.clear();
	}
	ChannelInfo5.mutexLock.unlock();

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON5)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		ChannelInfo5.isLogin = false;
		ChannelInfo5.isPlayingCV = false;

		int status = camera5.stop(std::ref(ChannelInfo5.nPort));
		if (status != 0){
			GetDlgItem(IDC_STATIC_STATUS5)->SetWindowText(errorCodeMap[status].c_str());
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}

		Sleep(1500);

		GetDlgItem(IDC_LOGIN_BUTTON5)->SetWindowText("开始");
		GetDlgItem(IDC_STATIC_STATUS5)->SetWindowText(errorCodeMap[UNCONNECTED].c_str());
		return;
	}

	ChannelInfo5.isPlayingCV = true;
	//显示播放控件
	GetDlgItem(IDC_STATIC_SCREEN5)->ShowWindow(1);

	// 获取相机ip, web服务器ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp5.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo5.deviceIp, 16, "%s", csTemp.GetBuffer(0));

	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);

	// 连接摄像机
	int status = camera5.start(g_RealDataCallBack_V30_5, std::ref(ChannelInfo5.nPort), ChannelInfo5.hPlayWnd, ChannelInfo5.deviceIp);
	if (status != 0){
		AfxMessageBox("连接失败，请联系管理员！");
		GetDlgItem(IDC_STATIC_STATUS5)->SetWindowText(errorCodeMap[status].c_str());
		return;
	}
	ChannelInfo5.isLogin = true;

	GetDlgItem(IDC_LOGIN_BUTTON5)->SetWindowText("停止");
	GetDlgItem(IDC_STATIC_STATUS5)->SetWindowText(errorCodeMap[CONNECTED].c_str());

	// 开启图像处理线程
	std::thread popListThread(g_popList_5, webServerIp, std::ref(ChannelInfo5));
	popListThread.detach();
	return;
}


void CBulletsJournalDlg::OnBnClickedConfigButton5()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp5.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo5.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(FALSE);
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo5.deviceIp), NULL, SW_SHOW);
}


void CBulletsJournalDlg::OnBnClickedTestWebButton()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	DWORD ip;
	m_WebServerIp.GetAddress(ip);
	CString temp = ipToStr(ip);
	std::string webServerIp = temp.GetBuffer();
	UpdateData(false);


	Mat testMat;
	testMat.create(5, 5, CV_8UC1);

	srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
	int goal = rand() % 20;//生成100以内的随机数
	int score = rand() % 100;//生成100以内的随机数

	bool res = g_postData(testMat, webServerIp, goal, score, "192.168.1.101");// 测试web服务器
	testMat.~Mat();

	if (res){
		AfxMessageBox("连接web服务器成功!");
	} else {
		AfxMessageBox("连接web服务器失败，请联系管理员!");
	}
	return;
}


void CBulletsJournalDlg::OnBnClickedOnekeyButton()
{
	AfxMessageBox("功能暂待开发!");
	return;
}


CString CBulletsJournalDlg::ipToStr(DWORD dwIP)
{
	CString strIP = _T("");
	WORD add1, add2, add3, add4;

	add1 = (WORD)(dwIP & 255);
	add2 = (WORD)((dwIP >> 8) & 255);
	add3 = (WORD)((dwIP >> 16) & 255);
	add4 = (WORD)((dwIP >> 24) & 255);
	strIP.Format("%d.%d.%d.%d", add4, add3, add2, add1);
	return strIP;
}

void CBulletsJournalDlg::OnBnClickedPreviewCheck1()
{
	// TODO:  在此添加控件通知处理程序代码
	// 0，按钮处于未选中状态。
	// 1，按钮处于选中状态。
	// 2，按钮状态不确定
	
	int state = g_previewCheck_1.GetCheck();
	if (state == 0){
		ChannelInfo1.enableRealPlay = false;
	} else if (state == 1){
		ChannelInfo1.enableRealPlay = true;
	} else {
		ChannelInfo1.enableRealPlay = false;
		TRACE("\n---------------------------------------------- checkbox【1】状态不确定 \n");
	}
}


void CBulletsJournalDlg::OnBnClickedPreviewCheck2()
{
	// TODO:  在此添加控件通知处理程序代码
	int state = g_previewCheck_2.GetCheck();
	if (state == 0){
		ChannelInfo2.enableRealPlay = false;
	}
	else if (state == 1){
		ChannelInfo2.enableRealPlay = true;
	}
	else {
		ChannelInfo2.enableRealPlay = false;
		TRACE("\n---------------------------------------------- checkbox【2】状态不确定 \n");
	}
}


void CBulletsJournalDlg::OnBnClickedPreviewCheck3()
{
	// TODO:  在此添加控件通知处理程序代码
	int state = g_previewCheck_3.GetCheck();
	if (state == 0){
		ChannelInfo3.enableRealPlay = false;
	}
	else if (state == 1){
		ChannelInfo3.enableRealPlay = true;
	}
	else {
		ChannelInfo3.enableRealPlay = false;
		TRACE("\n---------------------------------------------- checkbox【3】状态不确定 \n");
	}
}


void CBulletsJournalDlg::OnBnClickedPreviewCheck4()
{
	// TODO:  在此添加控件通知处理程序代码
	int state = g_previewCheck_4.GetCheck();
	if (state == 0){
		ChannelInfo4.enableRealPlay = false;
	}
	else if (state == 1){
		ChannelInfo4.enableRealPlay = true;
	}
	else {
		ChannelInfo4.enableRealPlay = false;
		TRACE("\n---------------------------------------------- checkbox【4】状态不确定 \n");
	}
}


void CBulletsJournalDlg::OnBnClickedPreviewCheck5()
{
	// TODO:  在此添加控件通知处理程序代码
	int state = g_previewCheck_5.GetCheck();
	if (state == 0){
		ChannelInfo5.enableRealPlay = false;
	}
	else if (state == 1){
		ChannelInfo5.enableRealPlay = true;
	}
	else {
		ChannelInfo5.enableRealPlay = false;
		TRACE("\n---------------------------------------------- checkbox【5】状态不确定 \n");
	}
}

