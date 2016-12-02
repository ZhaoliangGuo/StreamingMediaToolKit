// pxAnalyzeAudioSpecificConfigDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StreamingMediaTools.h"
#include "pxAnalyzeAudioSpecificConfigDlg.h"
#include "afxdialogex.h"
#include "pxCommonDef.h"


// CPxAnalyzeAudioSpecificConfigDlg 对话框

IMPLEMENT_DYNAMIC(CPxAnalyzeAudioSpecificConfigDlg, CDialog)

CPxAnalyzeAudioSpecificConfigDlg::CPxAnalyzeAudioSpecificConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPxAnalyzeAudioSpecificConfigDlg::IDD, pParent)
	, m_strAudioSpecicConfig(_T("1408"))
	, m_strObjectTypeIndex(_T("0"))
	, m_strSamplingFrequencyIndex(_T("0"))
	, m_strSamplingFrequency(_T("0"))
	, m_strChannelsConfiguration(_T("0"))
{

}

CPxAnalyzeAudioSpecificConfigDlg::~CPxAnalyzeAudioSpecificConfigDlg()
{
}

void CPxAnalyzeAudioSpecificConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_AUDIO_SPECIFIC_CONFIG, m_strAudioSpecicConfig);
	DDX_Text(pDX, IDC_EDIT_OBJECT_TYPE_INDEX, m_strObjectTypeIndex);
	DDX_Text(pDX, IDC_EDIT_SAMPLING_FREQ_INDEX, m_strSamplingFrequencyIndex);
	DDX_Text(pDX, IDC_EDIT_SAMPLING_FREQUENCY, m_strSamplingFrequency);
	DDX_Text(pDX, IDC_EDIT_CHANNEL_CONFIGURATION, m_strChannelsConfiguration);
}

BEGIN_MESSAGE_MAP(CPxAnalyzeAudioSpecificConfigDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ANALZYE, &CPxAnalyzeAudioSpecificConfigDlg::OnBnClickedButtonAnalzyeAudioSpecificConfig)
END_MESSAGE_MAP()

BOOL CPxAnalyzeAudioSpecificConfigDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Init();

	// TODO:  在此添加额外的初始化
	OnBnClickedButtonAnalzyeAudioSpecificConfig();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CPxAnalyzeAudioSpecificConfigDlg::OnBnClickedButtonAnalzyeAudioSpecificConfig()
{
	UpdateData();

	if (m_strAudioSpecicConfig.IsEmpty())
	{
		AfxMessageBox("请输出音频Config参数");

		return ;
	}

	string audioSpecicConfig = m_strAudioSpecicConfig.GetBuffer(0);

	char szAudioSpecicConfig[64] = {0};

	string strtmp          = audioSpecicConfig.substr(0,2);
	szAudioSpecicConfig[0] = strtol(strtmp.c_str(), NULL, 16); 
	strtmp                 = audioSpecicConfig.substr(2,2);
	szAudioSpecicConfig[1] = strtol(strtmp.c_str(), NULL, 16);

	/*strcpy(szAudioSpecicConfig, 
	m_strAudioSpecicConfig.GetBuffer(m_strAudioSpecicConfig.GetLength()));*/

	mp4AudioSpecificConfig mp4asc;
	char chret = 0;
	chret = pxAACDecodeAudioSpecificConfig((unsigned char*)szAudioSpecicConfig, 2, &mp4asc);
	if (chret < 0 )
	{
		g_strMsg.Format("解析 %s 失败.", szAudioSpecicConfig);
		AfxMessageBox("g_strMsg");

		return ;
	}

	m_strObjectTypeIndex.Format("%d",        mp4asc.objectTypeIndex);
	m_strSamplingFrequencyIndex.Format("%d", mp4asc.samplingFrequencyIndex);
	m_strSamplingFrequency.Format("%d",      mp4asc.samplingFrequency);
	m_strChannelsConfiguration.Format("%d",  mp4asc.channelsConfiguration);

	UpdateData(FALSE);

	SaveConfig();
}

void CPxAnalyzeAudioSpecificConfigDlg::SaveConfig()
{
	UpdateData();

	WritePrivateProfileString("Audio", "AudioSpecficConfig",  m_strAudioSpecicConfig,  g_strConfFile);

	UpdateData(FALSE);
}

void CPxAnalyzeAudioSpecificConfigDlg::Init()
{
	UpdateData();

	// 从ini文件中读取AudioSpecficConfig
	char szAudioSpecficConfig[_MAX_PATH] = {0};
	GetPrivateProfileString("Audio", 
		"AudioSpecficConfig",
		"1408", 
		szAudioSpecficConfig, 
		sizeof(szAudioSpecficConfig), 
		g_strConfFile);

	m_strAudioSpecicConfig.Format("%s", szAudioSpecficConfig);

	UpdateData(FALSE);
}



