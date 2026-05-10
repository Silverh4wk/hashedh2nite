#pragma once

#include <string>
#include <vector>


typedef enum TEAMS{
    AGENTS,
    SPIES,
} TEAMS;

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
    
    void setName( std::string name);
    void setReady( bool ready ) ;
    void setRole( TEAMS team ) ;
    TEAMS getRole( void ) ;
    bool isReady( void ) ;
    const std::string& getName( );
    
private:
    TEAMS role;
    std::string m_name;
    bool m_ready = false;
};

// forward declaration
class ChatServer;

/////////////////////////////////////////////////////////////////////////////
// Game 
/////////////////////////////////////////////////////////////////////////////

class Game
{
public:
    void startGame(ChatServer* server);

    void playerPropose(ChatServer* server,std::string playerName);

    void win(TEAMS team);
    
    const size_t GetMaxPlayers( void );
    
    const size_t GetCurrentn_Players( void );
    
    void setEveryoneNick(ChatServer* server,std::vector<std::string> &players);
	
    std::vector<std::string> generatePlayerNames(std::string word);
   
    void generateRoles(ChatServer * server);
    
    std::string genWord();
    
        
private:
    //ik we can just check the flag inside player
    //but maybe its cleaner to do it this way
    std::vector<std::string> list_of_player;
    std::vector<std::string> list_of_agents;
    std::vector<std::string> list_of_spies;
    size_t n_players;
    size_t max_players;
};
