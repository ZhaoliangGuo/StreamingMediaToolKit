#include "pxAACFileSink.h"
//#include "IPxRTSPStreamCallback.h"
#include ".\\Faad\\pxFaadMP4.h"
#include ".\\Faad\\pxFAADCommonDef.h"
#include "pxAACBase.h"
#include <iostream>
#include <vector>
using namespace std;

#include "atlstr.h"
#include "pxRTSPCommonDef.h"

CString g_strAACMsg;
extern HWND g_hAppWnd;         // 主界面句柄
extern int WM_ADD_LOG_TO_LIST;

#define RTSP_PACKET_BUF_SIZE (1024)
//RTSPPacket g_aRTSPPacket[RTSP_PACKET_BUF_SIZE];

vector<RTSPPacket> g_vsRTSPAACPacket(RTSP_PACKET_BUF_SIZE);
static int s_nAACPackageIndex = 0;
extern HWND  g_hWndRTSP;

CPxAACFileSink::CPxAACFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
	char const* perFrameFileNamePrefix, char* in_pszSDP)
	: FileSink(env, fid, bufferSize, perFrameFileNamePrefix)
{
	m_strSDP = in_pszSDP;
	m_pAACFrame = NULL;
	m_AudioProfile = 1;
	m_samplerateidx = 5;
	m_channel = 2;
	ParseSdpConfig();

	ZeroMemory(m_szMsgBuffer, MSG_BUF_SIZE);

	::InitializeCriticalSection(&m_csGettingFrame);   //初始化临界区
}
CPxAACFileSink::~CPxAACFileSink()
{
	if (m_pAACFrame)
	{
		delete [] m_pAACFrame;
		m_pAACFrame = NULL;
	}

	ZeroMemory(m_szMsgBuffer, MSG_BUF_SIZE);

	::DeleteCriticalSection(&m_csGettingFrame);    //释放里临界区
}

CPxAACFileSink* CPxAACFileSink::createNew(UsageEnvironment& env, char const* fileName, unsigned bufferSize /*= 10000*/, Boolean oneFilePerFrame /*= False*/, char* in_pszSDP)
{
	 return new CPxAACFileSink(env, NULL, bufferSize, NULL,  in_pszSDP);
}

Boolean CPxAACFileSink::sourceIsCompatibleWithUs(MediaSource& source)
{
	return true;
}

void CPxAACFileSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime)
{
	/*if (NULL == m_pAACFrame)
	{
		m_pAACFrame = new(std::nothrow)char[400000];
	}*/

	::EnterCriticalSection(&m_csGettingFrame);

	UsageEnvironment *env = &envir();

	INT64 nTimeStamp = timeGetTime();
	nTimeStamp = ((INT64)presentationTime.tv_sec)*1000 + presentationTime.tv_usec/1000;

	/*g_strAACMsg.Format("CPxAACFileSink nTimeStamp :%I64d,frameSize:%u, tv_sec:%ld, tv_usec:%ld",
		nTimeStamp,
		frameSize,
		presentationTime.tv_sec,
		presentationTime.tv_usec);

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strAACMsg.GetBuffer());*/

	//strcpy(poRtspStream->m_tagLiveStreamFrameInfo.AACConfig, m_config);

	if (presentationTime.tv_usec == fPrevPresentationTime.tv_usec &&
		presentationTime.tv_sec == fPrevPresentationTime.tv_sec) 
	{
		
	}
	else
	{
		if ((RTSP_PACKET_BUF_SIZE - 1) == s_nAACPackageIndex)
		{
			s_nAACPackageIndex = 0;
		}

		//sprintf(g_vsRTSPAACPacket[s_nAACPackageIndex].szId, "%08d", s_nFrameCnt);
		g_vsRTSPAACPacket[s_nAACPackageIndex].eMediaType  = kePxMediaType_Audio;
		//g_vsRTSPAACPacket[s_nAACPackageIndex].eNALUType   = (EPxNALUType)nFrameType;
		g_vsRTSPAACPacket[s_nAACPackageIndex].nTimeStamp  = nTimeStamp;
		g_vsRTSPAACPacket[s_nAACPackageIndex].uiFrameSize = frameSize;

		::PostMessage(g_hWndRTSP, 
			WM_ADD_RTSP_PACKAGE_TO_LIST, 
			NULL, 
			(LPARAM)&g_vsRTSPAACPacket[s_nAACPackageIndex]);

		if ((0xff == fBuffer[0]) && (0xff==(fBuffer[1]|0x0f)) )
		{
			// This frame has adts header already		
		}
		else
		{
			//This frame has not adts header ,add header
			/*adts_write_frame_header((uint8_t*)m_pAACFrame,frameSize,0, m_AudioProfile-1, m_samplerateidx, m_channel);

			memcpy(m_pAACFrame + 7,fBuffer,frameSize);
			poRtspStream->m_tagLiveStreamFrameInfo.pbuf    = (char *)m_pAACFrame;
			poRtspStream->m_tagLiveStreamFrameInfo.ibuflen = frameSize + 7;

			HRESULT hr = pJLiveStream->ReceiveData(&(poRtspStream->m_tagLiveStreamFrameInfo));
			NSD_SAFE_REPORT_ERROR(keLogPkgPxDeviceLiveSource, keLogPkgPxDeviceLiveSourceFuncGeneral, hr,
				"CAACFileSink::afterGettingFrame :: ReceiveData FAIL", true);*/
		}
	}

	fPrevPresentationTime.tv_sec  = presentationTime.tv_sec;
	fPrevPresentationTime.tv_usec = presentationTime.tv_usec;

	continuePlaying();

	::LeaveCriticalSection(&m_csGettingFrame);
}

void CPxAACFileSink::ParseSdpConfig()
{
	int ipos = m_strSDP.find("config=");
	ipos += strlen("config=");

	m_strconfig = m_strSDP.substr(ipos, 4);

	string strtmp = m_strconfig.substr(0,2);
	m_config[0] = strtol(strtmp.c_str(), NULL, 16); 
	strtmp = m_strconfig.substr(2,2);
	m_config[1] = strtol(strtmp.c_str(), NULL, 16);

	mp4AudioSpecificConfig mp4asc;
	char chret = 0;
	chret = pxAACDecodeAudioSpecificConfig((unsigned char*)m_config,2,&mp4asc);
	if (chret < 0 )
	{
		return ;
	}

	m_AudioProfile = mp4asc.objectTypeIndex;
	m_samplerateidx = mp4asc.samplingFrequencyIndex;
	m_channel = mp4asc.channelsConfiguration;

	return ;
}
