#include "ConsoleMenu.h"

#include <iostream>
#include <iterator>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

void configureConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

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

namespace {

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

}

void showBoard(const ScrumBoard& board, const std::string& name) {
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
