#include "global.h"
#include "helper.hpp"
#include "game.h"
#include "server.hpp"
#include <algorithm>
#include <cstddef>


VOTESTATE
Player::getVote( void )  { return m_vote;}

void
Player::setVote( VOTESTATE vote ) {this->m_vote = vote;}

Player::Player(std::string initName){
    this->m_name =initName;
    Printf((initName +": hello").c_str());
}

Player::Player() : m_name("") {}  
void
Player::setName( std::string name) { m_name = name; }

void
Player::setReady( bool ready ) { m_ready = ready; }

void
Player::setRole( TEAMS team ) { role = team; }

TEAMS
Player::getRole ( void ) const { return role; }

bool
Player::isReady()  const { return m_ready; }

const std::string&
Player::getName() const  { return m_name; }
    

void
Game::init( size_t max_players )
{
    this->m_max_players = max_players;
    m_players.clear();
}

void
Game::addPlayer( HSteamNetConnection conn, const std::string& name )
{
    m_players[conn] = Player(name);
}

void
Game::removePlayer( HSteamNetConnection conn )
{
    auto it = m_players.find( conn );
    if (it != m_players.end()) {
        std::string name = it->second.getName();
	m_players.erase(it);
	Printf( "Player %s removed from the game \n", name.c_str() );
    } else {
        Printf( "Connection not found." );
    }
    
}


Player*
Game::getPlayerByName(const std::string& name)
{
    for (auto& [conn, player] : m_players)
    {
        Printf("Comparing: stored='%s' vs searched='%s'\n", player.getName().c_str(), name.c_str());
        if (player.getName() == name)
            return &player;
    }
    Printf("Player %s not found.", name.c_str());
    return nullptr;
}

HSteamNetConnection 
Game::findConnectionByName(const std::string& name)
{
    for (auto& [conn, player] : m_players)
    {
        Printf("Comparing: stored='%s' vs searched='%s'\n", player.getName().c_str(), name.c_str());
        if (player.getName() == name)
            return conn;
    }
    Printf("Player connection handle %s not found.", name.c_str());
    return k_HSteamNetConnection_Invalid;
}

const Player*
Game::getPlayerByIndex(size_t index) const {
    if (index >= m_players.size()) return nullptr;
    auto it = m_players.begin();
    std::advance(it, index);
    return &it->second;
}


void
Game::proposal_voting( std::string voter, const char* cmd, int* tallyVoteState, int* voteCount, ChatServer* server ) 
{
    char temp[ 1024 ];
    int playerChoice;

    // find the player
    Player* player = getPlayerByName(voter);

    printf(temp, "%d",std::stoi(strdup(cmd))); 
    Printf(temp);

    if(player->getVote() == 2){
        Printf("%d",std::stoi(strdup(cmd)) );
        playerChoice = std::stoi(strdup(cmd)); 

        if(playerChoice == 1){
            (*tallyVoteState)++;
            (*voteCount)++;
        }
        else if(playerChoice == 0){
            (*tallyVoteState)--;
            (*voteCount)++;

        }
        if(*voteCount == (int)m_players.size()){
            m_current_state = STATE_PROPOSAL_VOTE_RESOLVE; 
        }
        sprintf(temp, "%s voted %d. Currently the tally is %d", voter.c_str(), playerChoice, *tallyVoteState );

        server->SendStringToAllClients(temp);


        playerChoice = -1;


    }
}

void
Game::startGame( ChatServer* server )
{
    m_node = 0;   
    m_current_state = STATE_START;

    std::vector<std::string> playerNames;
    std::string word = genWord( );
    if(!word.empty())
    {
	playerNames = generatePlayerNames( word );
	generateRoles( server );
    }
    //server->SendStringToAllClients( currentNode( ) );

}

void
Game::playerPropose( ChatServer* server, const HSteamNetConnection *hconn )
{
    char temp[1024];
    int howManyInNode = 2;

    if ( m_node > 2 ) {
        howManyInNode = 3;
    }
    // Iterate and check, if player then send message...
    std::string strTemp = std::string( "%d", howManyInNode );
    sprintf( temp, "Propose %s players to enter the node with you >", strTemp.c_str( ) );
    if ( *hconn != k_HSteamNetConnection_Invalid ) {
        server->SendStringToClient( *hconn, temp );
        for ( int x = 0; x < howManyInNode; x++ ){
            sprintf( temp, "Enter the index of player %d", x );
            server->SendStringToClient( *hconn, temp );
        }
    }
}

size_t
Game::getReadyCount( ) const
{
    return n_players_ready;
}

size_t
Game::getCurrentPlayers( ) const
{
    return m_players.size(); 
}

void
Game::win( TEAMS team )
{
    if( team == AGENTS ){
        // agent won!!! 
    }
    else{
        // spy won!!1
    }   

}

size_t
Game::getMaxPlayers ( void ) const
{
    return m_max_players;    
}

void
Game::setMaxPlayers( size_t max )
{
    m_max_players = max;
}

void
Game::setState( GAME_STATES state )
{
    m_current_state = state;
}

void
Game::setProposingPlayerIndex( int index )
{
    m_player_currently_proposing = index;
}


GAME_STATES
Game::getState( ) const{
    return m_current_state;
}

int
Game::getProposingPlayerIndex( ) const
{
    return m_player_currently_proposing;
}

std::map< HSteamNetConnection, Player > &
Game::getPlayersMap() 
{
    return this->m_players;
}

void
Game::setEveryoneNick( ChatServer* server,std::vector<std::string> &players ){
    int x = 0;
    for ( auto& [conn, player] : m_players ) {
        if ( x >= (int)players.size() ) break;
        player.setName( players[ x ] );
        x++;
    }
}


std::vector<std::string>
Game::generatePlayerNames( std::string word ){

    // grab all names with each letters
    std::vector<std::string> playerNames;
    int randomNum, currNameIndex = 0;
    std::vector<char> vec( word.begin( ), word.end( ) );

    std::string playerName;
    std::vector<std::string> allNames = initWordsList( "player_list.txt" );

    while( true ){

        if( currNameIndex++ == (int)m_players.size() ){
            return playerNames;
        }


        int randomNum = rand( ) % allNames.size( ) + 1;
        playerName = allNames.at( randomNum );

        // find a letter if it's there delete it !! ( this is extremely dumb shoutout hazim
        for( int x = 0; x < vec.size( ); x++ ){
            if( playerName.find( vec[ x ] ) ) {
                playerNames.push_back( playerName );
                currNameIndex++;
                vec.erase( vec.begin( )+x );
            }
        }
    }
}



void Game::generateRoles( ChatServer* server )
{
    if ( m_players.empty( ) ) return;

    size_t num_of_spies;
    int random_num;

    char temp[1024];
    size_t curr_spies = 0;

    std::vector<TEAMS> roles( m_players.size( ), AGENTS ); //default is agent

    // bomboclat
    if(!(((int)m_players.size()-1) ==2)){
        num_of_spies = ( m_players.size()-1 )/ 2;
    }
    m_nodes_agents_can_lose = m_spies.size( ) + 1;

    while ( curr_spies < num_of_spies )
    {
        int index = rand( ) % m_players.size( );
        if ( roles[index] != SPIES )
        {
            roles[index] = SPIES;
            curr_spies++;
        }
    }
    int i = 0;
    for ( auto& [conn, player] : m_players ) {

        player.setRole( roles[i] );

        //( TODO: )need a better message
        if ( roles[i] == SPIES ) {
            sprintf( temp, "Your role is Spy!! You win if the agents cant find the words in %d nodes", m_nodes_agents_can_lose );
            server->SendStringToClient( conn, temp ); //reminder to set the client side too

        }
        else{
            sprintf( temp, "Your role is Agent!! You win if you figure out the word in less than %d nodes", m_nodes_agents_can_lose );
            server->SendStringToClient( conn, temp );
        }
        i++;
    }

}

void  Game::incReadyCount( ) { ++n_players_ready; }

void  Game::decReadyCount( ) { --n_players_ready; }

std::string Game::genWord( ) {
    std::string word;
    std::vector<std::string> wordList;
    srand( time( 0 ) );

    // open file
    // generate word
    // wow

    wordList = initWordsList( );
    if(!wordList.empty()){
	size_t numOfWords = wordList.size( );
	int indexOfRandom = rand( ) % numOfWords + 1;
	return wordList.at( indexOfRandom ) ;     
    }
    return {};
}
