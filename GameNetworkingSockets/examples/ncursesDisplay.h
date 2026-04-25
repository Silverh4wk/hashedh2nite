#ifndef NCURSES_DISPLAY_H
#define NCURSES_DISPLAY_H
#define FIELD_MAX_CHARS 32

#include <curses.h>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include "example_chat.cpp"

class Client; 

struct _viewwin {
	std::vector<std::string> _fields; 
};

namespace NcursesDisplay {
typedef struct _viewwin viewwin;

void Display(char *&ipAddress, char *&portNum);
void DisplayMessages(WINDOW *window, viewwin *view,  ChatServer *client); 
void DisplayUsers(WINDOW *window, std::map< HSteamNetConnection, struct Client_t >* client); 
void TextBox(viewwin *view);

} // namespace NcursesDisplay

#endif
