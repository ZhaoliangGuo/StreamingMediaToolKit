
#ifndef AAC_FILE_SINK_H
#define AAC_FILE_SINK_H

#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif
#include <string>
using namespace std;

#define  MSG_BUF_SIZE 1024

class CPxAACFileSink: public FileSink
{
public:
	static CPxAACFileSink* createNew(UsageEnvironment& env, char const* fileName,
		unsigned bufferSize,
		Boolean oneFilePerFrame ,
		char* in_pszSDP);
	// (See "FileSink.hh" for a description of these parameters.)

protected:
	CPxAACFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		char const* perFrameFileNamePrefix, char* in_pszSDP);
	// called only by createNew()
	virtual ~CPxAACFileSink();

	virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);

	virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime);

	void	ParseSdpConfig();
	
	char	m_AudioProfile;
	char	m_samplerateidx ;
	char	m_channel;

private:
	char*	m_pAACFrame;
	string	m_strconfig;
	char	m_config[2];
	string	m_strSDP;

private:
	char m_szMsgBuffer[MSG_BUF_SIZE];

private:
	CRITICAL_SECTION  m_csGettingFrame; 
};

#endif
