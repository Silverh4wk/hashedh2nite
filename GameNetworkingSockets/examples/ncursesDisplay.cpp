#include "ncursesDisplay.h"
#include "servers.hpp"
#include "helper.hpp"
#include <curses.h>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

static std::vector<std::string> chatLines;
static std::mutex chatMutex;
static const int MAX_CHAT_LINES = 1000;

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

    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    WINDOW* chatWin  = newwin(maxy - 3, maxx, 0, 0);
    WINDOW* inputWin = newwin(3, maxx, maxy - 3, 0);

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
        mvwprintw(chatWin, 0, 2, " Chat ");

        int chat_h, chat_w;
        getmaxyx(chatWin, chat_h, chat_w);
        int maxVisible = chat_h - 2;
        int start = std::max(0, (int)chatLines.size() - maxVisible);

        {
            std::lock_guard<std::mutex> lock(chatMutex);
            for (int i = 0; i < maxVisible && (start + i) < (int)chatLines.size(); ++i) {
                std::string line = clipToWidth(chatLines[start + i], chat_w - 2);
                mvwprintw(chatWin, i + 1, 1, "%s", line.c_str());
            }
        }
        wrefresh(chatWin);

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
        mvwprintw(inputWin, 0, 2, " Message ");

        int input_w = getmaxx(inputWin);
        std::string visibleInput = clipToWidth(userInput, input_w - 4);
        mvwprintw(inputWin, 1, 1, "> %s", visibleInput.c_str());
        wmove(inputWin, 1, 3 + (int)visibleInput.size());
        wrefresh(inputWin);

        napms(10);
    }

    // --- Cleanup ---
    g_bQuit = true;
    networkThread.join();

    delwin(chatWin);
    delwin(inputWin);
    endwin();
    system("stty sane");   

    ShutdownSteamDatagramConnectionSockets();
}
