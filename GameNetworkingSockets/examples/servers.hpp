#pragma once

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <map>
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



// Forward Declaration
class Player; 
/////////////////////////////////////////////////////////////////////////////
//
// ChatServer
//
/////////////////////////////////////////////////////////////////////////////

class ChatServer
{
public:

    static ChatServer  *s_pCallbackInstance;
    
    HSteamListenSocket m_hListenSock;

    HSteamNetPollGroup m_hPollGroup;

    ISteamNetworkingSockets *m_pInterface;

    struct Client_t
    {
        std::string m_sNick;
        Player* player;
        // 2 -> hasnt voted yet
        // 0 -> voted no
        // 1 -> voted yes
        int voteState = 2;

    };

    static void SteamNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo )
	{
	    s_pCallbackInstance->OnSteamNetConnectionStatusChanged( pInfo );
	}
    
    std::map< HSteamNetConnection, Client_t > m_mapClients;

    void Run( uint16 nPort, size_t mxplayers );
   
    void KickPlayerByName(const std::string& name);

    void SendStringToClient( HSteamNetConnection conn, const char *str );

    void SendStringToAllClients( const char *str, HSteamNetConnection except = k_HSteamNetConnection_Invalid );
    
    void PollIncomingMessages();
     
    std::vector<std::string> generatePlayerNames(std::string word);
    
    void setEveryoneNick(std::vector<std::string> &players);

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

    // for functions that ncurses file rely on
    std::queue<std::string> m_incomingMessages;
    std::queue<std::string> m_outgoingMessages;

    std::mutex m_outgoingMutex;
    std::mutex m_incomingMutex;
    
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

    // functions for ncurses file
    void pushIncomingMessage(const std::string& msg);
    bool popIncomingMessage(std::string& outMsg);
    void pushOutgoingMessage(const std::string& msg);
    bool popOutgoingMessage(std::string& outMsg);
    
    void sendUserMessage(const std::string& msg);
};

