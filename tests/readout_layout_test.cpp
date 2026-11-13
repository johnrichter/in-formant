#include <gtest/gtest.h>
#include "../src/gui/readoutlayout.h"

using namespace Gui;

// The order: an empty text gives the default order.
TEST(ReadoutLayout, TheOrderAnEmptyTextGivesTheDefaultOrder)
{
    const auto order = normalizeReadoutOrder("");
    EXPECT_TRUE(joinReadoutOrder(order) == "legend,pitch,f1,f2,f3,f4");
}

// The order: a partial text keeps its keys first, and the missing keys follow.
TEST(ReadoutLayout, TheOrderAPartialTextKeepsItsKeysFirstAndTheMissingKeysFollow)
{
    const auto order = normalizeReadoutOrder("f2, pitch");
    EXPECT_TRUE(joinReadoutOrder(order) == "f2,pitch,legend,f1,f3,f4");
}

// The order: unknown keys and repeated keys go away, and the case does not matter.
TEST(ReadoutLayout, TheOrderUnknownKeysAndRepeatedKeysGoAwayAndTheCaseDoesNot)
{
    const auto order = normalizeReadoutOrder("F1,xyz,f1,Legend,,pitch");
    EXPECT_TRUE(joinReadoutOrder(order) == "f1,legend,pitch,f2,f3,f4");
}

// A fixed width sample replaces each digit and keeps the other characters.
TEST(ReadoutLayout, AFixedWidthSampleReplacesEachDigitAndKeepsTheOtherCharacters)
{
    EXPECT_TRUE(fixedWidthSample("0000.0", '8') == "8888.8");
    EXPECT_TRUE(fixedWidthSample("100.0%", '8') == "888.8%");
    EXPECT_TRUE(fixedWidthSample("G#4 -00c", '8') == "G#8 -88c");
}

// The row: boxes that fit stand next to each other with the gap.
TEST(ReadoutLayout, TheRowBoxesThatFitStandNextToEachOtherWithTheGap)
{
    const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 50, 30 }, { "c", 80, 20 } };
    const auto placed = placeInRow(boxes, 10, 5, 400, 8);
    EXPECT_TRUE(placed.size() == 3);
    EXPECT_TRUE(placed[0].visible && placed[0].x == 10 && placed[0].y == 5);
    EXPECT_TRUE(placed[1].visible && placed[1].x == 118);
    EXPECT_TRUE(placed[2].visible && placed[2].x == 176);
}

// The row: a box that does not fit is not visible, and a smaller box after it can still fit.
TEST(ReadoutLayout, TheRowABoxThatDoesNotFitIsNotVisibleAndASmallerBoxAfterItCan)
{
    const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 200, 30 }, { "c", 50, 20 } };
    const auto placed = placeInRow(boxes, 0, 0, 200, 10);
    EXPECT_TRUE(placed[0].visible);
    EXPECT_TRUE(!placed[1].visible);
    EXPECT_TRUE(placed[2].visible && placed[2].x == 110);
}

// The columns: boxes stack with the gap while they fit in the height.
TEST(ReadoutLayout, TheColumnsBoxesStackWithTheGapWhileTheyFitInTheHeight)
{
    const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 120, 40 } };
    double total = 0;
    const auto placed = placeInColumns(boxes, 10, 20, 200, 8, total);
    EXPECT_TRUE(placed[0].x == 10 && placed[0].y == 20);
    EXPECT_TRUE(placed[1].x == 10 && placed[1].y == 68);
    EXPECT_NEAR(total, 120.0, 1e-9);
}

// The columns: a box that does not fit in the height starts a new column.
TEST(ReadoutLayout, TheColumnsABoxThatDoesNotFitInTheHeightStartsANewColumn)
{
    const std::vector<BoxSize> boxes = { { "a", 100, 60 }, { "b", 80, 60 }, { "c", 90, 60 } };
    double total = 0;
    const auto placed = placeInColumns(boxes, 0, 0, 130, 10, total);
    EXPECT_TRUE(placed[0].x == 0 && placed[0].y == 0);
    EXPECT_TRUE(placed[1].x == 0 && placed[1].y == 70);
    EXPECT_TRUE(placed[2].x == 110 && placed[2].y == 0);
    EXPECT_NEAR(total, 200.0, 1e-9);
}

// The columns: a box that is higher than the space still gets a place in its own column.
TEST(ReadoutLayout, TheColumnsABoxThatIsHigherThanTheSpaceStillGetsAPlaceInIts)
{
    const std::vector<BoxSize> boxes = { { "a", 100, 500 } };
    double total = 0;
    const auto placed = placeInColumns(boxes, 0, 0, 100, 10, total);
    EXPECT_TRUE(placed.size() == 1 && placed[0].x == 0 && placed[0].y == 0);
    EXPECT_NEAR(total, 100.0, 1e-9);
}
