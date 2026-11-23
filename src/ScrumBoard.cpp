#include "ScrumBoard.h"

const std::vector<Developer>& ScrumBoard::getDevelopers() const {
    return developers;
}

const std::vector<Task>& ScrumBoard::getTasks() const {
    return tasks;
}
