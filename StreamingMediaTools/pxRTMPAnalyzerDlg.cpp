// pxRTMPAnalyzerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StreamingMediaTools.h"
#include "pxRTMPAnalyzerDlg.h"
#include "afxdialogex.h"
#include <stdio.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include <time.h>
#include "pxCommonDef.h"

int InitSockets()
{
#ifdef WIN32
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	return (WSAStartup(version, &wsaData) == 0);
#endif
}

void CleanupSockets()
{
#ifdef WIN32
	WSACleanup();
#endif
}


DWORD WINAPI ThreadStartRecord(LPVOID pParam);

// CPxRTMPAnalyzerDlg 对话框

IMPLEMENT_DYNAMIC(CPxRTMPAnalyzerDlg, CDialogEx)

CPxRTMPAnalyzerDlg::CPxRTMPAnalyzerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPxRTMPAnalyzerDlg::IDD, pParent)
	, m_strRTMP_URL(_T(""))
{
	m_strRTMP_URL = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
}

CPxRTMPAnalyzerDlg::~CPxRTMPAnalyzerDlg()
{
}

void CPxRTMPAnalyzerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RTMP_URL, m_strRTMP_URL);
}

BEGIN_MESSAGE_MAP(CPxRTMPAnalyzerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START_RECORD, &CPxRTMPAnalyzerDlg::OnBnClickedButtonStartRecord)
	ON_BN_CLICKED(IDC_BUTTON_STOP_RECORD, &CPxRTMPAnalyzerDlg::OnBnClickedButtonStopRecord)
END_MESSAGE_MAP()

// CPxRTMPAnalyzerDlg 消息处理程序
bool g_bStop = false; // 用于控制开始和结束

void CPxRTMPAnalyzerDlg::OnBnClickedButtonStartRecord()
{
	UpdateData();

	if (m_strRTMP_URL.IsEmpty())
	{
		AfxMessageBox("RTMP URL不能为空, 请输入 !!!");

		return ;
	}

	g_bStop = false;

	//GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(FALSE);
	//GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);

	AfxBeginThread((AFX_THREADPROC)ThreadStartRecord,
		this,
		THREAD_PRIORITY_NORMAL);

	SaveConfig();

	UpdateData(FALSE);
}

DWORD WINAPI ThreadStartRecord(LPVOID pParam)
{
	CPxRTMPAnalyzerDlg *poRTMPAnalyzerDlg = (CPxRTMPAnalyzerDlg *)pParam;

	char *pszRTMP_URL = (char *)poRTMPAnalyzerDlg->m_strRTMP_URL.GetBuffer(poRTMPAnalyzerDlg->m_strRTMP_URL.GetLength());
	if (NULL == pszRTMP_URL)
	{
		AfxMessageBox("RTMP URL 不能为空!!!");

		return 0;
	}

	InitSockets();

	int nRead;

	bool bLiveStream = true;				

	int nBufSize   = 1024*1024*10;
	char *szBuffer = (char*)malloc(nBufSize);
	memset(szBuffer, 0, nBufSize);

	long nCountBufSize = 0;

	char szAppPath[MAX_PATH] = {0};
	DWORD dwRet = GetModuleFileNameA(NULL, szAppPath, MAX_PATH);
	if (dwRet == MAX_PATH)
	{
		strcpy(szAppPath, ".");
	}

	(strrchr(szAppPath,'\\'))[0] = '\0';

	SYSTEMTIME sSystemTime;
	GetLocalTime(&sSystemTime);

	char szFileName[_MAX_PATH] = {0};
	sprintf_s(szFileName,
		_MAX_PATH, 
		"%s\\rtmp_stream_%4d%02d%02d%02d%02d%02d.flv",
		szAppPath,
		sSystemTime.wYear, 
		sSystemTime.wMonth, 
		sSystemTime.wDay,
		sSystemTime.wHour,
		sSystemTime.wSecond,
		sSystemTime.wSecond);

	FILE *fp=fopen(szFileName, "wb");
	if (!fp)
	{
		g_strMsg.Format("Open File Error(%s).", szFileName);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_LogPrintf("Open File Error.\n");
		CleanupSockets();

		return -1;
	}

	/* set log level */
	//RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
	//RTMP_LogSetLevel(loglvl);

	RTMP *rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);

	//set connection timeout,default 30s
	rtmp->Link.timeout = 10;

	char szRTMP_URL[_MAX_PATH] = {0};
	sprintf_s(szRTMP_URL, 
		_MAX_PATH, 
		"%s", 
		pszRTMP_URL);
		//"rtmp://live.hkstv.hk.lxdns.com/live/hks");
	//"rtmp://localhost:1935/vod/mp4:sample.mp4");
	//"rtmp://192.168.2.109:1935/vod/mp4:sample.mp4");

	// HKS's live URL
	if(!RTMP_SetupURL(rtmp, szRTMP_URL))
	{		
		g_strMsg.Format("RTMP_SetupURL Error.");
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	if (bLiveStream)
	{
		rtmp->Link.lFlags|=RTMP_LF_LIVE;
	}

	//1hour
	RTMP_SetBufferMS(rtmp, 3600*1000);		

	if(!RTMP_Connect(rtmp,NULL))
	{
		g_strMsg.Format("RTMP_Connect Error.");
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	if(!RTMP_ConnectStream(rtmp,0))
	{
		g_strMsg.Format("RTMP_ConnectStream Error.");
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		CleanupSockets();

		return -1;
	}

	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(FALSE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);

	g_strMsg.Format("开始录制. 文件名:%s, URL:%s", szFileName, szRTMP_URL);
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	time_t tmStartTime;
	time_t tmEndTime;

	tmStartTime = time(NULL);

	double dAudioVideoFrameCount = 0;

	while (!g_bStop)
	{
		// RTMP_Read返回0时说明RTMP_READ_COMPLETE
		nRead = RTMP_Read(rtmp, szBuffer, nBufSize);

		if (0 == nRead)
		{
			g_strMsg.Format("RTMP流接收结束(也可能是流存在文件读取失败). RTMP_Read 0 Bytes");
			::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

			g_bStop = true;

			break;
		}

		fwrite(szBuffer, 1, nRead, fp);
		nCountBufSize += nRead;

		dAudioVideoFrameCount++;

		/*g_strMsg.Format("%04d: Receive: %5d Bytes, Total: %5.2f KB\n", 
			nAudioVideoFrameCount, 
			nRead, 
			nCountBufSize*1.0/1024);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());*/

		//RTMP_LogPrintf("%04d: Receive: %5d Bytes, Total: %5.2f KB\n", nAudioVideoFrameCount, nRead, nCountBufSize*1.0/1024);	
	}

	tmEndTime = time(NULL);

	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if(szBuffer)
	{
		free(szBuffer);
		szBuffer = NULL;
	}

	if(rtmp)
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		CleanupSockets();

		rtmp = NULL;
	}

	/*RTMP_LogPrintf("FileName: %s.\n Total: %5.2f MB, UseTime: %ld s\n", 
		szFileName, 
		nCountBufSize*1.0/(1024*1024),
		tmEndTime - tmStartTime);*/

	g_strMsg.Format("结束录制. 文件名:%s. 文件大小: %5.2f MB, 音视频总帧数: %.0f, 历时: %ld s", 
					szFileName, 
					nCountBufSize*1.0/(1024*1024),
					dAudioVideoFrameCount,
					tmEndTime - tmStartTime
					);

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(TRUE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);

	//AfxMessageBox(szMsgBuffer);

	return 0;
}

void CPxRTMPAnalyzerDlg::OnBnClickedButtonStopRecord()
{
	// TODO: 在此添加控件通知处理程序代码
	g_bStop = true;

	GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);
}

BOOL CPxRTMPAnalyzerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	Init();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CPxRTMPAnalyzerDlg::Init()
{
	UpdateData();

	// 从ini文件中读取RTMP_URL

	char szRTMP_URL[_MAX_PATH] = {0};
	GetPrivateProfileString("RTMP", 
							"URL", 
							"rtmp://live.hkstv.hk.lxdns.com/live/hks", 
							szRTMP_URL, 
							sizeof(szRTMP_URL), 
							g_strConfFile);

	m_strRTMP_URL.Format("%s", szRTMP_URL);

	UpdateData(FALSE);
}

void CPxRTMPAnalyzerDlg::SaveConfig()
{
	UpdateData();

	WritePrivateProfileString("RTMP", "URL", m_strRTMP_URL, g_strConfFile);

	UpdateData(FALSE);
}