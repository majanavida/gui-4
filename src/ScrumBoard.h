#ifndef SCRUM_BOARD_H
#define SCRUM_BOARD_H

#include <string>
#include <vector>

struct Developer {
    int id;
    std::string name;
};

struct Task {
    int id;
    std::string title;
};

class ScrumBoard {
private:
    std::vector<Developer> developers;
    std::vector<Task> tasks;

public:
    const std::vector<Developer>& getDevelopers() const;
    const std::vector<Task>& getTasks() const;
};

#endif
