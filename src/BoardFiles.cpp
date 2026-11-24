#include "BoardFiles.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <string_view>

namespace fs = std::filesystem;

namespace {

const fs::path folder = "boards";

bool isJsonFile(const fs::path& path) {
    std::string extension = path.extension().u8string();
    for (char& letter : extension) {
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }
    return extension == ".json";
}

std::string filenameFromTitle(std::string title) {
    const std::string_view forbidden = "<>:\"/\\|?*";
    for (char& letter : title) {
        if (static_cast<unsigned char>(letter) < 32 || forbidden.find(letter) != std::string_view::npos) {
            letter = '_';
        }
    }

#ifdef _WIN32
    // windows резервирует имена устройств даже с расширением
    std::string device = title.substr(0, title.find('.'));
    const auto lastLetter = device.find_last_not_of(' ');
    if (lastLetter != std::string::npos) {
        device.erase(lastLetter + 1);
    }
    for (char& letter : device) {
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }
    if (device == "con" || device == "prn" || device == "aux" || device == "nul") {
        title.insert(0, "_");
    } else if (device.size() > 3 && (device.compare(0, 3, "com") == 0 || device.compare(0, 3, "lpt") == 0)) {
        const std::string port = device.substr(3);
        if ((port.size() == 1 && port[0] >= '1' && port[0] <= '9') || port == "¹" || port == "²" || port == "³") {
            title.insert(0, "_");
        }
    }
#endif
    return title + ".json";
}

bool createBoardFile(const fs::path& path) {
    // режим wbx создаёт файл только если имя ещё свободно
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

std::vector<SavedBoard> findSavedBoards(std::string& errorMessage) {
    std::vector<SavedBoard> boards;
    std::error_code error;
    for (fs::directory_iterator entry(folder, error), end; entry != end && !error; entry.increment(error)) {
        const fs::path& path = entry->path();
        std::error_code fileError;
        ScrumBoard board;
        if (isJsonFile(path) && entry->is_regular_file(fileError) && board.loadFromFile(path)) {
            boards.push_back({path, boardName(board, path)});
        }
    }
    if (error && error != std::errc::no_such_file_or_directory) {
        errorMessage = "Не удалось прочитать папку boards.";
    }
    std::sort(boards.begin(), boards.end(), [](const SavedBoard& a, const SavedBoard& b) {
        return a.path < b.path;
    });
    return boards;
}

std::string saveBoard(const ScrumBoard& board, fs::path& currentPath) {
    fs::path path = currentPath;
    if (!board.getName().empty() || path.empty()) {
        const std::string title = board.getName().empty() ? "Без названия" : board.getName();
        path = folder / fs::u8path(filenameFromTitle(title));
    }

    std::error_code error;
    fs::create_directories(folder, error);
    if (error) {
        return "Не удалось создать папку boards.";
    }
    const bool newFile = path != currentPath && (currentPath.empty() || !fs::equivalent(path, currentPath, error));
    if (newFile && !createBoardFile(path)) {
        return errno == EEXIST ? "Файл с таким именем уже существует." : "Не удалось создать файл доски.";
    }
    if (!board.saveToFile(path)) {
        if (newFile) {
            fs::remove(path, error);
        }
        return "Не удалось сохранить доску.";
    }
    currentPath = path;
    return {};
}
