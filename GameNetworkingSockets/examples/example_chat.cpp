//====== Copyright Valve Corporation, All rights reserved. ====================
//
// Example client/server chat application using SteamNetworkingSockets


#include "helper.hpp"
#include "servers.hpp"
#include "ncursesDisplay.h"
#include <iostream>


size_t n_players;
struct termios orig_termios;
bool g_bQuit = false;
std::mutex mutexUserInputQueue;
std::queue< std::string > queueUserInput;
std::thread *s_pThreadUserInput = nullptr;
ChatServer *ChatServer::s_pCallbackInstance = nullptr;
ChatClient *ChatClient::s_pCallbackInstance = nullptr;
const uint16 DEFAULT_SERVER_PORT = 27020;
SteamNetworkingMicroseconds g_logTimeZero;
bool g_bSuppressPrintf = false;
int nodesAgentsCanLose = 0; 
int player_currently_proposing = 0;
GAME_STATES CURRENT_STATE;
int node = 0;

void PrintUsageAndExit( int rc = 1 )
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
    /* comment these 2 down.
     Issue:
        Basically when a player types something and presses enter it will show what they typed then what they received from the server (SendStringToClient() ).
        This is to make it the chat uniform. 
        The former is ofc just echo terminal behaviour. 
        3 Options:
            1. No need to have the player get sent what they type (aka a players chat will look like so: 
                Labubu1: Hey hey hey!
                i am typing guys
                Labubu2: Yeah we see you typing
            2. Suppress echo but then that means the player cannot see character by character what they are typing only after they press enter they can see.
            3. Dont suppress echo and still SendStringToClient() so it will be: 
                Labubu1: Erm hello?
                i am typing guys
                (you) Labubu2: i am typing guys
                Labubu3: okay!
    ALLL THIS IS IRRELVANT AND GETS FIXED WHEN/IF WE MOVE TO NCURSES!!!!!!!!!!!!!!!!!!
    */

    bool bServer = false; 
    bool bClient = false;
    int nPort = DEFAULT_SERVER_PORT;
    SteamNetworkingIPAddr addrServer; addrServer.Clear();

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
    //LocalUserInput_Init();
    
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
     system("stty sane");
     Printf("Number of players: ");
     while (!(std::cin >> n_players)) {
         std::cin.clear();
         std::cin.ignore(10000, '\n');
         Printf("Invalid input. Enter a number:\n");
    }

    Printf("Number of players: %zu\n", n_players);
    ChatServer server;
        server.Run( (uint16)nPort, n_players );
    }    ShutdownSteamDatagramConnectionSockets();

    // Ug, why is there no simple solution for portable, non-blocking console user input?
    // Just nuke the process
    //LocalUserInput_Kill();
    NukeProcess(0);
}
