#include <algorithm>
#include <chrono>
#include<stdio.h>
#include <string>
#include <vector>
#include <cstring>    
#include <stdarg.h>   
#include <fstream>
#include <mutex>
#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"
#include "global.h"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#ifdef _WIN32
	#include <windows.h> // Ug, for NukeProcess -- see below
#else
	#include <unistd.h>
	#include <signal.h>
#endif



 void Printf( const char *fmt, ... );
 void FatalError(const char *fmt, ...);
 void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType,
                        const char *pszMsg);


// We do this because I won't want to figure out how to cleanly shut
// down the thread that is reading from stdin.
 void inline NukeProcess( int rc )
{
	#ifdef _WIN32
		ExitProcess( rc );
	#else
		(void)rc; // Unused formal parameter
		kill( getpid(), SIGKILL );
	#endif
}

 void inline DebugOutput( ESteamNetworkingSocketsDebugOutputType eType, const char *pszMsg )
{
	SteamNetworkingMicroseconds time = SteamNetworkingUtils()->GetLocalTimestamp() - g_logTimeZero;
	printf( "%10.6f %s\n", time*1e-6, pszMsg );
	fflush(stdout);
	if ( eType == k_ESteamNetworkingSocketsDebugOutputType_Bug )
	{
		fflush(stdout);
		fflush(stderr);
		NukeProcess(1);
	}
}



 void inline Printf( const char *fmt, ... )
{
	char text[ 2048 ];
	va_list ap;
	va_start( ap, fmt );
	vsprintf( text, fmt, ap );
	va_end(ap);
	char *nl = strchr( text, '\0' ) - 1;
	if ( nl >= text && *nl == '\n' )
		*nl = '\0';
	DebugOutput( k_ESteamNetworkingSocketsDebugOutputType_Msg, text );
}

 void inline FatalError( const char *fmt, ... )
{
	char text[ 2048 ];
	va_list ap;
	va_start( ap, fmt );
	vsprintf( text, fmt, ap );
	va_end(ap);
	char *nl = strchr( text, '\0' ) - 1;
	if ( nl >= text && *nl == '\n' )
		*nl = '\0';
	DebugOutput( k_ESteamNetworkingSocketsDebugOutputType_Bug, text );
}

 std::vector<std::string>  inline initWordsList(std::string file_name = "wordlist.txt") {

    std::fstream myfile(file_name.c_str());

   if (myfile.is_open())
   {
      std::string line;
      std::vector<std::string> v;

      while (std::getline(myfile, line))
      {
         v.push_back(line);
      }

      myfile.close();

      Printf( "TOTAL: %d \n" ,v.size());

      for (size_t index { }; index < v.size(); index++)
      {
	  //Printf( "Line # %zu -   %s << \n",(index+1) ,v[index]) ;
      }
      return v;
   }
   else
   {
       Printf("Unable to open file\n");
   }
   
   exit(0);
}


// You really gotta wonder what kind of pedantic garbage was
// going through the minds of people who designed std::string
// that they decided not to include trim.
// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// Read the next line of input from stdin, if anything is available.
inline bool LocalUserInput_GetNext( std::string &result )
{
	bool got_input = false;
	mutexUserInputQueue.lock();
	while ( !queueUserInput.empty() && !got_input )
	{
		result = queueUserInput.front();
		queueUserInput.pop();
		ltrim(result);
		rtrim(result);
		got_input = !result.empty(); // ignore blank lines
	}
	mutexUserInputQueue.unlock();
	return got_input;
}


 

inline void InitSteamDatagramConnectionSockets()
{
	#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
		SteamDatagramErrMsg errMsg;
		if ( !GameNetworkingSockets_Init( nullptr, errMsg ) )
			FatalError( "GameNetworkingSockets_Init failed.  %s", errMsg );
	#else
		SteamDatagram_SetAppID( 570 ); // Just set something, doesn't matter what
		SteamDatagram_SetUniverse( false, k_EUniverseDev );

		SteamDatagramErrMsg errMsg;
		if ( !SteamDatagramClient_Init( errMsg ) )
			FatalError( "SteamDatagramClient_Init failed.  %s", errMsg );

		// Disable authentication when running with Steam, for this
		// example, since we're not a real app.
		//
		// Authentication is disabled automatically in the open-source
		// version since we don't have a trusted third party to issue
		// certs.
		SteamNetworkingUtils()->SetGlobalConfigValueInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1 );
	#endif

	g_logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();

	SteamNetworkingUtils()->SetDebugOutputFunction( k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput );
}

inline void ShutdownSteamDatagramConnectionSockets()
{
	// Give connections time to finish up.  This is an application layer protocol
	// here, it's not TCP.  Note that if you have an application and you need to be
	// more sure about cleanup, you won't be able to do this.  You will need to send
	// a message and then either wait for the peer to close the connection, or
	// you can pool the connection to see if any reliable data is pending.
	std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
		GameNetworkingSockets_Kill();
	#else
		SteamDatagramClient_Kill();
	#endif
}
