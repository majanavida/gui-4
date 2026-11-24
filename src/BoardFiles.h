#ifndef BOARD_FILES_H
#define BOARD_FILES_H

#include "ScrumBoard.h"

#include <filesystem>
#include <string>

std::string boardName(const ScrumBoard& board, const std::filesystem::path& path);
void saveBoard(const ScrumBoard& board, std::filesystem::path& boardPath);
void loadBoard(ScrumBoard& board, std::filesystem::path& boardPath);

#endif
