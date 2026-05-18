#include "client.h"
#include "helper.hpp"
#include <assert.h>

void ChatClient::Run( const SteamNetworkingIPAddr &serverAddr )
{
    // Select instance to use.  For now we'll always use the default.
    m_pInterface = SteamNetworkingSockets();

    // Start connecting
    char szAddr[ SteamNetworkingIPAddr::k_cchMaxString ];
    serverAddr.ToString( szAddr, sizeof(szAddr), true );
    Printf( "Connecting to chat server at %s", szAddr );
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback );
    m_hConnection = m_pInterface->ConnectByIPAddress( serverAddr, 1, &opt );
    if ( m_hConnection == k_HSteamNetConnection_Invalid )
	FatalError( "Failed to create connection" );

    while ( !g_bQuit )
    {
	PollIncomingMessages();
	PollConnectionStateChanges();
	PollLocalUserInput();
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
}
void ChatClient:: PollIncomingMessages()
{
    while ( !g_bQuit )
    {
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	int numMsgs = m_pInterface->ReceiveMessagesOnConnection( m_hConnection, &pIncomingMsg, 1 );
	if ( numMsgs == 0 )
	    break;
	if ( numMsgs < 0 )
	    FatalError( "Error checking for messages" );
	
	// Just echo anything we get from the server
	//fwrite( pIncomingMsg->m_pData, 1, pIncomingMsg->m_cbSize, stdout );
	fputc( '\n', stdout );

	std::string msg( ( const char* )pIncomingMsg->m_pData, pIncomingMsg->m_cbSize );
	

	// we can set to agents but i dont think its needed as the default role is agent anyway
	
	// if (  msg.find("Your role is Agent") != std::string::npos  )
	//     m_player.setRole(AGENTS);
	
	if (  msg.find("Your role is Spy") != std::string::npos  )
	    m_player.setRole(SPIES);
	
        pushIncomingMessage(msg);   
	// We don't need this anymore.
	pIncomingMsg->Release();
    }
}


void ChatClient::PollLocalUserInput()
{
    std::string cmd;
    while ( popOutgoingMessage(cmd))
    {

	// Check for known commands
	if ( strcmp( cmd.c_str(), "/quit" ) == 0 )
	{
	    g_bQuit = true;
	    Printf( "Disconnecting from chat server" );
		    
	    // Close the connection gracefully.
	    // We use linger mode to ask for any remaining reliable data
	    // to be flushed out.  But remember this is an application
	    // protocol on UDP.  See ShutdownSteamDatagramConnectionSockets
	    m_pInterface->CloseConnection( m_hConnection, 0, "Goodbye", true );
	    break;
	}

	// Anything else, just send it to the server and let them parse it
	m_pInterface->SendMessageToConnection( m_hConnection, cmd.c_str(), (uint32)cmd.length(), k_nSteamNetworkingSend_Reliable, nullptr );
    }
}


void ChatClient::pushIncomingMessage(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_incomingMutex);
    m_incomingMessages.push(msg);
}

bool ChatClient::popIncomingMessage(std::string& outMsg)
{
    std::lock_guard<std::mutex> lock(m_incomingMutex);
    if (m_incomingMessages.empty()) return false;
    outMsg = m_incomingMessages.front();
    m_incomingMessages.pop();
    return true;
}

void ChatClient::pushOutgoingMessage(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_outgoingMutex);
    m_outgoingMessages.push(msg);
}

bool ChatClient::popOutgoingMessage(std::string& outMsg)
{
    std::lock_guard<std::mutex> lock(m_outgoingMutex);
    if (m_outgoingMessages.empty()) return false;
    outMsg = m_outgoingMessages.front();
    m_outgoingMessages.pop();
    return true;
}

// void ChatClient::sendUserMessage(const std::string& msg)
// {
//     if (m_hConnection != k_HSteamNetConnection_Invalid)
//     {
//         m_pInterface->SendMessageToConnection(m_hConnection, msg.c_str(),
//         (uint32)msg.size(),
//                                               k_nSteamNetworkingSend_Reliable,
//                                               nullptr);
//     }
// }
void ChatClient::sendUserMessage(const std::string& msg)
{
    if (m_hConnection != k_HSteamNetConnection_Invalid)
    {
        m_pInterface->SendMessageToConnection(m_hConnection, msg.c_str(), (uint32)msg.size(),
                                              k_nSteamNetworkingSend_Reliable, nullptr);
    }
}


void ChatClient::PollConnectionStateChanges()
{
    s_pCallbackInstance = this;
    m_pInterface->RunCallbacks();
}
 
void ChatClient::OnSteamNetConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t *pInfo )
{
    assert( pInfo->m_hConn == m_hConnection || m_hConnection == k_HSteamNetConnection_Invalid );

    // What's the state of the connection?
    switch ( pInfo->m_info.m_eState )
    {
    case k_ESteamNetworkingConnectionState_None:
	// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
	break;
	
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
    {
	g_bQuit = true;

	// Print an appropriate message
	if ( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting )
	{
	    // Note: we could distinguish between a timeout, a rejected connection,
	    // or some other transport problem.
	    Printf( "We sought the remote host, yet our efforts were met with defeat.  (%s)", pInfo->m_info.m_szEndDebug );
	}
	else if ( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
	{
	    Printf( "Alas, troubles beset us; we have lost contact with the host.  (%s)", pInfo->m_info.m_szEndDebug );
	}
	else
	{
	    // NOTE: We could check the reason code for a normal disconnection
	    Printf( "The host hath bidden us farewell.  (%s)", pInfo->m_info.m_szEndDebug );
	}

	// Clean up the connection.  This is important!
	// The connection is "closed" in the network sense, but
	// it has not been destroyed.  We must close it on our end, too
	// to finish up.  The reason information do not matter in this case,
	// and we cannot linger because it's already closed on the other end,
	// so we just pass 0's.
	m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
	m_hConnection = k_HSteamNetConnection_Invalid;
	break;
    }

    case k_ESteamNetworkingConnectionState_Connecting:
	// We will get this callback when we start connecting.
	// We can ignore this.
	break;

    case k_ESteamNetworkingConnectionState_Connected:
	Printf( "Connected to server OK" );
	break;

    default:
	// Silences -Wswitch
	break;
    }
}


