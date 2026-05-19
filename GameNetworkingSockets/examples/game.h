#pragma once

#include <string>
#include <vector>
#include "../include/steam/isteamnetworkingsockets.h"

typedef enum TEAMS{
    AGENTS,
    SPIES,
} TEAMS;

typedef enum VOTESTATE {
// 2 -> hasnt voted yet
    // 0 -> voted no
    // 1 -> voted yes
    VOTED_NO,  
    VOTED_YES,  
    DIDNT_VOTE,  
}VOTESTATE;
// struct used to track down the game status
typedef enum GAME_STATES{
    STATE_TALKING,
    STATE_START,
    STATE_PROPOSAL_VOTING, 
    STATE_LETTER_VOTING,
    STATE_AGENT_WIN,
    STATE_SPY_WIN,
    STATE_PROPOSE,
    STATE_GAMEINIT,
    STATE_PROPOSE_WAIT,
    STATE_PROPOSAL_VOTING_WAIT,
    STATE_PROPOSAL_VOTE_RESOLVE,
} GAME_STATES;



/////////////////////////////////////////////////////////////////////////////
// Player 
/////////////////////////////////////////////////////////////////////////////

class Player
{
public:
    
    Player();
    Player (std::string);
    VOTESTATE getVote( void ) ;
    void  setVote( VOTESTATE vote ) ;
    void setName( std::string name );
    void setReady( bool ready ) ;
    void setRole( TEAMS team ) ;
    void setConnection( HSteamNetConnection conn ) ;

    TEAMS getRole( void ) const ;
    bool isReady( void ) const ;
    const std::string& getName( ) const;
    HSteamNetConnection getConnection( ) const;
    
private:
    std::string m_name;
    TEAMS role = AGENTS; //just set to agents by default when init a player
    bool m_ready = false;
    VOTESTATE m_vote = DIDNT_VOTE;
    HSteamNetConnection m_connection; 
    
    };

// forward declaration
class ChatServer;
class ChatClient;

/////////////////////////////////////////////////////////////////////////////
// Game 
/////////////////////////////////////////////////////////////////////////////

class Game
{
public:

    void init( size_t max_players );

    void startGame( ChatServer* server );

    void addPlayer( const std::string& name );
    
    void removePlayer( const std::string& name );

    void playerPropose( ChatServer* server,const std::string playerName );

    void win( TEAMS team );
    
    Player* getPlayer( const std::string& name );
    
    size_t getMaxPlayers( void ) const;
    
    void setMaxPlayers( size_t max );
    
    size_t getCurrentn_Players( void ) const;
    
    void setEveryoneNick( ChatServer* server,std::vector<std::string> &players );

    void setState(GAME_STATES state);

    void setProposingPlayerIndex(int index);

    std::vector< std::string > generatePlayerNames( std::string word );
   
    void generateRoles( ChatServer * server );
    
    std::string genWord( );
    
    void  incReadyCount( );
    
    void  decReadyCount( );
    
    size_t getReadyCount( ) const;

    size_t getMaxPlyers( ) const;
    
    size_t getCurrentPlayers( ) const;

    GAME_STATES getState() const ;

    const Player* getPlayerByIndex(size_t index) const;
    
    int getProposingPlayerIndex() const; 

private:
    //ik we can just check the flag inside player
    //but maybe its cleaner to do it this way
    
    std::vector<Player> m_players ;
    std::vector<Player*> m_agents;
    std::vector<Player*> m_spies;

    GAME_STATES m_current_state = STATE_GAMEINIT;
    int m_node = 0;
    int m_nodes_agents_can_lose = 0;
    int m_player_currently_proposing = 0;
    
    const char* first_guy;
    const char* second_guy;
    
    
    size_t n_players = 0;
    size_t n_players_ready = 0;
    size_t m_max_players = 0;

    std::string m_secret_word;
};
