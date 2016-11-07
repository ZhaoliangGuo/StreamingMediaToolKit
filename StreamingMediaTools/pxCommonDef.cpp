#include "stdafx.h"
#include "pxCommonDef.h"

// Log
CLogFile g_logFile;
CString  g_strMsg;

CString  g_strConfFile   = ".\\config.ini";
CString  g_strAppTitle   = "StreamingMediaTools.exe";
HWND     g_hAppWnd;         // Ö÷½çÃæ¾ä±ú

CString GetCurTime()
{
	return CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S");
}

bool g_ParseTime(const CString in_kstrTime, vector <int> &out_rvstrTime)
{
	ASSERT(in_kstrTime != "");
	if (in_kstrTime == "")
	{
		return false;
	}

	CString strCmdLine("");
	strCmdLine.Format("%s", in_kstrTime);

	CString strToken;
	CString strSeps = "- :";
	int curPos = 0;

	strToken = strCmdLine.Tokenize(strSeps, curPos);
	while (strToken != _T(""))
	{
		strToken.TrimLeft();
		strToken.TrimRight();
		out_rvstrTime.push_back(_ttoi(strToken));

		strToken = strCmdLine.Tokenize(strSeps, curPos);
	};

	if (out_rvstrTime.size() != 6)
	{
		return false;
	}

	return true;
}

void g_GetLastError()
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER
		|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
		);
	// Process any inserts in lpMsgBuf.
	// Display the string.
	MessageBox(NULL,(LPCTSTR)lpMsgBuf, _T("Message"), MB_OK | MB_ICONINFORMATION );
	// Free the buffer.
	LocalFree( lpMsgBuf );
}

BOOL IsFileExist(LPCTSTR lpFile)
{
	HANDLE hFile = CreateFile( lpFile ,
		GENERIC_READ ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL , 
		OPEN_EXISTING ,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED ,
		NULL ) ;

	if( hFile == INVALID_HANDLE_VALUE )
	{
		DWORD dwError = ::GetLastError() ;

		hFile = NULL ;
		return FALSE;
	}

	CloseHandle(hFile);
	hFile = NULL ;

	return TRUE;
}

int DeleteRunningProcess(LPCTSTR in_lpstrClientName)
{
	HANDLE Snapshot;  
	Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); 
	if( Snapshot == INVALID_HANDLE_VALUE )
	{
		g_logFile.WriteLogInfo("DeleteRunningProcess::CreateToolhelp32Snapshot Error");
		return -1;
	}

	PROCESSENTRY32 ProcessListStr;   
	ProcessListStr.dwSize = sizeof(PROCESSENTRY32);   

	BOOL return_value;   
	return_value = Process32First(Snapshot, &ProcessListStr);   

	HANDLE hProcess;
	while(return_value)   
	{
		//if (strcmp( ProcessListStr.szExeFile, in_lpstrClientName) == 0)  
		if (!stricmp(ProcessListStr.szExeFile, in_lpstrClientName))
		{   
			hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, ProcessListStr.th32ProcessID );

			if (NULL != hProcess)
			{
				if (!TerminateProcess(hProcess,0))
				{
					g_logFile.WriteLogInfo("DeleteRunningProcess:: IRS Close fail.");
				}

				break;
			}
		}

		return_value = Process32Next(Snapshot,&ProcessListStr);    
	}     

	CloseHandle(Snapshot);

	return 0;
}