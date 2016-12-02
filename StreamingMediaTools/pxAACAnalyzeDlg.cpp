// pxAACAnalyzeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StreamingMediaTools.h"
#include "pxAACAnalyzeDlg.h"
#include "afxdialogex.h"
#include "pxAnalyzeAudioSpecificConfigDlg.h"



// CPxAACAnalyzeDlg 对话框

IMPLEMENT_DYNAMIC(CPxAACAnalyzeDlg, CDialogEx)

CPxAACAnalyzeDlg::CPxAACAnalyzeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPxAACAnalyzeDlg::IDD, pParent)
{

}

CPxAACAnalyzeDlg::~CPxAACAnalyzeDlg()
{
}

void CPxAACAnalyzeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPxAACAnalyzeDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_ANALYZE_AudioSpecificConfig, &CPxAACAnalyzeDlg::OnBnClickedButtonAnalyzeAudiospecificconfig)
END_MESSAGE_MAP()


// CPxAACAnalyzeDlg 消息处理程序


void CPxAACAnalyzeDlg::OnBnClickedButtonAnalyzeAudiospecificconfig()
{
	// TODO: 在此添加控件通知处理程序代码
	CPxAnalyzeAudioSpecificConfigDlg oAnalyzeAudioSpecificConfigDlg;
	oAnalyzeAudioSpecificConfigDlg.DoModal();
}
