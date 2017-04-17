
// StreamingMediaToolsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StreamingMediaTools.h"
#include "StreamingMediaToolsDlg.h"
#include "afxdialogex.h"
#include "pxCommonDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CStreamingMediaToolsDlg 对话框
CStreamingMediaToolsDlg::CStreamingMediaToolsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CStreamingMediaToolsDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);	

	ZeroMemory(m_szMsgBuffer, MESSAGE_BUFFER_SIZE);
}

void CStreamingMediaToolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAINTAB,   m_TabCtrl);
	DDX_Control(pDX, IDC_LIST_INFO, m_listboxLogInfo);
}

BEGIN_MESSAGE_MAP(CStreamingMediaToolsDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON_TEST,          &CStreamingMediaToolsDlg::OnBnClickedButtonTest)
	ON_NOTIFY(NM_CLICK, IDC_MAINTAB,        &CStreamingMediaToolsDlg::OnNMClickTab1)
	ON_MESSAGE(WM_ADD_LOG_TO_LIST,          &CStreamingMediaToolsDlg::AddLog2List)
	ON_BN_CLICKED(IDC_CHECK_CLEAR_LOG_LIST, &CStreamingMediaToolsDlg::OnClearLogList)
	ON_COMMAND(ID_RIGHT_MENU_MINIMIZE, &CStreamingMediaToolsDlg::OnRightMenuMinimize)
	ON_COMMAND(ID_RIGHT_MENU_OPEN_LOG_DIR, &CStreamingMediaToolsDlg::OnRightMenuOpenLogDir)
	ON_COMMAND(ID_RIGHT_MENU_OPEN_TODAY_LOG, &CStreamingMediaToolsDlg::OnRightMenuOpenTodayLog)
	ON_COMMAND(ID_RIGHT_MENU_OPEN_INSTALL_DIR, &CStreamingMediaToolsDlg::OnRightMenuOpenInstallDir)
	ON_COMMAND(ID_RIGHT_MENU_EXIT, &CStreamingMediaToolsDlg::OnRightMenuExit)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_RIGHT_MENU_OPEN_RECORD_DIR, &CStreamingMediaToolsDlg::OnRightMenuOpenRecordDir)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CStreamingMediaToolsDlg 消息处理程序

BOOL CStreamingMediaToolsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 获取主界面的句柄
	CWnd *pWnd  = AfxGetMainWnd();
	pWnd->GetWindowText(g_strAppTitle);
	g_hAppWnd = ::FindWindow(NULL, g_strAppTitle);

	Init();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CStreamingMediaToolsDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CStreamingMediaToolsDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CStreamingMediaToolsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CStreamingMediaToolsDlg::OnNMClickTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	int index = m_TabCtrl.GetCurSel();

	switch(index)
	{
	case 0:
		m_pRTMPAnalyzerDlg->ShowWindow(SW_SHOW);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pAACAnalyzeDlg->ShowWindow(SW_HIDE);

		WritePrivateProfileString("DefaultPage", "Name",  "RTMP", g_strConfFile);

		break;

	case 1:
		m_pRTMPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_SHOW);
		m_pAACAnalyzeDlg->ShowWindow(SW_HIDE);

		WritePrivateProfileString("DefaultPage", "Name",  "RTSP", g_strConfFile);

		break;

	case 2:
		m_pRTMPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pAACAnalyzeDlg->ShowWindow(SW_SHOW);

		WritePrivateProfileString("DefaultPage", "Name",  "AAC", g_strConfFile);

		break;

		/*default:
		m_pRTMPAnalyzerDlg->ShowWindow(SW_SHOW);
		m_pRTMPAnalyzerDlg->ShowWindow(SW_HIDE);

		break;*/
	}

	*pResult = 0;
}

void CStreamingMediaToolsDlg::Init()
{
	m_TabCtrl.InsertItem(0, _T("RTMP"));
	m_TabCtrl.InsertItem(1, _T("RTSP"));
	m_TabCtrl.InsertItem(2, _T("AAC"));

	// 创建RTMP 分析界面
	m_pRTMPAnalyzerDlg = new CPxRTMPAnalyzerDlg;
	int bret = m_pRTMPAnalyzerDlg->Create(IDD_PAGE_RTMP, GetDlgItem(IDC_MAINTAB));
	if (FALSE == bret)
	{
		AfxMessageBox(_T("创建RTMP 分析对话框失败"));

		return ;
	}

	// 创建RTSP 分析界面
	m_pRTSPAnalyzerDlg = new CPxRTSPAnalyzerDlg;
	bret = m_pRTSPAnalyzerDlg->Create(IDD_PAGE_RTSP, GetDlgItem(IDC_MAINTAB));
	if (FALSE == bret)
	{
		AfxMessageBox(_T("创建RTSP 分析对话框失败"));

		return ;
	}

	m_pAACAnalyzeDlg = new CPxAACAnalyzeDlg;
	bret = m_pAACAnalyzeDlg->Create(IDD_OLE_PROPPAGE_LARGE_AAC, GetDlgItem(IDC_MAINTAB));
	if (FALSE == bret)
	{
		AfxMessageBox(_T("创建AAC 分析对话框失败"));

		return ;
	}

	CRect rtTabClient;
	m_TabCtrl.GetClientRect(&rtTabClient);
	rtTabClient.top += 30;
	rtTabClient.bottom -= 30;
	rtTabClient.left += 1;
	rtTabClient.right -= 2;

	m_pRTMPAnalyzerDlg->MoveWindow(&rtTabClient,FALSE);
	m_pRTSPAnalyzerDlg->MoveWindow(&rtTabClient,FALSE);
	m_pAACAnalyzeDlg->MoveWindow(&rtTabClient,FALSE);

	// 显示默认界面
	char szDefaultPageName[_MAX_PATH] = {0};
	GetPrivateProfileString("DefaultPage", 
		"Name", 
		"RTMP", 
		szDefaultPageName, 
		sizeof(szDefaultPageName), 
		g_strConfFile);

	if (0 == strcmp(szDefaultPageName, "RTMP"))
	{
		m_TabCtrl.SetCurSel(0);
		m_pRTMPAnalyzerDlg->ShowWindow(SW_SHOW);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pAACAnalyzeDlg->ShowWindow(SW_HIDE);
	}
	else if (0 == strcmp(szDefaultPageName, "RTSP"))
	{
		m_TabCtrl.SetCurSel(1);
		m_pRTMPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_SHOW);
		m_pAACAnalyzeDlg->ShowWindow(SW_HIDE);
	}
	else if (0 == strcmp(szDefaultPageName, "AAC"))
	{
		m_TabCtrl.SetCurSel(2);
		m_pRTMPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pRTSPAnalyzerDlg->ShowWindow(SW_HIDE);
		m_pAACAnalyzeDlg->ShowWindow(SW_SHOW);
	}

	m_listboxLogInfo.SetHorizontalExtent(5000);

	::InitializeCriticalSection(&m_csListBox);   //初始化临界区
}

void CStreamingMediaToolsDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_pRTSPAnalyzerDlg)
	{
		m_pRTSPAnalyzerDlg->DestroyWindow();
		delete m_pRTSPAnalyzerDlg;
		m_pRTSPAnalyzerDlg = NULL;
	}

	if (m_pRTMPAnalyzerDlg)
	{
		m_pRTMPAnalyzerDlg->DestroyWindow();
		delete m_pRTMPAnalyzerDlg;
		m_pRTMPAnalyzerDlg = NULL;
	}

	if (m_pAACAnalyzeDlg)
	{
		m_pAACAnalyzeDlg->DestroyWindow();
		delete m_pAACAnalyzeDlg;
		m_pAACAnalyzeDlg = NULL;
	}

	::DeleteCriticalSection(&m_csListBox);    //释放里临界区
}

LRESULT CStreamingMediaToolsDlg::AddLog2List( WPARAM wParam, LPARAM lParam )
{
	::EnterCriticalSection(&m_csListBox); 

	char *pszMsg = (char *)lParam;

	if (NULL == pszMsg)
	{
		::LeaveCriticalSection(&m_csListBox); 

		return -1;
	}

	CString strMsg;

	//int nLen = strlen(pszMsg);

	ZeroMemory(m_szMsgBuffer, MESSAGE_BUFFER_SIZE);
	strcpy_s(m_szMsgBuffer, MESSAGE_BUFFER_SIZE, pszMsg);

	try
	{
		strMsg.Format("%s", m_szMsgBuffer);
	}
	catch (CException* e)
	{
		::LeaveCriticalSection(&m_csListBox); 

		return -1;
	}

	/*if (strMsg == "")
	{
		::LeaveCriticalSection(&m_csListBox); 

		return -1;
	}*/

	CString strGmt = "[" + GetCurTime();
	strGmt += "] ";
	CString strLine = strGmt + strMsg;

	m_listboxLogInfo.AddString(strLine);

	int nCount = m_listboxLogInfo.GetCount();
	if (nCount > 0)
	{
		m_listboxLogInfo.SetCurSel(nCount - 1); 
	}

	g_logFile.WriteLogInfo(strMsg);

	::LeaveCriticalSection(&m_csListBox); 

	return 0;
}

// 清空日志列表
void CStreamingMediaToolsDlg::OnClearLogList()
{
	//UpdateData();
	if (((CButton *)GetDlgItem(IDC_CHECK_CLEAR_LOG_LIST))->GetCheck() == BST_CHECKED)
	{
		m_listboxLogInfo.ResetContent();
	}
	//UpdateData(FALSE);
}


void CStreamingMediaToolsDlg::OnBnClickedButtonTest()
{
	// 测试添加日志到listbox
	TestAddLog();
}

// test unit - TestAddLog
void CStreamingMediaToolsDlg::TestAddLog()
{
	g_strMsg = "#1 CStreamingMediaToolsDlg::TestAddLog() \
#2 CStreamingMediaToolsDlg::TestAddLog() \
#3 CStreamingMediaToolsDlg::TestAddLog() \
#4 CStreamingMediaToolsDlg::TestAddLog()";

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMsg.GetBuffer());
}


void CStreamingMediaToolsDlg::OnRightMenuMinimize()
{
	AfxGetMainWnd()->ShowWindow(SW_MINIMIZE);
}


void CStreamingMediaToolsDlg::OnRightMenuOpenLogDir()
{
	char szAppPath[MAX_PATH] = {0};
	/*GetAppPath(szAppPath);
	strcat(szAppPath, ".\\LogFile\\");*/

	strcpy(szAppPath, ".\\LogFile\\");

	ShellExecuteA(NULL, "open", szAppPath, NULL, NULL, SW_NORMAL);
}


void CStreamingMediaToolsDlg::OnRightMenuOpenTodayLog()
{
	char szAppPath[MAX_PATH] = {0};
	GetAppPath(szAppPath);

	//读取时间
	SYSTEMTIME   time;
	GetLocalTime(&time);

	char szLogFilePath[_MAX_PATH]      = {0};

	/*sprintf_s(strLogFilePath,
		_MAX_PATH, 
		".\\LogFile\\LogFile_%4d-%02d-%02d.txt", 
		szAppPath,
		time.wYear, 
		time.wMonth , 
		time.wDay);*/

	sprintf_s(szLogFilePath,
		_MAX_PATH, 
		".\\LogFile\\LogFile_%4d-%02d-%02d.txt", 
		time.wYear, 
		time.wMonth , 
		time.wDay);

	CString strLogFileName;
	//strLogFileName.Format("%S", szLogFilePath);
	strLogFileName.Format("%s", szLogFilePath);

	if (IsFileExist(strLogFileName))
	{
		ShellExecute(NULL, "open", strLogFileName, NULL, NULL, SW_SHOWNORMAL);
	}
	else
	{
		AfxMessageBox("暂时未生成日志文件， 请确认.");
	}
}

void CStreamingMediaToolsDlg::OnRightMenuOpenInstallDir()
{
	char szAppPath[MAX_PATH] = {0};
	GetAppPath(szAppPath);
	ShellExecuteA(NULL, "open", szAppPath, NULL, NULL, SW_NORMAL);
}


void CStreamingMediaToolsDlg::OnRightMenuExit()
{
	if (IDYES == ::MessageBox(NULL, "您确认要退出流媒体分析工具吗?", "退出确认", MB_YESNO))
	{
		CDialog::OnOK();	
	}
}

void CStreamingMediaToolsDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MENU_MAIN_RIGHT); //载入菜单

	CMenu *pPopup = NULL;
	pPopup=popMenu.GetSubMenu(0); //获得子菜单指针

	pPopup->EnableMenuItem(ID_RIGHT_MENU_MINIMIZE,         MF_BYCOMMAND|MF_ENABLED); 
	pPopup->EnableMenuItem(ID_RIGHT_MENU_OPEN_INSTALL_DIR, MF_BYCOMMAND|MF_ENABLED); 
	pPopup->EnableMenuItem(ID_RIGHT_MENU_OPEN_LOG_DIR,     MF_BYCOMMAND|MF_ENABLED); 
	pPopup->EnableMenuItem(ID_RIGHT_MENU_OPEN_TODAY_LOG,   MF_BYCOMMAND|MF_ENABLED); 
	pPopup->EnableMenuItem(ID_RIGHT_MENU_EXIT,             MF_BYCOMMAND|MF_ENABLED); 

	pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, this);
	pPopup->Detach();
	popMenu.DestroyMenu();
}

void CStreamingMediaToolsDlg::OnRightMenuOpenRecordDir()
{
	char szAppPath[MAX_PATH] = {0};
	GetAppPath(szAppPath);
	strcat(szAppPath, "\\RecordFile\\");

	ShellExecuteA(NULL, "open", szAppPath, NULL, NULL, SW_NORMAL);
}


void CStreamingMediaToolsDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
}
