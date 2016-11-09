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

/*
References:
http://blog.chinaunix.net/uid-15063109-id-4273162.html
*/

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
DWORD WINAPI ThreadAnalyzeTimestamp(LPVOID pParam);

// CPxRTMPAnalyzerDlg 对话框

IMPLEMENT_DYNAMIC(CPxRTMPAnalyzerDlg, CDialogEx)

CPxRTMPAnalyzerDlg::CPxRTMPAnalyzerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPxRTMPAnalyzerDlg::IDD, pParent)
	, m_strRTMP_URL(_T(""))
	, m_strAVNotSyncThreshold(_T(""))
{
	m_strRTMP_URL        = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
	m_strAVNotSyncThreshold = "300";
	InitSockets();
	m_nLastVideoTimestamp = -1;
	::InitializeCriticalSection(&m_csListCtrl);   //初始化临界区
}

CPxRTMPAnalyzerDlg::~CPxRTMPAnalyzerDlg()
{
	CleanupSockets();
	m_nLastVideoTimestamp = -1;
	::DeleteCriticalSection(&m_csListCtrl);    //释放里临界区
}

void CPxRTMPAnalyzerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RTMP_URL, m_strRTMP_URL);
	DDX_Control(pDX, IDC_LIST_RTMP_FRAME, m_lcPackage);
	DDX_Text(pDX, IDC_EDIT_AV_NOT_SYNC_THRESHOLD, m_strAVNotSyncThreshold);
}

BEGIN_MESSAGE_MAP(CPxRTMPAnalyzerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START_RECORD, &CPxRTMPAnalyzerDlg::OnBnClickedButtonStartRecord)
	ON_BN_CLICKED(IDC_BUTTON_STOP_RECORD,  &CPxRTMPAnalyzerDlg::OnBnClickedButtonStopRecord)
	ON_MESSAGE(WM_ADD_PACKAGE_TO_LIST,     &CPxRTMPAnalyzerDlg::AddPackage2ListCtrl)
	ON_BN_CLICKED(IDC_BUTTON_RTMP_TEST, &CPxRTMPAnalyzerDlg::OnBnClickedButtonRtmpTest)
	ON_BN_CLICKED(IDC_CHECK_CLEAR_PACKAGE_LIST, &CPxRTMPAnalyzerDlg::OnBnClickedCheckClearPackageList)
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

	m_nLastVideoTimestamp = 0;

	m_lcPackage.DeleteAllItems();

	AfxBeginThread((AFX_THREADPROC)ThreadStartRecord,
		this,
		THREAD_PRIORITY_NORMAL);

	SaveConfig();

	UpdateData(FALSE);
}

RTMPPacket pkt = { 0 };

DWORD WINAPI ThreadStartRecord(LPVOID pParam)
{
	CPxRTMPAnalyzerDlg *poRTMPAnalyzerDlg = (CPxRTMPAnalyzerDlg *)pParam;

	char *pszRTMP_URL = (char *)poRTMPAnalyzerDlg->m_strRTMP_URL.GetBuffer(poRTMPAnalyzerDlg->m_strRTMP_URL.GetLength());
	if (NULL == pszRTMP_URL)
	{
		AfxMessageBox("RTMP URL 不能为空!!!");

		return 0;
	}

	//InitSockets();

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
		sSystemTime.wMinute,
		sSystemTime.wSecond);

	char szH264FileName[_MAX_PATH] = {0};
	sprintf_s(szH264FileName,
		_MAX_PATH, 
		"%s\\rtmp_stream_%4d%02d%02d%02d%02d%02d.264",
		szAppPath,
		sSystemTime.wYear, 
		sSystemTime.wMonth, 
		sSystemTime.wDay,
		sSystemTime.wHour,
		sSystemTime.wMinute,
		sSystemTime.wSecond);

	FILE *fp=fopen(szFileName, "wb");
	if (!fp)
	{
		g_strMsg.Format("Open File Error(%s).", szFileName);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_LogPrintf("Open File Error.\n");
		//CleanupSockets();

		return -1;
	}

	FILE *fpH264 = fopen(szH264FileName, "wb");
	if (!fp)
	{
		g_strMsg.Format("Open File Error(%s).", szH264FileName);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_LogPrintf("Open File Error.\n");
		//CleanupSockets();

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
		g_strMsg.Format("RTMP_URL: %s. RTMP_SetupURL Error. ", szRTMP_URL);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmp);
		//CleanupSockets();

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
		g_strMsg.Format("RTMP_URL: %s. RTMP_Connect Error. ", szRTMP_URL);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmp);

		//CleanupSockets();

		return -1;
	}

	if(!RTMP_ConnectStream(rtmp,0))
	{
		g_strMsg.Format("RTMP_URL: %s. RTMP_ConnectStream Error. ", szRTMP_URL);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		
		//CleanupSockets();

		return -1;
	}

	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(FALSE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_DETAIL_ANALYZE)->EnableWindow(FALSE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_EDIT_AV_NOT_SYNC_THRESHOLD)->EnableWindow(FALSE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_CLEAR_PACKAGE_LIST)->EnableWindow(FALSE);

	g_strMsg.Format("开始录制. 文件名:%s, URL:%s", szFileName, szRTMP_URL);
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

	time_t tmStartTime;
	time_t tmEndTime;

	tmStartTime = time(NULL);

	double dAudioVideoFrameCount = 0;

	//RTMPPacket pkt = { 0 };

	CString strMsgInfo("");
	bool bFirst = true;

	while (!g_bStop)
	{
		if (((CButton *)poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_DETAIL_ANALYZE))->GetCheck() == BST_CHECKED)
		{
			/*
				RTMP_ReadPacket() -> 读取通过Socket接收下来的消息（Message）包，但是不做任何处理
			*/
			BOOL bRet = RTMP_ReadPacket(rtmp, &pkt);
			if (!bRet)
			{
				g_strMsg.Format("RTMP流接收结束(也可能是流存在文件读取失败). RTMP_ReadPacket 0 Bytes");
				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());

				g_bStop = true;

				break;
			}

			/*
				RTMP_ClientPacket() -> 用来处理消息，根据不同的消息，做不同的调用
			*/

			if (RTMPPacket_IsReady(&pkt))
			{
				if (!pkt.m_nBodySize) 
				{
					continue;
				}
	
				if (pkt.m_packetType == RTMP_PACKET_TYPE_VIDEO 
					|| pkt.m_packetType == RTMP_PACKET_TYPE_AUDIO
					|| pkt.m_packetType == RTMP_PACKET_TYPE_INFO)
				{
					if (NULL != pkt.m_body)
					{
						::PostMessage(poRTMPAnalyzerDlg->GetSafeHwnd(), 
							WM_ADD_PACKAGE_TO_LIST, 
							NULL, 
							(LPARAM)&pkt);
					}

					RTMP_Log(RTMP_LOGWARNING, "Received FLV packet before play()! Ignoring.");

					if (pkt.m_packetType == RTMP_PACKET_TYPE_VIDEO)
					{
						/* FLV pkt too small */
						if (pkt.m_nBodySize < 11)
						{
							continue;
						}

						static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

						//int ret = fwrite(pc.m_body + 9, 1, pc.m_nBodySize-9, pf);

						char *data = pkt.m_body;

						if(bFirst) 
						{
							// AVCsequence header

							// SPS
							int spsnum     = data[10]&0x1f;
							int number_sps = 11;
							int count_sps  = 1;

							while (count_sps<=spsnum)
							{
								int spslen = (data[number_sps]&0x000000FF)<<8 |(data[number_sps+1]&0x000000FF);

								number_sps += 2;

								fwrite(start_code, 1, 4, fpH264 );
								fwrite(data+number_sps, 1, spslen, fpH264 );
								fflush(fpH264);
								
								number_sps += spslen;
								count_sps ++;
							}

							// PPS
							int ppsnum     = data[number_sps]&0x1f;
							int number_pps = number_sps+1;
							int count_pps  = 1;

							while (count_pps<=ppsnum)
							{
								int ppslen =(data[number_pps]&0x000000FF)<<8|data[number_pps+1]&0x000000FF;
								number_pps += 2;

								fwrite(start_code, 1, 4, fpH264 );
								fwrite(data+number_pps, 1, ppslen, fpH264 );

								fflush(fpH264);
								
								number_pps += ppslen;
								count_pps ++;
							}

							bFirst = false;
						} 
						else 
						{
							// AVC NALU
							int len = 0;
							int num = 5;

							while(num < pkt.m_nBodySize) 
							{
								len = (data[num]&0x000000FF)<<24|(data[num+1]&0x000000FF)<<16|(data[num+2]&0x000000FF)<<8|data[num+3]&0x000000FF;
								num = num+4;

								fwrite(start_code, 1, 4, fpH264 );
								fwrite(data+num, 1, len, fpH264 );
																
								fflush(fpH264);

								num = num + len;
							}
						}       
					}

					/*fwrite(pkt.m_body, 1, pkt.m_nBodySize, fp);
					nCountBufSize += pkt.m_nBodySize;*/

					dAudioVideoFrameCount++;

					continue;
				}
				/* test */
				/*else
				{
					::PostMessage(poRTMPAnalyzerDlg->GetSafeHwnd(), 
						WM_ADD_PACKAGE_TO_LIST, 
						NULL, 
						(LPARAM)&pkt);
				}*/

				// 处理一下这个数据包，其实里面就是处理服务端发送过来的各种消息等
				RTMP_ClientPacket(rtmp, &pkt);

				// RTMPPacket_Free(&pkt);
			}

			RTMPPacket_Free(&pkt);
		}
		else
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

			if (((CButton *)poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_LOG_READ_INFO))->GetCheck() == BST_CHECKED)
			{
				int nMediaType = (szBuffer[0] & 0xFF);
				char szMediaType[64] = {0};

				if (RTMP_PACKET_TYPE_VIDEO == nMediaType)
				{
					strcpy_s(szMediaType, 64, "Video");
				}
				else if (RTMP_PACKET_TYPE_AUDIO == nMediaType)
				{
					strcpy_s(szMediaType, 64, "Audio");
				}
				else if (RTMP_PACKET_TYPE_INFO == nMediaType)
				{
					strcpy_s(szMediaType, 64, "Info");
				}
				else
				{
					strcpy_s(szMediaType, 64, "Other");
				}

				char szMsgBuffer[1024] = {0};
				sprintf_s(szMsgBuffer, 
					1024, 
					"%.0f: Type: %s, Receive: %d Bytes, Total: %10.2f KB",
					dAudioVideoFrameCount, 
					szMediaType,
					nRead, 
					nCountBufSize*1.0/1024);

				strMsgInfo.Format("%s", szMsgBuffer);

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)strMsgInfo.GetBuffer());
			}
		}

		dAudioVideoFrameCount++;
	}

	tmEndTime = time(NULL);

	if(fp) 
	{
		fclose(fp);
		fp = NULL;
	}

	if (fpH264)
	{
		fclose(fpH264);
		fpH264 = NULL;
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

		//CleanupSockets();

		rtmp = NULL;
	}

	RTMPPacket_Free(&pkt);

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
	poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_DETAIL_ANALYZE)->EnableWindow(TRUE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_EDIT_AV_NOT_SYNC_THRESHOLD)->EnableWindow(TRUE);
	poRTMPAnalyzerDlg->GetDlgItem(IDC_CHECK_CLEAR_PACKAGE_LIST)->EnableWindow(TRUE);

	//AfxMessageBox(szMsgBuffer);

	return 0;
}

void CPxRTMPAnalyzerDlg::OnBnClickedButtonStopRecord()
{
	// TODO: 在此添加控件通知处理程序代码
	g_bStop = true;

	GetDlgItem(IDC_BUTTON_START_RECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP_RECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_DETAIL_ANALYZE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_AV_NOT_SYNC_THRESHOLD)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_CLEAR_PACKAGE_LIST)->EnableWindow(TRUE);
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

	char szAVNotSyncThreshold[_MAX_PATH] = {0};
	GetPrivateProfileString("RTMP", 
		"AVNotSyncThreshold", 
		"300", 
		szAVNotSyncThreshold, 
		sizeof(szAVNotSyncThreshold), 
		g_strConfFile);

	m_strAVNotSyncThreshold.Format("%s", szAVNotSyncThreshold);

	m_lcPackage.ModifyStyle(0, LVS_REPORT);
	m_lcPackage.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES);

	/*m_lcAgentClient.SetBkColor(RGB(96,96,96)); 
	m_lcAgentClient.SetTextBkColor(RGB(96,96,96));
	m_lcAgentClient.SetTextColor(RGB(255,255,255));*/ // 显示字体的颜色

	m_lcPackage.InsertColumn(0,_T("选择"),LVCFMT_RIGHT,80,-1);
	m_lcPackage.InsertColumn(1,_T("头类型/长度"),LVCFMT_LEFT,100,-1);
	m_lcPackage.InsertColumn(2,_T("包类型"),LVCFMT_CENTER,80,-1);
	m_lcPackage.InsertColumn(3,_T("时间戳类型"),LVCFMT_CENTER,80,-1);
	m_lcPackage.InsertColumn(4,_T("时间戳"),LVCFMT_CENTER,100,-1);
	m_lcPackage.InsertColumn(5,_T("时间戳差值"), LVCFMT_CENTER,150,-1);
	m_lcPackage.InsertColumn(6,_T("大小(字节)"), LVCFMT_LEFT,100,-1);

	((CButton*)GetDlgItem(IDC_CHECK_LOG_READ_INFO))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_DETAIL_ANALYZE))->SetCheck(BST_CHECKED);

	UpdateData(FALSE);
}

void CPxRTMPAnalyzerDlg::SaveConfig()
{
	UpdateData();

	WritePrivateProfileString("RTMP", "URL",                m_strRTMP_URL,           g_strConfFile);
	WritePrivateProfileString("RTMP", "AVNotSyncThreshold", m_strAVNotSyncThreshold, g_strConfFile);

	UpdateData(FALSE);
}

LRESULT CPxRTMPAnalyzerDlg::AddPackage2ListCtrl( WPARAM wParam, LPARAM lParam )
{
	//::EnterCriticalSection(&m_csListCtrl); 

	RTMPPacket *psRTMPPackage = (RTMPPacket *)lParam;

	int nCnt = m_lcPackage.GetItemCount();

	CString strDelta2LastVideoTs = "No";

	int maxIndex = m_lcPackage.InsertItem(nCnt, _T(""));//The line_pos is the Index value

	CString strIndex;
	strIndex.Format("%d", nCnt + 1);

	m_lcPackage.SetItemText(maxIndex, 0, strIndex);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 0, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 0, RGB(96,96,96));*/

	CString strHeaderType;

	if (RTMP_PACKET_SIZE_LARGE == psRTMPPackage->m_headerType)
	{
		strHeaderType.Format("0x%2x / 12字节", psRTMPPackage->m_headerType);
	}
	else if (RTMP_PACKET_SIZE_MEDIUM == psRTMPPackage->m_headerType)
	{
		strHeaderType.Format("0x%2x / 8字节", psRTMPPackage->m_headerType);
	}
	else if (RTMP_PACKET_SIZE_SMALL == psRTMPPackage->m_headerType)
	{
		strHeaderType.Format("0x%2x / 4字节", psRTMPPackage->m_headerType);
	}
	else if (RTMP_PACKET_SIZE_MINIMUM == psRTMPPackage->m_headerType)
	{
		strHeaderType.Format("0x%2x / 1字节", psRTMPPackage->m_headerType);
	}
	else
	{
		strHeaderType.Format("0x%2x", psRTMPPackage->m_headerType);
	}
	
	m_lcPackage.SetItemText(maxIndex, 1, strHeaderType);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 1, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 1, RGB(96,96,96));*/

	CString strPackageType("");

	if (RTMP_PACKET_TYPE_AUDIO == psRTMPPackage->m_packetType)
	{
		strPackageType = "Audio";

		/*for (int i = 0; i < 7; i++)
		{
			m_lcAgentClient.SetItemTextColor(maxIndex, i, RGB(255, 255, 255));
			m_lcAgentClient.SetItemBkColor(maxIndex,   i, RGB(51, 161,  201));
		}*/
	}
	else if (RTMP_PACKET_TYPE_VIDEO == psRTMPPackage->m_packetType)
	{
		strPackageType = "##Video##";

		if (NULL != psRTMPPackage->m_body)
		{
			int nVideoType = psRTMPPackage->m_body[0];
			if (nVideoType == 0x17)
			{
				strPackageType = "Video (I帧)";

				for (int i = 0; i < 7; i++)
				{
					m_lcPackage.SetItemTextColor(maxIndex, i, RGB(255, 255, 255));		
					m_lcPackage.SetItemBkColor(maxIndex,   i, RGB(61, 145,  64));
				}
			}
			else if ((nVideoType == 0x27))
			{
				strPackageType = "Video (P帧)";
			}
			else
			{
				strPackageType.Format("Video (0x%x)", nVideoType);
			}

			if (-1 != m_nLastVideoTimestamp)
			{
				strDelta2LastVideoTs.Format("%d", psRTMPPackage->m_nTimeStamp - m_nLastVideoTimestamp);
			}
			else
			{
				strDelta2LastVideoTs = "No";
			}

			m_nLastVideoTimestamp = psRTMPPackage->m_nTimeStamp;
		}
		else 
		{
			strDelta2LastVideoTs = "No. (m_body is NULL)";
			for (int i = 0; i < 7; i++)
			{		
				m_lcPackage.SetItemTextColor(maxIndex, i, RGB(255, 255, 255));		
				m_lcPackage.SetItemBkColor(maxIndex,   i, RGB(255,  153, 18));
				// m_lcPackage.SetItemBkColor(maxIndex,   i, RGB(65,  105, 225));
			}
		}	
	}
	else if (RTMP_PACKET_TYPE_INFO == psRTMPPackage->m_packetType)
	{
		strPackageType = "Info";
	}
	else
	{
		strPackageType.Format("%d", psRTMPPackage->m_packetType);
	}

	m_lcPackage.SetItemText(maxIndex, 2, strPackageType);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 2, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 2, RGB(96,96,96));*/

	CString strTimestampType("");
	if (psRTMPPackage->m_hasAbsTimestamp == 0)
	{
		strTimestampType = "相对";
	}
	else
	{
		strTimestampType = "绝对";
	}

	m_lcPackage.SetItemText(maxIndex, 3, strTimestampType);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 3, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 3, RGB(96,96,96));*/

	CString strTimeStamp("");
	strTimeStamp.Format("%d", psRTMPPackage->m_nTimeStamp);	

	m_lcPackage.SetItemText(maxIndex, 4, strTimeStamp);
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 4, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 4, RGB(96,96,96));*/

	if (RTMP_PACKET_TYPE_AUDIO == psRTMPPackage->m_packetType)
	{
		if (-1 != strDelta2LastVideoTs)
		{
			strDelta2LastVideoTs.Format("%d", psRTMPPackage->m_nTimeStamp - m_nLastVideoTimestamp);
		}
		else
		{
			strDelta2LastVideoTs = "No";
		}
	}

	m_lcPackage.SetItemText(maxIndex, 5, strDelta2LastVideoTs);

	// > 200ms
	int nAVNotSyncThreshold = _ttoi(m_strAVNotSyncThreshold);
	if (_ttoi(strDelta2LastVideoTs) > nAVNotSyncThreshold)
	{	
		for (int i = 0; i < 7; i++)
		{
			m_lcPackage.SetItemTextColor(maxIndex, i, RGB(255,255,255));
			//m_lcAgentClient.SetItemBkColor(maxIndex,i, RGB(210, 105, 30));
			m_lcPackage.SetItemBkColor(maxIndex,   i, RGB(227, 23,  13));
		}	
	}
	
	m_lcPackage.SetItemText(maxIndex, 6, psRTMPPackage->m_nBodySize);

	// > 16K 时
	if (psRTMPPackage->m_nBodySize > 16 * 1024)
	{
		CString strBodySize;
		strBodySize.Format("%d(%.2fKB)", psRTMPPackage->m_nBodySize, psRTMPPackage->m_nBodySize/1024.0);
		m_lcPackage.SetItemText(maxIndex, 6, strBodySize);
	}
	/*m_lcAgentClient.SetItemTextColor(maxIndex, 6, RGB(255,255,255));
	m_lcAgentClient.SetItemBkColor(maxIndex, 6, RGB(0,0,255));*/

	//设置最后一行被选中
	m_lcPackage.SetItemState( m_lcPackage.GetItemCount() - 1, 
		LVIS_ACTIVATING | LVIS_FOCUSED | LVIS_SELECTED,  
		LVIS_SELECTED | LVIS_FOCUSED );
	//滚动到最后一行
	m_lcPackage.Scroll( CSize( 0, 100000 ) );

	//::LeaveCriticalSection(&m_csListCtrl); 

	return 0;
}

// test unit

RTMPPacket g_sTestPackage;

void CPxRTMPAnalyzerDlg::TestAddPackage2ListCtrl()
{
	memset(&g_sTestPackage, 0, sizeof(RTMPPacket));

	g_sTestPackage.m_packetType      = 0x08;
	g_sTestPackage.m_packetType      = RTMP_PACKET_TYPE_AUDIO;
	g_sTestPackage.m_nTimeStamp      = 22222;
	g_sTestPackage.m_hasAbsTimestamp = 0;	/* timestamp absolute or relative? */
	g_sTestPackage.m_nBodySize       = 250;
	
	::PostMessage(GetSafeHwnd(), WM_ADD_PACKAGE_TO_LIST, NULL, (LPARAM)&g_sTestPackage);
}


void CPxRTMPAnalyzerDlg::OnBnClickedButtonRtmpTest()
{
	TestAddPackage2ListCtrl();
}

void CPxRTMPAnalyzerDlg::OnBnClickedCheckClearPackageList()
{
	if (((CButton *)GetDlgItem(IDC_CHECK_CLEAR_PACKAGE_LIST))->GetCheck() == BST_CHECKED)
	{
		m_lcPackage.DeleteAllItems();
	}
}
