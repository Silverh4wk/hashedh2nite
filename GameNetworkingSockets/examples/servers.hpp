#pragma once

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <termios.h> // for terminal/echo suppression
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <cctype>
#include <sstream>
#include <fstream>
#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif


#ifdef _WIN32
	#include <windows.h> // Ug, for NukeProcess -- see below
#else
	#include <unistd.h>
	#include <signal.h>
#endif

//externs
extern SteamNetworkingMicroseconds g_logTimeZero; //define this lates
extern size_t n_players;
extern struct termios orig_termios;
extern bool g_bQuit;
/////////////////////////////////////////////////////////////////////////////
//
// Non-blocking console user input.  Sort of.
// Why is this so hard?
//
/////////////////////////////////////////////////////////////////////////////

extern std::mutex mutexUserInputQueue;
extern std::queue< std::string > queueUserInput;

extern std::thread *s_pThreadUserInput ; // set to nullptr





/////////////////////////////////////////////////////////////////////////////
// Player 
/////////////////////////////////////////////////////////////////////////////

class Player
{
public:
    //Player(std::string name) : m_name(name) {}
    void setName( std::string name) { m_name = name; }
    void setReady( bool ready ) { m_ready = ready; }
    bool isReady() const { return m_ready; }
    bool isSpy;
    const std::string& getName() const { return m_name; }
    
private:
    std::string m_name;
    bool m_ready = false;
    
};


/////////////////////////////////////////////////////////////////////////////
//
// ChatServer
//
/////////////////////////////////////////////////////////////////////////////

class ChatServer
{
public:
    int numReadied = 0;

    static ChatServer  *s_pCallbackInstance;
    
    HSteamListenSocket m_hListenSock;

    HSteamNetPollGroup m_hPollGroup;

    ISteamNetworkingSockets *m_pInterface;

    size_t m_maxPlayers;

    struct Client_t
    {
        std::string m_sNick;
        Player player;
    };

    static void SteamNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo )
	{
	    s_pCallbackInstance->OnSteamNetConnectionStatusChanged( pInfo );
	}
    
    std::map< HSteamNetConnection, Client_t > m_mapClients;

    void Run( uint16 nPort, size_t mxplayers );
    
    const size_t GetMaxPlayers( void );
    
    void KickPlayerByName(const std::string& name);

    void SendStringToClient( HSteamNetConnection conn, const char *str );

    void SendStringToAllClients( const char *str, HSteamNetConnection except = k_HSteamNetConnection_Invalid );
    
    void PollIncomingMessages();
     
    std::vector<std::string> generatePlayerNames(std::string word);
    
    void setEveryoneNick(std::vector<std::string> &players);

    void generateRoles();
    
    std::string genWord();
    
    void startGame();
    
    void PollLocalUserInput();
    
    void SetClientNick( HSteamNetConnection hConn, const char *nick );

    void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo );

    void PollConnectionStateChanges();

    
    };




/////////////////////////////////////////////////////////////////////////////
//
// ChatClient
//
/////////////////////////////////////////////////////////////////////////////

class ChatClient
{
    public:
    HSteamNetConnection m_hConnection;
    ISteamNetworkingSockets *m_pInterface;
    static void SteamNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo )
	{
	    s_pCallbackInstance->OnSteamNetConnectionStatusChanged( pInfo );
	}
    
    static ChatClient *s_pCallbackInstance;
    void Run( const SteamNetworkingIPAddr &serverAddr );
    void PollIncomingMessages();
    void PollLocalUserInput();
    void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo );
    void PollConnectionStateChanges();
};

