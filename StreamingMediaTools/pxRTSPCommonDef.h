#ifndef _PX_RTSP_COMMON_DEF_H
#define _PX_RTSP_COMMON_DEF_H

const int  WM_ADD_RTSP_PACKAGE_TO_LIST = WM_USER + 3001;

#define RTSP_URL_LEN (1024)

typedef enum EPxStreamingMode
{
	kePxStreamingMode_Invalid = -1,
	kePxStreamingMode_TCP = 0,
	kePxStreamingMode_UDP = 1,
	kePxStreamingMode_Cnt
}EPxStreamingMode;

typedef enum EPxRecordMediaType
{
	kePxRecordMediaType_Invalid = -1,
	kePxRecordMediaType_MP4 = 0,
	kePxRecordMediaType_AVI,
	kePxRecordMediaType_Cnt
}EPxRecordMediaType;

typedef enum EPxMediaType
{
	kePxMediaType_Invalid = -1,
	kePxMediaType_Video = 0,
	kePxMediaType_Audio, 
	kePxMediaType_Cnt
}EPxMediaType;

typedef enum EPxNALUType
{
	kePxNALUType_Invalid = -1,
	kePxNALUType_P_Frame = 1, 
	kePxNALUType_I_Frame = 5, 
	kePxNALUType_SEI     = 6, 
	kePxNALUType_SPS     = 7, 
	kePxNALUType_PPS     = 8, 
	kePxNALUType_Cnt
}EPxNALUType;

typedef struct SPxRTSPArg
{
	char               szURL[RTSP_URL_LEN];
	EPxStreamingMode   eStreamingMode;
	bool               bRecord;
	char               szRecordFileName[_MAX_PATH];
	EPxRecordMediaType eRecordMediaType;
	int                nRecordDuration;
	bool               bVideo;
	bool               bAudio;
	char               szRTSPProcessInfoFileName[_MAX_PATH];
	bool               bRTSPProcessInfo2File; // 是否将于rtsp server交互过程信息保存在文件

	SPxRTSPArg()
	{
		ZeroMemory(szURL, RTSP_URL_LEN);
		ZeroMemory(szRecordFileName, _MAX_PATH);
		eStreamingMode   = kePxStreamingMode_TCP;
		bRecord          = false;
		eRecordMediaType = kePxRecordMediaType_MP4;
		nRecordDuration  = 30;
		bVideo           = true;
		bAudio           = true;
		ZeroMemory(szRTSPProcessInfoFileName, _MAX_PATH);
		bRTSPProcessInfo2File = false;
	}

	~SPxRTSPArg()
	{
		ZeroMemory(szURL, RTSP_URL_LEN);
		ZeroMemory(szRecordFileName, _MAX_PATH);
		eStreamingMode   = kePxStreamingMode_TCP;
		bRecord          = false;
		eRecordMediaType = kePxRecordMediaType_MP4;
		nRecordDuration  = 30;
		bVideo           = true;
		bAudio           = true;
		ZeroMemory(szRTSPProcessInfoFileName, _MAX_PATH);
		bRTSPProcessInfo2File = false;
	}
}SPxRTSPArg;

typedef struct RTSPPacket
{
	char         szId[16];
	EPxMediaType eMediaType;
	EPxNALUType  eNALUType;
	INT64        nTimeStamp;
	unsigned int uiFrameSize;

	RTSPPacket()
	{
		ZeroMemory(szId, 16);
		eMediaType  = kePxMediaType_Invalid;
		eNALUType   = kePxNALUType_Invalid;
		nTimeStamp  = 0;
		uiFrameSize = 0;
	}

	~RTSPPacket()
	{
		ZeroMemory(szId, 16);
		eMediaType  = kePxMediaType_Invalid;
		eNALUType   = kePxNALUType_Invalid;
		nTimeStamp  = 0;
		uiFrameSize = 0;
	}
}RTSPPacket;

#endif