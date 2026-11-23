#include <gtest/gtest.h>

#include "ScrumBoard.h"

TEST(ScrumBoardTest, NewBoardIsEmpty) {
    ScrumBoard board;

    EXPECT_TRUE(board.getDevelopers().empty());
    EXPECT_TRUE(board.getTasks().empty());
}
