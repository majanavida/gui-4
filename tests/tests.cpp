#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "ScrumBoard.h"

TEST(ScrumBoardTest, NewBoardIsEmpty) {
    ScrumBoard board;

    EXPECT_TRUE(board.getDevelopers().empty());
    EXPECT_TRUE(board.getTasks().empty());
}

TEST(ScrumBoardTest, AddsDevelopersAndTasksWithSeparateIds) {
    ScrumBoard board;

    EXPECT_EQ(board.addDeveloper("Anna"), 1);
    EXPECT_EQ(board.addTask("Write tests"), 1);
    EXPECT_EQ(board.addDeveloper("Ivan"), 2);
    EXPECT_EQ(board.addTask("Fix bugs"), 2);

    const auto& developers = board.getDevelopers();
    ASSERT_EQ(developers.size(), 2u);
    EXPECT_EQ(developers[0].id, 1);
    EXPECT_EQ(developers[0].name, "Anna");
    EXPECT_EQ(developers[1].id, 2);
    EXPECT_EQ(developers[1].name, "Ivan");

    const auto& tasks = board.getTasks();
    ASSERT_EQ(tasks.size(), 2u);
    EXPECT_EQ(tasks[0].id, 1);
    EXPECT_EQ(tasks[0].title, "Write tests");
    EXPECT_EQ(tasks[1].id, 2);
    EXPECT_EQ(tasks[1].title, "Fix bugs");
}

TEST(ScrumBoardTest, RejectsBlankNamesAndTitles) {
    ScrumBoard board;

    EXPECT_EQ(board.addDeveloper(""), 0);
    EXPECT_EQ(board.addDeveloper("   "), 0);
    EXPECT_EQ(board.addDeveloper(" \t"), 0);
    EXPECT_EQ(board.addTask(""), 0);
    EXPECT_EQ(board.addTask("   "), 0);
    EXPECT_EQ(board.addTask(" \t"), 0);
    EXPECT_TRUE(board.getDevelopers().empty());
    EXPECT_TRUE(board.getTasks().empty());

    EXPECT_EQ(board.addDeveloper("Anna"), 1);
    EXPECT_EQ(board.addTask("Write tests"), 1);
}

TEST(ScrumBoardTest, NewTaskIsInBacklog) {
    ScrumBoard board;
    board.addTask("Write tests");

    ASSERT_EQ(board.getTasks().size(), 1u);
    const Task& task = board.getTasks()[0];
    EXPECT_EQ(task.developerId, 0);
    EXPECT_FALSE(task.status.has_value());
}

TEST(ScrumBoardTest, AssignsBacklogTaskToDeveloper) {
    ScrumBoard board;
    board.addDeveloper("Anna");
    int developerId = board.addDeveloper("Ivan");
    board.addTask("Write tests");
    int taskId = board.addTask("Fix bugs");

    ASSERT_TRUE(board.assignTask(taskId, developerId));

    const auto& tasks = board.getTasks();
    ASSERT_EQ(tasks.size(), 2u);
    EXPECT_EQ(tasks[0].developerId, 0);
    EXPECT_FALSE(tasks[0].status.has_value());
    EXPECT_EQ(tasks[1].developerId, developerId);
    EXPECT_EQ(tasks[1].status, TaskStatus::Assigned);
}

TEST(ScrumBoardTest, ChangesTaskStatusAndKeepsDeveloper) {
    ScrumBoard board;
    int developerId = board.addDeveloper("Anna");
    int taskId = board.addTask("Write tests");
    ASSERT_TRUE(board.assignTask(taskId, developerId));

    const Task& task = board.getTasks()[0];
    for (TaskStatus status : {TaskStatus::InProgress, TaskStatus::Blocked,
                             TaskStatus::Done, TaskStatus::Assigned}) {
        EXPECT_TRUE(board.changeTaskStatus(taskId, status));
        EXPECT_EQ(task.status, status);
        EXPECT_EQ(task.developerId, developerId);
    }
}

TEST(ScrumBoardTest, RejectsAssignmentWithUnknownIds) {
    ScrumBoard board;
    int developerId = board.addDeveloper("Anna");
    int taskId = board.addTask("Write tests");

    EXPECT_FALSE(board.assignTask(taskId + 1, developerId));
    EXPECT_FALSE(board.assignTask(taskId, developerId + 1));

    ASSERT_EQ(board.getTasks().size(), 1u);
    const Task& task = board.getTasks()[0];
    EXPECT_EQ(task.developerId, 0);
    EXPECT_FALSE(task.status.has_value());
}

TEST(ScrumBoardTest, RejectsReassigningTask) {
    ScrumBoard board;
    int developerId = board.addDeveloper("Anna");
    int otherDeveloperId = board.addDeveloper("Ivan");
    int taskId = board.addTask("Write tests");
    ASSERT_TRUE(board.assignTask(taskId, developerId));
    ASSERT_TRUE(board.changeTaskStatus(taskId, TaskStatus::InProgress));

    EXPECT_FALSE(board.assignTask(taskId, otherDeveloperId));

    const Task& task = board.getTasks()[0];
    EXPECT_EQ(task.developerId, developerId);
    EXPECT_EQ(task.status, TaskStatus::InProgress);
}

TEST(ScrumBoardTest, RejectsStatusChangeForBacklogOrUnknownTask) {
    ScrumBoard board;
    int developerId = board.addDeveloper("Anna");
    int assignedTaskId = board.addTask("Write tests");
    int backlogTaskId = board.addTask("Fix bugs");
    ASSERT_TRUE(board.assignTask(assignedTaskId, developerId));

    EXPECT_FALSE(board.changeTaskStatus(backlogTaskId, TaskStatus::Done));
    EXPECT_FALSE(board.changeTaskStatus(backlogTaskId + 1, TaskStatus::Done));

    const auto& tasks = board.getTasks();
    ASSERT_EQ(tasks.size(), 2u);
    EXPECT_EQ(tasks[0].developerId, developerId);
    EXPECT_EQ(tasks[0].status, TaskStatus::Assigned);
    EXPECT_EQ(tasks[1].developerId, 0);
    EXPECT_FALSE(tasks[1].status.has_value());
}

class ScrumBoardFileTest : public testing::Test {
protected:
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("gui-4 board-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");

    void TearDown() override {
        std::filesystem::remove(path);
    }
};

TEST_F(ScrumBoardFileTest, SavesAndLoadsEmptyBoard) {
    ScrumBoard empty;
    ASSERT_TRUE(empty.saveToFile(path.string()));

    ScrumBoard board;
    board.addDeveloper("Anna");
    board.addTask("Write tests");
    ASSERT_TRUE(board.loadFromFile(path.string()));
    EXPECT_TRUE(board.getDevelopers().empty());
    EXPECT_TRUE(board.getTasks().empty());
}

TEST_F(ScrumBoardFileTest, SavesAndLoadsAllCategoriesWithGapsInIds) {
    const char* data = u8R"({
        "developers": [{"id": 7, "name": "Анна \"QA\""}, {"id": 3, "name": "Ivan Petrov"}],
        "tasks": [
            {"id": 5, "title": "Написать тесты", "developerId": 0, "status": null},
            {"id": 12, "title": "Fix a \"bug\"", "developerId": 7, "status": "Assigned"},
            {"id": 7, "title": "Update README", "developerId": 3, "status": "InProgress"},
            {"id": 9, "title": "Review changes", "developerId": 7, "status": "Blocked"},
            {"id": 10, "title": "Build project", "developerId": 3, "status": "Done"}
        ]
    })";
    std::ofstream(path) << data;

    ScrumBoard board;
    board.addDeveloper("Old developer");
    board.addTask("Old task");
    ASSERT_TRUE(board.loadFromFile(path.string()));

    const auto& developers = board.getDevelopers();
    ASSERT_EQ(developers.size(), 2u);
    EXPECT_EQ(developers[0].id, 7);
    EXPECT_EQ(developers[0].name, u8"Анна \"QA\"");
    EXPECT_EQ(developers[1].id, 3);
    EXPECT_EQ(developers[1].name, "Ivan Petrov");

    const Task expected[] = {
        {5, u8"Написать тесты", 0, std::nullopt},
        {12, "Fix a \"bug\"", 7, TaskStatus::Assigned},
        {7, "Update README", 3, TaskStatus::InProgress},
        {9, "Review changes", 7, TaskStatus::Blocked},
        {10, "Build project", 3, TaskStatus::Done}
    };
    const auto& tasks = board.getTasks();
    ASSERT_EQ(tasks.size(), 5u);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        EXPECT_EQ(tasks[i].id, expected[i].id);
        EXPECT_EQ(tasks[i].title, expected[i].title);
        EXPECT_EQ(tasks[i].developerId, expected[i].developerId);
        EXPECT_EQ(tasks[i].status, expected[i].status);
    }

    ASSERT_TRUE(board.saveToFile(path.string()));
    std::ifstream savedFile(path);
    EXPECT_EQ(nlohmann::json::parse(savedFile), nlohmann::json::parse(data));
    EXPECT_EQ(board.addDeveloper("Pavel"), 8);
    EXPECT_EQ(board.addTask("New task"), 13);
}

TEST_F(ScrumBoardFileTest, KeepsCurrentBoardWhenLoadFails) {
    ScrumBoard board;
    int developerId = board.addDeveloper("Anna");
    int taskId = board.addTask("Write tests");
    ASSERT_TRUE(board.assignTask(taskId, developerId));

    const char* invalidFiles[] = {
        nullptr,
        R"({"developers":[],"tasks":[)",
        R"({"developers":[],"tasks":[{"id":9,"title":"A","developerId":0}]})",
        R"({"developers":[{"id":7.5,"name":"Ivan"}],"tasks":[]})",
        R"({"developers":[{"id":7,"name":"Ivan"}],)"
        R"("tasks":[{"id":9,"title":"A","developerId":7,"status":"Unknown"}]})",
        R"({"developers":[{"id":7,"name":"Ivan"},{"id":7,"name":"Pavel"}],"tasks":[]})",
        R"({"developers":[],"tasks":[{"id":9,"title":"A","developerId":0,"status":null},)"
        R"({"id":9,"title":"B","developerId":0,"status":null}]})",
        R"({"developers":[],"tasks":[{"id":9,"title":"A","developerId":8,"status":"Done"}]})",
        R"({"developers":[{"id":7,"name":"Ivan"}],)"
        R"("tasks":[{"id":9,"title":"A","developerId":7,"status":null}]})"
    };
    for (const char* data : invalidFiles) {
        SCOPED_TRACE(data == nullptr ? "missing file" : data);
        if (data != nullptr) {
            std::ofstream file(path);
            ASSERT_TRUE(file << data);
        }
        EXPECT_FALSE(board.loadFromFile(path.string()));
        ASSERT_EQ(board.getDevelopers().size(), 1u);
        EXPECT_EQ(board.getDevelopers()[0].id, developerId);
        EXPECT_EQ(board.getDevelopers()[0].name, "Anna");
        ASSERT_EQ(board.getTasks().size(), 1u);
        EXPECT_EQ(board.getTasks()[0].id, taskId);
        EXPECT_EQ(board.getTasks()[0].title, "Write tests");
        EXPECT_EQ(board.getTasks()[0].developerId, developerId);
        EXPECT_EQ(board.getTasks()[0].status, TaskStatus::Assigned);
    }

    EXPECT_EQ(board.addDeveloper("Pavel"), 2);
    EXPECT_EQ(board.addTask("New task"), 2);
}

TEST_F(ScrumBoardFileTest, ReturnsFalseWhenSavePathIsUnavailable) {
    ScrumBoard board;
    EXPECT_FALSE(board.saveToFile((path / "board.json").string()));
}
