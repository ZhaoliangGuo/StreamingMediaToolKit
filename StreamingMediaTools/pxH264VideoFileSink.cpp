/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2016 Live Networks, Inc.  All rights reserved.
// H.264 Video File sinks
// Implementation

#include "pxH264VideoFileSink.h"
#include "OutputFile.hh"
#include "atlstr.h"
#include <vector>
using namespace std;

CString     g_strMessage;
extern HWND g_hAppWnd;         // 主界面句柄
int WM_ADD_LOG_TO_LIST          = WM_USER + 1001;

#include "pxRTSPCommonDef.h"
#include "H264VideoRTPSource.hh" // for "parseSPropParameterSets()"

//
#define RTSP_PACKET_BUF_SIZE (1024)
//RTSPPacket g_aRTSPPacket[RTSP_PACKET_BUF_SIZE];

vector<RTSPPacket> g_vsRTSPPacket(RTSP_PACKET_BUF_SIZE);
static int s_nPackageIndex = 0;
static int s_nFrameCnt     = 0;

extern HWND  g_hWndRTSP;

static FILE* fid = NULL;

////////// H264VideoFileSink //////////

CPxH264VideoFileSink
	::CPxH264VideoFileSink(UsageEnvironment& env, 
	FILE* fid,
	char const* sPropParameterSetsStr,
	unsigned bufferSize, 
	char const* perFrameFileNamePrefix)
	: 
H264or5VideoFileSink(env, 
	fid, 
	bufferSize, 
	perFrameFileNamePrefix,
	sPropParameterSetsStr, 
	NULL, 
	NULL) 
{
	::InitializeCriticalSection(&m_csGettingFrame);   //初始化临界区

	fid = NULL;

	m_bHasWriteSPS = false;
	m_bHasWritePPS = false;
}

CPxH264VideoFileSink::~CPxH264VideoFileSink() 
{
	::DeleteCriticalSection(&m_csGettingFrame);    //释放里临界区

	CloseOutputFile(fid);
}

CPxH264VideoFileSink* CPxH264VideoFileSink::createNew(UsageEnvironment& env, 
	char const* fileName,
	char const* sPropParameterSetsStr,
	unsigned bufferSize, 
	Boolean oneFilePerFrame) 
{
	do 
	{
		char const* perFrameFileNamePrefix;

		if (oneFilePerFrame) 
		{
			// Create the fid for each frame
			fid = NULL;
			perFrameFileNamePrefix = fileName;
		} 
		else 
		{
			// Normal case: create the fid once
			fid = OpenOutputFile(env, fileName);
			if (fid == NULL) 
			{
				break;
			}

			perFrameFileNamePrefix = NULL;
		}

		return new CPxH264VideoFileSink(env, fid, sPropParameterSetsStr, bufferSize, perFrameFileNamePrefix);
	} while (0);

	return NULL;
}

CString g_strH264Msg;
unsigned int n_FrameLength = 0;

INT64 g_nTestLastTimeStamp = 0;

void CPxH264VideoFileSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime)
{
	unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};

	/*FILE *fp = fopen("C:\\rtsp_test_time.txt", "at");
	char szMsgBuffer[1024 * 8] = {0};*/

	INT64 nTestTimeStamp = ((INT64)presentationTime.tv_sec) * 1000 + presentationTime.tv_usec / 1000;

	/*if (g_nTestLastTimeStamp > nTestTimeStamp)
	{
		sprintf_s(szMsgBuffer, _MAX_PATH, "\nCPxH264VideoFileSink::afterGettingFrame!!!!!!!!!!!! \
										  g_nTestLastTimeStamp:I64d, nTestTimeStamp:%I64d \n\n", 
										  g_nTestLastTimeStamp,
										  nTestTimeStamp);

		fprintf(fp, "%s", szMsgBuffer);

		fflush(fp);
	}*/
	

	g_nTestLastTimeStamp = nTestTimeStamp;

	/*if (fp)
	{
		sprintf_s(szMsgBuffer, _MAX_PATH, "CPxH264VideoFileSink::afterGettingFrame \
presentationTime.tv_sec:%lu, presentationTime.tv_usec:%lu, nTestTimeStamp:%I64d \n", 
			presentationTime.tv_sec, 
			presentationTime.tv_usec,
			nTestTimeStamp);

		fprintf(fp, "%s", szMsgBuffer);

		fflush(fp);
		fclose(fp);	
	}*/

	//if (!fHaveWrittenFirstFrame) 
	//{
	//	// If we have NAL units encoded in "sprop parameter strings", prepend these to the file:
	//	for (unsigned j = 0; j < 3; ++j) {
	//		unsigned numSPropRecords;
	//		SPropRecord* sPropRecords
	//			= parseSPropParameterSets(fSPropParameterSetsStr[j], numSPropRecords);
	//		for (unsigned i = 0; i < numSPropRecords; ++i) {
	//			addData(start_code, 4, presentationTime);
	//			addData(sPropRecords[i].sPropBytes, sPropRecords[i].sPropLength, presentationTime);
	//		}
	//		delete[] sPropRecords;
	//	}

	//	fHaveWrittenFirstFrame = True; // for next time
	//}

	::EnterCriticalSection(&m_csGettingFrame);

	char chNALUType  = fBuffer[0];
	int  nFrameType  = chNALUType & 0x1F;  // 5 IDR; 7 SPS; 8 PPS

	g_strMessage.Format("CPxH264VideoFileSink nFrameType:%d.", nFrameType);
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMessage.GetBuffer());

	if (m_bHasWriteSPS && m_bHasWritePPS)
	{
		addData(start_code, 4,         presentationTime); 
		addData(fBuffer,    frameSize, presentationTime);
	}

	if (kePxNALUType_SPS == nFrameType && !m_bHasWriteSPS)
	{
		addData(start_code, 4,         presentationTime); 
		addData(fBuffer,    frameSize, presentationTime);

		m_bHasWriteSPS = true;
	}
	else if (kePxNALUType_PPS == nFrameType && !m_bHasWritePPS)
	{
		addData(start_code, 4,         presentationTime); 
		addData(fBuffer,    frameSize, presentationTime);

		m_bHasWritePPS = true;
	}

	// 时间戳 单位: 毫秒
	INT64 nTimeStamp = ((INT64)presentationTime.tv_sec) * 1000 + presentationTime.tv_usec / 1000;

	if (presentationTime.tv_usec == fPrevPresentationTime.tv_usec &&
		presentationTime.tv_sec == fPrevPresentationTime.tv_sec && 
		kePxNALUType_SPS != nFrameType &&
		kePxNALUType_PPS != nFrameType &&
		kePxNALUType_I_Frame != nFrameType) 
	{
		// The presentation time is unchanged from the previous frame, so we add a 'counter'
		// suffix to the file name, to distinguish them:
		/*sprintf(fPerFrameFileNameBuffer, "%s-%lu.%06lu-%u", fPerFrameFileNamePrefix,
		presentationTime.tv_sec, presentationTime.tv_usec, ++fSamePresentationTimeCounter);*/

		// test begin
		/*{
		n_FrameLength += frameSize;
		g_strH264Msg.Format("CPxH264VideoFileSink tv_sec:%ld, tv_usec:%ld, frameSize:%u, n_FrameLength: %u",
		presentationTime.tv_sec,
		presentationTime.tv_usec,
		frameSize,
		n_FrameLength);

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strH264Msg.GetBuffer());
		}*/
		// test end
	}
	else
	{
		/*g_strMessage.Format("CPxH264VideoFileSink nTimeStamp :%I64d, nFrameType:%d, frameSize:%u, tv_sec:%ld, tv_usec:%ld",
		nTimeStamp,
		nFrameType,
		frameSize,
		presentationTime.tv_sec,
		presentationTime.tv_usec);

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMessage.GetBuffer());*/

		n_FrameLength = 0;

		if (0 == s_nPackageIndex)
		{
			s_nFrameCnt = 0;
		}

		s_nFrameCnt++;

		if ((RTSP_PACKET_BUF_SIZE - 1) == s_nPackageIndex)
		{
			s_nPackageIndex = 0;
		}

		/*g_aRTSPPacket[s_nPackageIndex].eMediaType  = kePxMediaType_Video;
		g_aRTSPPacket[s_nPackageIndex].eNALUType   = (EPxNALUType)nFrameType;
		g_aRTSPPacket[s_nPackageIndex].nTimeStamp  = nTimeStamp;
		g_aRTSPPacket[s_nPackageIndex].uiFrameSize = frameSize;

		::PostMessage(g_hWndRTSP, 
		WM_ADD_RTSP_PACKAGE_TO_LIST, 
		NULL, 
		(LPARAM)&g_aRTSPPacket[s_nPackageIndex]);*/

		sprintf(g_vsRTSPPacket[s_nPackageIndex].szId, "%08d", s_nFrameCnt);
		g_vsRTSPPacket[s_nPackageIndex].eMediaType  = kePxMediaType_Video;
		g_vsRTSPPacket[s_nPackageIndex].eNALUType   = (EPxNALUType)nFrameType;
		g_vsRTSPPacket[s_nPackageIndex].nTimeStamp  = nTimeStamp;
		g_vsRTSPPacket[s_nPackageIndex].uiFrameSize = frameSize;

		::PostMessage(g_hWndRTSP, 
			WM_ADD_RTSP_PACKAGE_TO_LIST, 
			NULL, 
			(LPARAM)&g_vsRTSPPacket[s_nPackageIndex]);

		/*g_strMessage.Format("CPxH264VideoFileSink nTimeStamp :%I64d, g_vsRTSPPacket[s_nPackageIndex].nTimeStamp:%I64d, s_nPackageIndex: %d.",
		nTimeStamp,
		g_vsRTSPPacket[s_nPackageIndex].nTimeStamp,
		s_nPackageIndex);

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_strMessage.GetBuffer());*/

		s_nPackageIndex++;
	}

	fPrevPresentationTime.tv_sec  = presentationTime.tv_sec;
	fPrevPresentationTime.tv_usec = presentationTime.tv_usec;

	// Then try getting the next frame:
	continuePlaying();

	::LeaveCriticalSection(&m_csGettingFrame);
}
