#pragma once


// CPxAnalyzeAudioSpecificConfigDlg 对话框

class CPxAnalyzeAudioSpecificConfigDlg : public CDialog
{
	DECLARE_DYNAMIC(CPxAnalyzeAudioSpecificConfigDlg)

public:
	CPxAnalyzeAudioSpecificConfigDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPxAnalyzeAudioSpecificConfigDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_ANALZYE_AAC_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_strAudioSpecicConfig;
	CString m_strObjectTypeIndex;
	CString m_strSamplingFrequencyIndex;
	CString m_strSamplingFrequency;
	CString m_strChannelsConfiguration;
	afx_msg void OnBnClickedButtonAnalzyeAudioSpecificConfig();
	virtual BOOL OnInitDialog();
	void SaveConfig();
	void Init();
};
