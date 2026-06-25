#pragma once


#include <string>
#include <mutex>
#include <queue>
#include <vector>
#include "game.h"
#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"


#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif






/////////////////////////////////////////////////////////////////////////////
//
// ChatClient
//
/////////////////////////////////////////////////////////////////////////////

class ChatClient
{
public:
    // each client hold the player
    Player m_player; //store the player details 
    HSteamNetConnection m_hConnection;
    ISteamNetworkingSockets *m_pInterface;
    
    // for functions that ncurses file rely on
    std::queue<std::string> m_incomingMessages;
    std::queue<std::string> m_outgoingMessages;
    
    std::mutex m_outgoingMutex;
    std::mutex m_incomingMutex;
    
    std::vector<std::string> m_connectedPlayers;
    std::mutex m_playerMutex;
    
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

