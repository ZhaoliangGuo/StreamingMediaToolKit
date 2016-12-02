#pragma once


// CPxAACAnalyzeDlg 对话框

class CPxAACAnalyzeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPxAACAnalyzeDlg)

public:
	CPxAACAnalyzeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPxAACAnalyzeDlg();

// 对话框数据
	enum { IDD = IDD_OLE_PROPPAGE_LARGE_AAC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonAnalyzeAudiospecificconfig();
};
