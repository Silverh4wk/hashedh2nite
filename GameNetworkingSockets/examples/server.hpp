#pragma once

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include "../include/steam/steamnetworkingsockets.h"
#include "../include/steam/isteamnetworkingutils.h"


#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif




// Forward Declaration

class Player; 
class Game;

/////////////////////////////////////////////////////////////////////////////
//
// ChatServer
//
/////////////////////////////////////////////////////////////////////////////

class ChatServer
{
public:

    ~ChatServer();
    
    static ChatServer  *s_pCallbackInstance;
    
    HSteamListenSocket m_hListenSock;

    HSteamNetPollGroup m_hPollGroup;

    ISteamNetworkingSockets *m_pInterface;


    
    static void SteamNetConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo )
	{
	    s_pCallbackInstance->OnSteamNetConnectionStatusChanged( pInfo );
	}
 
    void Run( uint16 nPort, size_t max_players );
   
    void KickPlayerByName(const std::string& name);

    void SendStringToClient( HSteamNetConnection conn, const char *str );

    void SendStringToAllClients( const char *str, HSteamNetConnection except = k_HSteamNetConnection_Invalid );
    
    void PollIncomingMessages();
     
    std::vector<std::string> generatePlayerNames(std::string word);
    
    //void setEveryoneNick(std::vector<std::string> &players);

    void PollLocalUserInput();
    
    void SetClientNick( HSteamNetConnection hConn, const char *nick );

    void OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo );

    void PollConnectionStateChanges();

    void LocalUserInput_Init();
private:
    // server hold the game
    std::unique_ptr < Game > m_game;
};

