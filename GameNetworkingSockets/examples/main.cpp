
#include "game.h"
#include "helper.hpp"
#include "game.h"
#include "server.hpp"
#include "client.h"
#include "ncursesDisplay.h"
#include <iostream>
    
ChatServer *ChatServer::s_pCallbackInstance = nullptr;
ChatClient *ChatClient::s_pCallbackInstance = nullptr;

// External Variables initializations
const uint16 DEFAULT_SERVER_PORT = 27020;
SteamNetworkingMicroseconds g_logTimeZero; // A local timestamp

bool g_bQuit = false;
bool g_bSuppressPrintf = false;
    
std::mutex mutexUserInputQueue;                     
std::queue< std::string > queueUserInput;           
std::thread *s_pThreadUserInput =nullptr;

static inline void PrintUsageAndExit( int rc = 1 )
{
    fflush(stderr);
    printf(
            R"usage(Usage:
    example_chat client SERVER_ADDR
    example_chat server [--port PORT]
)usage"
          );
    fflush(stdout);
    exit(rc);
}



int main( int argc, const char *argv[] )
{

    //  (NOTE:Hazim) moved all of the external inits here inside main instead of just keeping their scope globally
    size_t n_players; 
    //server global flags 
    bool bServer = false; 
    bool bClient = false;
    
    //port init
    int nPort = DEFAULT_SERVER_PORT;

    SteamNetworkingIPAddr addrServer;
    addrServer.Clear();

    for ( int i = 1 ; i < argc ; ++i )
    {
        if ( !bClient && !bServer )
        {
            if ( !strcmp( argv[i], "client" ) )
            {
                bClient = true;
                continue;
            }
	    
            if ( !strcmp( argv[i], "server" ) )
            {
		bServer = true;
                continue;
            }
        }

        if ( !strcmp( argv[i], "--port" ) )
        {
            ++i;
            if ( i >= argc )
                PrintUsageAndExit();
            nPort = atoi( argv[i] );
            if ( nPort <= 0 || nPort > 65535 )
                FatalError( "Invalid port %d", nPort );
            continue;
        }
        // Anything else, must be server address to connect to
        if ( bClient && addrServer.IsIPv6AllZeros() )
        {
            if ( !addrServer.ParseString( argv[i] ) )
                FatalError( "Invalid server address '%s'", argv[i] );
            if ( addrServer.m_port == 0 )
                addrServer.m_port = DEFAULT_SERVER_PORT;
            continue;
        }

        PrintUsageAndExit();
    }

    if ( bClient == bServer || ( bClient && addrServer.IsIPv6AllZeros() ) )
        PrintUsageAndExit();

    // Create client and server sockets
    InitSteamDatagramConnectionSockets();
    
    if ( bClient )
    {
    //Create a full address string for the ncurses client
    char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
    addrServer.ToString(szAddr, sizeof(szAddr), true);
    RunNcursesFormClient(szAddr);
    return 0; 
    }

    else
    {
     n_players = 0;
    // Lobby Setup
     system("stty sane"); //restore the terminal to a sane state 
     Printf("Number of players: ");
     while (!(std::cin >> n_players)) {
         std::cin.clear();
         std::cin.ignore(10000, '\n');
         Printf("Invalid input. Enter a number:\n");
    }

    Printf("Number of players: %zu\n", n_players);
    ChatServer server;
    server.LocalUserInput_Init();
    server.Run( (uint16)nPort, n_players );
    }
    
    ShutdownSteamDatagramConnectionSockets();

    // Ug, why is there no simple solution for portable, non-blocking console user input?
    // Just nuke the process
    //LocalUserInput_Kill();
    NukeProcess(0);
}
