
// BulletsJournalDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "BulletsJournal.h"
#include "BulletsJournalDlg.h"
#include "afxdialogex.h"
#include "CameraStream.h"
#include <atomic>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	// 相机成功连接标志位
	bool isReady = false;
	// 相机SDK异常返回
	char des[20]; 
	// 是否正在进行预览
	bool isRealPlaying = false;
	// 是否已经登录
	bool isLogin = false;
	// 是否可进行图像处理
	bool isPlayingCV = true;

	int samplingInterval = 0;

} ChannelInfo1, ChannelInfo2, ChannelInfo3，ChannelInfo4，ChannelInfo5;

CameraStream camera1;

std::atomic<bool> forcedReturn(false);

//Mat g_BGRImage;
//int g_count = 0;
//clock_t g_start=0, g_ends=0;


//数据解码回调函数，
//功能：将YV_12格式的视频数据流转码为可供opencv处理的BGR类型的图片数据，并实时显示。
void CALLBACK decCBFun(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 秒
	if (ChannelInfo1.samplingInterval > 25){
		ChannelInfo1.samplingInterval = 0;

	} else if (ChannelInfo1.samplingInterval % 5 == 0){  
		ChannelInfo1.samplingInterval++;
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
			Rect rect(1000, 200, 800, 600);
			Mat ROI = YUVImage(rect);

			ChannelInfo1.mutexLock.lock();
			ChannelInfo1.matQueque.push_back(ROI.clone());
			ChannelInfo1.mutexLock.unlock();
			//srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
			//int RandomNumber = rand() % 10;//生成100以内的随机数
			//Sleep(50 + RandomNumber);
			///*Mat dst;
			//cvtColor(ROI, dst, CV_BGR2GRAY);
			//namedWindow("FHUT", WINDOW_AUTOSIZE);
			//imshow("FHUT", ROI);
			//waitKey(15);
			TRACE("\n ----------------------------------------------入队：%d \n", ChannelInfo1.matQueque.size());
			//char image_name[25];
			//sprintf(image_name, "%s%d%s", "", start, ".jpg");//保存的图片名 
			//imwrite(image_name, ROI); //保存一帧图片 
			//Sleep(1 * 1000);
			//delete[] image_name;

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
	} else{
		ChannelInfo1.samplingInterval++;
	}
};


//实时视频码流数据获取 回调函数
void CALLBACK realDataCallBack_V30(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser)
{
	if (dwDataType == NET_DVR_STREAMDATA)//码流数据
	{
		if (dwBufSize > 0 && ChannelInfo1.nPort != -1)
		{
			if (!PlayM4_InputData(ChannelInfo1.nPort, pBuffer, dwBufSize))
			{
				//std::cout << "fail input data" << std::endl;
			}
			else
			{
				//std::cout << "success input data" << std::endl;
			}

		}
	}
};


void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
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
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo1.nPort, decCBFun, NULL, 0, NULL)){
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

void popList() {
	while (ChannelInfo1.isPlayingCV){
		ChannelInfo1.mutexLock.lock();
		if (ChannelInfo1.matQueque.size() > 0){

			TRACE("\n---------------------------------------------- 元素首部出队 BY OpenCV: %d \n", ChannelInfo1.matQueque.size());
			// TODO 图像处理在这里调用
			Mat pop = ChannelInfo1.matQueque.front();
			/*imshow("IPCamera1", pop);
			waitKey(1);*/
			ChannelInfo1.matQueque.pop_front();
				
			pop.~Mat();
		}

		ChannelInfo1.mutexLock.unlock();
		srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
		int RandomNumber = rand() % 10;//生成100以内的随机数
		Sleep(50 + RandomNumber);
	}
	return;
}

void pushList(char *deviceIp, CameraStream camera) {
	bool status = camera.start(g_RealDataCallBack_V30, decCBFun, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, deviceIp);
	
	if (!status){
		AfxMessageBox("连接失败，请联系管理员！");
		return;
	}
	ChannelInfo1.isReady = status;
	while (true){
		if (forcedReturn){
			return;
		}
		Sleep(3000);
	}
	TRACE(" ---------------------------------------------- Login successful \n");
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
}

BEGIN_MESSAGE_MAP(CBulletsJournalDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGIN_BUTTON1, &CBulletsJournalDlg::OnBnClickedLoginButton1)
	ON_BN_CLICKED(IDC_ONEKEY_BUTTON, &CBulletsJournalDlg::OnBnClickedOnekeyButton)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON1, &CBulletsJournalDlg::OnBnClickedConfigButton1)
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

	// 设置默认IP
	m_deviceIp1.SetAddress(192, 168, 1, 64);

	// 将opencv imshow绑定到MFC pictrue control控件
	namedWindow("IPCamera", 0);
	CRect screen1;
	CWnd *pWnd = GetDlgItem(IDC_STATIC_SCREEN1);//IDC_PICTURE为控件ID号
	pWnd->GetClientRect(&screen1);
	resizeWindow("IPCamera", screen1.Width(), screen1.Height());
	HWND hWnd_CAM1 = (HWND)cvGetWindowHandle("IPCamera");
	HWND hParent_CAM1 = ::GetParent(hWnd_CAM1);
	::SetParent(hWnd_CAM1, GetDlgItem(IDC_STATIC_SCREEN1)->m_hWnd);
	::ShowWindow(hParent_CAM1, SW_HIDE); //隐藏运行程序框
	//GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(0);//创建时不显示播放控件

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CBulletsJournalDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
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
	// TODO:  在此添加控件通知处理程序代码

	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON1)->GetWindowText(capital);
	if (capital == TEXT("停止")){
		bool status = camera1.stop(std::ref(ChannelInfo1.nPort));
		if (!status){
			AfxMessageBox("连接失败，请联系管理员！");
			return;
		}
		ChannelInfo1.isReady = false;
		ChannelInfo1.isRealPlaying = false;
		Sleep(1500);
		GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("预览");
		GetDlgItem(IDC_ONEKEY_BUTTON)->EnableWindow(TRUE);
		return;
	}

	GetDlgItem(IDC_ONEKEY_BUTTON)->EnableWindow(FALSE);

	// 每次启动预览，先清一次
	if (ChannelInfo1.matQueque.size() > 0){
		ChannelInfo1.matQueque.clear();
	}

	// 首次预览，需先 UpdateData 相机ip
	if (!ChannelInfo1.isLogin){
		UpdateData(TRUE);
		DWORD dwDeviceIP;
		CString csTemp;
		m_deviceIp1.GetAddress(dwDeviceIP);
		csTemp = ipToStr(dwDeviceIP);
		sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));
		UpdateData(false);
	}

	bool status = camera1.start(g_RealDataCallBack_V30, decCBFun, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, ChannelInfo1.deviceIp);
	if (!status){
		AfxMessageBox("连接失败，请联系管理员！");
		return;
	}
	ChannelInfo1.isReady = true;
	ChannelInfo1.isLogin = true;
	ChannelInfo1.isRealPlaying = true;

	GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("停止");
	

	while (ChannelInfo1.isReady){
		ChannelInfo1.mutexLock.lock();
		if (ChannelInfo1.matQueque.size() > 0){

			TRACE("############################################################ 元素首部出栈: %d \n", ChannelInfo1.matQueque.size());

			Mat pop = ChannelInfo1.matQueque.front();
			ChannelInfo1.matQueque.pop_front();

			imshow("IPCamera", pop);
			GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(1); //显示播放控件
			pop.~Mat();
			waitKey(1);
		}

		ChannelInfo1.mutexLock.unlock();
		srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
		int RandomNumber = rand() % 10;//生成100以内的随机数
		Sleep(50 + RandomNumber);
	}
	return;
}

void CBulletsJournalDlg::OnBnClickedOnekeyButton()
{
	// TODO:  在此添加控件通知处理程序代码
	CString capital;
	GetDlgItem(IDC_ONEKEY_BUTTON)->GetWindowText(capital);
	if (capital == TEXT("结束报靶")){
		ChannelInfo1.isPlayingCV = false;
		bool status = camera1.stop(std::ref(ChannelInfo1.nPort));
		Sleep(1500);
		GetDlgItem(IDC_ONEKEY_BUTTON)->SetWindowText("开始报靶");
		GetDlgItem(IDC_LOGIN_BUTTON1)->EnableWindow(TRUE);
		return;
	}
	if (ChannelInfo1.isRealPlaying){
		AfxMessageBox("请停止所有预览！");
		return;
	} else {
		// 首次预览，需先 UpdateData 相机ip
		GetDlgItem(IDC_LOGIN_BUTTON1)->EnableWindow(FALSE);

		if (!ChannelInfo1.isLogin){
			UpdateData(TRUE);
			DWORD dwDeviceIP;
			CString csTemp;
			m_deviceIp1.GetAddress(dwDeviceIP);
			csTemp = ipToStr(dwDeviceIP);
			sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));
			UpdateData(false);
		}
		ChannelInfo1.isPlayingCV = true;
		bool status = camera1.start(g_RealDataCallBack_V30, decCBFun, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, ChannelInfo1.deviceIp);
		Sleep(500);
		std::thread popListThread(popList);
		ChannelInfo1.isRealPlaying = false;
		GetDlgItem(IDC_ONEKEY_BUTTON)->SetWindowText("结束报靶");
		GetDlgItem(IDC_LOGIN_BUTTON1)->EnableWindow(FALSE);
		popListThread.detach();
	}
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


void CBulletsJournalDlg::OnBnClickedConfigButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T("http://192.168.1.64/"), NULL, SW_SHOW);
}
