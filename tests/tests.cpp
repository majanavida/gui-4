#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

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
        ("gui-4 board-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");

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
    const std::string data =
        "2\r\n7 \"Anna \\\"QA\\\"\"\r\n3 \"Ivan Petrov\"\r\n"
        "5\r\n5 0 Backlog \"Write tests\"\r\n12 7 Assigned \"Fix a \\\"bug\\\"\"\r\n"
        "7 3 InProgress \"Update README\"\r\n9 7 Blocked \"Review changes\"\r\n"
        "10 3 Done \"Build project\"\r\n";
    std::ofstream(path, std::ios::binary) << data;

    ScrumBoard board;
    board.addDeveloper("Old developer");
    board.addTask("Old task");
    ASSERT_TRUE(board.loadFromFile(path.string()));
    const auto& developers = board.getDevelopers();
    ASSERT_EQ(developers.size(), 2u);
    EXPECT_EQ(developers[0].id, 7);
    EXPECT_EQ(developers[0].name, "Anna \"QA\"");
    EXPECT_EQ(developers[1].id, 3);
    EXPECT_EQ(developers[1].name, "Ivan Petrov");

    const Task expected[] = {
        {5, "Write tests", 0, std::nullopt},
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
    std::ifstream savedFile(path, std::ios::binary);
    std::ostringstream savedData;
    savedData << savedFile.rdbuf();
    EXPECT_EQ(savedData.str(), data);
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
        "1\n7 \"Ivan\"\n1\n9 7 Assigned \"Fix bugs",
        "1\n7 \"Ivan\"\n1\n9 7 Unknown \"Fix bugs\"\n",
        "2\n7 \"Ivan\"\n7 \"Pavel\"\n0\n",
        "0\n2\n9 0 Backlog \"Fix bugs\"\n9 0 Backlog \"Build project\"\n",
        "1\n7 \"Ivan\"\n1\n9 8 Done \"Fix bugs\"\n"
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
    EXPECT_FALSE(board.saveToFile((path / "board.txt").string()));
}
