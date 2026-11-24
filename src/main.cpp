#include "BoardFiles.h"

#include <ftxui.hpp>

#include <sstream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

enum class Page {
    Main, Board, Create, Developer, Task, Load,
    AssignTask, AssignDeveloper, StatusTask, StatusValue
};

const std::vector<std::string> commands = {
    "1. Создать пустую доску", "2. Посмотреть доску", "3. Добавить разработчика",
    "4. Добавить задачу", "5. Назначить исполнителя", "6. Изменить статус задачи",
    "7. Сохранить доску", "8. Загрузить доску", "0. Выйти"
};

const std::vector<std::string> statuses = {"Assigned", "InProgress", "Blocked", "Done"};

std::string taskLabel(const Task& task) {
    return "#" + std::to_string(task.id) + " " + task.title;
}

std::vector<std::string> boardRows(const ScrumBoard& board) {
    std::vector<std::string> rows;
    for (int i = -1; i < static_cast<int>(statuses.size()); ++i) {
        rows.push_back(i == -1 ? "Backlog" : statuses[i]);
        const auto sectionSize = rows.size();
        for (const Task& task : board.getTasks()) {
            const int status = task.status ? static_cast<int>(*task.status) : -1;
            if (status != i) {
                continue;
            }
            std::string row = "  " + taskLabel(task);
            for (const Developer& developer : board.getDevelopers()) {
                if (developer.id == task.developerId) {
                    row += " — " + developer.name;
                    break;
                }
            }
            rows.push_back(row);
        }
        if (rows.size() == sectionSize) {
            rows.push_back("  (нет задач)");
        }
    }
    return rows;
}

}

int main() {
    using namespace ftxui;

#ifdef _WIN32
    const auto outputCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

    ScrumBoard board;
    std::filesystem::path boardPath;
    auto screen = App::Fullscreen();
    Page page = Page::Main;
    int tab = 0;
    int selected = 0;
    int focused = 0;
    int mainSelection = 0;
    int taskId = 0;
    std::string heading;
    std::string message;
    std::string inputText;
    std::string boardNumber;
    std::vector<std::string> choices = commands;
    std::vector<int> ids;
    std::vector<SavedBoard> savedBoards;

    auto showList = [&](Page next, const std::string& title, std::vector<std::string> items) {
        page = next;
        tab = 0;
        selected = focused = next == Page::Main ? mainSelection : 0;
        heading = title;
        message.clear();
        boardNumber.clear();
        choices = std::move(items);
    };

    auto showMain = [&](const std::string& result = "") {
        showList(Page::Main, "", commands);
        message = result;
    };

    auto showInput = [&](Page next, const std::string& title) {
        page = next;
        tab = 1;
        heading = title;
        inputText.clear();
        message.clear();
    };

    auto showTasks = [&](bool assigned) {
        std::vector<std::string> items;
        ids.clear();
        for (const auto& task : board.getTasks()) {
            if (task.status.has_value() != assigned) {
                continue;
            }
            items.push_back(taskLabel(task));
            if (assigned) {
                items.back() += " — " + statuses[static_cast<int>(*task.status)];
            }
            ids.push_back(task.id);
        }
        if (items.empty()) {
            showMain(assigned ? "Нет назначенных задач." : "В Backlog нет задач.");
            return;
        }
        showList(assigned ? Page::StatusTask : Page::AssignTask,
                 assigned ? "Назначенные задачи" : "Задачи из Backlog", std::move(items));
    };

    auto openSavedBoards = [&] {
        std::string error;
        savedBoards = findSavedBoards(error);
        if (!error.empty() || savedBoards.empty()) {
            showMain(error.empty() ? "Нет сохранённых досок." : error);
            return;
        }
        std::vector<std::string> names;
        for (const SavedBoard& saved : savedBoards) {
            names.push_back(std::to_string(names.size() + 1) + ". " + saved.name);
        }
        names.push_back("0. Вернуться в меню");
        showList(Page::Load, "Сохранённые доски", std::move(names));
    };

    auto loadBoard = [&](int number) {
        if (number == 0) {
            showMain();
        } else if (number < 0 || static_cast<std::size_t>(number) > savedBoards.size()) {
            message = "Неверный номер доски.";
        } else if (board.loadFromFile(savedBoards[number - 1].path)) {
            boardPath = savedBoards[number - 1].path;
            showMain("Загружена доска «" + boardName(board, boardPath) + "».");
        } else {
            message = "Не удалось загрузить доску.";
        }
    };

    auto runCommand = [&](int command) {
        mainSelection = selected;
        switch (command) {
            case 1: showInput(Page::Create, "Название доски"); break;
            case 2: showList(Page::Board, "Задачи доски", boardRows(board)); break;
            case 3: showInput(Page::Developer, "Имя разработчика"); break;
            case 4: showInput(Page::Task, "Название задачи"); break;
            case 5:
                if (board.getDevelopers().empty()) {
                    showMain("Сначала добавьте разработчика.");
                } else {
                    showTasks(false);
                }
                break;
            case 6: showTasks(true); break;
            case 7: {
                const auto error = saveBoard(board, boardPath);
                showMain(error.empty() ? "Доска сохранена в " + boardPath.generic_u8string() + "." : error);
                break;
            }
            case 8: openSavedBoards(); break;
            case 0: screen.Exit(); break;
        }
    };

    auto submitInput = [&] {
        switch (page) {
            case Page::Create:
                if (inputText.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
                    message = "Введите название доски.";
                } else {
                    board = ScrumBoard(inputText);
                    boardPath.clear();
                    showMain("Создана доска «" + board.getName() + "».");
                }
                break;
            case Page::Developer: {
                const int id = board.addDeveloper(inputText);
                if (id == 0) {
                    message = "Введите имя разработчика.";
                } else {
                    showMain("Добавлен разработчик #" + std::to_string(id) + ".");
                }
                break;
            }
            case Page::Task: {
                const int id = board.addTask(inputText);
                if (id == 0) {
                    message = "Введите название задачи.";
                } else {
                    showMain("Добавлена задача #" + std::to_string(id) + " в Backlog.");
                }
                break;
            }
            default: break;
        }
    };

    MenuOption menuOptions;
    menuOptions.focused_entry = &focused;
    menuOptions.on_enter = [&] {
        switch (page) {
            case Page::Main:
                runCommand(selected == 8 ? 0 : selected + 1);
                break;
            case Page::Board:
                showMain();
                break;
            case Page::Load:
                loadBoard(selected == static_cast<int>(savedBoards.size()) ? 0 : selected + 1);
                break;
            case Page::AssignTask: {
                taskId = ids[selected];
                ids.clear();
                std::vector<std::string> developers;
                for (const Developer& developer : board.getDevelopers()) {
                    developers.push_back("#" + std::to_string(developer.id) + " " + developer.name);
                    ids.push_back(developer.id);
                }
                showList(Page::AssignDeveloper, "Выберите исполнителя", std::move(developers));
                break;
            }
            case Page::AssignDeveloper:
                showMain(board.assignTask(taskId, ids[selected]) ? "Задача назначена." : "Не удалось назначить задачу.");
                break;
            case Page::StatusTask:
                taskId = ids[selected];
                showList(Page::StatusValue, "Новый статус", statuses);
                break;
            case Page::StatusValue:
                showMain(board.changeTaskStatus(taskId, static_cast<TaskStatus>(selected))
                         ? "Статус изменён." : "Не удалось изменить статус.");
                break;
            default: break;
        }
    };

    auto menu = Menu(&choices, &selected, menuOptions);
    auto input = Input(&inputText, "");
    auto panels = Container::Tab({menu, input}, &tab);
    auto view = Renderer(panels, [&] {
        const std::string name = boardName(board, boardPath);
        Elements content = {text("Scrum-доска") | bold,
                            text("Текущая доска — " + (name.empty() ? "Без названия" : name)), separator()};
        if (!heading.empty()) {
            content.push_back(text(heading) | bold);
        }
        if (tab == 1) {
            content.push_back(input->Render() | border);
            content.push_back(filler());
        } else {
            content.push_back(menu->Render() | vscroll_indicator | frame | flex);
        }
        if (page == Page::Load) {
            content.push_back(text("Номер доски > " + boardNumber));
        }
        if (!message.empty()) {
            content.push_back(text(message));
        }
        content.push_back(separator());
        std::string help = tab == 1 ? "Enter — подтвердить" : "↑↓ — выбрать  ·  Enter — подтвердить";
        if (page != Page::Main) {
            help += "  ·  Esc — назад";
        }
        content.push_back(text(help) | dim);
        return vbox(std::move(content)) | border;
    });

    view |= CatchEvent([&](Event event) {
        if (event == Event::Escape && page != Page::Main) {
            showMain();
            return true;
        }
        if (event == Event::Return && tab == 1) {
            submitInput();
            return true;
        }
        if (page == Page::Load && !boardNumber.empty()) {
            if (event == Event::Return) {
                std::istringstream inputNumber(boardNumber);
                int number;
                boardNumber.clear();
                loadBoard(inputNumber >> number ? number : -1);
                return true;
            }
            if (event == Event::Backspace) {
                boardNumber.pop_back();
                return true;
            }
            if (event == Event::ArrowUp || event == Event::ArrowDown) {
                boardNumber.clear();
            }
        }
        if (event.is_character() && (page == Page::Main || page == Page::Load)) {
            const auto& key = event.character();
            if (key.size() == 1 && key[0] >= '0' && key[0] <= '9') {
                if (page == Page::Load) {
                    boardNumber += key;
                } else if (key[0] <= '8') {
                    selected = focused = key[0] == '0' ? 8 : key[0] - '1';
                    message.clear();
                } else {
                    message = "Неверный номер команды.";
                }
                return true;
            }
        }
        return false;
    });
    screen.Loop(view);
#ifdef _WIN32
    SetConsoleOutputCP(outputCodePage);
#endif
}
