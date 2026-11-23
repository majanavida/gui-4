#include "ScrumBoard.h"

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

bool ScrumBoard::assignTask(int taskId, int developerId) {
    Task* task = findTask(taskId);
    if (task == nullptr || task->status.has_value()) {
        return false;
    }

    for (const Developer& developer : developers) {
        if (developer.id == developerId) {
            task->developerId = developerId;
            task->status = TaskStatus::Assigned;
            return true;
        }
    }
    return false;
}

bool ScrumBoard::changeTaskStatus(int taskId, TaskStatus status) {
    Task* task = findTask(taskId);
    if (task == nullptr || !task->status.has_value()) {
        return false;
    }

    task->status = status;
    return true;
}

const std::vector<Developer>& ScrumBoard::getDevelopers() const {
    return developers;
}

const std::vector<Task>& ScrumBoard::getTasks() const {
    return tasks;
}
