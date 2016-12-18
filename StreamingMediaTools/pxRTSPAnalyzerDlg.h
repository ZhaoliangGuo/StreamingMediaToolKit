#pragma once
#include "pxCommonDef.h"
#include "afxcmn.h"
#include "pxRTSPCommonDef.h"

// CPxRTSPAnalyzerDlg 对话框

class CPxRTSPAnalyzerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPxRTSPAnalyzerDlg)

public:
	CPxRTSPAnalyzerDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPxRTSPAnalyzerDlg();

// 对话框数据
	enum { IDD = IDD_PAGE_RTSP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CString m_strRTSP_URL;
	afx_msg void OnBnClickedButtonStartAnalzye();
	afx_msg void OnBnClickedButtonStopAnalzye();
	afx_msg void OnBnClickedCheckShowVideoInfo();
	afx_msg void OnBnClickedCheckShowAudioInfo();
	afx_msg void OnBnClickedCheckGenerate264File();
	afx_msg void OnBnClickedButtonSaveAnalzyeInfo2File();
	virtual BOOL OnInitDialog();

	void Init();
	void SaveConfig();
	afx_msg LRESULT AddPackage2ListCtrl( WPARAM wParam, LPARAM lParam );
	afx_msg void OnBnClickedButtonRtspTest();

public:
	CReportCtrl       m_lcPackage;
	CRITICAL_SECTION  m_csListCtrl; 
	INT64             m_nLastVideoTimestamp;
    INT64             m_nLastAudioTimestamp;
	bool              m_bShowVideo;
	bool              m_bShowAudio;
	bool              m_bGenerateH264File;     // 分析时是否同时生成.264文件
	CString           m_strAVNotSyncThreshold; // 音视频不同步的过滤阈值(毫秒)
	bool              m_bStop;                 // 用于控制开始和结束
	SPxRTSPArg        m_sRTSPArg;

public:
	afx_msg void OnBnClickedCheckSaveRtspProcessInfo2File();
	void ActivateItem();
}; 
