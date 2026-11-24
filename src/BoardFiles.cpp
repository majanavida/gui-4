#include "BoardFiles.h"
#include "ConsoleMenu.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {

const fs::path boardsFolder = "boards";

bool isJsonFile(const fs::path& path) {
    std::string extension = path.extension().u8string();
    for (char& letter : extension) {
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }
    return extension == ".json";
}

bool createBoardFile(const fs::path& path) {
    // режим wbx создаёт файл только если это имя ещё свободно
#ifdef _WIN32
    std::FILE* file = _wfopen(path.c_str(), L"wbx");
#else
    std::FILE* file = std::fopen(path.c_str(), "wbx");
#endif
    if (!file) {
        return false;
    }
    std::fclose(file);
    return true;
}

}

std::string boardName(const ScrumBoard& board, const fs::path& path) {
    return board.getName().empty() ? path.stem().u8string() : board.getName();
}

void saveBoard(const ScrumBoard& board, fs::path& boardPath) {
    if (!boardPath.empty()) {
        std::cout << "Текущий файл — " << boardPath.filename().u8string() << ". Enter оставит это имя.\n";
    }
    std::string name;
    if (!readLine("Имя файла > ", name)) {
        return;
    }

    fs::path path = boardPath;
    if (!name.empty() || path.empty()) {
        if (name.find_first_not_of(" \t\n\r\f\v") == std::string::npos || name == "." || name == ".."
            || name.find_first_of("<>:\"/\\|?*") != std::string::npos) {
            std::cout << "Введите допустимое имя файла без пути.\n";
            return;
        }
        fs::path fileName = fs::u8path(name);
        if (!isJsonFile(fileName)) {
            fileName += ".json";
        }
        path = boardsFolder / fileName;
    }

    std::error_code error;
    fs::create_directories(boardsFolder, error);
    if (error) {
        std::cout << "Не удалось создать папку boards.\n";
        return;
    }

    const bool newFile = path != boardPath && (boardPath.empty() || !fs::equivalent(path, boardPath, error));
    if (newFile && !createBoardFile(path)) {
        std::cout << (errno == EEXIST ? "Файл с таким именем уже существует.\n" : "Не удалось создать файл доски.\n");
        return;
    }
    if (!board.saveToFile(path)) {
        if (newFile) {
            fs::remove(path, error);
        }
        std::cout << "Не удалось сохранить доску.\n";
        return;
    }

    boardPath = path;
    std::cout << "Доска сохранена в " << path.generic_u8string() << ".\n";
}

void loadBoard(ScrumBoard& board, fs::path& boardPath) {
    std::vector<fs::path> files;
    std::error_code error;
    for (fs::directory_iterator entry(boardsFolder, error), end; entry != end && !error; entry.increment(error)) {
        if (isJsonFile(entry->path())) {
            files.push_back(entry->path());
        }
    }
    if (error && error != std::errc::no_such_file_or_directory) {
        std::cout << "Не удалось прочитать папку boards.\n";
        return;
    }
    std::sort(files.begin(), files.end());

    std::cout << "Сохранённые доски\n";
    std::vector<fs::path> available;
    ScrumBoard saved;
    for (const fs::path& path : files) {
        if (fs::is_regular_file(path, error) && saved.loadFromFile(path)) {
            available.push_back(path);
            std::cout << available.size() << ". " << boardName(saved, path) << '\n';
        }
    }
    if (available.empty()) {
        std::cout << "  (нет сохранённых досок)\n";
        return;
    }

    std::cout << "0. Вернуться в меню\n";
    const auto choice = readNumber("Номер доски > ");
    if (!std::cin || choice == 0) {
        return;
    }
    if (!choice || *choice < 1 || static_cast<std::size_t>(*choice) > available.size()) {
        std::cout << "Неверный номер доски.\n";
        return;
    }

    const fs::path& path = available[*choice - 1];
    if (board.loadFromFile(path)) {
        boardPath = path;
        std::cout << "Загружена доска «" << boardName(board, path) << "».\n";
    } else {
        std::cout << "Не удалось загрузить доску.\n";
    }
}
