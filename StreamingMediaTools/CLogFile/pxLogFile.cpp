#include "stdafx.h"
#include "pxLogFile.h"
/*********************************************************************
Author      : Zhaoliang Guo      
Date        : May. 20th 2014
Description : 实现写日志功能    
***********************************************************************/

// 构造函数(初始化变量和创建日志目录)
CLogFile::CLogFile()
{
	::InitializeCriticalSection(&m_cs);   //初始化临界区
	ZeroMemory(m_szCurTime, LOG_BUF_SIZE);
	ZeroMemory(m_szLogFileName, _MAX_PATH);

	// 获取程序的当前运行路径
	TCHAR szBuf[MAX_PATH];
	ZeroMemory(szBuf,MAX_PATH);
	GetCurrentDirectory(MAX_PATH, szBuf);
	m_strLogPath = szBuf;

	if (m_strLogPath.Right(1) != "\\")
	{
		m_strLogPath += "\\LogFile\\";
	}
	else
	{
		m_strLogPath += "LogFile\\";
	}

	// 创建日志目录
	DWORD dwAttr = GetFileAttributes(m_strLogPath);
	if((dwAttr == -1) || (dwAttr&FILE_ATTRIBUTE_DIRECTORY) == 0)//目录不存在
	{
		CreateDir(m_strLogPath);
	}
}

// 析构函数(清理工作)
CLogFile::~CLogFile()
{
	::DeleteCriticalSection(&m_cs);    //释放里临界区
	ZeroMemory(m_szCurTime, LOG_BUF_SIZE);
	ZeroMemory(m_szLogFileName, _MAX_PATH);
}

// 获取当前时间
void CLogFile::GetCurTime(void)
{
	ZeroMemory(m_szCurTime, LOG_BUF_SIZE);
	time_t now = time(0);
	tm* localtm = localtime(&now);

	sprintf(m_szCurTime,"%4d-%02d-%02d %02d:%02d:%02d",
			localtm->tm_year + 1900,
			localtm->tm_mon  + 1,
			localtm->tm_mday,
			localtm->tm_hour,
			localtm->tm_min,
			localtm->tm_sec);
}

// 写CString类型的数据到日志
void CLogFile::WriteLog( CString in_strText, const char *in_szFile, const unsigned long in_ulLine)
{
#if WRITE_LOG_STATE
	if (in_strText.IsEmpty())
		return ;

	::EnterCriticalSection(&m_cs); 
	ZeroMemory(m_szLogFileName, _MAX_PATH);
	time_t now = time(0);
	tm* localtm = localtime(&now);
	sprintf(m_szLogFileName, ".\\LogFile\\LogFile_%4d-%02d-%02d.txt",
		localtm->tm_year + 1900,
		localtm->tm_mon  + 1,
		localtm->tm_mday);

	GetCurTime();
	FILE *fp = NULL;
	fp = fopen(m_szLogFileName, "at");
	fprintf(fp,"[");
	fprintf(fp,m_szCurTime);
	fprintf(fp,"] ");
	
#ifdef _UNICODE
	fprintf(fp, "%S ", in_strText.GetBuffer(in_strText.GetLength() * 2));
#else
	fprintf(fp, "%s ", in_strText.GetBuffer(in_strText.GetLength()));
#endif // End of #ifdef _UNICODE
	fprintf(fp,"{%s(%d)}", in_szFile, in_ulLine);
	fprintf(fp,"\n");
	fflush(fp);
	fclose(fp);
	::LeaveCriticalSection(&m_cs);  
#endif // End of #if WRITE_LOG_STATE
}

#if 1
void CLogFile::WriteLogInfo(const char* kszFormat, ...)
{
#if WRITE_LOG_STATE
	::EnterCriticalSection(&m_cs);   
	try      
	{
		va_list argptr;          //分析字符串的格式
		va_start(argptr, kszFormat);
		_vsnprintf(m_szBuf, MAX_BUF_LENGTH, kszFormat, argptr);
		va_end(argptr);
	}
	catch (...)
	{
		m_szBuf[0] = 0;
	}

	ZeroMemory(m_szLogFileName, _MAX_PATH);
	time_t now = time(0);
	tm* localtm = localtime(&now);
	sprintf(m_szLogFileName, ".\\LogFile\\LogFile_%4d-%02d-%02d.txt",
		localtm->tm_year + 1900,
		localtm->tm_mon  + 1,
		localtm->tm_mday);

	GetCurTime();
	FILE *fp = fopen(m_szLogFileName, "at");
	fprintf(fp,"[");
	fprintf(fp,m_szCurTime);
	fprintf(fp,"] ");
	fprintf(fp, "%s ", m_szBuf);
	//fprintf(fp,"{%s(%d)}", __FILE__, __LINE__);
	fprintf(fp,"\n");
	fflush(fp);
	fclose(fp);

	::LeaveCriticalSection(&m_cs);  

#endif // End of #if WRITE_LOG_STATE
}

BOOL CreateDir(LPCTSTR pszPath)
{
	CString str = pszPath;

	// Add a '\' to the end of the path if there is not one
	int k = str.GetLength(); 
	if (k==0 || str[k-1]!=_T('\\')) str += _T('\\');

	// We must create directory level by level, any better Win API available ?
	bool bUNCPath = false;
	CString strUNC = str.Left(2);
	if (strUNC.CompareNoCase(_T("\\\\")) == 0)
	{
		bUNCPath = true;
	}
	int i = -1;
	if (bUNCPath)
	{
		i = str.Find(_T("\\"),2);
		i = str.Find(_T("\\"),i +1);
	}
	while ((i = str.Find(_T('\\'), i+1)) >= 0)
	{
		CString s = str.Left(i+1);
		if (!IsDirExists(s) && !CreateDirectory(s, NULL))
			return FALSE;
	}

	return TRUE;
}

BOOL IsDirExists(LPCTSTR pszPath)
{
	DWORD dwAttributes = ::GetFileAttributes(pszPath);

	if (dwAttributes == INVALID_FILE_ATTRIBUTES) {
		DWORD dwErr = ::GetLastError();

		if (dwErr == ERROR_ACCESS_DENIED)
			return TRUE;

		return FALSE;
	}

	return (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
}

#endif 



