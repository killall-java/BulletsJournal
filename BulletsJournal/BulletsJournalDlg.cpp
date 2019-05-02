
// BulletsJournalDlg.cpp : ʵ���ļ�
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

struct ChannelInfo{
	// ���ھ�������ã�����imshow��
	HWND hPlayWnd;
	// α����
	list<Mat> matQueque; // ȫ��
	// �߳���
	std::mutex mutexLock;
	// ������ſ�ͨ����
	LONG nPort = -1;
	// ���IP
	char deviceIp[16];
	// ����ɹ����ӱ�־λ
	bool isReady = false;
	// ���SDK�쳣����
	char des[20]; 
	// �Ƿ����ڽ���Ԥ��
	bool isRealPlaying = false;
	// �Ƿ��Ѿ���¼
	bool isLogin = false;
	// �Ƿ�ɽ���ͼ����
	bool isPlayingCV = true;

	int samplingInterval = 0;

} ChannelInfo1, ChannelInfo2, ChannelInfo3��ChannelInfo4��ChannelInfo5;

CameraStream camera1;

//std::atomic<bool> forcedReturn(false);

//Mat g_BGRImage;
//int g_count = 0;
//clock_t g_start=0, g_ends=0;


//���ݽ���ص�������
//���ܣ���YV_12��ʽ����Ƶ������ת��Ϊ�ɹ�opencv�����BGR���͵�ͼƬ���ݣ���ʵʱ��ʾ��
void CALLBACK decCBFun(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	// 10 ��
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
			//srand((unsigned)time(NULL));//Ϊrand()�������ɲ�ͬ���������
			//int RandomNumber = rand() % 10;//����100���ڵ������
			//Sleep(50 + RandomNumber);
			///*Mat dst;
			//cvtColor(ROI, dst, CV_BGR2GRAY);
			//namedWindow("FHUT", WINDOW_AUTOSIZE);
			//imshow("FHUT", ROI);
			//waitKey(15);
			TRACE("\n ----------------------------------------------��ӣ�%d \n", ChannelInfo1.matQueque.size());
			//char image_name[25];
			//sprintf(image_name, "%s%d%s", "", start, ".jpg");//�����ͼƬ�� 
			//imwrite(image_name, ROI); //����һ֡ͼƬ 
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

// �ص������Ƶ��
void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* dwUser)
{
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //ϵͳͷ
		if (!PlayM4_GetPort(&ChannelInfo1.nPort)) //��ȡ���ſ�δʹ�õ�ͨ����
		{
			break;
		}
		//m_iPort = lPort; 
		//��һ�λص�����ϵͳͷ������ȡ�Ĳ��ſ� port �Ÿ�ֵ��ȫ�� port���´λص�����ʱ��ʹ�ô� port �Ų���
		if (dwBufSize > 0)
		{
			if (!PlayM4_SetStreamOpenMode(ChannelInfo1.nPort, STREAME_REALTIME)) //����ʵʱ������ģʽ
			{
				break;
			}
			if (!PlayM4_OpenStream(ChannelInfo1.nPort, pBuffer, dwBufSize, 1024 * 1024)) //�����ӿ�
			{
				break;
			}
			if (!PlayM4_SetDecCallBackExMend(ChannelInfo1.nPort, decCBFun, NULL, 0, NULL)){
				break;
			}
			if (!PlayM4_Play(ChannelInfo1.nPort, NULL)) //���ſ�ʼ
			{
				break;
			}
		} 
		break; 
	case NET_DVR_STREAMDATA: //��������
		if (dwBufSize > 0 && ChannelInfo1.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo1.nPort, pBuffer, dwBufSize)){
				break;
			} 
		} 
		break; 
	default: //��������
		if (dwBufSize > 0 && ChannelInfo1.nPort != -1){
			if (!PlayM4_InputData(ChannelInfo1.nPort, pBuffer, dwBufSize)){
				break;
			} 
		} 
		break; 
	}
}

static int OnCurlDebug(CURL *, curl_infotype itype, char* pData, size_t size, void*){
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

bool postData(cv::Mat mat, std::string ip, int goal, int cnt) {
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
	// ��ȡͼƬ����ָ��
	unsigned char* imgPtr = &*data_encode.begin();


	//struct curl_slist* headers = NULL;
	//headers = curl_slist_append(headers, "Content-Type:multipart/form-data; charset=UTF-8");
	//headers = curl_slist_append(headers, "Accept:text/html,text/plain,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
	//headers = curl_slist_append(headers, "Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3");
	//headers = curl_slist_append(headers, "Accept-Encoding:gzip, deflate");
	//headers = curl_slist_append(headers, "User-Agent:Mozilla/5.0 (Windows NT 6.1; WOW64; rv:22.0) Gecko/20100101 Firefox/22.0");
	//headers = curl_slist_append(headers, "Referer:http://minki.com/");

	
	//��ʼ��easy interface����ʹ��easy interface��api�����ͱ������ȳ�ʼ��easy interface
	CURL *easy_handle = curl_easy_init();
	//����easy handle�����ԺͲ���

	std::string url = "http://"+ ip + ":80/app/upload.action";
	curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
	//curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 3);
	//curl_easy_setopt(easy_handle, CURLOPT_NOSIGNAL, 1);
	//curl_easy_setopt(easy_handle, CURLOPT_CONNECTTIMEOUT, 3);
	//curl_easy_setopt(easy_handle, CURLOPT_READFUNCTION, NULL);
	//curl_easy_setopt(easy_handle, CURLOPT_VERBOSE, 1);
	//curl_easy_setopt(easy_handle, CURLOPT_DEBUGFUNCTION, OnDebug);
	//curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, headers);
	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;

	srand((unsigned)time(NULL));//Ϊrand()�������ɲ�ͬ���������

	std::string cameraId = std::to_string(rand() % 20);

	//std::string value = "{\"camaraId\":" + cameraId + ",\"scores\":" + std::to_string(cnt) + ",\"goal\":" + std::to_string(goal) + ",\"username\":\"JoyChou\"}";
	std::string value = "{\"camaraId\":" + cameraId + ",\"score\":" + std::to_string(rand() % 100) + ",\"username\":\"JoyChou\"}";

	std::string param = "param";
	curl_formadd(&post, &last,
		CURLFORM_COPYNAME, param.c_str(), //json�ַ����Ĳ�����
		CURLFORM_COPYCONTENTS, value.c_str(), //json�ַ���
		CURLFORM_END
		);
	curl_formadd(&post, &last,
		CURLFORM_COPYNAME, "image", //ͼƬ�Ĳ�����
		CURLFORM_BUFFER, "image.jpg", //ͼƬ����,����������,������������
		CURLFORM_BUFFERPTR, imgPtr, //ͼƬ��ŵ�����
		CURLFORM_BUFFERLENGTH, imgLength, //���ͼƬ���鳤��
		CURLFORM_CONTENTTYPE, "application/x-jpg", // application/x-jpg
		CURLFORM_END);

	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, post);
	CURLcode code = curl_easy_perform(easy_handle);//���ӵ�Զ���������������󣬲�������Ӧ 

	TRACE("############################################################ code: %d \n", code);
	curl_easy_cleanup(easy_handle);//�ͷ���Դ
	
	return true;
}

void popList(std::string ip) {
	int i = 0;
	int goal = 0;
	int cnt = 0;
	int* goalPrt = &goal;
	int* cntPrt = &cnt;

	while (ChannelInfo1.isPlayingCV){
		
		if (ChannelInfo1.matQueque.size() > 0){
			
			TRACE("\n---------------------------------------------- Ԫ���ײ����� BY OpenCV: %d \n", ChannelInfo1.matQueque.size());
			ChannelInfo1.mutexLock.lock();
			Mat pop = ChannelInfo1.matQueque.front();
			//if (i % 3 == 0){
			//	postData(pop, ip,1,3);
			//	i++;
			//} else if (i > 9){
			//	i = 0;
			//} else {
			//	i++;
			//}
			
			ChannelInfo1.matQueque.pop_front();
			ChannelInfo1.mutexLock.unlock();
			// TODO ͼ�������������
			uploadRslt(&pop, (char*)ip.data(), goalPrt, cntPrt);
			// post��web������
			postData(pop, ip, goal, cnt);
			pop.~Mat();
		} else {
			// ������û��ͼƬ������200ms�������߳�һֱռ��cpu��Դ
			srand((unsigned)time(NULL));//Ϊrand()�������ɲ�ͬ���������
			int RandomNumber = rand() % 10;//����100���ڵ������
			Sleep(200 + RandomNumber);
		}		
	}
	return;
}


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CBulletsJournalDlg �Ի���



CBulletsJournalDlg::CBulletsJournalDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBulletsJournalDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBulletsJournalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_deviceIp1);
	DDX_Control(pDX, IDC_IPADDRESS6, m_WebServerIp);
}

BEGIN_MESSAGE_MAP(CBulletsJournalDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGIN_BUTTON1, &CBulletsJournalDlg::OnBnClickedLoginButton1)
	ON_BN_CLICKED(IDC_ONEKEY_BUTTON, &CBulletsJournalDlg::OnBnClickedOnekeyButton)
	ON_BN_CLICKED(IDC_CONFIG_BUTTON1, &CBulletsJournalDlg::OnBnClickedConfigButton1)
END_MESSAGE_MAP()


// CBulletsJournalDlg ��Ϣ�������

BOOL CBulletsJournalDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	ShowWindow(SW_MINIMIZE);
	//hPlayWnd = GetDlgItem(IDC_STATIC_SCREEN1)->m_hWnd; 

	// ��ʼ��SDK
	NET_DVR_Init();

	//��ʼ��libcurlͨ�ſ⣬����libcurl��ĺ����ͱ������ȳ�ʼ��libcurl
	CURLcode code;
	code = curl_global_init(CURL_GLOBAL_ALL);
	if (CURLE_OK != code) {
		TRACE("\n ---------------------------------------------- ��ʼ��ͨ�ſ�ʧ�� \n");
		AfxMessageBox("��ʼ��ͨ�ſ�ʧ�ܣ�����ϵ����Ա��");
		return false;
	}

	// ����Ĭ��IP
	m_deviceIp1.SetAddress(192, 168, 1, 64);
	m_WebServerIp.SetAddress(192, 168, 1, 5);

	// ��opencv imshow�󶨵�MFC pictrue control�ؼ�
	namedWindow("IPCamera", 0);
	CRect screen1;
	CWnd *pWnd = GetDlgItem(IDC_STATIC_SCREEN1);//IDC_PICTUREΪ�ؼ�ID��
	pWnd->GetClientRect(&screen1);
	resizeWindow("IPCamera", screen1.Width(), screen1.Height());
	HWND hWnd_CAM1 = (HWND)cvGetWindowHandle("IPCamera");
	HWND hParent_CAM1 = ::GetParent(hWnd_CAM1);
	::SetParent(hWnd_CAM1, GetDlgItem(IDC_STATIC_SCREEN1)->m_hWnd);
	::ShowWindow(hParent_CAM1, SW_HIDE); //�������г����
	//GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(0);//����ʱ����ʾ���ſؼ�

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CBulletsJournalDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}

	if (nID == SC_CLOSE){
		if (AfxMessageBox("��ȷ��Ҫ�˳�ϵͳ��?", MB_OKCANCEL) == IDCANCEL){
			return;
		}
	}
	// �ͷ�curllibͨ�ſ���Դ
	curl_global_cleanup();
	// �ͷ����SDK��Դ
	NET_DVR_Cleanup();

	CDialogEx::OnSysCommand(nID, lParam);
	
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CBulletsJournalDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CBulletsJournalDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CBulletsJournalDlg::OnBnClickedLoginButton1()
{
	CString capital;
	GetDlgItem(IDC_LOGIN_BUTTON1)->GetWindowText(capital);
	if (capital == TEXT("ֹͣ")){
		bool status = camera1.stop(std::ref(ChannelInfo1.nPort));
		if (!status){
			AfxMessageBox("����ʧ�ܣ�����ϵ����Ա��");
			return;
		}
		ChannelInfo1.isReady = false;
		ChannelInfo1.isRealPlaying = false;
		Sleep(1000);
		GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("Ԥ��");
		GetDlgItem(IDC_ONEKEY_BUTTON)->EnableWindow(TRUE);
		return;
	}

	GetDlgItem(IDC_ONEKEY_BUTTON)->EnableWindow(FALSE);

	// ÿ������Ԥ��������һ��
	if (ChannelInfo1.matQueque.size() > 0){
		ChannelInfo1.matQueque.clear();
	}

	// ��ȡ���ip
	UpdateData(TRUE);
	DWORD dwDeviceIP;
	CString csTemp;
	m_deviceIp1.GetAddress(dwDeviceIP);
	csTemp = ipToStr(dwDeviceIP);
	sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));
	UpdateData(false);
	
	// ���������
	bool status = camera1.start(g_RealDataCallBack_V30, decCBFun, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, ChannelInfo1.deviceIp);
	if (!status){
		AfxMessageBox("����ʧ�ܣ�����ϵ����Ա��");
		return;
	}
	ChannelInfo1.isReady = true;
	ChannelInfo1.isLogin = true;
	ChannelInfo1.isRealPlaying = true;

	GetDlgItem(IDC_LOGIN_BUTTON1)->SetWindowText("ֹͣ");
	
	// Ԥ��
	while (ChannelInfo1.isReady){
		ChannelInfo1.mutexLock.lock();
		if (ChannelInfo1.matQueque.size() > 0){

			TRACE("############################################################ Ԫ���ײ���ջ: %d \n", ChannelInfo1.matQueque.size());

			Mat pop = ChannelInfo1.matQueque.front();
			ChannelInfo1.matQueque.pop_front();

			imshow("IPCamera", pop);
			GetDlgItem(IDC_STATIC_SCREEN1)->ShowWindow(1); //��ʾ���ſؼ�
			pop.~Mat();
			waitKey(1);
		}

		ChannelInfo1.mutexLock.unlock();
		srand((unsigned)time(NULL));//Ϊrand()�������ɲ�ͬ���������
		int RandomNumber = rand() % 10;//����100���ڵ������
		Sleep(200 + RandomNumber);
	}
	return;
}

void CBulletsJournalDlg::OnBnClickedOnekeyButton()
{
	if (m_WebServerIp.IsBlank()){
		AfxMessageBox("Web������IP����Ϊ��!");
		return;
	}
	// ��ȡweb������ip
	UpdateData(TRUE);
	DWORD ip;
	CString csTemp;
	m_WebServerIp.GetAddress(ip);
	
	csTemp = ipToStr(ip);
	std::string webServerIp = csTemp.GetBuffer();
	UpdateData(false);
	
	CString capital;
	GetDlgItem(IDC_ONEKEY_BUTTON)->GetWindowText(capital);
	if (capital == TEXT("��������")){
		ChannelInfo1.isPlayingCV = false;
		bool status = camera1.stop(std::ref(ChannelInfo1.nPort));
		Sleep(1000);
		GetDlgItem(IDC_ONEKEY_BUTTON)->SetWindowText("��ʼ����");
		GetDlgItem(IDC_LOGIN_BUTTON1)->EnableWindow(TRUE);
		return;
	}
	if (ChannelInfo1.isRealPlaying){
		AfxMessageBox("��ֹͣ����Ԥ����");
		return;
	} else {
		// �״�Ԥ�������� UpdateData ���ip
		GetDlgItem(IDC_LOGIN_BUTTON1)->EnableWindow(FALSE);

		// ��ȡip
		UpdateData(TRUE);
		DWORD dwDeviceIP;
		CString csTemp;
		m_deviceIp1.GetAddress(dwDeviceIP);
		csTemp = ipToStr(dwDeviceIP);
		sprintf_s(ChannelInfo1.deviceIp, 16, "%s", csTemp.GetBuffer(0));
		UpdateData(false);
		
		ChannelInfo1.isPlayingCV = true;
		bool status = camera1.start(g_RealDataCallBack_V30, decCBFun, std::ref(ChannelInfo1.nPort), ChannelInfo1.hPlayWnd, ChannelInfo1.deviceIp);
		Sleep(500);
		// ����ͼ�����߳�
		std::thread popListThread(popList, webServerIp);
		ChannelInfo1.isRealPlaying = false;
		GetDlgItem(IDC_ONEKEY_BUTTON)->SetWindowText("��������");
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
	ChannelInfo1.deviceIp;
	ShellExecute(NULL, _T("open"), _T("iexplore.exe"), _T(ChannelInfo1.deviceIp), NULL, SW_SHOW);
	
	
}
