#include <gtest/gtest.h>

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
