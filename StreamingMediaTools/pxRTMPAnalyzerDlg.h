#pragma once
#include "afxcmn.h"
#include "pxCommonDef.h"

const int  WM_ADD_PACKAGE_TO_LIST = WM_USER + 2001;

// CPxRTMPAnalyzerDlg 对话框

class CPxRTMPAnalyzerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPxRTMPAnalyzerDlg)

public:
	CPxRTMPAnalyzerDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPxRTMPAnalyzerDlg();

// 对话框数据
	enum { IDD = IDD_PAGE_RTMP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_strRTMP_URL;
	afx_msg void OnBnClickedButtonStartRecord();
	afx_msg void OnBnClickedButtonStopRecord();
	virtual BOOL OnInitDialog();
	void Init();
	void SaveConfig();
	afx_msg LRESULT AddPackage2ListCtrl( WPARAM wParam, LPARAM lParam );
	void TestAddPackage2ListCtrl();
	CReportCtrl m_lcAgentClient;
	afx_msg void OnBnClickedButtonRtmpTest();
	int m_nLastVideoTimestamp;
	CRITICAL_SECTION  m_csListCtrl; 
};
