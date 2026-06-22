#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <algorithm>
#include <random>
#include <chrono>
#include <cctype>
#include <sstream>
#include <fstream>
#include "server.hpp"
#include "helper.hpp"
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////
//
// ChatServer
//
/////////////////////////////////////////////////////////////////////////////


ChatServer::~ChatServer() = default; // delete the game

void ChatServer::Run( uint16 nPort, size_t max_players )
{

    //create the game and init its status
    m_game = std::make_unique < Game >(  );
    m_game->init( max_players );
    m_game->setMaxPlayers( max_players );

    // Select instance to use.  For now we'll always use the default.
    // But we could use SteamGameServerNetworkingSockets() on Steam.
    m_pInterface = SteamNetworkingSockets();
    // Start listening
    
    SteamNetworkingIPAddr serverLocalAddr;
    serverLocalAddr.Clear();
    serverLocalAddr.m_port = nPort;
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
    m_hListenSock = m_pInterface->CreateListenSocketIP( serverLocalAddr, 1, &opt );

    if ( m_hListenSock == k_HSteamListenSocket_Invalid )
        FatalError( "Failed to listen on port %d", nPort );

    m_hPollGroup = m_pInterface->CreatePollGroup();

    if ( m_hPollGroup == k_HSteamNetPollGroup_Invalid )
        FatalError( "Failed to listen on port %d", nPort );
    Printf( "Server listening on port %d\n", nPort );
    
    while ( !g_bQuit )
    {
	char temp[1024];

	const Player* proposer;
	std::string current_proposal_name;
	PollIncomingMessages();
	PollConnectionStateChanges();
	PollLocalUserInput();
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	// Game loop

	// Agent wins
	if(m_game->getState() == STATE_AGENT_WIN){
	    //win(AGENTS);
	}
	// Spy wins
	if(m_game->getState() == STATE_SPY_WIN){
	    //win(SPIES);
	}

	if(m_game->getState() == STATE_START){
	    // get the first playe's position 
	    // modolu of m_players - 1
	    // i didnt read this code, i just changed variable so dont yell at me later
	    int proposing_player_index = m_game->getProposingPlayerIndex();
	    proposer = m_game->getPlayerByIndex(proposing_player_index);
	    if ( proposer ) current_proposal_name = proposer->getName();
	    sprintf(temp, "The player currently proposing is %s", current_proposal_name.c_str()); 
	    SendStringToAllClients(temp);
	    m_game->setState(STATE_PROPOSE);
	    //playerPropose(currentProposalName);
	}


    }

    // Close all the connections
    Printf( "Closing connections...\n" );
    m_game->forEachPlayer([this]( HSteamNetConnection conn, Player& )
    {
        // Send them one more goodbye message.  Note that we also have the
        // connection close reason as a place to send final data.  However,
        // that's usually best left for more diagnostic/debug text not actual
        // protocol strings.
        SendStringToClient( conn, "Server is shutting down.  Goodbye." );

        // Close the connection.  We use "linger mode" to ask SteamNetworkingSockets
        // to flush this out and close gracefully.
        m_pInterface->CloseConnection( conn, 0, "Server Shutdown", true );
    });

    m_pInterface->CloseListenSocket( m_hListenSock );
    m_hListenSock = k_HSteamListenSocket_Invalid;

    m_pInterface->DestroyPollGroup( m_hPollGroup );
    m_hPollGroup = k_HSteamNetPollGroup_Invalid;
}

void ChatServer:: KickPlayerByName(const std::string& name)
{
    HSteamNetConnection conn = m_game->getConnByName( name );
    if ( conn != k_HSteamNetConnection_Invalid )
    {
	SendStringToClient( conn, "You have been kicked. Goodbye Creature.");
	// At some point probably use an exit code from here: https://partner.steamgames.com/doc/api/steamnetworkingtypes#ESteamNetConnectionEnd
	// Unsure of the usage of the 4th argument (bEnableLinger)
	m_pInterface->CloseConnection( conn, 0, "Kicked", true);

	SendStringToAllClients((name + " was kicked.").c_str());

	m_game->removePlayer( conn );
	return;
    }

    Printf("Player not found.");
}


void ChatServer::SendStringToClient( HSteamNetConnection conn, const char *str )
{
    m_pInterface->SendMessageToConnection( conn, str, (uint32)strlen(str), k_nSteamNetworkingSend_Reliable, nullptr );
}

void ChatServer::SendStringToAllClients( const char *str, HSteamNetConnection except )
{
    m_game->forEachPlayer([this, str, except]( HSteamNetConnection conn, Player& )
    {
	if ( conn != except )
	    SendStringToClient( conn, str );
    });
}

void ChatServer::PollIncomingMessages()//checklater
{
    char temp[ 1024 ];
    char tempToClient[ 1024 ];	
    
    static int tallyVoteState = 0;
    static int voteCount = 0;

    while ( !g_bQuit )
    {
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup( m_hPollGroup, &pIncomingMsg, 1 );
	if ( numMsgs == 0 )
	    break;
	if ( numMsgs < 0 )
	    FatalError( "Error checking for messages" );
	assert( numMsgs == 1 && pIncomingMsg );
	HSteamNetConnection hConn = pIncomingMsg->m_conn;
	Player* player = m_game->getPlayerByConn( hConn );
	assert( player != nullptr );

	// '\0'-terminate it to make it easier to parse
	std::string sCmd;
	sCmd.assign( (const char *)pIncomingMsg->m_pData, pIncomingMsg->m_cbSize );
	const char *cmd = sCmd.c_str();

	// We don't need this anymore.
	pIncomingMsg->Release();

	// Check for known commands.  None of this example code is secure or robust.
	// Don't write a real server like this, please.

	if ( strncmp( cmd, "/nick", 5 ) == 0 )
	{
	    const std::string old_nick = player->getName();

	    const char *nick = cmd+5;
	    while ( isspace( *nick ) )
		++nick;

	    // Let everybody else know they changed their name
	    player->setName( nick );
	    sprintf( temp, "%s shall henceforth be known as %s", player->getName().c_str(), nick );
	    SendStringToAllClients( temp, hConn );

	    // Respond to client
	    sprintf( temp, "Ye shall henceforth be known as %s", nick );
	    SendStringToClient( hConn, temp );

	    // Actually change their name (We getting rid of this yes ?)
	    SetClientNick( hConn, nick );
	    continue;
	}
	if ( (strcmp( cmd, "/ready" ) ) == 0 )
	{
	    if( player->isReady() )
	    {
		SendStringToClient(hConn, "Already ready..");
	    }
	    else
	    {
		player->setReady(true);
		m_game->incReadyCount();

        SendStringToAllClients(
		    (player->getName() + "has readied up " + std::to_string(m_game->getMaxPlayers() - m_game->getReadyCount()) + " remain.").c_str() );
	    }
	    continue; // try suppress local echo :(
	}

	// if no. of players == lobby players, start game..
	if ( m_game->getReadyCount() == m_game->getMaxPlayers() )
	{
	    SendStringToAllClients("All players ready! Starting game...");
	    m_game->startGame( this );
	}


 int playerChoice;
    if(m_game->getState() == STATE_PROPOSAL_VOTING_WAIT){
        // unsure if tallyVoteState and voteCount can be moved to be owned by game
        m_game->proposal_voting(player->getName(), cmd, &tallyVoteState, &voteCount, this); 
       }


	// do we supress this if there are players inside a node?
	// and only limit communication to those
	// Assume it's just a ordinary chat message, dispatch to everybody else

	sprintf( temp, "%s: %s", player->getName().c_str(), cmd );
	sprintf(tempToClient, "(you) %s: %s", player->getName().c_str(), cmd);
	SendStringToAllClients( temp, hConn );
	SendStringToClient( hConn, tempToClient ); 
    }
}


void ChatServer::LocalUserInput_Init()
{
	s_pThreadUserInput = new std::thread( []()
	{
		while ( !g_bQuit )
		{
			char szLine[ 4000 ];
			if ( !fgets( szLine, sizeof(szLine), stdin ) )
			{
				// Well, you would hope that you could close the handle
				// from the other thread to trigger this.  Nope.
				if ( g_bQuit )
					return;
				g_bQuit = true;
				Printf( "Failed to read on stdin, quitting\n" );
				break;
			}

			mutexUserInputQueue.lock();
			queueUserInput.push( std::string( szLine ) );
			mutexUserInputQueue.unlock();
		}
	} );
}

void ChatServer::PollLocalUserInput()
{
    std::string cmd;
    while ( !g_bQuit && LocalUserInput_GetNext( cmd ))
    {
        if ( strcmp( cmd.c_str(), "/quit" ) == 0 )
        {
            g_bQuit = true;
            Printf( "Shutting down server" );
            break;
        }

        else if ( strncmp( cmd.c_str(), "/kick", 5 ) == 0 )
        {
            std::istringstream iss(cmd);
            std::string command, target;
	    iss >> command >> target;

	    if ( target.empty() )
	    {
		Printf("Usage: /kick <name>");
		continue;
	    }
	    KickPlayerByName( target );

	}


	// That's the only command we support
	Printf( "The server only knows one command: '/quit'" );
    }
}

void ChatServer::SetClientNick( HSteamNetConnection hConn, const char *nick )
{
    // Set the connection name, too, which is useful for debugging
    m_pInterface->SetConnectionName( hConn, nick );
}

void ChatServer::OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo )
{
    char temp[1024];

    // What's the state of the connection?
    switch ( pInfo->m_info.m_eState )
    {
    case k_ESteamNetworkingConnectionState_None:
	// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
	break;

    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
    {
	// Ignore if they were not previously connected.  (If they disconnected
	// before we accepted the connection.)
	if ( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected )
	{

	    // Locate the client.  Note that it should have been found, because this
	    // is the only codepath where we remove clients (except on shutdown),
	    // and connection change callbacks are dispatched in queue order.
	    Player* player = m_game->getPlayerByConn( pInfo->m_hConn );
	    assert( player != nullptr );

	    // Select appropriate log messages
	    const char *pszDebugLogAction;
	    if ( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
	    {
		pszDebugLogAction = "problem detected locally";
		m_game->removePlayer( pInfo->m_hConn );
                sprintf( temp, "Alas, %s hath fallen into shadow.  (%s)", player->getName().c_str(), pInfo->m_info.m_szEndDebug );
	    }
	    else
	    {
		// Note that here we could check the reason code to see if
		// it was a "usual" connection or an "unusual" one.
		pszDebugLogAction = "closed by peer";
		m_game->removePlayer( pInfo->m_hConn );
		sprintf( temp, "%s hath departed", player->getName().c_str() );
	    }

	    // Spew something to our own log.  Note that because we put their nick
	    // as the connection description, it will show up, along with their
	    // transport-specific data (e.g. their IP address)
	    Printf( "Connection %s %s, reason %d: %s\n",
		    pInfo->m_info.m_szConnectionDescription,
		    pszDebugLogAction,
		    pInfo->m_info.m_eEndReason,
		    pInfo->m_info.m_szEndDebug
		);


	    // Send a message so everybody else knows what happened
	    SendStringToAllClients( temp );
	}
	else
	{
	    assert( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting );
	}

	// Clean up the connection.  This is important!
	// The connection is "closed" in the network sense, but
	// it has not been destroyed.  We must close it on our end, too
	// to finish up.  The reason information do not matter in this case,
	// and we cannot linger because it's already closed on the other end,
	// so we just pass 0's.
	m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
	break;
    }

    case k_ESteamNetworkingConnectionState_Connecting:
    {
	// This must be a new connection

	assert( m_game->getPlayerByConn( pInfo->m_hConn ) == nullptr );

	Printf( "Connection request from %s", pInfo->m_info.m_szConnectionDescription );

	// A client is attempting to connect
	// Try to accept the connection.
	if ( m_pInterface->AcceptConnection( pInfo->m_hConn ) != k_EResultOK )
	{
	    // This could fail.  If the remote host tried to connect, but then
	    // disconnected, the connection may already be half closed.  Just
	    // destroy whatever we have on our side.
	    m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
	    Printf( "Can't accept connection.  (It was already closed?)" );
	    break;
	}

	// Assign the poll group
	if ( !m_pInterface->SetConnectionPollGroup( pInfo->m_hConn, m_hPollGroup ) )
	{
	    m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
	    Printf( "Failed to set poll group?" );
	    break;
	}

	// Generate a random nick.  A random temporary nick
	// is really dumb and not how you would write a real chat server.
	// You would want them to have some sort of signon message,
	// and you would keep their client in a state of limbo (connected,
	// but not logged on) until them.  I'm trying to keep this example
	// code really simple.
	char nick[ 64 ];
    long no_of_currently_joined_players = m_game->getCurrentPlayers() + 1;

	sprintf( nick, "Player%lu", 0 + ( rand() % no_of_currently_joined_players + 1 ) );

	m_game->addPlayer( pInfo->m_hConn, nick );
	// Send them a welcome message
	sprintf( temp, "Welcome, stranger.  Thou art known to us for now as '%s'; upon thine command '/nick' we shall know thee otherwise.", nick ); 
	SendStringToClient( pInfo->m_hConn, temp ); 

	// Also send them a list of everybody who is already connected
	if ( m_game->getCurrentPlayers() == 0 )
	{
	    SendStringToClient( pInfo->m_hConn, "Thou art utterly alone." ); 
	}
	else
	{
	    sprintf( temp, "%d companions greet you:", (int)m_game->getCurrentPlayers() ); 
	    m_game->forEachPlayer([this, pInfo]( HSteamNetConnection conn, Player& player ) {
		if ( conn != pInfo->m_hConn )
		    SendStringToClient( pInfo->m_hConn, player.getName().c_str() );
	    });
	}

	// Let everybody else know who they are for now
	sprintf( temp, "Hark!  A stranger hath joined this merry host.  For now we shall call them '%s'", nick ); 
	SendStringToAllClients( temp, pInfo->m_hConn ); 

	SetClientNick( pInfo->m_hConn, nick );
	break;
    }

    case k_ESteamNetworkingConnectionState_Connected:
	// We will get a callback immediately after accepting the connection.
	// Since we are the server, we can ignore this, it's not news to us.
	break;

    default:
	// Silences -Wswitch
	break;
    }
}


void ChatServer::PollConnectionStateChanges()
{
    s_pCallbackInstance = this;
    m_pInterface->RunCallbacks();
}
