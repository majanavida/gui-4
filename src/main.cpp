#include "ScrumBoard.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

namespace fs = std::filesystem;

const fs::path boardsFolder = "boards";

struct StatusOption {
    TaskStatus status;
    const char* name;
};

const StatusOption statusOptions[] = {
    {TaskStatus::Assigned, "Assigned"},
    {TaskStatus::InProgress, "InProgress"},
    {TaskStatus::Blocked, "Blocked"},
    {TaskStatus::Done, "Done"}
};

void clearScreen() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(console, &mode) && SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        // стираем строки с начала окна, чтобы старое меню не попадало в историю прокрутки
        std::cout << "\x1b[H\x1b[0J" << std::flush;
        SetConsoleMode(console, mode);
    }
#endif
}

void printMenu(const std::string& currentBoardName) {
    std::cout << "\n=== Scrum-доска ===\n"
              << "Текущая доска — " << (currentBoardName.empty() ? "Без названия" : currentBoardName) << "\n\n";
    std::cout << "1. Создать пустую доску\n"
                 "2. Посмотреть доску\n"
                 "3. Добавить разработчика\n"
                 "4. Добавить задачу\n"
                 "5. Назначить исполнителя\n"
                 "6. Изменить статус задачи\n"
                 "7. Сохранить доску\n"
                 "8. Загрузить доску\n"
                 "0. Выйти\n";
}

bool readLine(const char* prompt, std::string& line) {
    std::cout << prompt << std::flush;
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(console, &mode)) {
        // консоль windows отдаёт символы в UTF-16, а доска хранит их в UTF-8
        std::wstring input;
        wchar_t buffer[4096];
        DWORD count;
        do {
            if (!ReadConsoleW(console, buffer, std::size(buffer), &count, nullptr) || count == 0) {
                std::cin.setstate(std::ios::failbit);
                return false;
            }
            input.append(buffer, count);
        } while (input.back() != L'\n');

        input.pop_back();
        if (!input.empty() && input.back() == L'\r') {
            input.pop_back();
        }
        if (input == L"\x1a") {
            std::cin.setstate(std::ios::failbit);
            return false;
        }

        line.clear();
        if (!input.empty()) {
            const int length = static_cast<int>(input.size());
            const int bytes = WideCharToMultiByte(CP_UTF8, 0, input.data(), length, nullptr, 0, nullptr, nullptr);
            line.resize(bytes);
            WideCharToMultiByte(CP_UTF8, 0, input.data(), length, line.data(), bytes, nullptr, nullptr);
        }
        return true;
    }
#endif
    return static_cast<bool>(std::getline(std::cin, line));
}

std::optional<int> readNumber(const char* prompt) {
    std::string line;
    if (!readLine(prompt, line)) {
        return std::nullopt;
    }

    std::istringstream input(line);
    int number;
    char extra;
    if (input >> number && !(input >> extra)) {
        return number;
    }
    return std::nullopt;
}

bool printTasks(const ScrumBoard& board, std::optional<TaskStatus> status, const char* name) {
    std::cout << name << '\n';
    bool hasTasks = false;
    for (const Task& task : board.getTasks()) {
        if (task.status != status) {
            continue;
        }

        hasTasks = true;
        std::cout << "  #" << task.id << ' ' << task.title;
        if (task.status) {
            for (const Developer& developer : board.getDevelopers()) {
                if (developer.id == task.developerId) {
                    std::cout << " — " << developer.name;
                    break;
                }
            }
        }
        std::cout << '\n';
    }
    if (!hasTasks) {
        std::cout << "  (нет задач)\n";
    }
    return hasTasks;
}

std::string boardName(const ScrumBoard& board, const fs::path& path) {
    return board.getName().empty() ? path.stem().u8string() : board.getName();
}

void showBoard(const ScrumBoard& board, const fs::path& path) {
    const std::string name = boardName(board, path);
    if (!name.empty()) {
        std::cout << "Доска «" << name << "»\n\n";
    }
    printTasks(board, std::nullopt, "Backlog");
    for (const StatusOption& option : statusOptions) {
        printTasks(board, option.status, option.name);
    }
}

void assignTask(ScrumBoard& board) {
    if (!printTasks(board, std::nullopt, "Backlog")) {
        return;
    }

    const auto& developers = board.getDevelopers();
    if (developers.empty()) {
        std::cout << "Сначала добавьте разработчика.\n";
        return;
    }

    std::cout << "Разработчики\n";
    for (const Developer& developer : developers) {
        std::cout << "  #" << developer.id << ' ' << developer.name << '\n';
    }

    const auto taskId = readNumber("ID задачи > ");
    if (!taskId || *taskId <= 0) {
        std::cout << "Введите положительный ID задачи.\n";
        return;
    }

    const auto developerId = readNumber("ID разработчика > ");
    if (developerId && board.assignTask(*taskId, *developerId)) {
        std::cout << "Задача назначена.\n";
    } else {
        std::cout << "Укажите ID задачи из Backlog и существующего разработчика.\n";
    }
}

void changeStatus(ScrumBoard& board) {
    std::cout << "Назначенные задачи\n";
    bool hasAssignedTasks = false;
    for (const Task& task : board.getTasks()) {
        for (const StatusOption& option : statusOptions) {
            if (task.status == option.status) {
                std::cout << "  #" << task.id << ' ' << task.title << " — " << option.name << '\n';
                hasAssignedTasks = true;
                break;
            }
        }
    }
    if (!hasAssignedTasks) {
        std::cout << "  (нет задач)\n";
        return;
    }

    const auto taskId = readNumber("ID задачи > ");
    if (!taskId || *taskId <= 0) {
        std::cout << "Введите положительный ID задачи.\n";
        return;
    }

    const int statusCount = std::size(statusOptions);
    for (int i = 0; i < statusCount; ++i) {
        std::cout << i + 1 << ". " << statusOptions[i].name << '\n';
    }

    const auto choice = readNumber("Номер статуса > ");
    if (!choice || *choice < 1 || *choice > statusCount) {
        std::cout << "Неверный номер статуса.\n";
        return;
    }

    if (board.changeTaskStatus(*taskId, statusOptions[*choice - 1].status)) {
        std::cout << "Статус изменён.\n";
    } else {
        std::cout << "Укажите ID назначенной задачи.\n";
    }
}

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

}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    ScrumBoard board;
    fs::path boardPath;
    while (std::cin) {
        printMenu(boardName(board, boardPath));
        const auto command = readNumber("Выберите команду > ");
        if (!std::cin || command == 0) {
            break;
        }

        clearScreen();
        if (!command) {
            std::cout << "Неверный номер команды.\n";
            continue;
        }

        std::string text;
        switch (*command) {
            case 1:
                if (readLine("Название доски > ", text)) {
                    if (text.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
                        std::cout << "Введите название доски.\n";
                    } else {
                        board = ScrumBoard(text);
                        boardPath.clear();
                        std::cout << "Создана доска «" << board.getName() << "».\n";
                    }
                }
                break;
            case 2:
                showBoard(board, boardPath);
                break;
            case 3: {
                if (readLine("Имя разработчика > ", text)) {
                    const int id = board.addDeveloper(text);
                    if (id == 0) {
                        std::cout << "Введите имя разработчика.\n";
                    } else {
                        std::cout << "Добавлен разработчик #" << id << '\n';
                    }
                }
                break;
            }
            case 4: {
                if (readLine("Название задачи > ", text)) {
                    const int id = board.addTask(text);
                    if (id == 0) {
                        std::cout << "Введите название задачи.\n";
                    } else {
                        std::cout << "Добавлена задача #" << id << " в Backlog.\n";
                    }
                }
                break;
            }
            case 5:
                assignTask(board);
                break;
            case 6:
                changeStatus(board);
                break;
            case 7:
                saveBoard(board, boardPath);
                break;
            case 8:
                loadBoard(board, boardPath);
                break;
            default:
                std::cout << "Неверный номер команды.\n";
        }
    }
}
