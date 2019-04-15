
// BulletsJournalDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


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
};

