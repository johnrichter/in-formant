#include <gtest/gtest.h>

#include "../src/logformat.h"

#include <string>
#include <vector>

using namespace LogFormat;

namespace {

std::vector<std::string> run(const std::vector<std::string> &chunks, LineBuilder &builder)
{
    std::vector<std::string> lines;
    for (const std::string &chunk : chunks) {
        builder.feed(chunk.data(), chunk.size(), [] { return std::string("[T] "); },
                     [&lines](const std::string &line) { lines.push_back(line); });
    }
    return lines;
}

}

// The log goes to the folder that macOS shows in the Console app, and not to the working folder of the program.
TEST(LogFile, TP_LOG_01_TheLogFileIsInTheLogsFolderOfTheUser)
{
    EXPECT_EQ(folder("/Users/anna"), "/Users/anna/Library/Logs/InFormant");
    EXPECT_EQ(file("/Users/anna", "InFormant"), "/Users/anna/Library/Logs/InFormant/InFormant.log");
    EXPECT_EQ(rotatedFile("/Users/anna", "InFormant"), "/Users/anna/Library/Logs/InFormant/InFormant.1.log");
}

TEST(LogFile, TP_LOG_02_AFileOverFiveMegabytesMovesToTheSecondFile)
{
    EXPECT_FALSE(needsRotation(0));
    EXPECT_FALSE(needsRotation(maximumFileBytes));
    EXPECT_TRUE(needsRotation(maximumFileBytes + 1));
}

TEST(LogFile, TP_LOG_03_EachLineGetsOneStampAndLinesSpanChunks)
{
    LineBuilder builder;
    const auto lines = run({ "first\nsec", "ond line\nthird\n" }, builder);
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "[T] first\n");
    EXPECT_EQ(lines[1], "[T] second line\n");
    EXPECT_EQ(lines[2], "[T] third\n");
    EXPECT_FALSE(builder.open());
}

TEST(LogFile, TP_LOG_03_AnOpenLineWaitsForItsEnd)
{
    LineBuilder builder;
    EXPECT_TRUE(run({ "no end" }, builder).empty());
    EXPECT_TRUE(builder.open());
    const auto lines = run({ " yet\n" }, builder);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "[T] no end yet\n");
}

TEST(LogFile, TP_LOG_04_ALongLineBreaksIntoTwoLinesAndLosesNoByte)
{
    LineBuilder builder;
    const std::string text(maximumLineBytes * 2, 'x');
    const auto lines = run({ text + "\n" }, builder);
    ASSERT_GE(lines.size(), 2u);
    size_t xs = 0;
    for (const std::string &line : lines) {
        EXPECT_LE(line.size(), maximumLineBytes + 1);
        EXPECT_EQ(line.back(), '\n');
        for (char c : line) xs += c == 'x';
    }
    EXPECT_EQ(xs, text.size());
}
