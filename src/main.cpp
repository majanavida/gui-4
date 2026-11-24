#include "BoardFiles.h"
#include "ConsoleMenu.h"
#include "ScrumBoard.h"

#include <iostream>

namespace {

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

}

int main() {
    configureConsole();

    ScrumBoard board;
    std::filesystem::path boardPath;
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
                showBoard(board, boardName(board, boardPath));
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
