// pxRTSPAnalyzerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StreamingMediaTools.h"
#include "pxRTSPAnalyzerDlg.h"
#include "afxdialogex.h"
#include "pxCommonDef.h"
#include "playCommon.hh"

DWORD WINAPI ThreadStartAnalyzeRTSP(LPVOID pParam);

CString  g_strRTSPTitle;
HWND     g_hWndRTSP;

// CPxRTSPAnalyzerDlg 对话框

IMPLEMENT_DYNAMIC(CPxRTSPAnalyzerDlg, CDialogEx)

CPxRTSPAnalyzerDlg::CPxRTSPAnalyzerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPxRTSPAnalyzerDlg::IDD, pParent)
	, m_strRTSP_URL(_T(""))
	, m_strAVNotSyncThreshold(_T(""))
{
	m_bStop = false;
	m_bShowAudio = true; 
	m_bShowVideo = true;

	::InitializeCriticalSection(&m_csListCtrl);   //初始化临界区
}

CPxRTSPAnalyzerDlg::~CPxRTSPAnalyzerDlg()
{
	m_bStop = false;
	m_bShowAudio = true; 
	m_bShowVideo = true;

	::DeleteCriticalSection(&m_csListCtrl);    //释放里临界区
}

void CPxRTSPAnalyzerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RTSP_URL, m_strRTSP_URL);
	DDX_Control(pDX, IDC_LIST_RTSP_RTP_PACKAGE, m_lcPackage);
	DDX_Text(pDX, IDC_EDIT_RTSP_AV_NOT_SYNC_THRESHOLD, m_strAVNotSyncThreshold);
}

BEGIN_MESSAGE_MAP(CPxRTSPAnalyzerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_START_ANALZYE,            &CPxRTSPAnalyzerDlg::OnBnClickedButtonStartAnalzye)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_STOP_ANALZYE,             &CPxRTSPAnalyzerDlg::OnBnClickedButtonStopAnalzye)
	ON_BN_CLICKED(IDC_CHECK_RTSP_SHOW_VIDEO_INFO,           &CPxRTSPAnalyzerDlg::OnBnClickedCheckShowVideoInfo)
	ON_BN_CLICKED(IDC_CHECK_RTSP_SHOW_AUDIO_INFO,           &CPxRTSPAnalyzerDlg::OnBnClickedCheckShowAudioInfo)
	ON_BN_CLICKED(IDC_CHECK_RTSP_GENERATE_264_FILE,         &CPxRTSPAnalyzerDlg::OnBnClickedCheckGenerate264File)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_SAVE_ANALZYE_INFO_2_FILE, &CPxRTSPAnalyzerDlg::OnBnClickedButtonSaveAnalzyeInfo2File)
	ON_BN_CLICKED(IDC_BUTTON_RTSP_TEST,                     &CPxRTSPAnalyzerDlg::OnBnClickedButtonRtspTest)
	ON_MESSAGE(WM_ADD_RTSP_PACKAGE_TO_LIST,                 &CPxRTSPAnalyzerDlg::AddPackage2ListCtrl)
	ON_BN_CLICKED(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE, &CPxRTSPAnalyzerDlg::OnBnClickedCheckSaveRtspProcessInfo2File)
END_MESSAGE_MAP()

// CPxRTSPAnalyzerDlg 消息处理程序
void CPxRTSPAnalyzerDlg::OnBnClickedButtonStartAnalzye()
{
	UpdateData();

	if (m_strRTSP_URL.IsEmpty())
	{
		AfxMessageBox("RTMP URL不能为空, 请输入 !!!");

		return ;
	}

	m_bStop = false;

	m_nLastVideoTimestamp = 0;

	m_lcPackage.DeleteAllItems();

	GetDlgItem(IDC_BUTTON_RTSP_START_RECORD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP_RECORD)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_RTSP_AV_NOT_SYNC_THRESHOLD)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_RTSP_CLEAR_PACKAGE_LIST)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RTSP_START_ANALZYE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP_ANALZYE)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_RTSP_GENERATE_264_FILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_RTSP_SAVE_ANALZYE_INFO_2_FILE)->EnableWindow(FALSE);

	AfxBeginThread((AFX_THREADPROC)ThreadStartAnalyzeRTSP,
		this,
		THREAD_PRIORITY_NORMAL);

	SaveConfig();

	UpdateData(FALSE);
}


void CPxRTSPAnalyzerDlg::OnBnClickedButtonStopAnalzye()
{
	m_bStop = true;

	ActivateItem();

	StopOpenRTSP();
}

// 恢复按钮等到Enable状态
void CPxRTSPAnalyzerDlg::ActivateItem()
{
	GetDlgItem(IDC_BUTTON_RTSP_START_RECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP_RECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_RTSP_AV_NOT_SYNC_THRESHOLD)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_RTSP_CLEAR_PACKAGE_LIST)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RTSP_START_ANALZYE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RTSP_STOP_ANALZYE)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_RTSP_GENERATE_264_FILE)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RTSP_SAVE_ANALZYE_INFO_2_FILE)->EnableWindow(TRUE);
}

void CPxRTSPAnalyzerDlg::OnBnClickedCheckShowVideoInfo()
{
	UpdateData();

	if (((CButton *)GetDlgItem(IDC_CHECK_RTSP_SHOW_VIDEO_INFO))->GetCheck() == BST_CHECKED)
	{
		m_bShowVideo = true; 
		g_strMsg.Format("Show Video analyze info. RTSP");
	}
	else
	{
		m_bShowVideo = false; 
		g_strMsg.Format("Do not Show Video analyze info. RTSP");
	}

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	UpdateData(FALSE);
}

void CPxRTSPAnalyzerDlg::OnBnClickedCheckShowAudioInfo()
{
	UpdateData();

	if (((CButton *)GetDlgItem(IDC_CHECK_RTSP_SHOW_AUDIO_INFO))->GetCheck() == BST_CHECKED)
	{
		m_bShowAudio = true; 
		g_strMsg.Format("Show Audio analyze info. RTSP");
	}
	else
	{
		m_bShowAudio = false; 
		g_strMsg.Format("Do not Show Audio analyze info. RTSP");
	}

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	UpdateData(FALSE);
}

void CPxRTSPAnalyzerDlg::OnBnClickedCheckGenerate264File()
{
	
}

void CPxRTSPAnalyzerDlg::OnBnClickedButtonSaveAnalzyeInfo2File()
{
	
}

BOOL CPxRTSPAnalyzerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	/*CWnd *pWnd  = AfxGetMainWnd(); 
	pWnd->GetWindowText(g_strAppTitle);*/

	g_hWndRTSP = GetSafeHwnd();

	Init();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CPxRTSPAnalyzerDlg::Init()
{
	UpdateData();

	// 从ini文件中读取RTSP_URL
	char szRTMP_URL[_MAX_PATH] = {0};
	GetPrivateProfileString("RTSP", 
							"URL", 
							"rtmp://live.hkstv.hk.lxdns.com/live/hks", 
							szRTMP_URL, 
							sizeof(szRTMP_URL), 
							g_strConfFile);

	m_strRTSP_URL.Format("%s", szRTMP_URL);

	char szAVNotSyncThreshold[_MAX_PATH] = {0};
	GetPrivateProfileString("RTSP", 
		"AVNotSyncThreshold", 
		"300", 
		szAVNotSyncThreshold, 
		sizeof(szAVNotSyncThreshold), 
		g_strConfFile);

	m_strAVNotSyncThreshold.Format("%s", szAVNotSyncThreshold);

	m_lcPackage.ModifyStyle(0, LVS_REPORT);
	m_lcPackage.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES);

	m_lcPackage.InsertColumn(0,_T("序号"),LVCFMT_RIGHT,80,-1);
	//m_lcPackage.InsertColumn(0,_T("序号"),LVCFMT_RIGHT,120,-1);
	m_lcPackage.InsertColumn(1,_T("包类型"),LVCFMT_CENTER,100,-1);
	m_lcPackage.InsertColumn(2,_T("时间戳(毫秒)"),LVCFMT_CENTER,100,-1);
	m_lcPackage.InsertColumn(3,_T("时间戳差值"), LVCFMT_CENTER,150,-1);
	m_lcPackage.InsertColumn(4,_T("大小(字节)"), LVCFMT_LEFT,100,-1);

	((CButton*)GetDlgItem(IDC_CHECK_RTSP_LOG_READ_INFO))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_RTSP_SHOW_VIDEO_INFO))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_RTSP_SHOW_AUDIO_INFO))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_RTSP_GENERATE_264_FILE))->SetCheck(BST_UNCHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE))->SetCheck(BST_UNCHECKED);

	UpdateData(FALSE);
}

void CPxRTSPAnalyzerDlg::SaveConfig()
{
	UpdateData();

	WritePrivateProfileString("RTSP", "URL",                m_strRTSP_URL,           g_strConfFile);
	WritePrivateProfileString("RTSP", "AVNotSyncThreshold", m_strAVNotSyncThreshold, g_strConfFile);

	UpdateData(FALSE);
}

CString g_strTestMsg;

LRESULT CPxRTSPAnalyzerDlg::AddPackage2ListCtrl( WPARAM wParam, LPARAM lParam )
{
	::EnterCriticalSection(&m_csListCtrl);

	RTSPPacket *psRTSPPackage = (RTSPPacket *)lParam;

	if (kePxMediaType_Video == psRTSPPackage->eMediaType)
	{
		if (!m_bShowVideo)
		{
			::LeaveCriticalSection(&m_csListCtrl);

			return 0;
		}	
	}

	if (kePxMediaType_Audio == psRTSPPackage->eMediaType)
	{
		if (!m_bShowAudio)
		{
			::LeaveCriticalSection(&m_csListCtrl);

			return 0;
		}
	}

	// test begin 
	{
		/*g_strMsg.Format("AddPackage2ListCtrl nTimeStamp :%I64d, m_nLastVideoTimestamp:%I64d, psRTSPPackage->nTimeStamp - m_nLastVideoTimestamp:%I64d", 
			psRTSPPackage->nTimeStamp,
			m_nLastVideoTimestamp,
			psRTSPPackage->nTimeStamp - m_nLastVideoTimestamp);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());*/
	}
	// test end

	m_lcPackage.SetRedraw(FALSE);

	CString strDelta2LastVideoOrAudioTs = "No";
	INT64   nDelta2LastVideoOrAudioTs   = 0;

	if (kePxMediaType_Video == psRTSPPackage->eMediaType)
	{
		if (0 == m_nLastVideoTimestamp)
		{
			strDelta2LastVideoOrAudioTs = "0";
		}
		else
		{
			nDelta2LastVideoOrAudioTs = psRTSPPackage->nTimeStamp - m_nLastVideoTimestamp;
			strDelta2LastVideoOrAudioTs.Format("%I64d", nDelta2LastVideoOrAudioTs);
		}
	}
	else if (kePxMediaType_Audio == psRTSPPackage->eMediaType)
	{
		//strDelta2LastVideoOrAudioTs = "####";
		if (0 == m_nLastAudioTimestamp)
		{
			strDelta2LastVideoOrAudioTs = "0";
		}
		else
		{
			nDelta2LastVideoOrAudioTs = psRTSPPackage->nTimeStamp - m_nLastAudioTimestamp;
			strDelta2LastVideoOrAudioTs.Format("%I64d", nDelta2LastVideoOrAudioTs);
		}
	}

	int nCnt     = m_lcPackage.GetItemCount();
	int maxIndex = m_lcPackage.InsertItem(nCnt, _T(""));//The line_pos is the Index value
	CString strIndex;
	strIndex.Format("%d", nCnt + 1);

	m_lcPackage.SetItemText(maxIndex, 0, strIndex);

	/*strIndex.Format("%s", psRTSPPackage->szId);
	m_lcPackage.SetItemText(maxIndex, 0, strIndex);*/

	/*m_lcAgentClient.SetItemTextColor(maxIndex, 0, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 0, RGB(96,96,96));*/

	CString strPackageType("");
	CString strVideoOrAudioCntFromLastIFrame("");

	if (kePxMediaType_Audio == psRTSPPackage->eMediaType)
	{
		strPackageType = "Audio";

		//m_nAudioCntFromLastIFrame++;

		//strVideoOrAudioCntFromLastIFrame.Format("@Audio@ %d", m_nAudioCntFromLastIFrame);

		/*for (int i = 0; i < 7; i++)
		{
			m_lcAgentClient.SetItemTextColor(maxIndex, i, RGB(255, 255, 255));
			m_lcAgentClient.SetItemBkColor(maxIndex,   i, RGB(51, 161,  201));
		}*/

		/*if (m_bShowAudio && m_bShowVideo)
		{
			m_lcPackage.SetItemTextColor(maxIndex, 7, RGB(255, 255, 255));		
			m_lcPackage.SetItemBkColor(maxIndex,   7, RGB(0x48, 0x76, 0xFF));
		}	*/

		m_nLastAudioTimestamp = psRTSPPackage->nTimeStamp;
	}
	else if (kePxMediaType_Video == psRTSPPackage->eMediaType)
	{
		strPackageType = "##Video##";

		//if (NULL != psRTMPPackage->uiFrameSize)
		{
			//m_nVideoCntFromLastIFrame++;

			if (kePxNALUType_I_Frame == psRTSPPackage->eNALUType)
			{
				strPackageType = "Video (I帧)";

				for (int i = 0; i < 5; i++)
				{
					m_lcPackage.SetItemTextColor(maxIndex, i, RGB(255, 255, 255));		
					m_lcPackage.SetItemBkColor(maxIndex,   i, RGB(61, 145,  64));
				}
			}
			else if (kePxNALUType_P_Frame == psRTSPPackage->eNALUType)
			{
				strPackageType = "Video (P帧)";
			}
			else if (kePxNALUType_SEI == psRTSPPackage->eNALUType)
			{
				strPackageType = "Video (SEI)";
			}
			else if (kePxNALUType_SPS == psRTSPPackage->eNALUType)
			{
				strPackageType = "Video (PPS)";
			}
			else if (kePxNALUType_PPS == psRTSPPackage->eNALUType)
			{
				strPackageType = "Video (SPS)";
			}

			/*if (-1 != m_nLastVideoTimestamp)
			{
			strDelta2LastVideoOrAudioTs.Format("%d", psRTMPPackage->m_nTimeStamp - m_nLastVideoTimestamp);
			}
			else
			{
			strDelta2LastVideoOrAudioTs = "No";
			}*/

			m_nLastVideoTimestamp = psRTSPPackage->nTimeStamp;

			// test begin 
			/*{
				g_strTestMsg.Format("AddPackage2ListCtrl SET m_nLastVideoTimestamp:%I64d", psRTSPPackage->nTimeStamp);
				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strTestMsg.GetBuffer());
			}*/
			// test end
		}

		//strVideoOrAudioCntFromLastIFrame.Format("#Video# %d", m_nVideoCntFromLastIFrame);
	}
	else
	{
		strPackageType.Format("%d", psRTSPPackage->eMediaType);
	}

	m_lcPackage.SetItemText(maxIndex, 1, strPackageType);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 2, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 2, RGB(96,96,96));*/


	CString strTimeStamp("");
	strTimeStamp.Format("%I64d", psRTSPPackage->nTimeStamp);

	m_lcPackage.SetItemText(maxIndex, 2, strTimeStamp);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 4, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 4, RGB(96,96,96));*/

	m_lcPackage.SetItemText(maxIndex, 3, strDelta2LastVideoOrAudioTs);

	CString strFrameSize("");
	strFrameSize.Format("%u", psRTSPPackage->uiFrameSize);
	m_lcPackage.SetItemText(maxIndex, 4, strFrameSize);

	//设置最后一行被选中
	m_lcPackage.SetItemState(m_lcPackage.GetItemCount() - 1, 
		LVIS_ACTIVATING | LVIS_FOCUSED | LVIS_SELECTED,  
		LVIS_SELECTED | LVIS_FOCUSED );
	//滚动到最后一行
	m_lcPackage.Scroll(CSize(0, 100000));

	m_lcPackage.SetRedraw(TRUE);

	::LeaveCriticalSection(&m_csListCtrl); 

	return 0;
}

DWORD WINAPI ThreadStartAnalyzeRTSP(LPVOID pParam)
{
	//AfxMessageBox("ThreadStartAnalyzeRTSP");

	CPxRTSPAnalyzerDlg* pRTSPAnalyzerDlg = (CPxRTSPAnalyzerDlg*)pParam;

	if (NULL == pRTSPAnalyzerDlg)
	{
		g_strMsg = "ThreadStartAnalyzeRTSP:: pRTSPAnalyzerDlg is NULL";	
		g_logFile.WriteLogInfo(g_strMsg);

		return -1;
	}

	char szRTSP_URL[_MAX_PATH] = {0};
	strncpy(szRTSP_URL, (LPCTSTR)pRTSPAnalyzerDlg->m_strRTSP_URL, sizeof(szRTSP_URL));

	g_strMsg.Format("URL:%s. 开始分析...", szRTSP_URL);		
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	Sleep(50);

	CString strStreamName = pRTSPAnalyzerDlg->m_strRTSP_URL;

	strStreamName.Replace("://", "_");
	strStreamName.Replace(":", "_");
	strStreamName.Replace("/", "_");

	if (strStreamName.IsEmpty())
	{
		g_strMsg = "Parse " + pRTSPAnalyzerDlg->m_strRTSP_URL;
		g_strMsg += " Fail, Get StreamName Fail.";

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		AfxMessageBox(g_strMsg);

		return 0;
	}

	char szAppPath[MAX_PATH] = {0};
	DWORD dwRet = GetModuleFileNameA(NULL, szAppPath, MAX_PATH);
	if (dwRet == MAX_PATH)
	{
		strcpy(szAppPath, ".");
	}

	(strrchr(szAppPath,'\\'))[0] = '\0';

	strcat(szAppPath, "\\RecordFile");

	CString strRecordFilePath("");
	strRecordFilePath.Format("%s", szAppPath);

	if (!pxIsDirExists(strRecordFilePath))
	{
		BOOL bRet = pxCreateDir(strRecordFilePath);
		if (!bRet)
		{
			g_strMsg = strRecordFilePath + "创建失败";
			g_logFile.WriteLog(g_strMsg, __FILE__, __LINE__);
		}	
	}

	SYSTEMTIME sSystemTime;
	GetLocalTime(&sSystemTime);

	sprintf_s(pRTSPAnalyzerDlg->m_sRTSPArg.szRecordFileName,
		_MAX_PATH, 
		"%s\\%s_%4d-%02d-%02d_%02d%02d%02d.mp4",
		szAppPath,
		strStreamName.GetBuffer(strStreamName.GetLength()),
		sSystemTime.wYear, 
		sSystemTime.wMonth, 
		sSystemTime.wDay,
		sSystemTime.wHour,
		sSystemTime.wMinute,
		sSystemTime.wSecond);

	sprintf_s(pRTSPAnalyzerDlg->m_sRTSPArg.szRTSPProcessInfoFileName,
		_MAX_PATH, 
		"%s\\%s_%4d-%02d-%02d_%02d%02d%02d.txt",
		szAppPath,
		strStreamName.GetBuffer(strStreamName.GetLength()),
		sSystemTime.wYear, 
		sSystemTime.wMonth, 
		sSystemTime.wDay,
		sSystemTime.wHour,
		sSystemTime.wMinute,
		sSystemTime.wSecond);

	strcpy_s(pRTSPAnalyzerDlg->m_sRTSPArg.szURL, RTSP_URL_LEN, szRTSP_URL);
	pRTSPAnalyzerDlg->m_sRTSPArg.eRecordMediaType = kePxRecordMediaType_MP4;

	if (((CButton *)pRTSPAnalyzerDlg->GetDlgItem(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE))->GetCheck() == BST_CHECKED)
	{
		pRTSPAnalyzerDlg->m_sRTSPArg.bRTSPProcessInfo2File = true;
	}
	else
	{
		pRTSPAnalyzerDlg->m_sRTSPArg.bRTSPProcessInfo2File = false;
	}
	
	StartOpenRTSP(pRTSPAnalyzerDlg->m_sRTSPArg);

#if 0
	TaskScheduler     *scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment  *env       = BasicUsageEnvironment::createNew(*scheduler);
	  
	//env->streamUsingTCP = true;

	/*CNsPerfTimer *pPerfTimer = CNsPerfTimer::GetTimerClass();
	int nBeginTime           = pPerfTimer->GetTime();*/
	
	USES_CONVERSION;
	RTSPClient *pRTSPClient = (RTSPClient *)createClient(*env, szRTSP_URL, 0, "RTSP Client");
	if (NULL == pRTSPClient) 
	{
		//*env << "Failed to create " << clientProtocolName << " client: " << env->getResultMsg() << "\n";

		return -1;
	}

	continueAfterClientCreation1(*env); // getOptions

	bool m_bInitAudioVideoCodec = false;

	while (true)
	{
		((BasicTaskScheduler0*)scheduler)->SingleStep();

		if (env->m_eRTSPState == kePxRTSPState_OPTIONS)
		{

		}
		else if (env->m_eRTSPState == kePxRTSPState_DESCRIBE)// 如果DESCRIBE命令返回成功,意味着登录设备成功.
		{
			//int nEndTime    = pPerfTimer->GetTime();          // 单位uS
			//int nLogUseTime = (nEndTime - nBeginTime) / 1000; // 登录耗时(单位: 微秒)		
		}
		else if (env->m_eRTSPState == kePxRTSPState_SETUP)
		{

		}	
		else if (env->m_eRTSPState == kePxRTSPState_PLAY)
		{

		}
		else if (env->m_eRTSPState == kePxRTSPState_TEARDOWN)
		{

		}	
	}

	// All subsequent activity takes place within the event loop:
	//env->taskScheduler().doEventLoop(&pthis->m_cStopFlag); // does not return
	/*NsLogNotifyA_Add_file(0, 
		0, 
		"#######CPxRTSP_Stream::ThreadRtspRead Exit. StreamName %s, SOURCE:%s",
		W2A(psLiveStreamInfo->wszStreamName),
		W2A(psLiveStreamInfo->wszSourceURL)
		);
*/
	//shutdown(env, 0);

	if (env)
	{
		env->reclaim();
		env = NULL;
	}

	if (scheduler)
	{
		delete scheduler;
		scheduler = NULL;
	}
#endif

	return 0;
}

RTSPPacket g_sTestPackage;

void CPxRTSPAnalyzerDlg::OnBnClickedButtonRtspTest()
{
	//memset(&g_sTestPackage, 0, sizeof(RTSPPacket));

	g_sTestPackage.eMediaType     = kePxMediaType_Video;
	g_sTestPackage.eNALUType      = kePxNALUType_SPS;
	g_sTestPackage.nTimeStamp     = 22222;
	g_sTestPackage.uiFrameSize    = 0;	/* timestamp absolute or relative? */

	::PostMessage(GetSafeHwnd(), WM_ADD_RTSP_PACKAGE_TO_LIST, NULL, (LPARAM)&g_sTestPackage);
}


void CPxRTSPAnalyzerDlg::OnBnClickedCheckSaveRtspProcessInfo2File()
{
	UpdateData();

	if (((CButton *)GetDlgItem(IDC_CHECK_SAVE_RTSP_PROCESS_INFO_2_FILE))->GetCheck() == BST_CHECKED)
	{
		m_sRTSPArg.bRTSPProcessInfo2File = true;
		g_strMsg.Format("打开. 打印RTSP交互信息到文件(保存在录制文件目录).");
	}
	else
	{
		m_sRTSPArg.bRTSPProcessInfo2File = false;
		g_strMsg.Format("关闭. 打印RTSP交互信息到文件.");
	}

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	UpdateData(FALSE);
}
