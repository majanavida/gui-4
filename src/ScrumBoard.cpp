#include "ScrumBoard.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <utility>

namespace {

struct StatusName {
    std::optional<TaskStatus> status;
    const char* name;
};

const StatusName statusNames[] = {
    {std::nullopt, "Backlog"},
    {TaskStatus::Assigned, "Assigned"},
    {TaskStatus::InProgress, "InProgress"},
    {TaskStatus::Blocked, "Blocked"},
    {TaskStatus::Done, "Done"}
};

}

int ScrumBoard::addDeveloper(const std::string& name) {
    if (name.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
        return 0;
    }

    developers.push_back({nextDeveloperId, name});
    return nextDeveloperId++;
}

int ScrumBoard::addTask(const std::string& title) {
    if (title.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
        return 0;
    }

    tasks.push_back({nextTaskId, title, 0, std::nullopt});
    return nextTaskId++;
}

Task* ScrumBoard::findTask(int taskId) {
    for (Task& task : tasks) {
        if (task.id == taskId) {
            return &task;
        }
    }
    return nullptr;
}

bool ScrumBoard::hasDeveloper(int developerId) const {
    for (const Developer& developer : developers) {
        if (developer.id == developerId) {
            return true;
        }
    }
    return false;
}

bool ScrumBoard::assignTask(int taskId, int developerId) {
    Task* task = findTask(taskId);
    if (task == nullptr || task->status.has_value() || !hasDeveloper(developerId)) {
        return false;
    }

    task->developerId = developerId;
    task->status = TaskStatus::Assigned;
    return true;
}

bool ScrumBoard::changeTaskStatus(int taskId, TaskStatus status) {
    Task* task = findTask(taskId);
    if (task == nullptr || !task->status.has_value()) {
        return false;
    }

    task->status = status;
    return true;
}

bool ScrumBoard::saveToFile(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file << developers.size() << "\r\n";
    for (const Developer& developer : developers) {
        file << developer.id << ' ' << std::quoted(developer.name) << "\r\n";
    }

    file << tasks.size() << "\r\n";
    for (const Task& task : tasks) {
        const char* name = nullptr;
        for (const StatusName& entry : statusNames) {
            if (entry.status == task.status) {
                name = entry.name;
                break;
            }
        }
        if (name == nullptr) {
            return false;
        }
        file << task.id << ' ' << task.developerId << ' ' << name << ' '
             << std::quoted(task.title) << "\r\n";
    }

    file.close();
    return file.good();
}

bool ScrumBoard::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    ScrumBoard board;
    const int maxId = std::numeric_limits<int>::max();
    const char* whitespace = " \t\n\r\f\v";

    int developerCount;
    if (!(file >> developerCount) || developerCount < 0) {
        return false;
    }
    for (int i = 0; i < developerCount; ++i) {
        Developer developer;
        if (!(file >> developer.id >> std::ws) || file.peek() != '"'
            || !(file >> std::quoted(developer.name)) || developer.id <= 0 || developer.id == maxId
            || developer.name.find_first_not_of(whitespace) == std::string::npos
            || board.hasDeveloper(developer.id)) {
            return false;
        }
        board.developers.push_back(developer);
        board.nextDeveloperId = std::max(board.nextDeveloperId, developer.id + 1);
    }

    int taskCount;
    if (!(file >> taskCount) || taskCount < 0) {
        return false;
    }
    for (int i = 0; i < taskCount; ++i) {
        Task task;
        std::string name;
        if (!(file >> task.id >> task.developerId >> name >> std::ws) || file.peek() != '"'
            || !(file >> std::quoted(task.title)) || task.id <= 0 || task.id == maxId
            || task.title.find_first_not_of(whitespace) == std::string::npos
            || board.findTask(task.id) != nullptr) {
            return false;
        }

        bool statusFound = false;
        for (const StatusName& entry : statusNames) {
            if (name == entry.name) {
                task.status = entry.status;
                statusFound = true;
                break;
            }
        }
        if (!statusFound || (task.status && !board.hasDeveloper(task.developerId))
            || (!task.status && task.developerId != 0)) {
            return false;
        }

        board.tasks.push_back(task);
        board.nextTaskId = std::max(board.nextTaskId, task.id + 1);
    }

    file >> std::ws;
    if (file.bad() || !file.eof()) {
        return false;
    }

    *this = std::move(board);
    return true;
}

const std::vector<Developer>& ScrumBoard::getDevelopers() const {
    return developers;
}

const std::vector<Task>& ScrumBoard::getTasks() const {
    return tasks;
}
