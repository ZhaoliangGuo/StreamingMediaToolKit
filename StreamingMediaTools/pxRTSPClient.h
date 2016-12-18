#pragma once

#include <time.h>    

#include "liveMedia.hh"
#include "Groupsock.hh"
#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"

#if defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif

#ifndef uint64_t
typedef unsigned long long int uint64_t;
#endif

class qosMeasurementRecord 
{
public:
	qosMeasurementRecord(struct timeval const& startTime, RTPSource* src)
		: fSource(src), fNext(NULL),
		kbits_per_second_min(1e20), kbits_per_second_max(0),
		kBytesTotal(0.0),
		packet_loss_fraction_min(1.0), packet_loss_fraction_max(0.0),
		totNumPacketsReceived(0), totNumPacketsExpected(0) {
			measurementEndTime = measurementStartTime = startTime;

			RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
			// Assume that there's only one SSRC source (usually the case):
			RTPReceptionStats* stats = statsIter.next(True);
			if (stats != NULL) {
				kBytesTotal = stats->totNumKBytesReceived();
				totNumPacketsReceived = stats->totNumPacketsReceived();
				totNumPacketsExpected = stats->totNumPacketsExpected();
			}
	}

	virtual ~qosMeasurementRecord() { delete fNext; }

	void periodicQOSMeasurement(struct timeval const& timeNow);

public:
	RTPSource* fSource;
	qosMeasurementRecord* fNext;

public:
	struct timeval measurementStartTime, measurementEndTime;
	double kbits_per_second_min, kbits_per_second_max;
	double kBytesTotal;
	double packet_loss_fraction_min, packet_loss_fraction_max;
	unsigned totNumPacketsReceived, totNumPacketsExpected;
};

class CPxRTSPClient: public RTSPClient
{
public:
	CPxRTSPClient(void);
	~CPxRTSPClient(void);

// opentRTSP
public:
	Medium* createClient(UsageEnvironment& env, char const* URL, int verbosityLevel, char const* applicationName);
	void assignClient(Medium* client);
	//RTSPClient* ourRTSPClient;
	SIPClient* ourSIPClient;

	void getOptions(RTSPClient::responseHandler* afterFunc);

	void getSDPDescription(RTSPClient::responseHandler* afterFunc);

	void setupSubsession(MediaSubsession* subsession, Boolean streamUsingTCP, Boolean forceMulticastOnUnspecified, RTSPClient::responseHandler* afterFunc);

	void startPlayingSession(MediaSession* session, double start, double end, float scale, RTSPClient::responseHandler* afterFunc);

	void startPlayingSession(MediaSession* session, char const* absStartTime, char const* absEndTime, float scale, RTSPClient::responseHandler* afterFunc);
	// For playing by 'absolute' time (using strings of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z"

	void tearDownSession(MediaSession* session, RTSPClient::responseHandler* afterFunc);

	void setUserAgentString(char const* userAgentString);

	//Authenticator* ourAuthenticator;
	Boolean allowProxyServers;
	Boolean controlConnectionUsesTCP;
	Boolean supportCodecSelection;
	//char const* clientProtocolName;
	unsigned statusCode;

	char clientProtocolName[16];

	RTSPClient* ourRTSPClient;

// playCommon
public:
	void continueAfterClientCreation0(RTSPClient* client, Boolean requestStreamingOverTCP);
	void continueAfterClientCreation1();
	void continueAfterOPTIONS(RTSPClient* client, int resultCode, char* resultString);
	void continueAfterDESCRIBE(RTSPClient* client, int resultCode, char* resultString);
	void continueAfterSETUP(RTSPClient* client, int resultCode, char* resultString);
	void continueAfterPLAY(RTSPClient* client, int resultCode, char* resultString);
	void continueAfterTEARDOWN(RTSPClient* client, int resultCode, char* resultString);

	void createOutputFiles(char const* periodicFilenameSuffix);
	void createPeriodicOutputFiles();
	void setupStreams();
	void closeMediaSinks();
	void subsessionAfterPlaying(void* clientData);
	void subsessionByeHandler(void* clientData);
	void sessionAfterPlaying(void* clientData = NULL);
	void sessionTimerHandler(void* clientData);
	void periodicFileOutputTimerHandler(void* clientData);
	void shutdown(int exitCode = 1);
	void signalHandlerShutdown(int sig);
	void checkForPacketArrival(void* clientData);
	void checkInterPacketGaps(void* clientData);
	void checkSessionTimeoutBrokenServer(void* clientData);

	void beginQOSMeasurement();
	void printQOSData(int exitCode);

	char const* progName;
	UsageEnvironment* env;
	Medium* ourClient;
	Authenticator* ourAuthenticator;
	char const* streamURL;
	MediaSession* session;
	TaskToken sessionTimerTask;
	TaskToken sessionTimeoutBrokenServerTask;
	TaskToken arrivalCheckTimerTask;
	TaskToken interPacketGapCheckTimerTask;
	TaskToken qosMeasurementTimerTask;
	TaskToken periodicFileOutputTask;
	Boolean createReceivers;
	Boolean outputQuickTimeFile;
	Boolean generateMP4Format;
	QuickTimeFileSink* qtOut;
	Boolean outputAVIFile;
	AVIFileSink* aviOut;
	Boolean audioOnly;
	Boolean videoOnly;
	char const* singleMedium;
	int verbosityLevel; // by default, print verbose output
	double duration;
	double durationSlop; // extra seconds to play at the end
	double initialSeekTime;
	char* initialAbsoluteSeekTime;
	char* initialAbsoluteSeekEndTime;
	float scale;
	double endTime;
	unsigned interPacketGapMaxTime;
	unsigned totNumPacketsReceived; // used if checking inter-packet gaps
	Boolean playContinuously;
	int simpleRTPoffsetArg;
	Boolean sendOptionsRequest;
	Boolean sendOptionsRequestOnly;
	Boolean oneFilePerFrame;
	Boolean notifyOnPacketArrival;
	Boolean sendKeepAlivesToBrokenServers;
	unsigned sessionTimeoutParameter;
	Boolean streamUsingTCP;
	Boolean forceMulticastOnUnspecified;
	unsigned short desiredPortNum;
	portNumBits tunnelOverHTTPPortNum;
	char* username;
	char* password;
	char* proxyServerName;
	unsigned short proxyServerPortNum;
	unsigned char desiredAudioRTPPayloadFormat;
	char* mimeSubtype;
	unsigned short movieWidth; // default
	Boolean movieWidthOptionSet;
	unsigned short movieHeight; // default
	Boolean movieHeightOptionSet;
	unsigned movieFPS; // default
	Boolean movieFPSOptionSet;
	char const* fileNamePrefix;
	unsigned fileSinkBufferSize;
	unsigned socketInputBufferSize;
	Boolean packetLossCompensate;
	Boolean syncStreams;
	Boolean generateHintTracks;
	Boolean waitForResponseToTEARDOWN;
	unsigned qosMeasurementIntervalMS; // 0 means: Don't output QOS data
	char* userAgent;
	unsigned fileOutputInterval; // seconds
	unsigned fileOutputSecondsSoFar; // seconds
	Boolean createHandlerServerForREGISTERCommand;
	portNumBits handlerServerForREGISTERCommandPortNum;
	HandlerServerForREGISTERCommand* handlerServerForREGISTERCommand;
	char* usernameForREGISTER;
	char* passwordForREGISTER;
	UserAuthenticationDatabase* authDBForREGISTER;

	struct timeval startTime;

	MediaSubsession *subsession;
	Boolean madeProgress;

	Boolean areAlreadyShuttingDown;
	int shutdownExitCode;

public:
	int gettimeofday(struct timeval *tv, void* tz);

// QoS
public:
	/*static */void periodicQOSMeasurement(void* clientData); // forward
	/*static */void scheduleNextQOSMeasurement();

	/*static */qosMeasurementRecord* qosRecordHead;
	/*static */unsigned nextQOSMeasurementUSecs;
};



