#include "ScrumBoard.h"

#include <algorithm>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace {

using nlohmann::json;

struct StatusName {
    TaskStatus status;
    const char* name;
};

const StatusName statusNames[] = {
    {TaskStatus::Assigned, "Assigned"},
    {TaskStatus::InProgress, "InProgress"},
    {TaskStatus::Blocked, "Blocked"},
    {TaskStatus::Done, "Done"}
};

bool readId(const json& value, int& id) {
    if (!value.is_number_integer()) {
        return false;
    }

    id = value.get<int>();
    return true;
}

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
    json data = {{"developers", json::array()}, {"tasks", json::array()}};
    for (const Developer& developer : developers) {
        data["developers"].push_back({{"id", developer.id}, {"name", developer.name}});
    }

    for (const Task& task : tasks) {
        json status = nullptr;
        if (task.status) {
            for (const StatusName& entry : statusNames) {
                if (entry.status == *task.status) {
                    status = entry.name;
                    break;
                }
            }
            if (status.is_null()) {
                return false;
            }
        }
        data["tasks"].push_back({{"id", task.id}, {"title", task.title},
                                 {"developerId", task.developerId}, {"status", status}});
    }

    std::string text;
    try {
        text = data.dump(2);
    } catch (const json::type_error&) {
        return false;
    }

    std::ofstream file(path);
    file << text << '\n';
    file.close();
    return file.good();
}

bool ScrumBoard::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    ScrumBoard board;
    const char* whitespace = " \t\n\r\f\v";
    try {
        const json data = json::parse(file);
        const json& developerList = data.at("developers");
        const json& taskList = data.at("tasks");
        if (!developerList.is_array() || !taskList.is_array()) {
            return false;
        }

        for (const json& item : developerList) {
            Developer developer;
            developer.name = item.at("name").get<std::string>();
            if (!readId(item.at("id"), developer.id) || developer.id <= 0
                || board.hasDeveloper(developer.id)
                || developer.name.find_first_not_of(whitespace) == std::string::npos) {
                return false;
            }

            board.developers.push_back(developer);
            board.nextDeveloperId = std::max(board.nextDeveloperId, developer.id + 1);
        }

        for (const json& item : taskList) {
            Task task;
            task.title = item.at("title").get<std::string>();
            if (!readId(item.at("id"), task.id) || task.id <= 0 || board.findTask(task.id) != nullptr
                || !readId(item.at("developerId"), task.developerId)
                || task.title.find_first_not_of(whitespace) == std::string::npos) {
                return false;
            }

            const json& status = item.at("status");
            if (status.is_string()) {
                for (const StatusName& entry : statusNames) {
                    if (status == entry.name) {
                        task.status = entry.status;
                        break;
                    }
                }
                if (!task.status || !board.hasDeveloper(task.developerId)) {
                    return false;
                }
            } else if (!status.is_null() || task.developerId != 0) {
                return false;
            }

            board.tasks.push_back(task);
            board.nextTaskId = std::max(board.nextTaskId, task.id + 1);
        }
    } catch (const json::exception&) {
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
