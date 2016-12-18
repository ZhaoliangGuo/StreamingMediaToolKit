#ifndef _PX_COMMOND_DEF_H
#define _PX_COMMOND_DEF_H

#include <map>
#include <vector>
using namespace std;
#include <afxdisp.h>
#include "tlhelp32.h"
#include <afxole.h>

// Log
#include ".\\CLogFile\\pxLogFile.h"
#include ".\\CReportCtrl\\pxReportCtrl.h"

// AAC 
#include ".\\Faad\\pxFaadMP4.h"
#include ".\\faad\\pxFAADCommonDef.h"

// RTSP
#include "liveMedia.hh"
#include "Groupsock.hh"
#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"

extern CLogFile g_logFile;
extern CString  g_strMsg;

extern CString  g_strConfFile;
extern CString  g_strAppTitle;
extern HWND     g_hAppWnd;         // 主界面句柄

extern CString  g_strRecordFilePath;

static HBRUSH static_brush_gray = ::CreateSolidBrush(RGB(96,96,96));

const int WM_ADD_LOG_TO_LIST                    = WM_USER + 1001; // 添加日志到运行日志

static HBRUSH brush_red   = ::CreateSolidBrush(RGB(255,0,0));
static HBRUSH brush_green = ::CreateSolidBrush(RGB(0,255,0));

enum STATIC_BKCOLOR
{
	NULL_COLOR,
	RED_COLOR,
	GREEN_COLOR,
};

extern CString GetCurTime();
extern bool g_ParseTime(const CString in_kstrTime, vector <int> &out_rvstrTime);
extern void g_GetLastError();
extern BOOL IsFileExist(LPCTSTR lpFile);
extern CString GetServPort();
extern int DeleteRunningProcess(LPCTSTR in_lpstrClientName);

extern void GetAppPath(char *out_pszAppPath);

extern BOOL pxIsDirExists(LPCTSTR pszPath);
extern BOOL pxCreateDir(LPCTSTR pszPath);

#endif

