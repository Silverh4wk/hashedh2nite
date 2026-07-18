#include "ncursesDisplay.h"
#include "global.h"
#include "client.h"
#include "server.hpp"
#include "helper.hpp"
#include <curses.h>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

static std::vector<std::string> chatLines;
static std::mutex chatMutex;
static const int MAX_CHAT_LINES = 1000;
static int has_color = false;

// a helper function for the mvwprint() that ncurses uses
// reason: this function can pass a color pair arg,
// allowing us to choose what color pair we want to use

void printToWindow(WINDOW *win, int y, int x,int color_pair,
                  const char *fmt, ...)
{
    va_list argument_pointer;
    va_start(argument_pointer, fmt);

    wmove(win, y, x);
    wattron(win, COLOR_PAIR(color_pair));
    vw_printw(win, fmt, argument_pointer);
    wattroff(win, COLOR_PAIR(color_pair));

    va_end(argument_pointer);
}    

std::string clipToWidth(const std::string& s, int width)
{
    if (width <= 0) return "";
    if ((int)s.size() <= width) return s;
    return s.substr(0, width - 1);
}

void RunNcursesFormClient(const char* serverAddrStr)
{
    SteamNetworkingIPAddr addr;
    if (!addr.ParseString(serverAddrStr)) {
        fprintf(stderr, "Invalid server address\n");
        return;
    }
    if (addr.m_port == 0)
        addr.m_port = DEFAULT_SERVER_PORT;

    g_bSuppressPrintf = true;

    ChatClient client;
    std::thread networkThread([&]() {
        client.Run(addr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // --- ncurses setup ---
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
     if(has_colors() == false)
    {
	printf("This terminal doesn't support color, starting in color less mode");
    }
     else
    {
	start_color();
	init_pair(1, COLOR_YELLOW, 0); // yellow on default
	init_pair(2, COLOR_GREEN, 0); // green on default
    }
     
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    const int PLAYER_WIDTH = 30;
    // we can create windows here, then set them up down probably
    WINDOW* chatWin    = newwin(maxy - 3, maxx - PLAYER_WIDTH, 0, 0);
    WINDOW* playersWin = newwin(maxy - 3, PLAYER_WIDTH, 0, maxx - PLAYER_WIDTH); 
    // or have it grow with each player in length
    
    WINDOW* inputWin   = newwin(3, maxx, maxy - 3, 0);

    scrollok(chatWin, TRUE);
    keypad(inputWin, TRUE);
    wtimeout(inputWin, 10);

    std::string userInput;
    bool running = true;

    while (running && !g_bQuit)
    {
        // ---- Receive messages from server ----
        {
            std::lock_guard<std::mutex> lock(chatMutex);
            std::string msg;
            while (client.popIncomingMessage(msg))
            {
                if (!msg.empty() && msg.back() == '\n')
                    msg.pop_back();
                chatLines.push_back(msg);
                if (chatLines.size() > MAX_CHAT_LINES)
                    chatLines.erase(chatLines.begin());
            }
        }

        // ---- Draw chat window ----
        werase(chatWin); 
        box(chatWin, 0, 0);
        printToWindow(chatWin, 0, 2, 1," Chat ");

        int chat_h, chat_w;
        getmaxyx(chatWin, chat_h, chat_w);
        int maxVisible = chat_h - 2;
        int start = std::max(0, (int)chatLines.size() - maxVisible);

        {
            std::lock_guard<std::mutex> lock(chatMutex);
            for (int i = 0; i < maxVisible && (start + i) < (int)chatLines.size(); ++i) {
                std::string line = clipToWidth(chatLines[start + i], chat_w - 2);

		//if client. message green, else is yellow
		// this also means if someone send (you) will have that message show as green on other clients screen
                if (line.find("(you)") != std::string::npos)
		{
		    printToWindow(chatWin, i + 1, 1, 2,"%s", line.c_str());
		}
		else
		{
		    printToWindow(chatWin, i + 1, 1, 1,"%s", line.c_str());
		}
            }
        }
        wrefresh(chatWin);

        werase(playersWin);
        box(playersWin, 0, 0);
        {
            std::lock_guard<std::mutex> lock(client.m_playerMutex);
            printToWindow(playersWin, 0, 2, 1," Connected Players: (%zu) ", client.m_connectedPlayers.size());

            int ph, pw;
            getmaxyx(playersWin, ph, pw);
            int maxVisible = ph - 2;
            int start = std::max(0, (int)client.m_connectedPlayers.size() - maxVisible);

            for (int i = 0; i < maxVisible && (start + i) < (int)client.m_connectedPlayers.size(); ++i)
            {
                std::string name = clipToWidth(client.m_connectedPlayers[start + i], pw - 2);
		bool is_client = (client.m_player.getName() == name);
		std::string display = is_client ? "(you) " + name : name;
		int color = is_client ? 2 : 1;   // green for this client, yellow for others
		
		printToWindow(playersWin, i + 1, 1, color, "%s", display.c_str());
            }
        }
        wrefresh(playersWin);

        // ---- Input handling ----
        int ch = wgetch(inputWin);
        if (ch != ERR)
        {
            if (ch == '\n' || ch == KEY_ENTER)
            {
                std::string trimmed = userInput;
                // trim spaces
                auto s = std::find_if_not(trimmed.begin(), trimmed.end(), ::isspace);
                auto e = std::find_if_not(trimmed.rbegin(), trimmed.rend(), ::isspace).base();
                trimmed = (s < e) ? std::string(s, e) : "";

                if (!trimmed.empty())
                {
                    if (trimmed == "/quit")
                    {
                        running = false;
                        g_bQuit = true;
                    }
                    else
                    {
                        client.sendUserMessage(trimmed);
                    }
                }
                userInput.clear();
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!userInput.empty())
                    userInput.pop_back();
            }
            else if (ch >= 32 && ch <= 126)
            {
                userInput += (char)ch;
            }
        }

        // ---- Draw input window ----
        werase(inputWin);
        box(inputWin, 0, 0);
        printToWindow(inputWin, 0, 2, 1," Message ");

        int input_w = getmaxx(inputWin);
        std::string visibleInput = clipToWidth(userInput, input_w - 4);
        printToWindow(inputWin, 1, 1, 1,"> %s", visibleInput.c_str());
        wmove(inputWin, 1, 3 + (int)visibleInput.size());
        wrefresh(inputWin);

        napms(10);
    }

    // --- Cleanup ---
    g_bQuit = true;
    networkThread.join();

    delwin(chatWin);
    delwin(playersWin);
    delwin(inputWin);
    endwin();
    system("stty sane");   

    ShutdownSteamDatagramConnectionSockets();
}
