
// BulletsJournalDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

// CBulletsJournalDlg �Ի���
class CBulletsJournalDlg : public CDialogEx
{
// ����
public:
	CBulletsJournalDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_BULLETSJOURNAL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

private:
	CString ipToStr(DWORD dwIP);

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	
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

