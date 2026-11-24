#ifndef CONSOLE_MENU_H
#define CONSOLE_MENU_H

#include "ScrumBoard.h"

#include <optional>
#include <string>

void configureConsole();
void clearScreen();
bool readLine(const char* prompt, std::string& line);
std::optional<int> readNumber(const char* prompt);

void showBoard(const ScrumBoard& board, const std::string& name);
void assignTask(ScrumBoard& board);
void changeStatus(ScrumBoard& board);

#endif
