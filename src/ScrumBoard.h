#ifndef SCRUM_BOARD_H
#define SCRUM_BOARD_H

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
    std::vector<Developer> developers;
    std::vector<Task> tasks;
    int nextDeveloperId = 1;
    int nextTaskId = 1;

    Task* findTask(int taskId);

public:
    int addDeveloper(const std::string& name);
    int addTask(const std::string& title);
    bool assignTask(int taskId, int developerId);
    bool changeTaskStatus(int taskId, TaskStatus status);

    const std::vector<Developer>& getDevelopers() const;
    const std::vector<Task>& getTasks() const;
};

#endif
