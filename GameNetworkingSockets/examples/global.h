#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include "game.h"

#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"

#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

//servers externs
extern bool g_bQuit;
extern bool g_bSuppressPrintf;
extern const uint16 DEFAULT_SERVER_PORT;
extern SteamNetworkingMicroseconds g_logTimeZero; //define this later


/////////////////////////////////////////////////////////////////////////////
//
// Non-blocking console user input.  Sort of.
// Why is this so hard?
//
/////////////////////////////////////////////////////////////////////////////

extern std::mutex mutexUserInputQueue;
extern std::queue< std::string > queueUserInput;
extern std::thread *s_pThreadUserInput ; // set to nullptr
