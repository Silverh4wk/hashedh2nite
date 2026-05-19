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
    Printf("hello");
    this->m_name =initName;
}

Player::Player() : m_name("") {}  
void
Player::setName( std::string name) { m_name = name; }

void
Player::setReady( bool ready ) { m_ready = ready; }

void
Player::setRole( TEAMS team ) { role = team; }

void
Player::setConnection( HSteamNetConnection conn ) {
    m_connection = conn;
}

TEAMS
Player::getRole ( void ) const { return role; }

bool
Player::isReady()  const { return m_ready; }

const std::string&
Player::getName() const  { return m_name; }

HSteamNetConnection
Player::getConnection() const {
    return m_connection;
}
    


void
Game::init( size_t max_players )
{
    this->m_max_players = max_players;
    m_players.clear();
}

void
Game::addPlayer( const std::string& name )
{
    m_players.emplace_back( Player(name));
}

void
Game::removePlayer( const std::string& name )
{
    auto it = std::find_if(m_players.begin(), m_players.end(), [&name]( const Player &p )
	{
	    return p.getName() == name;
	});

    if (it != m_players.end()) {
	m_players.erase(it);
	Printf( "Player %s removed from the game \n", name.c_str() );
    } else {
        Printf( "Player %s not found.", name.c_str() );
    }
    
}



Player*
Game::getPlayer(const std::string& name)
{
    auto it = std::find_if(m_players.begin(), m_players.end(), [&name](const Player& p)
    {
        Printf("Comparing: stored='%s' vs searched='%s'\n", p.getName().c_str(), name.c_str());
        return p.getName() == name;
    });

    if (it != m_players.end()) {
        return &(*it);
    } else {
        Printf("Player %s not found.", name.c_str());
        return nullptr;
    }
}

const Player*
Game::getPlayerByIndex(size_t index) const {
    if (index >= m_players.size()) return nullptr;
    return &m_players[index];
}

void
Game::startGame( ChatServer* server )
{
    m_node = 0;   
    m_current_state = STATE_START;
    
    std::vector<std::string> playerNames;
    std::string word = genWord( );

    playerNames = generatePlayerNames( word );

    //setEveryoneNick( server, playerNames );

    // get the roles
    generateRoles( server );

    //server->SendStringToAllClients( currentNode( ) );
    
}

void Game::playerPropose( ChatServer* server, std::string playerName )
{
    char temp[1024];
    int howManyInNode = 2;

    if ( m_node > 2 ) {
        howManyInNode = 3;
    }

    // Iterate and check, if player then send message...
    std::string strTemp = std::string( "%d", howManyInNode );
    sprintf( temp, "Propose %s players", strTemp.c_str( ) );
    auto it = server->m_mapClients.begin( );
    if ( it != server->m_mapClients.end( ) )
        for ( auto &c :server-> m_mapClients ) {
            if ( c.second.c_str( ) == playerName ) {
                server->SendStringToClient( c.first, temp );
                for ( int x = 0; x<howManyInNode; x++ ){
                    sprintf( temp, "Enter the index of player %d", x );
                    server->SendStringToClient( c.first, temp );

                }
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

size_t Game::getMaxPlayers ( void ) const
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


size_t Game::getCurrentn_Players( void ) const
{
    return n_players;    
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
void
Game::setEveryoneNick( ChatServer* server,std::vector<std::string> &players ){
    
    for( int x = 0; x < n_players; x++ ){
        auto it = server->m_mapClients.begin( );
        it->second=  players[ x ];
        it++; 
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
	
	if( currNameIndex++ == n_players ){
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
    if(!((n_players-1) ==2)){
        num_of_spies = ( n_players-1 )/ 2;
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
    for ( size_t i = 0; i < m_players.size( ); ++i ) {

	m_players[i].setRole( roles[i] );
	
	//( TODO: )need a better message
	if ( roles[i] == SPIES ) {
	    sprintf( temp, "Your role is Spy!! You win if the agents cant find the words in %d nodes", m_nodes_agents_can_lose );
	server->SendStringToPlayer( m_players[i].getName( ), temp ); //reminder to set the client side too
	
	}
	else{
	    sprintf( temp, "Your role is Agent!! You win if you figure out the word in less than %d nodes", m_nodes_agents_can_lose );
	    server->SendStringToPlayer( m_players[i].getName( ), temp );
	}

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
    
    size_t numOfWords = wordList.size( );
    int indexOfRandom = rand( ) % numOfWords + 1;            
    return wordList.at( indexOfRandom ) ;     
}
