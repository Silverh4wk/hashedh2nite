#include "global.h"
#include "helper.hpp"
#include "hashed2nite.h"
#include "servers.hpp"


void
Player::setName( std::string name) { m_name = name; }

void
Player::setReady( bool ready ) { m_ready = ready; }

void
Player::setRole( TEAMS team ) { role = team; }

TEAMS
Player::getRole( void ) { return role; }

bool
Player::isReady()  { return m_ready; }

const std::string&
Player::getName()  { return m_name; }




void
Game::startGame(ChatServer* server)
{
    int node = 0;
    std::vector<std::string> playerNames;
    std::string word = genWord();

    playerNames = generatePlayerNames(word);

    server->setEveryoneNick(playerNames);

    // get the roles
    generateRoles(server);

    // tell the roles their role
    //SendString
    server->SendStringToAllClients("Node: 1");
    
}

void Game::playerPropose(ChatServer* server, std::string playerName)
{
    char temp[1024];
    int howManyInNode = 2
	;

    if (node > 2) {
        howManyInNode = 3;
    }

    // Iterate and check, if player then send message...
    std::string strTemp = std::string("%d", howManyInNode);
    sprintf(temp, "Propose %s players", strTemp.c_str());
    auto it = server->m_mapClients.begin();
    if (it != server->m_mapClients.end())
        for (auto &c :server-> m_mapClients) {
            if (c.second.player->getName() == playerName ) {
                server->SendStringToClient(c.first, temp);
                for (int x = 0; x<howManyInNode; x++){
                    sprintf(temp, "Enter the index of player %d", x);
                    server->SendStringToClient(c.first, temp);

                }
            }
        }    
}

void Game::win(TEAMS team)
{
    if(team == AGENTS){
	// agent won!!! 
    }
    else{
            // spy won!!1
    }   
    
}
const
size_t Game::GetMaxPlayers(void)
{
    return max_players;    
}

const
size_t Game::GetCurrentn_Players(void)
{
    return n_players;    
}

void Game::setEveryoneNick(ChatServer* server,std::vector<std::string> &players){
    
    for(int x = 0; x < n_players; x++){
	auto it = server->m_mapClients.begin();
	it->second.player->setName(players[x]);
	it++; 
    }
}


std::vector<std::string> Game::generatePlayerNames(std::string word){
    
// grab all names with each letters
    std::vector<std::string> playerNames;
    int randomNum, currNameIndex = 0;
    std::vector<char> vec(word.begin(), word.end());

    std::string playerName;
    std::vector<std::string> allNames = initWordsList("player_list.txt");

    while(true){
	
	if(currNameIndex++ == n_players){
	    return playerNames;
	}

	
	int randomNum = rand() % allNames.size() + 1;
	playerName = allNames.at(randomNum);

	// find a letter if it's there delete it !! (this is extremely dumb shoutout hazim
	for(int x = 0; x < vec.size(); x++){
	    if(playerName.find(vec[x])){
		playerNames.push_back(playerName);
		currNameIndex++;
		vec.erase(vec.begin()+x);
	    }
	}
    }
}



void Game::generateRoles(ChatServer* server)
{
    int numOfSpy;
    int randomNum;
    std::string nodeLose; 
    char temp[1024];
    int currSpies = 0;
    
    // bomboclat
    numOfSpy = (n_players-1)/2;
    nodesAgentsCanLose = numOfSpy+1;
	
    for(int x = 0; x<n_players;x++){
	auto it = server->m_mapClients.begin();
	randomNum = std::rand() % 2; 
        
	if(randomNum == 1 && (currSpies < numOfSpy)){
	    it->second.player->setRole(SPIES); 
	    currSpies++;
	    sprintf(temp, "Your role is Spy!! You win if the agents cant find you in %s nodes", nodeLose.c_str());
	    server->SendStringToClient(it->first, temp);

	}
	else{
	    sprintf(temp, "Your role is Agent!! You win if you figure out the word in less than %s nodes", nodeLose.c_str());
	    server->SendStringToClient(it->first, temp);
	    it->second.player->setRole(AGENTS) ;
	}
	it++; 

    }


}

std::string Game::genWord() {
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
