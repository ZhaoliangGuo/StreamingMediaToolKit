
// StreamingMediaToolsDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "pxRTMPAnalyzerDlg.h"
#include "pxRTSPAnalyzerDlg.h"
#include "pxAACAnalyzeDlg.h"
#include "afxwin.h"

#define MESSAGE_BUFFER_SIZE (64*1024)

// CStreamingMediaToolsDlg 对话框
class CStreamingMediaToolsDlg : public CDialogEx
{
// 构造
public:
	CStreamingMediaToolsDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_STREAMINGMEDIATOOLS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

protected:
	CPxRTMPAnalyzerDlg *m_pRTMPAnalyzerDlg;
	CPxRTSPAnalyzerDlg *m_pRTSPAnalyzerDlg;
	CPxAACAnalyzeDlg   *m_pAACAnalyzeDlg;
    
public:
	CTabCtrl m_TabCtrl;
	afx_msg void OnNMClickTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();
	afx_msg LRESULT AddLog2List( WPARAM wParam, LPARAM lParam );
	void OnClearLogList();
	void Init();
	void TestAddLog();
	CListBox m_listboxLogInfo;
	afx_msg void OnBnClickedButtonTest();

	CRITICAL_SECTION  m_csListBox;             // 临界区
	afx_msg void OnRightMenuMinimize();
	afx_msg void OnRightMenuOpenLogDir();
	afx_msg void OnRightMenuOpenTodayLog();
	afx_msg void OnRightMenuOpenInstallDir();
	afx_msg void OnRightMenuExit();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnRightMenuOpenRecordDir();

private:
	char m_szMsgBuffer[MESSAGE_BUFFER_SIZE];
};
