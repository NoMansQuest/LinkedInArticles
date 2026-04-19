#include <gtest/gtest.h>
#include "hello.h"   // This includes the function from src/

TEST(HelloQtTest, ReturnsCorrectMessage)
{
    QString message = getHelloMessage();
    EXPECT_EQ(message, "Hello, World from Qt + CMake project template!");
    EXPECT_FALSE(message.isEmpty());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}