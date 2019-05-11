
// BulletsJournalDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

// CBulletsJournalDlg 对话框
class CBulletsJournalDlg : public CDialogEx
{
// 构造
public:
	CBulletsJournalDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_BULLETSJOURNAL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	CString ipToStr(DWORD dwIP);

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedLoginButton1();
	CStatic m_picplay1;
	CIPAddressCtrl m_deviceIp1;
	afx_msg void OnBnClickedOnekeyButton();
	afx_msg void OnBnClickedConfigButton1();
	CIPAddressCtrl m_WebServerIp;
	CStatic g_status_1;
	CStatic g_status_2;
	CStatic g_status_3;
	CStatic g_status_4;
	CStatic g_status_5;
	afx_msg void OnBnClickedLoginButton2();
	afx_msg void OnBnClickedConfigButton2();
	afx_msg void OnBnClickedLoginButton3();
	afx_msg void OnBnClickedConfigButton3();
	afx_msg void OnBnClickedLoginButton4();
	afx_msg void OnBnClickedConfigButton4();
	afx_msg void OnBnClickedLoginButton5();
	afx_msg void OnBnClickedConfigButton5();
	CIPAddressCtrl m_deviceIp2;
	CIPAddressCtrl m_deviceIp3;
	CIPAddressCtrl m_deviceIp4;
	CIPAddressCtrl m_deviceIp5;
	afx_msg void OnBnClickedPreviewCheck1();
	CButton g_previewCheck_1;
	CButton g_previewCheck_2;
	CButton g_previewCheck_3;
	CButton g_previewCheck_4;
	CButton g_previewCheck_5;
	afx_msg void OnBnClickedPreviewCheck2();
	afx_msg void OnBnClickedPreviewCheck3();
	afx_msg void OnBnClickedPreviewCheck4();
	afx_msg void OnBnClickedPreviewCheck5();
	afx_msg void OnBnClickedTestWebButton();
};

