#ifndef BOARD_FILES_H
#define BOARD_FILES_H

#include "ScrumBoard.h"

struct SavedBoard {
    std::filesystem::path path;
    std::string name;
};

std::string boardName(const ScrumBoard& board, const std::filesystem::path& path);
std::vector<SavedBoard> findSavedBoards(std::string& errorMessage);
std::string saveBoard(const ScrumBoard& board, std::filesystem::path& currentPath);

#endif
