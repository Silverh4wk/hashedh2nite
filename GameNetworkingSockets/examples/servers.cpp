#include "servers.hpp"
#include "helper.hpp"
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////
//
// ChatServer
//
/////////////////////////////////////////////////////////////////////////////

void ChatServer::Run( uint16 nPort, size_t mxplayers )
{
    // Select instance to use.  For now we'll always use the default.
    // But we could use SteamGameServerNetworkingSockets() on Steam.
    m_pInterface = SteamNetworkingSockets();
    m_maxPlayers = mxplayers;
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
	PollIncomingMessages();
	PollConnectionStateChanges();
	PollLocalUserInput();
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    // Close all the connections
    Printf( "Closing connections...\n" );
    for ( auto it: m_mapClients )
    {
	// Send them one more goodbye message.  Note that we also have the
	// connection close reason as a place to send final data.  However,
	// that's usually best left for more diagnostic/debug text not actual
	// protocol strings.
	SendStringToClient( it.first, "Server is shutting down.  Goodbye." );

	// Close the connection.  We use "linger mode" to ask SteamNetworkingSockets
	// to flush this out and close gracefully.
	m_pInterface->CloseConnection( it.first, 0, "Server Shutdown", true );
    }
    m_mapClients.clear();

    m_pInterface->CloseListenSocket( m_hListenSock );
    m_hListenSock = k_HSteamListenSocket_Invalid;

    m_pInterface->DestroyPollGroup( m_hPollGroup );
    m_hPollGroup = k_HSteamNetPollGroup_Invalid;
}

const size_t ChatServer::  GetMaxPlayers( void )
{
    return m_maxPlayers;
}

void ChatServer:: KickPlayerByName(const std::string& name)
{
    // loop through clients and check their nick
    for ( auto it = m_mapClients.begin(); it != m_mapClients.end(); it++ )
    {
	if ( it->second.m_sNick == name )
	{
	    SendStringToClient(it->first, "You have been kicked. Goodbye Creature.");
	    // At some point probably use an exit code from here: https://partner.steamgames.com/doc/api/steamnetworkingtypes#ESteamNetConnectionEnd
	    // Unsure of the usage of the 4th argument (bEnableLinger)
	    m_pInterface->CloseConnection(it->first, 0, "Kicked", true);


	    SendStringToAllClients((name + " was kicked.").c_str());

	    m_mapClients.erase(it);
	    return;
	}
    }

    Printf("Player not found.");
}



void ChatServer::SendStringToClient( HSteamNetConnection conn, const char *str )
{
    m_pInterface->SendMessageToConnection( conn, str, (uint32)strlen(str), k_nSteamNetworkingSend_Reliable, nullptr );
}

void ChatServer::SendStringToAllClients( const char *str, HSteamNetConnection except )
{
    for ( auto &c: m_mapClients )
    {
	if ( c.first != except )
	    SendStringToClient( c.first, str );
    }
}

void ChatServer::PollIncomingMessages()
{
    char temp[ 1024 ];
    char tempToClient[ 1024 ];	

    while ( !g_bQuit )
    {
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup( m_hPollGroup, &pIncomingMsg, 1 );
	if ( numMsgs == 0 )
	    break;
	if ( numMsgs < 0 )
	    FatalError( "Error checking for messages" );
	assert( numMsgs == 1 && pIncomingMsg );
	auto itClient = m_mapClients.find( pIncomingMsg->m_conn );
	assert( itClient != m_mapClients.end() );

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
	    const char *nick = cmd+5;
	    while ( isspace(*nick) )
		++nick;

	    // Let everybody else know they changed their name
	    itClient->second.player.setName(nick);
	    sprintf( temp, "%s shall henceforth be known as %s", itClient->second.player.getName().c_str(), nick );
	    itClient->second.player.setName(nick);
	    SendStringToAllClients( temp, itClient->first );

	    // Respond to client
	    sprintf( temp, "Ye shall henceforth be known as %s", nick );
	    SendStringToClient( itClient->first, temp );

	    // Actually change their name (We getting rid of this yes ?)
	    SetClientNick( itClient->first, nick );
	    continue;
	}
	if ( (strcmp(cmd, "/ready" )) == 0  && !itClient->second.player.isReady() )
	{
	    if( itClient->second.player.isReady() )
	    {
		SendStringToClient(itClient->first, "Already ready..");

	    }
	    else
	    {
		itClient->second.player.setReady(true);
		numReadied++;
                //(CHANGED) hard coded num of players for now
                SendStringToAllClients(
		    (itClient->second.player.getName() + "has readied up " + std::to_string(GetMaxPlayers() - numReadied) + " remain.").c_str() );
	    }
	    continue; // try suppress local echo :(
	}

	// if no. of players == lobby players, start game..
	if (numReadied == m_maxPlayers)
	{
	    SendStringToAllClients("All players ready! Starting game...");
	    startGame();
	}
			
	// Assume it's just a ordinary chat message, dispatch to everybody else
	sprintf( temp, "%s: %s", itClient->second.m_sNick.c_str(), cmd );
	sprintf(tempToClient, "(you) %s: %s", itClient->second.m_sNick.c_str(), cmd);
	SendStringToAllClients( temp, itClient->first );
	SendStringToClient( itClient->first, tempToClient ); 
    }
}

std::string ChatServer::genWord()
{
    std::string word;
    std::vector<std::string> wordList;
    srand(time(0));
    
    // open file
    // generate word
    // wow
    
    wordList = initWordsList();
    
    size_t numOfWords = wordList.size();
    int indexOfRandom = rand() % numOfWords + 1;            
    return wordList.at(indexOfRandom); 
}

std::vector<std::string> ChatServer::generatePlayerNames(std::string word){
    // grab all names with each letters
    std::vector<std::string> playerNames;
    int howManyNamesWeGot = 0, randomNum, currNameIndex = 0;
    std::vector<char> vec(word.begin(), word.end());

    std::string playerName;
    std::vector<std::string> allNames = initWordsList("player_list.txt");

    while(true){
	if(howManyNamesWeGot == n_players){
	    return playerNames;
	}
	playerName = allNames.at(currNameIndex);
	int randomNum = rand() % n_players + 1;

	// find a letter if it's there delete it !! (this is extremely dumb shoutout hazim
	for(int x = 0; x < vec.size(); x++){
	    if(playerName.find(vec[x])){
		playerNames.push_back(playerName);
		howManyNamesWeGot++;
		vec.erase(vec.begin()+x);
	    }
	    currNameIndex++;
	}
    }
}

void ChatServer::setEveryoneNick(std::vector<std::string> &players){
    
    for(int x = 0; x < n_players; x++){
	auto it = m_mapClients.begin();
	it->second.player.setName(players[x]);
	it++; 
    }
}


void ChatServer:: generateRoles(){
    int numOfSpy;
    int randomNum;

    int currSpies = 0;
        
    numOfSpy = (n_players-1)/2;

    for(int x = 0; x<n_players;x++){
	auto it = m_mapClients.begin();
	randomNum = std::rand() % 2; 
            
	if(randomNum == 1 && (currSpies < numOfSpy)){
	    it->second.player.isSpy = 1; 
	    currSpies++;
	}
	else{
	    it->second.player.isSpy = 0;
	}

	it++; 
	
    }

}

void ChatServer::startGame(){
    int node = 0;
    std::vector<std::string> playerNames;
    std::string word = genWord();

    playerNames = generatePlayerNames(word);

    setEveryoneNick(playerNames);

    // get the roles
    generateRoles();

    // tell the roles their role
    //SendString
    SendStringToAllClients("Node: 1");



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

    // Remember their nick
    m_mapClients[hConn].m_sNick = nick;
    
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
	    auto itClient = m_mapClients.find( pInfo->m_hConn );
	    assert( itClient != m_mapClients.end() );

	    // Select appropriate log messages
	    const char *pszDebugLogAction;
	    if ( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
	    {
		pszDebugLogAction = "problem detected locally";
		sprintf( temp, "Alas, %s hath fallen into shadow.  (%s)", itClient->second.m_sNick.c_str(), pInfo->m_info.m_szEndDebug );
	    }
	    else
	    {
		// Note that here we could check the reason code to see if
		// it was a "usual" connection or an "unusual" one.
		pszDebugLogAction = "closed by peer";
		sprintf( temp, "%s hath departed", itClient->second.m_sNick.c_str() );
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


	    m_mapClients.erase( itClient );

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

	assert( m_mapClients.find( pInfo->m_hConn ) == m_mapClients.end() );

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
	sprintf( nick, "BraveWarrior%d", 10000 + ( rand() % 100000 ) );

	// Send them a welcome message
	sprintf( temp, "Welcome, stranger.  Thou art known to us for now as '%s'; upon thine command '/nick' we shall know thee otherwise.", nick ); 
	SendStringToClient( pInfo->m_hConn, temp ); 
		    
	// Also send them a list of everybody who is already connected
	if ( m_mapClients.empty() )
	{
	    SendStringToClient( pInfo->m_hConn, "Thou art utterly alone." ); 
	}
	else
	{
	    sprintf( temp, "%d companions greet you:", (int)m_mapClients.size() ); 
	    for ( auto &c: m_mapClients )
		SendStringToClient( pInfo->m_hConn, c.second.m_sNick.c_str() ); 
	}

	// Let everybody else know who they are for now
	sprintf( temp, "Hark!  A stranger hath joined this merry host.  For now we shall call them '%s'", nick ); 
	SendStringToAllClients( temp, pInfo->m_hConn ); 

	// Add them to the client list, using std::map wacky syntax
	m_mapClients[ pInfo->m_hConn ];
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



/////////////////////////////////////////////////////////////////////////////
//
// ChatClient
//
/////////////////////////////////////////////////////////////////////////////

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

	std::string msg((const char*)pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
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

