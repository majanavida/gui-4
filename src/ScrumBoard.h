#ifndef SCRUM_BOARD_H
#define SCRUM_BOARD_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct Developer {
    int id;
    std::string name;
};

enum class TaskStatus {
    Assigned,
    InProgress,
    Blocked,
    Done
};

struct Task {
    int id;
    std::string title;
    int developerId;
    std::optional<TaskStatus> status;
};

class ScrumBoard {
private:
    std::string name;
    std::vector<Developer> developers;
    std::vector<Task> tasks;
    int nextDeveloperId = 1;
    int nextTaskId = 1;

    bool hasDeveloper(int developerId) const;
    Task* findTask(int taskId);

public:
    ScrumBoard() = default;
    explicit ScrumBoard(const std::string& name);

    int addDeveloper(const std::string& name);
    int addTask(const std::string& title);
    bool assignTask(int taskId, int developerId);
    bool changeTaskStatus(int taskId, TaskStatus status);

    bool saveToFile(const std::filesystem::path& path) const;
    bool loadFromFile(const std::filesystem::path& path);

    const std::string& getName() const;
    const std::vector<Developer>& getDevelopers() const;
    const std::vector<Task>& getTasks() const;
};

#endif
