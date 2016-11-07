/*********************************************************************
Author      : Zhaoliang Guo      
Date        : May. 20th 2014
Description : 定义写日志的类.    
***********************************************************************/
#ifndef _PX_LOG_H
#define _PX_LOG_H

#include <windows.h>
#include <atltime.h>
#include <sys/timeb.h>
#include <atlstr.h>
#include <iostream>
#include <stdarg.h>
using namespace std;

#define LOG_BUF_SIZE 32
#define WRITE_LOG_STATE 1// 是否启用写日志 1：写日志, 0：不写日志

BOOL CreateDir(LPCTSTR pszPath);
BOOL IsDirExists(LPCTSTR pszPath);


class CLogFile
{
public:
	CLogFile();
	~CLogFile();

public:
	void WriteLogInfo(const char* kszFormat, ...); // 可变参数形式写日志
	void WriteLog(CString in_strText, const char *in_szFile, const unsigned long in_ulLine);         // 将CString类型数据写入日志
	void GetCurTime();                         // 获取当前时间

private:
	enum { MAX_BUF_LENGTH = 1024 };     // 缓冲区
	char m_szBuf[MAX_BUF_LENGTH];       // 用于保存可变参数
	CRITICAL_SECTION  m_cs;             // 临界区
	CString m_strLogPath;               // 日志目录
	char    m_szLogFileName[_MAX_PATH]; // 当前日志文件名字
	char    m_szCurTime[LOG_BUF_SIZE];      // 当前时间
};


#endif