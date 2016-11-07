#pragma once


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
};
