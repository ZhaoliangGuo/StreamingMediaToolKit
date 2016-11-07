#pragma once


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
};
