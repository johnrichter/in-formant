#include "check.h"
#include "../src/gui/readoutlayout.h"

using namespace Gui;

int main()
{
    // The order: an empty text gives the default order.
    {
        const auto order = normalizeReadoutOrder("");
        CHECK(joinReadoutOrder(order) == "legend,pitch,f1,f2,f3,f4");
    }

    // The order: a partial text keeps its keys first, and the missing keys follow.
    {
        const auto order = normalizeReadoutOrder("f2, pitch");
        CHECK(joinReadoutOrder(order) == "f2,pitch,legend,f1,f3,f4");
    }

    // The order: unknown keys and repeated keys go away, and the case does not matter.
    {
        const auto order = normalizeReadoutOrder("F1,xyz,f1,Legend,,pitch");
        CHECK(joinReadoutOrder(order) == "f1,legend,pitch,f2,f3,f4");
    }

    // A fixed width sample replaces each digit and keeps the other characters.
    {
        CHECK(fixedWidthSample("0000.0", '8') == "8888.8");
        CHECK(fixedWidthSample("100.0%", '8') == "888.8%");
        CHECK(fixedWidthSample("G#4 -00c", '8') == "G#8 -88c");
    }

    // The row: boxes that fit stand next to each other with the gap.
    {
        const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 50, 30 }, { "c", 80, 20 } };
        const auto placed = placeInRow(boxes, 10, 5, 400, 8);
        CHECK(placed.size() == 3);
        CHECK(placed[0].visible && placed[0].x == 10 && placed[0].y == 5);
        CHECK(placed[1].visible && placed[1].x == 118);
        CHECK(placed[2].visible && placed[2].x == 176);
    }

    // The row: a box that does not fit is not visible, and a smaller box after it can still fit.
    {
        const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 200, 30 }, { "c", 50, 20 } };
        const auto placed = placeInRow(boxes, 0, 0, 200, 10);
        CHECK(placed[0].visible);
        CHECK(!placed[1].visible);
        CHECK(placed[2].visible && placed[2].x == 110);
    }

    // The columns: boxes stack with the gap while they fit in the height.
    {
        const std::vector<BoxSize> boxes = { { "a", 100, 40 }, { "b", 120, 40 } };
        double total = 0;
        const auto placed = placeInColumns(boxes, 10, 20, 200, 8, total);
        CHECK(placed[0].x == 10 && placed[0].y == 20);
        CHECK(placed[1].x == 10 && placed[1].y == 68);
        CHECK_NEAR(total, 120.0, 1e-9);
    }

    // The columns: a box that does not fit in the height starts a new column.
    {
        const std::vector<BoxSize> boxes = { { "a", 100, 60 }, { "b", 80, 60 }, { "c", 90, 60 } };
        double total = 0;
        const auto placed = placeInColumns(boxes, 0, 0, 130, 10, total);
        CHECK(placed[0].x == 0 && placed[0].y == 0);
        CHECK(placed[1].x == 0 && placed[1].y == 70);
        CHECK(placed[2].x == 110 && placed[2].y == 0);
        CHECK_NEAR(total, 200.0, 1e-9);
    }

    // The columns: a box that is higher than the space still gets a place in its own column.
    {
        const std::vector<BoxSize> boxes = { { "a", 100, 500 } };
        double total = 0;
        const auto placed = placeInColumns(boxes, 0, 0, 100, 10, total);
        CHECK(placed.size() == 1 && placed[0].x == 0 && placed[0].y == 0);
        CHECK_NEAR(total, 100.0, 1e-9);
    }

    return TEST_RESULT();
}
