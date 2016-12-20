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
// Copyright (c) 1996-2016, Live Networks, Inc.  All rights reserved
// A common framework, used for the "openRTSP" and "playSIP" applications
// Implementation
//
// NOTE: If you want to develop your own RTSP client application (or embed RTSP client functionality into your own application),
// then we don't recommend using this code as a model, because it is too complex (with many options).
// Instead, we recommend using the "testRTSPClient" application code as a model.

#include "playCommon.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

#if defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif

// add by ZL Guo Begin
#define MSG_BUFFER_LEN (1024*16)
char g_szMessage[MSG_BUFFER_LEN] = {0};
extern HWND     g_hAppWnd;         // 主界面句柄
const int WM_ADD_LOG_TO_LIST                    = WM_USER + 1001;

#define OUTPUT_FILENAME_LEN (1024)
char outFileName[OUTPUT_FILENAME_LEN]           = {0};
char szRTSPProcessInfoFileName[OUTPUT_FILENAME_LEN] = {0}; // 用于保存交互过程的信息

SPxRTSPArg sCurrentRTSPArg;

// add by ZL Guo End

#include "pxH264VideoFileSink.h"
#include "pxAACFileSink.h"

char const* progName = "openRTSP";
UsageEnvironment* env;
Medium* ourClient = NULL;
Authenticator* ourAuthenticator = NULL;
//char const* streamURL = NULL;
char streamURL[RTSP_URL_LEN] = {0};
MediaSession* session = NULL;
TaskToken sessionTimerTask = NULL;
TaskToken sessionTimeoutBrokenServerTask = NULL;
TaskToken arrivalCheckTimerTask = NULL;
TaskToken interPacketGapCheckTimerTask = NULL;
TaskToken qosMeasurementTimerTask = NULL;
TaskToken periodicFileOutputTask = NULL;
Boolean createReceivers = True;
Boolean outputQuickTimeFile = False;
Boolean generateMP4Format = False;
QuickTimeFileSink* qtOut = NULL;
Boolean outputAVIFile = False;
AVIFileSink* aviOut = NULL;
Boolean audioOnly = False;
Boolean videoOnly = False;
char const* singleMedium = NULL;
int verbosityLevel = 1; // by default, print verbose output
double duration = 0;
double durationSlop = -1.0; // extra seconds to play at the end
double initialSeekTime = 0.0f;
char* initialAbsoluteSeekTime = NULL;
char* initialAbsoluteSeekEndTime = NULL;
float scale = 1.0f;
double endTime;
unsigned interPacketGapMaxTime = 0;
unsigned totNumPacketsReceived = ~0; // used if checking inter-packet gaps
Boolean playContinuously = False;
int simpleRTPoffsetArg = -1;
Boolean sendOptionsRequest = True;
Boolean sendOptionsRequestOnly = False;
Boolean oneFilePerFrame = False;
Boolean notifyOnPacketArrival = False;
Boolean sendKeepAlivesToBrokenServers = False;
unsigned sessionTimeoutParameter = 0;
Boolean streamUsingTCP = False;
Boolean forceMulticastOnUnspecified = False;
unsigned short desiredPortNum = 0;
portNumBits tunnelOverHTTPPortNum = 0;
char* username = NULL;
char* password = NULL;
char* proxyServerName = NULL;
unsigned short proxyServerPortNum = 0;
unsigned char desiredAudioRTPPayloadFormat = 0;
char* mimeSubtype = NULL;
unsigned short movieWidth = 240; // default
Boolean movieWidthOptionSet = False;
unsigned short movieHeight = 180; // default
Boolean movieHeightOptionSet = False;
unsigned movieFPS = 15; // default
Boolean movieFPSOptionSet = False;
char const* fileNamePrefix = "";
unsigned fileSinkBufferSize = 100000;
unsigned socketInputBufferSize = 0;
Boolean packetLossCompensate = False;
Boolean syncStreams = False;
Boolean generateHintTracks = False;
Boolean waitForResponseToTEARDOWN = True;
unsigned qosMeasurementIntervalMS = 0; // 0 means: Don't output QOS data
char* userAgent = NULL;
unsigned fileOutputInterval = 0; // seconds
unsigned fileOutputSecondsSoFar = 0; // seconds
Boolean createHandlerServerForREGISTERCommand = False;
portNumBits handlerServerForREGISTERCommandPortNum = 0;
HandlerServerForREGISTERCommand* handlerServerForREGISTERCommand;
char* usernameForREGISTER = NULL;
char* passwordForREGISTER = NULL;
UserAuthenticationDatabase* authDBForREGISTER = NULL;

struct timeval startTime;

void usage() 
{
	*env << "Usage: " << progName
		<< " [-p <startPortNum>] [-r|-q|-4|-i] [-a|-v] [-V] [-d <duration>] [-D <max-inter-packet-gap-time> [-c] [-S <offset>] [-n] [-O]"
		<< (controlConnectionUsesTCP ? " [-t|-T <http-port>]" : "")
		<< " [-u <username> <password>"
		<< (allowProxyServers ? " [<proxy-server> [<proxy-server-port>]]" : "")
		<< "]" << (supportCodecSelection ? " [-A <audio-codec-rtp-payload-format-code>|-M <mime-subtype-name>]" : "")
		<< " [-s <initial-seek-time>]|[-U <absolute-seek-time>] [-E <absolute-seek-end-time>] [-z <scale>] [-g user-agent]"
		<< " [-k <username-for-REGISTER> <password-for-REGISTER>]"
		<< " [-P <interval-in-seconds>] [-K]"
		<< " [-w <width> -h <height>] [-f <frames-per-second>] [-y] [-H] [-Q [<measurement-interval>]] [-F <filename-prefix>] [-b <file-sink-buffer-size>] [-B <input-socket-buffer-size>] [-I <input-interface-ip-address>] [-m] [<url>|-R [<port-num>]] (or " << progName << " -o [-V] <url>)\n";
	shutdown();
}

/*
Function: 用于打印信息到文件
          本文件中主要用于将于客户端交互的信息打印到文件.
*/
void WriteInfo2File(char *in_szFileName, char *in_szMsgBuffer)
{
	if (!sCurrentRTSPArg.bRTSPProcessInfo2File)
	{
		return ;
	}
	
	if ((NULL == in_szFileName) || (NULL == in_szMsgBuffer))
	{
		return ;
	}

	FILE *fp = fopen(in_szFileName, "at");
	if (NULL != fp)
	{
		fprintf(fp, "%s", in_szMsgBuffer);
		fflush(fp);
		fclose(fp);
	}
	
	return ;
}

TaskScheduler* scheduler = NULL;
char eventLoopWatchVariable = 0;

int StartOpenRTSP(SPxRTSPArg &rsRTSPArg)
{
	eventLoopWatchVariable = 0;

	// Begin by setting up our usage environment:
	scheduler = BasicTaskScheduler::createNew();
	env       = BasicUsageEnvironment::createNew(*scheduler);

	sCurrentRTSPArg = rsRTSPArg;

	strcpy(env->szRTSPProcessInfoFileName, rsRTSPArg.szRTSPProcessInfoFileName);
	env->bRTSPProcessInfo2File = rsRTSPArg.bRTSPProcessInfo2File;

	strcpy_s(streamURL, RTSP_URL_LEN, rsRTSPArg.szURL);

	if (kePxRecordMediaType_MP4 == rsRTSPArg.eRecordMediaType)
	{
		outputQuickTimeFile = True;
		generateMP4Format = True;
	}
	else if (kePxRecordMediaType_AVI == rsRTSPArg.eRecordMediaType)
	{
		outputAVIFile = True;
	}

	if (kePxStreamingMode_TCP == rsRTSPArg.eStreamingMode)
	{
		streamUsingTCP = True;
	}
	else
	{
		streamUsingTCP = False;
	}

	// receive/record a video stream only
	if (rsRTSPArg.bVideo && !rsRTSPArg.bAudio)
	{
		videoOnly = True;
		singleMedium = "video";
	}

	// receive/record an audio stream only
	if (!rsRTSPArg.bVideo && rsRTSPArg.bAudio)
	{
		audioOnly = True;
		singleMedium = "audio";
	}

	env->bRTSPProcessInfo2File = rsRTSPArg.bRTSPProcessInfo2File;

	if (rsRTSPArg.bRTSPProcessInfo2File)
	{
		if (NULL != rsRTSPArg.szRTSPProcessInfoFileName)
		{
			strcpy_s(szRTSPProcessInfoFileName,      OUTPUT_FILENAME_LEN, rsRTSPArg.szRTSPProcessInfoFileName);
			strcpy_s(env->szRTSPProcessInfoFileName, OUTPUT_FILENAME_LEN, rsRTSPArg.szRTSPProcessInfoFileName);
		}
	}
		
	if (rsRTSPArg.bRecord)
	{
		if (rsRTSPArg.nRecordDuration != 0)
		{
			duration     = rsRTSPArg.nRecordDuration;
			durationSlop = 0;
		}

		strcpy_s(outFileName, OUTPUT_FILENAME_LEN, rsRTSPArg.szRecordFileName);
	}
	else
	{
		outputQuickTimeFile = false;
		outputAVIFile       = false;
	}
	
	// Create (or arrange to create) our client object:
	if (createHandlerServerForREGISTERCommand) 
	{
		handlerServerForREGISTERCommand
			= HandlerServerForREGISTERCommand::createNew(*env, continueAfterClientCreation0,
			handlerServerForREGISTERCommandPortNum, authDBForREGISTER,
			verbosityLevel, progName);

		if (handlerServerForREGISTERCommand == NULL) 
		{
			*env << "Failed to create a server for handling incoming \"REGISTER\" commands: " << env->getResultMsg() << "\n";
		} 
		else 
		{
			*env << "Awaiting an incoming \"REGISTER\" command on port " << handlerServerForREGISTERCommand->serverPortNum() << "\n";
		}
	} 
	else 
	{
		ourClient = createClient(*env, streamURL, verbosityLevel, progName);
		if (ourClient == NULL) 
		{
			*env << "Failed to create " << clientProtocolName << " client: " << env->getResultMsg() << "\n";

			sprintf_s(g_szMessage, 
				MSG_BUFFER_LEN, 
				"Failed to create %s client: %s \n", 
				clientProtocolName,
				env->getResultMsg());
			::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);

			WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

			shutdown();
		}

		continueAfterClientCreation1();
	}

	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable); // does not return

	return 0;
}

int StopOpenRTSP()
{
	eventLoopWatchVariable = 1;

	Sleep(100);

	if (env)
	{
		env->reclaim(); 
		env = NULL;
	}
	
	if (scheduler)
	{
		delete scheduler; 
		scheduler = NULL;
	}

	if (session)
	{
		session = NULL;
	}

	/*if (setupIter)
	{
		setupIter = NULL;
	}*/
	

	/*if (subsession)
	{
		subsession = NULL;
	}*/

	return 0;
}


#if 0
//int main(int argc, char** argv) 
int test1()
{
	// We need at least one "rtsp://" URL argument:
	//if (argc < 2) 
	//{
	//  usage(*env, argv[0]);
	//  return 1;
	//}

	// There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
	//for (int i = 1; i <= argc-1; ++i) 
	//{
	//  //openURL(*env, argv[0], argv[i]);
	//}

	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
	// This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

	return 0;

	// If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
	// and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
	// then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
	/*
	env->reclaim(); env = NULL;
	delete scheduler; scheduler = NULL;
	*/
}
//int main(int argc, char** argv) 
int test2(int argc, char** argv)
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	/*
	-d [time] -- record time by secend
	-i        -- output format: .avi。
	-4        -- output format:.mp4
	-q        -- output format:.mov
	*/

	char *pszArgv[] =
	{
		"openRTSP", 
		//"-4", // .mp4
		//"-q", // .mov
		//"-d", 
		//"20", // record time
		//"-w", 
		//"380", // width 
		//"-h", 
		//"480", // height
		// "-f", 
		//"30", 
		//"rtsp://admin:admin12345@172.16.41.75",
		"rtsp://192.168.1.4:554/test.mkv",
		/*">",
		".\\test.mp4"*/
	};

	argc = 2;
	for (int i = 0; i < argc; i++)
	{
		argv[i] = pszArgv[i];
	}

	progName = argv[0];

	gettimeofday(&startTime, NULL);

#ifdef USE_SIGNALS
	// Allow ourselves to be shut down gracefully by a SIGHUP or a SIGUSR1:
	signal(SIGHUP, signalHandlerShutdown);
	signal(SIGUSR1, signalHandlerShutdown);
#endif

	// unfortunately we can't use getopt() here, as Windoze doesn't have it
	while (argc > 1) {
		char* const opt = argv[1];
		if (opt[0] != '-') {
			if (argc == 2) break; // only the URL is left
			usage();
		}

		switch (opt[1]) {
		case 'p': { // specify start port number
			int portArg;
			if (sscanf(argv[2], "%d", &portArg) != 1) {
				usage();
			}
			if (portArg <= 0 || portArg >= 65536 || portArg&1) {
				*env << "bad port number: " << portArg
					<< " (must be even, and in the range (0,65536))\n";
				usage();
			}
			desiredPortNum = (unsigned short)portArg;
			++argv; --argc;
			break;
				  }

		case 'r': { // do not receive data (instead, just 'play' the stream(s))
			createReceivers = False;
			break;
				  }

		case 'q': { // output a QuickTime file (to stdout)
			outputQuickTimeFile = True;
			break;
				  }

		case '4': { // output a 'mp4'-format file (to stdout)
			outputQuickTimeFile = True;
			generateMP4Format = True;
			break;
				  }

		case 'i': { // output an AVI file (to stdout)
			outputAVIFile = True;
			break;
				  }

		case 'I': { // specify input interface...
			NetAddressList addresses(argv[2]);
			if (addresses.numAddresses() == 0) {
				*env << "Failed to find network address for \"" << argv[2] << "\"";
				break;
			}
			ReceivingInterfaceAddr = *(unsigned*)(addresses.firstAddress()->data());
			++argv; --argc;
			break;
				  }

		case 'a': { // receive/record an audio stream only
			audioOnly = True;
			singleMedium = "audio";
			break;
				  }

		case 'v': { // receive/record a video stream only
			videoOnly = True;
			singleMedium = "video";
			break;
				  }

		case 'V': { // disable verbose output
			verbosityLevel = 0;
			break;
				  }

		case 'd': 
			{ // specify duration, or how much to delay after end time
			float arg;
			if (sscanf(argv[2], "%g", &arg) != 1) {
				usage();
			}
			if (argv[2][0] == '-') { // not "arg<0", in case argv[2] was "-0"
				// a 'negative' argument was specified; use this for "durationSlop":
				duration = 0; // use whatever's in the SDP
				durationSlop = -arg;
			} else {
				duration = arg;
				durationSlop = 0;
			}
			++argv; --argc;
			break;
		    }

		case 'D': { // specify maximum number of seconds to wait for packets:
			if (sscanf(argv[2], "%u", &interPacketGapMaxTime) != 1) {
				usage();
			}
			++argv; --argc;
			break;
				  }

		case 'c': { // play continuously
			playContinuously = True;
			break;
				  }

		case 'S': { // specify an offset to use with "SimpleRTPSource"s
			if (sscanf(argv[2], "%d", &simpleRTPoffsetArg) != 1) {
				usage();
			}
			if (simpleRTPoffsetArg < 0) {
				*env << "offset argument to \"-S\" must be >= 0\n";
				usage();
			}
			++argv; --argc;
			break;
				  }

		case 'm': { // output multiple files - one for each frame
			oneFilePerFrame = True;
			break;
				  }

		case 'n': { // notify the user when the first data packet arrives
			notifyOnPacketArrival = True;
			break;
				  }

		case 'O': { // Don't send an "OPTIONS" request before "DESCRIBE"
			sendOptionsRequest = False;
			break;
				  }

		case 'o': { // Send only the "OPTIONS" request to the server
			sendOptionsRequestOnly = True;
			break;
				  }

		case 'P': 
			{ // specify an interval (in seconds) between writing successive output files
				int fileOutputIntervalInt;
				if (sscanf(argv[2], "%d", &fileOutputIntervalInt) != 1 || fileOutputIntervalInt <= 0) {
					usage();
				}
				fileOutputInterval = (unsigned)fileOutputIntervalInt;
				++argv; --argc;
				break;
			}

		case 't': 
			{
				// stream RTP and RTCP over the TCP 'control' connection
				if (controlConnectionUsesTCP) 
				{
					streamUsingTCP = True;
				} 
				else 
				{
					usage();
				}

				break;
			}

		case 'T': {
			// stream RTP and RTCP over a HTTP connection
			if (controlConnectionUsesTCP) {
				if (argc > 3 && argv[2][0] != '-') {
					// The next argument is the HTTP server port number:
					if (sscanf(argv[2], "%hu", &tunnelOverHTTPPortNum) == 1
						&& tunnelOverHTTPPortNum > 0) {
							++argv; --argc;
							break;
					}
				}
			}

			// If we get here, the option was specified incorrectly:
			usage();
			break;
				  }

		case 'u': { // specify a username and password
			if (argc < 4) usage(); // there's no argv[3] (for the "password")
			username = argv[2];
			password = argv[3];
			argv+=2; argc-=2;
			if (allowProxyServers && argc > 3 && argv[2][0] != '-') {
				// The next argument is the name of a proxy server:
				proxyServerName = argv[2];
				++argv; --argc;

				if (argc > 3 && argv[2][0] != '-') {
					// The next argument is the proxy server port number:
					if (sscanf(argv[2], "%hu", &proxyServerPortNum) != 1) {
						usage();
					}
					++argv; --argc;
				}
			}

			ourAuthenticator = new Authenticator(username, password);
			break;
				  }

		case 'k': { // specify a username and password to be used to authentication an incoming "REGISTER" command (for use with -R)
			if (argc < 4) usage(); // there's no argv[3] (for the "password")
			usernameForREGISTER = argv[2];
			passwordForREGISTER = argv[3];
			argv+=2; argc-=2;

			if (authDBForREGISTER == NULL) authDBForREGISTER = new UserAuthenticationDatabase;
			authDBForREGISTER->addUserRecord(usernameForREGISTER, passwordForREGISTER);
			break;
				  }

		case 'K': { // Send periodic 'keep-alive' requests to keep broken server sessions alive
			sendKeepAlivesToBrokenServers = True;
			break;
				  }

		case 'A': { // specify a desired audio RTP payload format
			unsigned formatArg;
			if (sscanf(argv[2], "%u", &formatArg) != 1
				|| formatArg >= 96) {
					usage();
			}
			desiredAudioRTPPayloadFormat = (unsigned char)formatArg;
			++argv; --argc;
			break;
				  }

		case 'M': { // specify a MIME subtype for a dynamic RTP payload type
			mimeSubtype = argv[2];
			if (desiredAudioRTPPayloadFormat==0) desiredAudioRTPPayloadFormat =96;
			++argv; --argc;
			break;
				  }

		case 'w': { // specify a width (pixels) for an output QuickTime or AVI movie
			if (sscanf(argv[2], "%hu", &movieWidth) != 1) {
				usage();
			}
			movieWidthOptionSet = True;
			++argv; --argc;
			break;
				  }

		case 'h': { // specify a height (pixels) for an output QuickTime or AVI movie
			if (sscanf(argv[2], "%hu", &movieHeight) != 1) {
				usage();
			}
			movieHeightOptionSet = True;
			++argv; --argc;
			break;
				  }

		case 'f': { // specify a frame rate (per second) for an output QT or AVI movie
			if (sscanf(argv[2], "%u", &movieFPS) != 1) {
				usage();
			}
			movieFPSOptionSet = True;
			++argv; --argc;
			break;
				  }

		case 'F': { // specify a prefix for the audio and video output files
			fileNamePrefix = argv[2];
			++argv; --argc;
			break;
				  }

		case 'g': { // specify a user agent name to use in outgoing requests
			userAgent = argv[2];
			++argv; --argc;
			break;
				  }

		case 'b': { // specify the size of buffers for "FileSink"s
			if (sscanf(argv[2], "%u", &fileSinkBufferSize) != 1) {
				usage();
			}
			++argv; --argc;
			break;
				  }

		case 'B': { // specify the size of input socket buffers
			if (sscanf(argv[2], "%u", &socketInputBufferSize) != 1) {
				usage();
			}
			++argv; --argc;
			break;
				  }

				  // Note: The following option is deprecated, and may someday be removed:
		case 'l': { // try to compensate for packet loss by repeating frames
			packetLossCompensate = True;
			break;
				  }

		case 'y': { // synchronize audio and video streams
			syncStreams = True;
			break;
				  }

		case 'H': { // generate hint tracks (as well as the regular data tracks)
			generateHintTracks = True;
			break;
				  }

		case 'Q': { // output QOS measurements
			qosMeasurementIntervalMS = 1000; // default: 1 second

			if (argc > 3 && argv[2][0] != '-') {
				// The next argument is the measurement interval,
				// in multiples of 100 ms
				if (sscanf(argv[2], "%u", &qosMeasurementIntervalMS) != 1) {
					usage();
				}
				qosMeasurementIntervalMS *= 100;
				++argv; --argc;
			}
			break;
				  }

		case 's': { // specify initial seek time (trick play)
			double arg;
			if (sscanf(argv[2], "%lg", &arg) != 1 || arg < 0) {
				usage();
			}
			initialSeekTime = arg;
			++argv; --argc;
			break;
				  }

		case 'U': {
			// specify initial absolute seek time (trick play), using a string of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z"
			initialAbsoluteSeekTime = argv[2];
			++argv; --argc;
			break;
				  }

		case 'E': {
			// specify initial absolute seek END time (trick play), using a string of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z"
			initialAbsoluteSeekEndTime = argv[2];
			++argv; --argc;
			break;
				  }
		case 'z': { // scale (trick play)
			float arg;
			if (sscanf(argv[2], "%g", &arg) != 1 || arg == 0.0f) {
				usage();
			}
			scale = arg;
			++argv; --argc;
			break;
				  }

		case 'R': {
			// set up a handler server for incoming "REGISTER" commands
			createHandlerServerForREGISTERCommand = True;
			if (argc > 2 && argv[2][0] != '-') {
				// The next argument is the REGISTER handler server port number:
				if (sscanf(argv[2], "%hu", &handlerServerForREGISTERCommandPortNum) == 1 && handlerServerForREGISTERCommandPortNum > 0) {
					++argv; --argc;
					break;
				}
			}
			break;
				  }

		case 'C': {
			forceMulticastOnUnspecified = True;
			break;
				  }

		default: {
			*env << "Invalid option: " << opt << "\n";
			usage();
			break;
				 }
		}

		++argv; --argc;
	}

	// There must be exactly one "rtsp://" URL at the end (unless '-R' was used, in which case there's no URL)
	if (!( (argc == 2 && !createHandlerServerForREGISTERCommand) || (argc == 1 && createHandlerServerForREGISTERCommand) )) usage();
	if (outputQuickTimeFile && outputAVIFile) 
	{
		*env << "The -i and -q (or -4) options cannot both be used!\n";
		usage();
	}

	Boolean outputCompositeFile = outputQuickTimeFile || outputAVIFile;
	if (!createReceivers && (outputCompositeFile || oneFilePerFrame || fileOutputInterval > 0)) 
	{
		*env << "The -r option cannot be used with -q, -4, -i, -m, or -P!\n";
		usage();
	}

	if (oneFilePerFrame && fileOutputInterval > 0) 
	{
		*env << "The -m and -P options cannot both be used!\n";
		usage();
	}

	if (outputCompositeFile && !movieWidthOptionSet) 
	{
		*env << "Warning: The -q, -4 or -i option was used, but not -w.  Assuming a video width of "
			<< movieWidth << " pixels\n";
	}

	if (outputCompositeFile && !movieHeightOptionSet) 
	{
		*env << "Warning: The -q, -4 or -i option was used, but not -h.  Assuming a video height of "
			<< movieHeight << " pixels\n";
	}

	if (outputCompositeFile && !movieFPSOptionSet) 
	{
		*env << "Warning: The -q, -4 or -i option was used, but not -f.  Assuming a video frame rate of "
			<< movieFPS << " frames-per-second\n";
	}

	if (audioOnly && videoOnly) 
	{
		*env << "The -a and -v options cannot both be used!\n";
		usage();
	}

	if (sendOptionsRequestOnly && !sendOptionsRequest) 
	{
		*env << "The -o and -O options cannot both be used!\n";
		usage();
	}

	if (initialAbsoluteSeekTime != NULL && initialSeekTime != 0.0f) 
	{
		*env << "The -s and -U options cannot both be used!\n";
		usage();
	}

	if (initialAbsoluteSeekTime == NULL && initialAbsoluteSeekEndTime != NULL) 
	{
		*env << "The -E option requires the -U option!\n";
		usage();
	}

	if (authDBForREGISTER != NULL && !createHandlerServerForREGISTERCommand) 
	{
		*env << "If \"-k <username> <password>\" is used, then -R (or \"-R <port-num>\") must also be used!\n";
		usage();
	}

	if (tunnelOverHTTPPortNum > 0) 
	{
		if (streamUsingTCP) 
		{
			*env << "The -t and -T options cannot both be used!\n";
			usage();
		} 
		else 
		{
			streamUsingTCP = True;
		}
	}

	if (!createReceivers && notifyOnPacketArrival) 
	{
		*env << "Warning: Because we're not receiving stream data, the -n flag has no effect\n";
	}

	if (durationSlop < 0) 
	{
		// This parameter wasn't set, so use a default value.
		// If we're measuring QOS stats, then don't add any slop, to avoid
		// having 'empty' measurement intervals at the end.
		durationSlop = qosMeasurementIntervalMS > 0 ? 0.0 : 5.0;
	}

	//streamURL = argv[1];

	// Create (or arrange to create) our client object:
	if (createHandlerServerForREGISTERCommand) 
	{
		handlerServerForREGISTERCommand
			= HandlerServerForREGISTERCommand::createNew(*env, continueAfterClientCreation0,
			handlerServerForREGISTERCommandPortNum, authDBForREGISTER,
			verbosityLevel, progName);

		if (handlerServerForREGISTERCommand == NULL) 
		{
			*env << "Failed to create a server for handling incoming \"REGISTER\" commands: " << env->getResultMsg() << "\n";
		} 
		else 
		{
			*env << "Awaiting an incoming \"REGISTER\" command on port " << handlerServerForREGISTERCommand->serverPortNum() << "\n";
		}
	} 
	else 
	{
		ourClient = createClient(*env, streamURL, verbosityLevel, progName);
		if (ourClient == NULL) 
		{
			*env << "Failed to create " << clientProtocolName << " client: " << env->getResultMsg() << "\n";
			shutdown();
		}

		continueAfterClientCreation1();
	}

	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(); // does not return

	getchar();

	return 0; // only to prevent compiler warning
}
#endif 

void continueAfterClientCreation0(RTSPClient* newRTSPClient, Boolean requestStreamingOverTCP) 
{
	if (NULL == newRTSPClient) 
	{
		return ;
	}

	streamUsingTCP = requestStreamingOverTCP;

	assignClient(ourClient = newRTSPClient);
	//streamURL = newRTSPClient->url();

	strcpy_s(streamURL, RTSP_URL_LEN, newRTSPClient->url());

	// Having handled one "REGISTER" command (giving us a "rtsp://" URL to stream from), we don't handle any more:
	Medium::close(handlerServerForREGISTERCommand); 
	handlerServerForREGISTERCommand = NULL;

	continueAfterClientCreation1();
}

void continueAfterClientCreation1() 
{
	setUserAgentString(userAgent);

	if (sendOptionsRequest) 
	{
		// Begin by sending an "OPTIONS" command:
		getOptions(continueAfterOPTIONS);
	} 
	else 
	{
		continueAfterOPTIONS(NULL, 0, NULL);
	}
}

void continueAfterClientCreation1(UsageEnvironment& env) 
{
	setUserAgentString(userAgent);

	if (sendOptionsRequest) 
	{
		// Begin by sending an "OPTIONS" command:
		getOptions(continueAfterOPTIONS);
	} 
	else 
	{
		continueAfterOPTIONS(NULL, 0, NULL);
	}
}

void continueAfterOPTIONS(RTSPClient*, int resultCode, char* resultString) 
{
	env->m_eRTSPState = kePxRTSPState_OPTIONS;

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncontinueAfterOPTIONS...\n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	if (sendOptionsRequestOnly) 
	{
		if (resultCode != 0) 
		{
			*env << clientProtocolName << " \"OPTIONS\" request failed: " << resultString << "\n";

			sprintf_s(g_szMessage, 
				      MSG_BUFFER_LEN, 
				      "continueAfterOPTIONS:: clientProtocolName:%s, \"OPTIONS\" request failed:%s \n", 
				      clientProtocolName,
				      resultString);
			::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
			WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
		} 
		else 
		{
			*env << clientProtocolName << " \"OPTIONS\" request returned: " << resultString << "\n";

			sprintf_s(g_szMessage, 
				MSG_BUFFER_LEN, 
				"continueAfterOPTIONS:: clientProtocolName:%s, \"OPTIONS\" request returned:%s \n", 
				clientProtocolName,
				resultString);
			::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
			WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
		}

		shutdown();
	}

	delete[] resultString;
	resultString = NULL;

	// Next, get a SDP description for the stream:
	getSDPDescription(continueAfterDESCRIBE);
}

void continueAfterDESCRIBE(RTSPClient*, int resultCode, char* resultString) 
{
	env->m_eRTSPState = kePxRTSPState_DESCRIBE;

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncontinueAfterDESCRIBE...\n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	if (resultCode != 0) 
	{
		*env << "Failed to get a SDP description for the URL \"" << streamURL << "\": " << resultString << "\n";

		sprintf_s(g_szMessage, 
			MSG_BUFFER_LEN, 
			"continueAfterDESCRIBE:: Failed to get a SDP description for the URL: %s, %s\n", 
			streamURL,
			resultString);
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
		WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

		delete[] resultString;
		resultString = NULL;

		shutdown();
	}

	char* sdpDescription = resultString;
	*env << "\nOpened URL \"" << streamURL << "\", returning a SDP description:\n" << sdpDescription << "\n";

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
		"continueAfterDESCRIBE:: Opened URL %s, returning a SDP description:%s\n",
		streamURL,
		sdpDescription);
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	// Create a media session object from this SDP description:
	session = MediaSession::createNew(*env, sdpDescription);
	delete[] sdpDescription;
	if (session == NULL) 
	{
		*env << "Failed to create a MediaSession object from the SDP description: " << env->getResultMsg() << "\n";

		sprintf_s(g_szMessage, 
			MSG_BUFFER_LEN, 
			"continueAfterDESCRIBE:: Failed to create a MediaSession object from the SDP description: %s\n", 
			env->getResultMsg());
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
		WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
		
		shutdown();
	} 
	else if (!session->hasSubsessions()) 
	{
		*env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";

		sprintf_s(g_szMessage, 
			      MSG_BUFFER_LEN, 
			      "continueAfterDESCRIBE:: This session has no media subsessions (i.e., no \"m=\" lines)\n");
		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
		WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
		
		shutdown();
	}

	// Then, setup the "RTPSource"s for the session:
	MediaSubsessionIterator iter(*session);

	MediaSubsession *subsession    = NULL;
	Boolean madeProgress           = False;
	char const* singleMediumToTest = singleMedium;

	while ((subsession = iter.next()) != NULL) 
	{
		// If we've asked to receive only a single medium, then check this now:
		if (singleMediumToTest != NULL) 
		{
			if (strcmp(subsession->mediumName(), singleMediumToTest) != 0) 
			{
				*env << "Ignoring \"" << subsession->mediumName()
					<< "/" << subsession->codecName()
					<< "\" subsession, because we've asked to receive a single " << singleMedium
					<< " session only\n";

				sprintf_s(g_szMessage, 
					MSG_BUFFER_LEN, 
					"continueAfterDESCRIBE:: Ignoring %s/%s subsession, because we've asked to receive a single %s session only\n",
					subsession->mediumName(),
					subsession->codecName(),
					singleMedium);
				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

				continue;
			} 
			else 
			{
				// Receive this subsession only
				singleMediumToTest = "xxxxx";
				// this hack ensures that we get only 1 subsession of this type
			}
		}

		if (desiredPortNum != 0) 
		{
			subsession->setClientPortNum(desiredPortNum);
			desiredPortNum += 2;
		}

		if (createReceivers) 
		{
			if (!subsession->initiate(simpleRTPoffsetArg)) 
			{
				*env << "Unable to create receiver for \"" << subsession->mediumName()
					<< "/" << subsession->codecName()
					<< "\" subsession: " << env->getResultMsg() << "\n";

				sprintf_s(g_szMessage, 
					MSG_BUFFER_LEN, 
					"continueAfterDESCRIBE:: Unable to create receiver for %s/%s, subsession: %s\n",
					subsession->mediumName(),
					subsession->codecName(),
					env->getResultMsg());
				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
			} 
			else 
			{
				*env << "Created receiver for \"" << subsession->mediumName()
					<< "/" << subsession->codecName() << "\" subsession (";

				if (subsession->rtcpIsMuxed()) 
				{
					*env << "client port " << subsession->clientPortNum();
					sprintf_s(g_szMessage, 
						MSG_BUFFER_LEN, 
						"continueAfterDESCRIBE:: Created receiver for %s/%s subsession (client port: %u)\n",
						subsession->mediumName(),
						subsession->codecName(),
						subsession->clientPortNum());
				} 
				else 
				{
					*env << "client ports " << subsession->clientPortNum()
						<< "-" << subsession->clientPortNum()+1;

					sprintf_s(g_szMessage, 
						MSG_BUFFER_LEN, 
						"continueAfterDESCRIBE:: Created receiver for %s/%s subsession (client port: %u-%u)\n",
						subsession->mediumName(),
						subsession->codecName(),
						subsession->clientPortNum(),
						subsession->clientPortNum()+1);
				}

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

				*env << ")\n";
				madeProgress = True;

				if (subsession->rtpSource() != NULL) 
				{
					// Because we're saving the incoming data, rather than playing
					// it in real time, allow an especially large time threshold
					// (1 second) for reordering misordered incoming packets:
					unsigned const thresh = 1000000; // 1 second
					subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);

					// Set the RTP source's OS socket buffer size as appropriate - either if we were explicitly asked (using -B),
					// or if the desired FileSink buffer size happens to be larger than the current OS socket buffer size.
					// (The latter case is a heuristic, on the assumption that if the user asked for a large FileSink buffer size,
					// then the input data rate may be large enough to justify increasing the OS socket buffer size also.)
					int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
					unsigned curBufferSize = getReceiveBufferSize(*env, socketNum);

					if (socketInputBufferSize > 0 || fileSinkBufferSize > curBufferSize) 
					{
						unsigned newBufferSize = socketInputBufferSize > 0 ? socketInputBufferSize : fileSinkBufferSize;
						newBufferSize = setReceiveBufferTo(*env, socketNum, newBufferSize);

						if (socketInputBufferSize > 0) 
						{ // The user explicitly asked for the new socket buffer size; announce it:
							*env << "Changed socket receive buffer size for the \""
								<< subsession->mediumName()
								<< "/" << subsession->codecName()
								<< "\" subsession from "
								<< curBufferSize << " to "
								<< newBufferSize << " bytes\n";

							sprintf_s(g_szMessage, 
								MSG_BUFFER_LEN, 
								"continueAfterDESCRIBE:: Changed socket receive buffer size for the %s/%s subsession from %u to %u bytes\n",
								subsession->mediumName(),
								subsession->codecName(),
								curBufferSize,
								newBufferSize);

							::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);

							WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
						}
					}
				}
			}
		} 
		else 
		{
			if (subsession->clientPortNum() == 0) 
			{
				*env << "No client port was specified for the \""
					<< subsession->mediumName()
					<< "/" << subsession->codecName()
					<< "\" subsession.  (Try adding the \"-p <portNum>\" option.)\n";

				sprintf_s(g_szMessage, 
					MSG_BUFFER_LEN, 
					"continueAfterDESCRIBE:: No client port was specified for the %s/%s ubsession.  (Try adding the \"-p <portNum>\" option.)\n",
					subsession->mediumName(),
					subsession->codecName());

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);

				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
			} 
			else 
			{
				madeProgress = True;
			}
		}
	}

	if (!madeProgress) 
	{
		shutdown();
	}

	// Perform additional 'setup' on each subsession, before playing them:
	setupStreams();
}

MediaSubsession *subsession = NULL;
Boolean madeProgress = False;

void continueAfterSETUP(RTSPClient* client, int resultCode, char* resultString) 
{
	env->m_eRTSPState = kePxRTSPState_SETUP;

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncontinueAfterSETUP...\n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	if (resultCode == 0) 
	{
		*env << "Setup \"" << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< "\" subsession (";

		if (subsession->rtcpIsMuxed()) 
		{
			*env << "client port " << subsession->clientPortNum();

			sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
				"continueAfterSETUP:: Setup %s/%s subsession(client port: %u)\n",
				subsession->mediumName(),
				subsession->codecName(),
				subsession->clientPortNum());
		} 
		else 
		{
			*env << "client ports " << subsession->clientPortNum()
				<< "-" << subsession->clientPortNum()+1;

			sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
				"continueAfterSETUP:: Setup %s/%s subsession(client port: %u-%u)\n",
				subsession->mediumName(),
				subsession->codecName(),
				subsession->clientPortNum(),
				subsession->clientPortNum()+1);
		}

		*env << ")\n";

		madeProgress = True;
	} 
	else 
	{
		*env << "Failed to setup \"" << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< "\" subsession: " << resultString << "\n";

		sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
			"continueAfterSETUP::  Failed to setup %s/%s subsession: %s \n",
			subsession->mediumName(),
			subsession->codecName(),
			resultString);

	}

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	delete[] resultString;
	resultString = NULL;

	if (client != NULL)
	{
		sessionTimeoutParameter = client->sessionTimeoutParameter();
	}

	// Set up the next subsession, if any:
	setupStreams();
}

void createOutputFiles(char const* periodicFilenameSuffix) 
{
	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncreateOutputFiles... \n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	if (outputQuickTimeFile || outputAVIFile) 
	{
		if (periodicFilenameSuffix[0] == '\0') 
		{
			// Normally (unless the '-P <interval-in-seconds>' option was given) we output to 'stdout':
			sprintf(outFileName, "stdout");
		} 
		else 
		{
			if ("" == outFileName)
			{
				// Otherwise output to a type-specific file name, containing "periodicFilenameSuffix":
				char const* prefix = fileNamePrefix[0] == '\0' ? "output" : fileNamePrefix;
				snprintf(outFileName, sizeof outFileName, "%s%s.%s", prefix, periodicFilenameSuffix,
					outputAVIFile ? "avi" : generateMP4Format ? "mp4" : "mov");
			}
		}

		if (outputQuickTimeFile) 
		{
			qtOut = QuickTimeFileSink::createNew(*env, *session, outFileName,
				fileSinkBufferSize,
				movieWidth, movieHeight,
				movieFPS,
				packetLossCompensate,
				syncStreams,
				generateHintTracks,
				generateMP4Format);

			if (qtOut == NULL) 
			{
				*env << "Failed to create a \"QuickTimeFileSink\" for outputting to \""
					<< outFileName << "\": " << env->getResultMsg() << "\n";

				sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
					"createOutputFiles:: Failed to create a \"QuickTimeFileSink\" for outputting to %s:%s \n",
					outFileName,
					env->getResultMsg());

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

				shutdown();
			} 
			else 
			{
				*env << "Outputting to the file: \"" << outFileName << "\"\n";

				sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
					"createOutputFiles:: Outputting to the file: %s \n",
					outFileName);

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
			}

			qtOut->startPlaying(sessionAfterPlaying, NULL);
		} 
		else 
		{ // outputAVIFile
			aviOut = AVIFileSink::createNew(*env, *session, outFileName,
				fileSinkBufferSize,
				movieWidth, movieHeight,
				movieFPS,
				packetLossCompensate);

			if (aviOut == NULL) 
			{
				*env << "Failed to create an \"AVIFileSink\" for outputting to \""
					<< outFileName << "\": " << env->getResultMsg() << "\n";

				shutdown();
			} 
			else 
			{
				*env << "Outputting to the file: \"" << outFileName << "\"\n";
			}

			aviOut->startPlaying(sessionAfterPlaying, NULL);
		}
	} 
	else 
	{
		// Create and start "FileSink"s for each subsession:
		madeProgress = False;
		MediaSubsessionIterator iter(*session);
		while ((subsession = iter.next()) != NULL) 
		{
			if (subsession->readSource() == NULL) continue; // was not initiated

			// Create an output file for each desired stream:
			if (singleMedium == NULL || periodicFilenameSuffix[0] != '\0') 
			{
				// Output file name is
				//     "<filename-prefix><medium_name>-<codec_name>-<counter><periodicFilenameSuffix>"
				static unsigned streamCounter = 0;
				snprintf(outFileName, sizeof outFileName, "%s%s-%s-%d%s",
					fileNamePrefix, subsession->mediumName(),
					subsession->codecName(), ++streamCounter, periodicFilenameSuffix);
			} 
			else 
			{
				// When outputting a single medium only, we output to 'stdout
				// (unless the '-P <interval-in-seconds>' option was given):
				sprintf(outFileName, "stdout");
			}

			FileSink* fileSink = NULL;
			Boolean createOggFileSink = False; // by default

			if (strcmp(subsession->mediumName(), "video") == 0) 
			{
				if (strcmp(subsession->codecName(), "H264") == 0) 
				{
					// For H.264 video stream, we use a special sink that adds 'start codes',
					// and (at the start) the SPS and PPS NAL units:
					/* fileSink = H264VideoFileSink::createNew(*env, outFileName,
					subsession->fmtp_spropparametersets(),
					fileSinkBufferSize, oneFilePerFrame);*/

					// For H.264 video stream, we use a special sink that insert start_codes:     
					unsigned int  num = 0;    
					SPropRecord * sps = parseSPropParameterSets(subsession->fmtp_spropparametersets(),num); 

					fileSink = CPxH264VideoFileSink::createNew(*env, outFileName, 
						subsession->fmtp_spropparametersets(),
						fileSinkBufferSize, oneFilePerFrame); 

					struct  timeval tv={0, 0};    
					unsigned char  start_code[4] = {0x00, 0x00, 0x00, 0x01};
					fileSink->addData(start_code, 4, tv);    
					fileSink->addData(sps[0].sPropBytes, sps[0].sPropLength, tv);    
					fileSink->addData(start_code, 4, tv);    
					fileSink->addData(sps[1].sPropBytes, sps[1].sPropLength, tv);  

					delete [] sps;  
					sps = NULL;
				} 
				else if (strcmp(subsession->codecName(), "H265") == 0) 
				{
					// For H.265 video stream, we use a special sink that adds 'start codes',
					// and (at the start) the VPS, SPS, and PPS NAL units:
					fileSink = H265VideoFileSink::createNew(*env, outFileName,
						subsession->fmtp_spropvps(),
						subsession->fmtp_spropsps(),
						subsession->fmtp_sproppps(),
						fileSinkBufferSize, 
						oneFilePerFrame);
				} 
				else if (strcmp(subsession->codecName(), "THEORA") == 0) 
				{
					createOggFileSink = True;
				}
			} 
			else if (strcmp(subsession->mediumName(), "audio") == 0) 
			{
				if (strcmp(subsession->codecName(), "AMR") == 0 ||
					strcmp(subsession->codecName(), "AMR-WB") == 0) 
				{
					// For AMR audio streams, we use a special sink that inserts AMR frame hdrs:
					fileSink = AMRAudioFileSink::createNew(*env, outFileName, fileSinkBufferSize, oneFilePerFrame);
				} 
				else if (strcmp(subsession->codecName(), "VORBIS") == 0 || strcmp(subsession->codecName(), "OPUS") == 0) 
				{
					createOggFileSink = True;
				}
				else if (strcmp(subsession->codecName(), "MPEG4-GENERIC") == 0) 
				{
					/*fReadSource = fRTPSource
						= MPEG4GenericRTPSource::createNew(env(), 
						fRTPSocket,
						fRTPPayloadFormat,
						fRTPTimestampFrequency,
						fMediumName, attrVal_strToLower("mode"),
						attrVal_unsigned("sizelength"),
						attrVal_unsigned("indexlength"),
						attrVal_unsigned("indexdeltalength"));*/

					fileSink = CPxAACFileSink::createNew(*env, 
						                                 outFileName,
														 fileSinkBufferSize, 
														 oneFilePerFrame,
														 (char*)subsession->savedSDPLines());
				} 
			}

			if (createOggFileSink) 
			{
				fileSink = OggFileSink
					::createNew(*env, outFileName,
					subsession->rtpTimestampFrequency(), subsession->fmtp_config());
			} 
			else if (fileSink == NULL) 
			{
				// Normal case:
				fileSink = FileSink::createNew(*env, outFileName,
					fileSinkBufferSize, oneFilePerFrame);
			}

			subsession->sink = fileSink;

			if (subsession->sink == NULL) 
			{
				*env << "Failed to create FileSink for \"" << outFileName
					<< "\": " << env->getResultMsg() << "\n";

				sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
					"createOutputFiles:: Failed to create FileSink for %s:%s \n",
					outFileName,
					env->getResultMsg());

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
			} 
			else 
			{
				if (singleMedium == NULL) 
				{
					*env << "Created output file: \"" << outFileName << "\"\n";

					sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
						"createOutputFiles:: Created output file:  %s \n",
						outFileName);
				} 
				else 
				{
					*env << "Outputting data from the \"" << subsession->mediumName()
						<< "/" << subsession->codecName()
						<< "\" subsession to \"" << outFileName << "\"\n";

					sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
						"createOutputFiles:: Outputting data from the %s/%s subsession to %s \n",
						subsession->mediumName(),
						subsession->codecName(),
						outFileName);
				}

				::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
				WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

				if (strcmp(subsession->mediumName(), "video") == 0 &&
					strcmp(subsession->codecName(), "MP4V-ES") == 0 &&
					subsession->fmtp_config() != NULL) 
				{
					// For MPEG-4 video RTP streams, the 'config' information
					// from the SDP description contains useful VOL etc. headers.
					// Insert this data at the front of the output file:
					unsigned configLen;
					unsigned char* configData
						= parseGeneralConfigStr(subsession->fmtp_config(), configLen);
					struct timeval timeNow;
					gettimeofday(&timeNow, NULL);
					fileSink->addData(configData, configLen, timeNow);
					delete[] configData;
				}

				subsession->sink->startPlaying(*(subsession->readSource()),
					subsessionAfterPlaying,
					subsession);

				// Also set a handler to be called if a RTCP "BYE" arrives
				// for this subsession:
				if (subsession->rtcpInstance() != NULL) 
				{
					subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, subsession);
				}

				madeProgress = True;
			}
		}
		if (!madeProgress) shutdown();
	}
}

void createPeriodicOutputFiles() 
{
	// Create a filename suffix that notes the time interval that's being recorded:
	char periodicFileNameSuffix[100] = {0};
	/*snprintf(periodicFileNameSuffix, sizeof periodicFileNameSuffix, "-%05d-%05d",
	fileOutputSecondsSoFar, fileOutputSecondsSoFar + fileOutputInterval);*/
	sprintf(periodicFileNameSuffix, "-%05d-%05d", fileOutputSecondsSoFar, fileOutputSecondsSoFar + fileOutputInterval);

	createOutputFiles(periodicFileNameSuffix);

	// Schedule an event for writing the next output file:
	periodicFileOutputTask = env->taskScheduler().scheduleDelayedTask(fileOutputInterval*1000000,
		                                                             (TaskFunc*)periodicFileOutputTimerHandler,		                                                             (void*)NULL);
}

void setupStreams() 
{
	static MediaSubsessionIterator* setupIter = NULL;
	if (setupIter == NULL) 
	{
		setupIter = new MediaSubsessionIterator(*session);
	}

	while ((subsession = setupIter->next()) != NULL) 
	{
		// We have another subsession left to set up:
		if (subsession->clientPortNum() == 0) continue; // port # was not set

		setupSubsession(subsession, streamUsingTCP, forceMulticastOnUnspecified, continueAfterSETUP);

		return;
	}

	// We're done setting up subsessions.
	delete setupIter;
	setupIter = NULL;

	if (!madeProgress) 
	{
		shutdown();
	}

	// Create output files:
	if (createReceivers) 
	{
		//fileOutputInterval = 50; // add by gzl

		if (fileOutputInterval > 0) 
		{
			createPeriodicOutputFiles();
		} 
		else 
		{
			createOutputFiles("");
		}
	}

	// Finally, start playing each subsession, to start the data flow:
	if (duration == 0) 
	{
		if (scale > 0) 
		{
			duration = session->playEndTime() - initialSeekTime; // use SDP end time
		}
		else if (scale < 0) 
		{
			duration = initialSeekTime;
		}
	}

	if (duration < 0) 
	{
		duration = 0.0;
	}

	endTime = initialSeekTime;
	if (scale > 0) 
	{
		if (duration <= 0) 
		{
			endTime = -1.0f;
		}		
		else 
		{
			endTime = initialSeekTime + duration;
		}			
	} 
	else 
	{
		endTime = initialSeekTime - duration;
		if (endTime < 0) 
		{
			endTime = 0.0f;
		}
	}

	char const* absStartTime = initialAbsoluteSeekTime != NULL ? initialAbsoluteSeekTime : session->absStartTime();
	char const* absEndTime = initialAbsoluteSeekEndTime != NULL ? initialAbsoluteSeekEndTime : session->absEndTime();
	
	if (absStartTime != NULL) 
	{
		// Either we or the server have specified that seeking should be done by 'absolute' time:
		startPlayingSession(session, absStartTime, absEndTime, scale, continueAfterPLAY);
	} 
	else 
	{
		// Normal case: Seek by relative time (NPT):
		startPlayingSession(session, initialSeekTime, endTime, scale, continueAfterPLAY);
	}
}

void continueAfterPLAY(RTSPClient*, int resultCode, char* resultString) 
{
	env->m_eRTSPState = kePxRTSPState_PLAY;

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncontinueAfterPLAY...\n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	if (resultCode != 0) 
	{
		*env << "Failed to start playing session: " << resultString << "\n";

		sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
			"continueAfterPLAY:: Failed to start playing session: %s \n",
			resultString);

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
		WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

		delete[] resultString;
		resultString = NULL;
		shutdown();

		return;
	} 
	else 
	{
		*env << "Started playing session\n";

		sprintf_s(g_szMessage, MSG_BUFFER_LEN, "continueAfterPLAY:: Started playing session\n");

		::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
		WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);
	}

	delete[] resultString;
	resultString = NULL;

	if (qosMeasurementIntervalMS > 0) 
	{
		// Begin periodic QOS measurements:
		beginQOSMeasurement();
	}

	// Figure out how long to delay (if at all) before shutting down, or
	// repeating the playing
	Boolean timerIsBeingUsed = False;
	double secondsToDelay = duration;
	if (duration > 0) {
		// First, adjust "duration" based on any change to the play range (that was specified in the "PLAY" response):
		double rangeAdjustment = (session->playEndTime() - session->playStartTime()) - (endTime - initialSeekTime);
		if (duration + rangeAdjustment > 0.0) duration += rangeAdjustment;

		timerIsBeingUsed = True;
		double absScale = scale > 0 ? scale : -scale; // ASSERT: scale != 0
		secondsToDelay = duration/absScale + durationSlop;

		int64_t uSecsToDelay = (int64_t)(secondsToDelay*1000000.0);
		sessionTimerTask = env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)sessionTimerHandler, (void*)NULL);
	}

	char const* actionString = createReceivers? "Receiving streamed data":"Data is being streamed";

	if (timerIsBeingUsed) 
	{
		*env << actionString
			<< " (for up to " << secondsToDelay
			<< " seconds)...\n";

		sprintf_s(g_szMessage, MSG_BUFFER_LEN, "continueAfterPLAY:: %s (for up to %.2f seconds)...\n", 
			actionString,
			secondsToDelay);
	} 
	else 
	{
#ifdef USE_SIGNALS
		pid_t ourPid = getpid();
		*env << actionString
			<< " (signal with \"kill -HUP " << (int)ourPid
			<< "\" or \"kill -USR1 " << (int)ourPid
			<< "\" to terminate)...\n";
#else
		*env << actionString << "...\n";

		sprintf_s(g_szMessage, MSG_BUFFER_LEN, "continueAfterPLAY:: %s ...\n", 
			actionString);
#endif
	}

	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	sessionTimeoutBrokenServerTask = NULL;

	// Watch for incoming packets (if desired):
	checkForPacketArrival(NULL);
	checkInterPacketGaps(NULL);
	checkSessionTimeoutBrokenServer(NULL);
}

void closeMediaSinks() 
{
	Medium::close(qtOut); qtOut = NULL;
	Medium::close(aviOut); aviOut = NULL;

	if (session == NULL)
	{
		return;
	}

	MediaSubsessionIterator iter(*session);
	MediaSubsession* subsession = NULL;
	while ((subsession = iter.next()) != NULL) 
	{
		Medium::close(subsession->sink);
		subsession->sink = NULL;
	}
}

void subsessionAfterPlaying(void* clientData) 
{
	// Begin by closing this media subsession's stream:
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed
	sessionAfterPlaying();
}

void subsessionByeHandler(void* clientData) 
{
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	unsigned secsDiff = timeNow.tv_sec - startTime.tv_sec;

	MediaSubsession* subsession = (MediaSubsession*)clientData;
	*env << "Received RTCP \"BYE\" on \"" << subsession->mediumName()
		<< "/" << subsession->codecName()
		<< "\" subsession (after " << secsDiff
		<< " seconds)\n";

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, 
		"subsessionByeHandler:: Received RTCP \"BYE\" on %s/%s subsession (after %u seconds)\n", 
		subsession->mediumName(),
		subsession->codecName(),
		secsDiff);


	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	// Act now as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void sessionAfterPlaying(void* /*clientData*/) 
{
	if (!playContinuously) 
	{
		shutdown(0);
	} 
	else 
	{
		// We've been asked to play the stream(s) over again.
		// First, reset state from the current session:
		if (env != NULL) 
		{
			env->taskScheduler().unscheduleDelayedTask(periodicFileOutputTask);
			env->taskScheduler().unscheduleDelayedTask(sessionTimerTask);
			env->taskScheduler().unscheduleDelayedTask(sessionTimeoutBrokenServerTask);
			env->taskScheduler().unscheduleDelayedTask(arrivalCheckTimerTask);
			env->taskScheduler().unscheduleDelayedTask(interPacketGapCheckTimerTask);
			env->taskScheduler().unscheduleDelayedTask(qosMeasurementTimerTask);
		}

		totNumPacketsReceived = ~0;

		startPlayingSession(session, initialSeekTime, endTime, scale, continueAfterPLAY);
	}
}

void sessionTimerHandler(void* /*clientData*/) 
{
	sessionTimerTask = NULL;

	sessionAfterPlaying();
}

void periodicFileOutputTimerHandler(void* /*clientData*/) 
{
	fileOutputSecondsSoFar += fileOutputInterval;

	// First, close the existing output files:
	closeMediaSinks();

	// Then, create new output files:
	createPeriodicOutputFiles();
}

class qosMeasurementRecord {
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

static qosMeasurementRecord* qosRecordHead = NULL;

static void periodicQOSMeasurement(void* clientData); // forward

static unsigned nextQOSMeasurementUSecs;

static void scheduleNextQOSMeasurement() {
	nextQOSMeasurementUSecs += qosMeasurementIntervalMS*1000;
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	unsigned timeNowUSecs = timeNow.tv_sec*1000000 + timeNow.tv_usec;
	int usecsToDelay = nextQOSMeasurementUSecs - timeNowUSecs;

	qosMeasurementTimerTask = env->taskScheduler().scheduleDelayedTask(
		usecsToDelay, (TaskFunc*)periodicQOSMeasurement, (void*)NULL);
}

static void periodicQOSMeasurement(void* /*clientData*/) {
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);

	for (qosMeasurementRecord* qosRecord = qosRecordHead;
		qosRecord != NULL; qosRecord = qosRecord->fNext) {
			qosRecord->periodicQOSMeasurement(timeNow);
	}

	// Do this again later:
	scheduleNextQOSMeasurement();
}

void qosMeasurementRecord
	::periodicQOSMeasurement(struct timeval const& timeNow) {
		unsigned secsDiff = timeNow.tv_sec - measurementEndTime.tv_sec;
		int usecsDiff = timeNow.tv_usec - measurementEndTime.tv_usec;
		double timeDiff = secsDiff + usecsDiff/1000000.0;
		measurementEndTime = timeNow;

		RTPReceptionStatsDB::Iterator statsIter(fSource->receptionStatsDB());
		// Assume that there's only one SSRC source (usually the case):
		RTPReceptionStats* stats = statsIter.next(True);
		if (stats != NULL) {
			double kBytesTotalNow = stats->totNumKBytesReceived();
			double kBytesDeltaNow = kBytesTotalNow - kBytesTotal;
			kBytesTotal = kBytesTotalNow;

			double kbpsNow = timeDiff == 0.0 ? 0.0 : 8*kBytesDeltaNow/timeDiff;
			if (kbpsNow < 0.0) kbpsNow = 0.0; // in case of roundoff error
			if (kbpsNow < kbits_per_second_min) kbits_per_second_min = kbpsNow;
			if (kbpsNow > kbits_per_second_max) kbits_per_second_max = kbpsNow;

			unsigned totReceivedNow = stats->totNumPacketsReceived();
			unsigned totExpectedNow = stats->totNumPacketsExpected();
			unsigned deltaReceivedNow = totReceivedNow - totNumPacketsReceived;
			unsigned deltaExpectedNow = totExpectedNow - totNumPacketsExpected;
			totNumPacketsReceived = totReceivedNow;
			totNumPacketsExpected = totExpectedNow;

			double lossFractionNow = deltaExpectedNow == 0 ? 0.0
				: 1.0 - deltaReceivedNow/(double)deltaExpectedNow;
			//if (lossFractionNow < 0.0) lossFractionNow = 0.0; //reordering can cause
			if (lossFractionNow < packet_loss_fraction_min) {
				packet_loss_fraction_min = lossFractionNow;
			}
			if (lossFractionNow > packet_loss_fraction_max) {
				packet_loss_fraction_max = lossFractionNow;
			}
		}
}

void beginQOSMeasurement() 
{
	// Set up a measurement record for each active subsession:
	struct timeval startTime;
	gettimeofday(&startTime, NULL);
	nextQOSMeasurementUSecs = startTime.tv_sec*1000000 + startTime.tv_usec;
	qosMeasurementRecord* qosRecordTail = NULL;
	MediaSubsessionIterator iter(*session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		RTPSource* src = subsession->rtpSource();
		if (src == NULL) continue;

		qosMeasurementRecord* qosRecord
			= new qosMeasurementRecord(startTime, src);
		if (qosRecordHead == NULL) qosRecordHead = qosRecord;
		if (qosRecordTail != NULL) qosRecordTail->fNext = qosRecord;
		qosRecordTail  = qosRecord;
	}

	// Then schedule the first of the periodic measurements:
	scheduleNextQOSMeasurement();
}

void printQOSData(int exitCode) 
{
	*env << "begin_QOS_statistics\n";

	// Print out stats for each active subsession:
	qosMeasurementRecord* curQOSRecord = qosRecordHead;
	if (session != NULL) {
		MediaSubsessionIterator iter(*session);
		MediaSubsession* subsession;
		while ((subsession = iter.next()) != NULL) {
			RTPSource* src = subsession->rtpSource();
			if (src == NULL) continue;

			*env << "subsession\t" << subsession->mediumName()
				<< "/" << subsession->codecName() << "\n";

			unsigned numPacketsReceived = 0, numPacketsExpected = 0;

			if (curQOSRecord != NULL) {
				numPacketsReceived = curQOSRecord->totNumPacketsReceived;
				numPacketsExpected = curQOSRecord->totNumPacketsExpected;
			}
			*env << "num_packets_received\t" << numPacketsReceived << "\n";
			*env << "num_packets_lost\t" << int(numPacketsExpected - numPacketsReceived) << "\n";

			if (curQOSRecord != NULL) {
				unsigned secsDiff = curQOSRecord->measurementEndTime.tv_sec
					- curQOSRecord->measurementStartTime.tv_sec;
				int usecsDiff = curQOSRecord->measurementEndTime.tv_usec
					- curQOSRecord->measurementStartTime.tv_usec;
				double measurementTime = secsDiff + usecsDiff/1000000.0;
				*env << "elapsed_measurement_time\t" << measurementTime << "\n";

				*env << "kBytes_received_total\t" << curQOSRecord->kBytesTotal << "\n";

				*env << "measurement_sampling_interval_ms\t" << qosMeasurementIntervalMS << "\n";

				if (curQOSRecord->kbits_per_second_max == 0) {
					// special case: we didn't receive any data:
					*env <<
						"kbits_per_second_min\tunavailable\n"
						"kbits_per_second_ave\tunavailable\n"
						"kbits_per_second_max\tunavailable\n";
				} else {
					*env << "kbits_per_second_min\t" << curQOSRecord->kbits_per_second_min << "\n";
					*env << "kbits_per_second_ave\t"
						<< (measurementTime == 0.0 ? 0.0 : 8*curQOSRecord->kBytesTotal/measurementTime) << "\n";
					*env << "kbits_per_second_max\t" << curQOSRecord->kbits_per_second_max << "\n";
				}

				*env << "packet_loss_percentage_min\t" << 100*curQOSRecord->packet_loss_fraction_min << "\n";
				double packetLossFraction = numPacketsExpected == 0 ? 1.0
					: 1.0 - numPacketsReceived/(double)numPacketsExpected;
				if (packetLossFraction < 0.0) packetLossFraction = 0.0;
				*env << "packet_loss_percentage_ave\t" << 100*packetLossFraction << "\n";
				*env << "packet_loss_percentage_max\t"
					<< (packetLossFraction == 1.0 ? 100.0 : 100*curQOSRecord->packet_loss_fraction_max) << "\n";

				RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
				// Assume that there's only one SSRC source (usually the case):
				RTPReceptionStats* stats = statsIter.next(True);
				if (stats != NULL) {
					*env << "inter_packet_gap_ms_min\t" << stats->minInterPacketGapUS()/1000.0 << "\n";
					struct timeval totalGaps = stats->totalInterPacketGaps();
					double totalGapsMS = totalGaps.tv_sec*1000.0 + totalGaps.tv_usec/1000.0;
					unsigned totNumPacketsReceived = stats->totNumPacketsReceived();
					*env << "inter_packet_gap_ms_ave\t"
						<< (totNumPacketsReceived == 0 ? 0.0 : totalGapsMS/totNumPacketsReceived) << "\n";
					*env << "inter_packet_gap_ms_max\t" << stats->maxInterPacketGapUS()/1000.0 << "\n";
				}

				curQOSRecord = curQOSRecord->fNext;
			}
		}
	}

	*env << "end_QOS_statistics\n";
	delete qosRecordHead;
}

Boolean areAlreadyShuttingDown = False;
int shutdownExitCode;
void shutdown(int exitCode) 
{
	if (areAlreadyShuttingDown) return; // in case we're called after receiving a RTCP "BYE" while in the middle of a "TEARDOWN".
	areAlreadyShuttingDown = True;

	shutdownExitCode = exitCode;
	if (env != NULL) 
	{
		env->taskScheduler().unscheduleDelayedTask(periodicFileOutputTask);
		env->taskScheduler().unscheduleDelayedTask(sessionTimerTask);
		env->taskScheduler().unscheduleDelayedTask(sessionTimeoutBrokenServerTask);
		env->taskScheduler().unscheduleDelayedTask(arrivalCheckTimerTask);
		env->taskScheduler().unscheduleDelayedTask(interPacketGapCheckTimerTask);
		env->taskScheduler().unscheduleDelayedTask(qosMeasurementTimerTask);
	}

	if (qosMeasurementIntervalMS > 0) 
	{
		printQOSData(exitCode);
	}

	// Teardown, then shutdown, any outstanding RTP/RTCP subsessions
	Boolean shutdownImmediately = True; // by default
	if (session != NULL) 
	{
		RTSPClient::responseHandler* responseHandlerForTEARDOWN = NULL; // unless:
		if (waitForResponseToTEARDOWN) {
			shutdownImmediately = False;
			responseHandlerForTEARDOWN = continueAfterTEARDOWN;
		}

		tearDownSession(session, responseHandlerForTEARDOWN);
	}

	if (shutdownImmediately) continueAfterTEARDOWN(NULL, 0, NULL);
}

void continueAfterTEARDOWN(RTSPClient*, int /*resultCode*/, char* resultString) 
{
	env->m_eRTSPState = kePxRTSPState_TEARDOWN;

	sprintf_s(g_szMessage, MSG_BUFFER_LEN, "\ncontinueAfterTEARDOWN...\n");
	::PostMessage(g_hAppWnd, WM_ADD_LOG_TO_LIST, NULL, (LPARAM)g_szMessage);
	WriteInfo2File(szRTSPProcessInfoFileName, g_szMessage);

	delete[] resultString;
	resultString = NULL;

	// Now that we've stopped any more incoming data from arriving, close our output files:
	closeMediaSinks();
	Medium::close(session);

	// Finally, shut down our client:
	delete ourAuthenticator;
	delete authDBForREGISTER;
	Medium::close(ourClient);

	// Adios...
	exit(shutdownExitCode);
}

void signalHandlerShutdown(int /*sig*/) 
{
	*env << "Got shutdown signal\n";
	waitForResponseToTEARDOWN = False; // to ensure that we end, even if the server does not respond to our TEARDOWN
	shutdown(0);
}

void checkForPacketArrival(void* /*clientData*/) 
{
	if (!notifyOnPacketArrival) return; // we're not checking

	// Check each subsession, to see whether it has received data packets:
	unsigned numSubsessionsChecked = 0;
	unsigned numSubsessionsWithReceivedData = 0;
	unsigned numSubsessionsThatHaveBeenSynced = 0;

	MediaSubsessionIterator iter(*session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		RTPSource* src = subsession->rtpSource();
		if (src == NULL) continue;
		++numSubsessionsChecked;

		if (src->receptionStatsDB().numActiveSourcesSinceLastReset() > 0) {
			// At least one data packet has arrived
			++numSubsessionsWithReceivedData;
		}
		if (src->hasBeenSynchronizedUsingRTCP()) {
			++numSubsessionsThatHaveBeenSynced;
		}
	}

	unsigned numSubsessionsToCheck = numSubsessionsChecked;
	// Special case for "QuickTimeFileSink"s and "AVIFileSink"s:
	// They might not use all of the input sources:
	if (qtOut != NULL) {
		numSubsessionsToCheck = qtOut->numActiveSubsessions();
	} else if (aviOut != NULL) {
		numSubsessionsToCheck = aviOut->numActiveSubsessions();
	}

	Boolean notifyTheUser;
	if (!syncStreams) {
		notifyTheUser = numSubsessionsWithReceivedData > 0; // easy case
	} else {
		notifyTheUser = numSubsessionsWithReceivedData >= numSubsessionsToCheck
			&& numSubsessionsThatHaveBeenSynced == numSubsessionsChecked;
		// Note: A subsession with no active sources is considered to be synced
	}
	if (notifyTheUser) {
		struct timeval timeNow;
		gettimeofday(&timeNow, NULL);
		char timestampStr[100];
		sprintf(timestampStr, "%ld%03ld", timeNow.tv_sec, (long)(timeNow.tv_usec/1000));
		*env << (syncStreams ? "Synchronized d" : "D")
			<< "ata packets have begun arriving [" << timestampStr << "]\007\n";
		return;
	}

	// No luck, so reschedule this check again, after a delay:
	int uSecsToDelay = 100000; // 100 ms
	arrivalCheckTimerTask
		= env->taskScheduler().scheduleDelayedTask(uSecsToDelay,
		(TaskFunc*)checkForPacketArrival, NULL);
}

void checkInterPacketGaps(void* /*clientData*/) 
{
	if (interPacketGapMaxTime == 0) return; // we're not checking

	// Check each subsession, counting up how many packets have been received:
	unsigned newTotNumPacketsReceived = 0;

	MediaSubsessionIterator iter(*session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		RTPSource* src = subsession->rtpSource();
		if (src == NULL) continue;
		newTotNumPacketsReceived += src->receptionStatsDB().totNumPacketsReceived();
	}

	if (newTotNumPacketsReceived == totNumPacketsReceived) {
		// No additional packets have been received since the last time we
		// checked, so end this stream:
		*env << "Closing session, because we stopped receiving packets.\n";
		interPacketGapCheckTimerTask = NULL;
		sessionAfterPlaying();
	} else {
		totNumPacketsReceived = newTotNumPacketsReceived;
		// Check again, after the specified delay:
		interPacketGapCheckTimerTask
			= env->taskScheduler().scheduleDelayedTask(interPacketGapMaxTime*1000000,
			(TaskFunc*)checkInterPacketGaps, NULL);
	}
}

void checkSessionTimeoutBrokenServer(void* /*clientData*/) 
{
	if (!sendKeepAlivesToBrokenServers) return; // we're not checking

	// Send an "OPTIONS" request, starting with the second call
	if (sessionTimeoutBrokenServerTask != NULL) {
		getOptions(NULL);
	}

	unsigned sessionTimeout = sessionTimeoutParameter == 0 ? 60/*default*/ : sessionTimeoutParameter;
	unsigned secondsUntilNextKeepAlive = sessionTimeout <= 5 ? 1 : sessionTimeout - 5;
	// Reduce the interval a little, to be on the safe side

	sessionTimeoutBrokenServerTask 
		= env->taskScheduler().scheduleDelayedTask(secondsUntilNextKeepAlive*1000000,
		(TaskFunc*)checkSessionTimeoutBrokenServer, NULL);
}
