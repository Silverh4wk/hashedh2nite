#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif





//externs
extern int node;
extern std::thread *s_pThreadUserInput;
extern const uint16 DEFAULT_SERVER_PORT;
extern SteamNetworkingMicroseconds g_logTimeZero; //define this lates
extern size_t n_players;
extern struct termios orig_termios;
extern bool g_bQuit;
extern const char* firstGuy;
extern const char* secondGuy;
extern bool g_bSuppressPrintf;
extern int nodesAgentsCanLose;
extern int player_currently_proposing;
typedef enum GAME_STATES{
    STATE_TALKING,
    STATE_START,
    STATE_PROPOSAL_VOTING, 
    STATE_LETTER_VOTING,
    STATE_AGENT_WIN,
    STATE_SPY_WIN,
    STATE_PROPOSE,
    STATE_GAMEINIT,
    STATE_PROPOSE_WAIT,
    STATE_PROPOSAL_VOTING_WAIT,
    STATE_PROPOSAL_VOTE_RESOLVE,
} GAME_STATES;

extern GAME_STATES CURRENT_STATE;
/////////////////////////////////////////////////////////////////////////////
//
// Non-blocking console user input.  Sort of.
// Why is this so hard?
//
/////////////////////////////////////////////////////////////////////////////

extern std::mutex mutexUserInputQueue;
extern std::queue< std::string > queueUserInput;

extern std::thread *s_pThreadUserInput ; // set to nullptr
