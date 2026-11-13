#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstring>

#include "../src/context/viewrange.h"
#include "../src/gui/graphlayout.h"
#include "../src/gui/plotmap.h"
#include "../src/gui/readoutstats.h"
#include "../src/gui/ticks.h"

using namespace Gui;

namespace {

// A font with no window. The width of a character is a fixed number of pixels, and the digits can
// have their own widths, so a test can make a proportional font. The height follows the size index.
struct FakeFont {
    double dpr = 1.0;
    std::array<int, 10> digit = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
    int other = 8;

    int charWidth(char c) const { return c >= '0' && c <= '9' ? digit[c - '0'] : other; }
    int textWidth(const std::string &text) const
    {
        int w = 0;
        for (char c : text) w += charWidth(c);
        return (int) std::lround(w * dpr);
    }
    QRect operator()(int size, const std::string &text) const
    {
        return QRect(0, 0, textWidth(text), (int) std::lround((8 + size) * dpr));
    }
};

const std::vector<std::string> allTracks = { "Pitch", "F1", "F2", "F3", "F4" };

RenderOptions leftOptions()
{
    RenderOptions o;
    o.readoutLeft = true;
    o.showDebug = true;
    o.showLegend = true;
    return o;
}

GraphLayout compute(const RenderOptions &o, int width = 1400, int height = 900, double dpr = 1.0,
                    const std::vector<std::string> &tracks = allTracks, FakeFont font = FakeFont())
{
    font.dpr = dpr;
    return computeGraphLayout(o, tracks, dpr, width, height, [font](int size, const std::string &text) { return font(size, text); });
}

std::vector<std::string> keysOf(const GraphLayout &layout)
{
    std::vector<std::string> keys;
    for (const BoxPlacement &b : layout.readout.boxes) keys.push_back(b.key);
    return keys;
}

PlotFrame frameOf(const GraphLayout &layout, int width, int height)
{
    return { layout.plotLeft, layout.plotTop, layout.plotWidth(width), layout.plotHeight(height), 0.0, 15.0, false };
}

}

// The legend and the frame times stand side by side and centred over the graph.
TEST(GraphLayout, TP_LAY_01_TheLegendAndTheFrameTimesStandSideBySideAndCentredOverTheGraph)
{
    const GraphLayout l = compute(leftOptions());
    const ReadoutLayout &r = l.readout;
    ASSERT_TRUE(r.hasLegend);
    ASSERT_TRUE(r.hasDebug);

    const double gap = UiScale::spaceSm;
    EXPECT_DOUBLE_EQ(r.legend.x + r.legend.width + gap, r.debugX);
    EXPECT_DOUBLE_EQ(r.legend.y, r.debugY);

    const double centreOfBar = 0.5 * (r.legend.x + r.debugX + r.debugWidth);
    const double centreOfGraph = l.plotLeft + 0.5 * l.plotWidth(1400);
    EXPECT_NEAR(centreOfBar, centreOfGraph, 1.0);
    // The bar is above the graph with a gap.
    EXPECT_LE(r.legend.y + r.legend.height + gap, l.plotTop + 1e-9);
    EXPECT_GE(r.legend.y, graphPadding - 1e-9);
}

// The frame times that a recording hides keep their space, so the graph does not move.
TEST(GraphLayout, TP_LAY_02_ARecordingKeepsTheSpaceOfTheFrameTimes)
{
    RenderOptions shown = leftOptions();
    RenderOptions hidden = leftOptions();
    hidden.hideDebug = true;

    const GraphLayout a = compute(shown);
    const GraphLayout b = compute(hidden);
    EXPECT_EQ(a.plotTop, b.plotTop);
    EXPECT_EQ(a.plotLeft, b.plotLeft);
    EXPECT_EQ(a.plotBottom, b.plotBottom);
    EXPECT_EQ(a.plotRight, b.plotRight);
    EXPECT_TRUE(b.readout.hasDebug);
    EXPECT_DOUBLE_EQ(a.readout.legend.x, b.readout.legend.x);
    EXPECT_DOUBLE_EQ(a.readout.debugX, b.readout.debugX);
}

// With both boxes off, the top margin is the padding of the graph and nothing more.
TEST(GraphLayout, TP_LAY_03_WithBothBoxesOffTheTopMarginIsThePaddingOnly)
{
    RenderOptions o = leftOptions();
    o.showDebug = false;
    o.showLegend = false;
    for (double dpr : { 1.0, 2.0 }) {
        const GraphLayout l = compute(o, 1400, 900, dpr);
        EXPECT_FALSE(l.readout.hasLegend);
        EXPECT_FALSE(l.readout.hasDebug);
        EXPECT_EQ(l.plotTop, (int) (graphPadding * dpr)) << dpr;
    }

    // One box on gives a bar, and a bar makes the margin larger.
    RenderOptions legendOnly = o;
    legendOnly.showLegend = true;
    EXPECT_GT(compute(legendOnly).plotTop, compute(o).plotTop);
}

// A bar that is wider than the graph starts at the left edge of the graph, and it stays in the window.
TEST(GraphLayout, TP_LAY_04_ABarThatIsWiderThanTheGraphStartsAtTheLeftEdgeOfTheGraph)
{
    const RenderOptions o = leftOptions();
    const GraphLayout wide = compute(o, 3000);
    const double barWidth = wide.readout.debugX + wide.readout.debugWidth - wide.readout.legend.x;

    // The window holds the bar after the left margin, but the graph is narrower than the bar.
    const int width = (int) std::ceil(wide.plotLeft + barWidth + 0.5 * wide.plotRight);
    const GraphLayout l = compute(o, width);
    ASSERT_LT(l.plotWidth(width), barWidth);
    EXPECT_DOUBLE_EQ(l.readout.legend.x, l.plotLeft);
    EXPECT_LE(l.readout.debugX + l.readout.debugWidth, width);

    // A graph that is wider than the bar keeps the bar in the middle.
    EXPECT_GT(wide.readout.legend.x, wide.plotLeft);
}

// With the boxes at the left, the column runs from F4 at the top to the pitch at the bottom.
TEST(GraphLayout, TP_LAY_05_TheColumnRunsFromF4AtTheTopToThePitchAtTheBottom)
{
    const GraphLayout l = compute(leftOptions());
    EXPECT_EQ(keysOf(l), std::vector<std::string>({ "f4", "f3", "f2", "f1", "pitch" }));
    const auto &boxes = l.readout.boxes;
    for (size_t i = 1; i < boxes.size(); ++i) {
        EXPECT_DOUBLE_EQ(boxes[i].x, boxes[0].x);
        EXPECT_DOUBLE_EQ(boxes[i].y, boxes[i - 1].y + boxes[i - 1].height + UiScale::spaceSm);
    }
    EXPECT_DOUBLE_EQ(boxes[0].y, l.plotTop);
}

// A custom order gives the reverse order in the column.
TEST(GraphLayout, TP_LAY_06_ACustomOrderGivesTheReverseOrderInTheColumn)
{
    RenderOptions o = leftOptions();
    o.readoutOrder = "f2,pitch,f4";
    const GraphLayout l = compute(o, 1400, 900, 1.0, { "Pitch", "F2", "F4" });
    EXPECT_EQ(keysOf(l), std::vector<std::string>({ "f4", "pitch", "f2" }));

    // With all tracks the missing keys follow in the default order, and the legend never has a box.
    const GraphLayout all = compute(o);
    EXPECT_EQ(keysOf(all), std::vector<std::string>({ "f3", "f1", "f4", "pitch", "f2" }));
}

// A column that is taller than the graph wraps into a second column, and the order continues from its top.
TEST(GraphLayout, TP_LAY_07_ATallColumnWrapsIntoASecondColumn)
{
    const GraphLayout l = compute(leftOptions(), 1400, 420);
    const auto &boxes = l.readout.boxes;
    ASSERT_EQ(boxes.size(), 5u);

    std::vector<double> columns;
    for (const BoxPlacement &b : boxes) {
        if (columns.empty() || b.x > columns.back() + 1e-9) columns.push_back(b.x);
    }
    ASSERT_GE(columns.size(), 2u);

    // The order of the keys goes on in the next column. Each column starts at the top of the graph.
    EXPECT_EQ(keysOf(l), std::vector<std::string>({ "f4", "f3", "f2", "f1", "pitch" }));
    double lastY = -1.0;
    double lastX = boxes[0].x;
    for (const BoxPlacement &b : boxes) {
        if (b.x > lastX + 1e-9) {
            EXPECT_DOUBLE_EQ(b.y, l.plotTop);
            EXPECT_GT(b.x, lastX);
        }
        else if (lastY >= 0.0) {
            EXPECT_GT(b.y, lastY);
        }
        lastX = b.x;
        lastY = b.y;
    }
    // The graph starts after all columns.
    EXPECT_GT(l.plotLeft, boxes.back().x + boxes.back().width);
}

// A hidden track box leaves no gap in the column or in the row.
TEST(GraphLayout, TP_LAY_08_AHiddenBoxLeavesNoGap)
{
    RenderOptions o = leftOptions();
    o.trackBoxShown[2] = false;   // F2
    const GraphLayout column = compute(o);
    EXPECT_EQ(keysOf(column), std::vector<std::string>({ "f4", "f3", "f1", "pitch" }));
    const auto &c = column.readout.boxes;
    for (size_t i = 1; i < c.size(); ++i) {
        EXPECT_DOUBLE_EQ(c[i].y, c[i - 1].y + c[i - 1].height + UiScale::spaceSm);
    }

    o.readoutLeft = false;
    const GraphLayout row = compute(o);
    EXPECT_EQ(keysOf(row), std::vector<std::string>({ "pitch", "f1", "f3", "f4" }));
    const auto &w = row.readout.boxes;
    EXPECT_DOUBLE_EQ(w[0].x, graphPadding);
    for (size_t i = 1; i < w.size(); ++i) {
        EXPECT_DOUBLE_EQ(w[i].x, w[i - 1].x + w[i - 1].width + UiScale::spaceSm);
    }
}

// A track that the graph hides has no box and no entry in the legend.
TEST(GraphLayout, TP_LAY_09_ATrackThatTheGraphHidesHasNoBox)
{
    const GraphLayout all = compute(leftOptions());
    const GraphLayout noF3 = compute(leftOptions(), 1400, 900, 1.0, { "Pitch", "F1", "F2", "F4" });

    EXPECT_EQ(keysOf(noF3), std::vector<std::string>({ "f4", "f2", "f1", "pitch" }));
    EXPECT_EQ(noF3.readout.content.count("f3"), 0u);
    EXPECT_EQ(all.readout.content.count("f3"), 1u);

    // The legend loses the entry: the swatch, the name "F3" (two digits wide), and the gap.
    const double entry = all.readout.swatchWidth + 2 * 8 + UiScale::spaceMd;
    EXPECT_DOUBLE_EQ(all.readout.legend.width - noF3.readout.legend.width, entry);

    // With no track, no box exists, and no legend.
    const GraphLayout none = compute(leftOptions(), 1400, 900, 1.0, {});
    EXPECT_TRUE(none.readout.boxes.empty());
    EXPECT_FALSE(none.readout.hasLegend);
}

// A value that changes never changes the size or the place of a box. The text of each value fits its column.
TEST(GraphLayout, TP_LAY_10_AChangingValueNeverChangesABox)
{
    RenderOptions o = leftOptions();
    o.statsMask = 0x1FFF;   // all statistics
    const FakeFont font;
    const GraphLayout before = compute(o);

    const BoxContent &pitch = before.readout.content.at("pitch");
    ASSERT_TRUE(pitch.showCurrent);

    auto widthOf = [&](const std::string &text) { return font.textWidth(text); };
    auto sweep = [&](double from, double to, double step) {
        for (double v = from; v <= to + 1e-9; v += step) {
            // The text of the live value in the header, and the text of a statistic in its column.
            const std::string text = stats::number(v);
            EXPECT_LE(widthOf(text), pitch.headerValueWidth) << v;
            EXPECT_LE(widthOf(text), pitch.valueWidth) << v;
        }
    };
    sweep(9.9, 100.0, 0.1);
    sweep(999.9, 1000.0, 0.01);
    sweep(0.0, 5500.0, 0.37);

    // A second call gives the same layout. The layout has no input from the values.
    const GraphLayout after = compute(o);
    ASSERT_EQ(before.readout.boxes.size(), after.readout.boxes.size());
    for (size_t i = 0; i < before.readout.boxes.size(); ++i) {
        EXPECT_DOUBLE_EQ(before.readout.boxes[i].width, after.readout.boxes[i].width);
        EXPECT_DOUBLE_EQ(before.readout.boxes[i].height, after.readout.boxes[i].height);
        EXPECT_DOUBLE_EQ(before.readout.boxes[i].x, after.readout.boxes[i].x);
        EXPECT_DOUBLE_EQ(before.readout.boxes[i].y, after.readout.boxes[i].y);
    }
}

// In a proportional font, the widest digit sets the width of each pattern, for each digit from 0 to 9.
TEST(GraphLayout, TP_LAY_11_TheWidestDigitSetsTheWidthInAProportionalFont)
{
    for (int widest = 0; widest <= 9; ++widest) {
        FakeFont font;
        font.digit = { 5, 6, 5, 6, 5, 6, 5, 6, 5, 6 };
        font.digit[widest] = 10;
        font.other = 4;

        const GraphLayout l = compute(leftOptions(), 1400, 900, 1.0, allTracks, font);
        const BoxContent &c = l.readout.content.at("pitch");

        // The pattern "0000.0" has five digits and one point. Each digit counts with the widest width.
        EXPECT_DOUBLE_EQ(c.headerValueWidth, 5 * 10 + 4) << "widest digit " << widest;
        EXPECT_DOUBLE_EQ(c.valueWidth, 5 * 10 + 4) << "widest digit " << widest;
    }
}

// The pattern of each statistic is as wide as the widest value that the statistic can have.
TEST(GraphLayout, TP_LAY_12_EachPatternIsAsWideAsTheWidestRealValue)
{
    // The widest real values: a pitch or a formant of up to 5500 Hz, with a value of 999.95 that rounds to 1000.0,
    // a CV of up to 999.9 percent, a share of 100 percent, a count of a window of 24 hours at 100 frames each second,
    // and a note with a sharp and a negative offset.
    HudTrack t;
    t.name = "Pitch";
    t.isPitch = true;
    t.values = { 999.95, 5500.0, 20.0, 999.95, 5499.5 };
    t.frames = 8'640'000;
    t.count = 8'640'000;
    HudTrack cv;
    cv.name = "F1";
    cv.values = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 400.0 };
    cv.frames = 11;
    HudTrack note;
    note.name = "Pitch";
    note.isPitch = true;
    note.values = { 466.0 };
    note.frames = 1;

    const std::vector<std::pair<const HudTrack *, int>> cases = {
        { &t, StatCurrent }, { &t, StatMin }, { &t, StatP5 }, { &t, StatMean }, { &t, StatMedian }, { &t, StatStdDev },
        { &t, StatP95 }, { &t, StatMax }, { &t, StatRange }, { &t, StatVoiced }, { &t, StatCount },
        { &cv, StatCV }, { &note, StatNote },
    };
    for (const auto &entry : cases) {
        const HudTrack &track = *entry.first;
        const stats::TrackStats s = stats::computeStats(track);
        const std::string text = stats::statText(track, s, entry.second, false);
        for (const StatColumn &column : statColumns()) {
            if (column.bit != entry.second) continue;
            EXPECT_LE(text.size(), std::strlen(column.pattern)) << column.title << ": " << text;
        }
    }
    EXPECT_EQ(statColumns().size(), 13u);
}

// The legend key in the stored order is ignored in the row and in the column.
TEST(GraphLayout, TP_LAY_14_TheLegendKeyInTheOrderIsIgnored)
{
    RenderOptions o = leftOptions();
    o.readoutOrder = "f1,legend,pitch";
    for (bool left : { true, false }) {
        o.readoutLeft = left;
        const GraphLayout l = compute(o);
        for (const BoxPlacement &b : l.readout.boxes) EXPECT_NE(b.key, "legend") << (left ? "column" : "row");
        EXPECT_EQ(l.readout.boxes.size(), 5u);
        // The legend stays in the bar.
        EXPECT_TRUE(l.readout.hasLegend);
    }
}

// A narrow or short window gives no negative size.
TEST(GraphLayout, TP_LAY_16_ASmallWindowGivesNoNegativeSize)
{
    for (bool left : { true, false }) {
        for (double dpr : { 1.0, 2.0 }) {
            for (int width : { 1, 2, 10, 100, 200, 300, 400 }) {
                for (int height : { 1, 2, 50, 300, 900 }) {
                    RenderOptions o = leftOptions();
                    o.readoutLeft = left;
                    const GraphLayout l = compute(o, width, height, dpr);
                    const std::string where = std::to_string(width) + "x" + std::to_string(height) + (left ? " column" : " row");
                    EXPECT_GE(l.plotLeft, 0) << where;
                    EXPECT_GE(l.plotRight, 0) << where;
                    EXPECT_GE(l.plotTop, 0) << where;
                    EXPECT_GE(l.plotBottom, 0) << where;
                    EXPECT_GE(l.plotWidth(width), 1) << where;
                    EXPECT_GE(l.plotHeight(height), 1) << where;
                    EXPECT_GE(l.stripHeight, 0) << where;
                    for (const BoxPlacement &b : l.readout.boxes) {
                        EXPECT_GE(b.width, 0.0) << where;
                        EXPECT_GE(b.height, 0.0) << where;
                    }
                }
            }
        }
    }

    // A window of 300 pixels keeps the graph and hides the boxes that do not fit.
    const GraphLayout l = compute(leftOptions(), 300, 800);
    EXPECT_GE(l.plotWidth(300), 1);
}

// A click in the bar above the graph is not a pick.
TEST(GraphLayout, TP_LAY_19_AClickInTheBarAboveTheGraphIsNotAPick)
{
    const GraphLayout l = compute(leftOptions());
    const PlotFrame f = frameOf(l, 1400, 900);
    const double x = f.left + 10;

    EXPECT_FALSE(plotContainsPoint(f, x, 0.0));
    EXPECT_FALSE(plotContainsPoint(f, x, l.readout.legend.y + 2));
    EXPECT_FALSE(plotContainsPoint(f, x, f.top - 1));
}

// The first and the last row of the graph are a pick. The row below the last row belongs to the time axis.
TEST(GraphLayout, TP_LAY_20_TheFirstAndTheLastRowOfTheGraphAreAPick)
{
    const GraphLayout l = compute(leftOptions());
    const PlotFrame f = frameOf(l, 1400, 900);
    const double x = f.left + 10;

    EXPECT_TRUE(plotContainsPoint(f, x, f.top));
    EXPECT_TRUE(plotContainsPoint(f, x, f.top + f.height - 1));
    EXPECT_FALSE(plotContainsPoint(f, x, f.top + f.height));
}

// A click on the space at the right of the graph is not a pick. A click on the last column is.
TEST(GraphLayout, TP_LAY_23_TheSpaceAtTheRightIsNotAPickAndTheLastColumnIs)
{
    const GraphLayout l = compute(leftOptions());
    const PlotFrame f = frameOf(l, 1400, 900);
    const double y = f.top + 10;

    EXPECT_TRUE(plotContainsPoint(f, f.left, y));
    EXPECT_FALSE(plotContainsPoint(f, f.left - 1, y));
    EXPECT_TRUE(plotContainsPoint(f, f.left + f.width - 1, y));
    EXPECT_FALSE(plotContainsPoint(f, f.left + f.width, y));
    EXPECT_FALSE(plotContainsPoint(f, 1399, y));
}

// Each margin of the axes is the sum of the ticks, the numbers, the title, and the smallest steps.
TEST(GraphLayout, TP_LAY_29_EachMarginOfTheAxesIsASumOfTheirParts)
{
    for (double dpr : { 1.0, 2.0 }) {
        for (bool labels : { false, true }) {
            for (bool titles : { false, true }) {
                RenderOptions o = leftOptions();
                o.showAxisLabels = labels;
                o.showAxisTitles = titles;
                o.showLegend = false;
                o.showDebug = false;
                o.showStats = false;     // no box, so the margins are the margins of the axes
                const GraphLayout l = compute(o, 1400, 900, dpr);

                const FakeFont font{ dpr };
                const double xs = UiScale::spaceXs * dpr;
                const double padding = graphPadding * dpr;
                const double major = tickMajor * dpr;
                const double labelHeight = font(4, "0").height();
                const double titleHeight = font(1, "0").height();
                const double widest = font(3, "00000").width();

                double bottom = major + xs;
                if (labels) bottom += labelHeight + xs;
                if (titles) bottom += titleHeight + xs;
                double left = major + xs;
                if (labels) left += widest + xs;
                if (titles) left += titleHeight;

                const std::string where = "dpr " + std::to_string(dpr) + " labels " + std::to_string(labels) + " titles " + std::to_string(titles);
                EXPECT_EQ(l.plotBottom, (int) std::ceil(bottom + padding)) << where;
                EXPECT_EQ(l.plotLeft, (int) std::ceil(padding + left)) << where;
                EXPECT_EQ(l.plotRight, (int) std::ceil(padding)) << where;
                EXPECT_EQ(l.plotTop, (int) std::ceil(padding)) << where;
            }
        }
    }
}

// The row of a box is a multiple of 4 times the pixel ratio, at each ratio from 1 to 3.
TEST(GraphLayout, TP_LAY_30_TheRowHeightIsAMultipleOfFourTimesThePixelRatio)
{
    for (double dpr : { 1.0, 1.25, 1.5, 2.0, 2.5, 3.0 }) {
        const GraphLayout l = compute(leftOptions(), 1400, 900, dpr);
        const double grid = UiScale::gridUnit * dpr;
        const double rows = l.readout.rowHeight / grid;
        EXPECT_NEAR(rows, std::round(rows), 1e-9) << dpr;
        EXPECT_GE(l.readout.rowHeight, 1.0);
        // Each box is a padding, a header row, and a row for each statistic.
        for (const BoxPlacement &b : l.readout.boxes) {
            const double lines = (b.height - 2.0 * l.readout.pad) / l.readout.rowHeight;
            EXPECT_NEAR(lines, std::round(lines), 1e-9) << dpr;
        }
    }
}

// The time at a pixel and the pixel at a time are inverse, with and without the flip, for narrow and wide graphs.
TEST(PlotMap, TP_FLIP_07_TheTimeAtAPixelAndThePixelAtATimeAreInverse)
{
    for (bool flipped : { false, true }) {
        for (int width : { 1, 2, 4000 }) {
            PlotFrame f;
            f.left = 70;
            f.top = 40;
            f.width = width;
            f.height = 300;
            f.timeStart = -3.0;
            f.timeEnd = 12.0;
            f.flipped = flipped;

            for (double t : { -3.0, -1.5, 0.0, 0.25, 6.0, 11.999, 12.0 }) {
                const double x = plotTimeToX(f, t);
                EXPECT_NEAR(plotXToTime(f, x), t, 1e-9) << "t " << t << " width " << width << (flipped ? " flipped" : "");
            }
            // The two ends of the time range lie at the two edges of the graph.
            const double xStart = plotTimeToX(f, f.timeStart);
            const double xEnd = plotTimeToX(f, f.timeEnd);
            EXPECT_DOUBLE_EQ(xStart, flipped ? f.left + width : f.left);
            EXPECT_DOUBLE_EQ(xEnd, flipped ? f.left : f.left + width);
            // The time grows to the left with the flip, and to the right without it.
            EXPECT_EQ(plotTimeToX(f, 5.0) < plotTimeToX(f, 4.0), flipped);
        }
    }
}

// A young capture with the flip on starts below zero. With the flip off, it starts at zero.
TEST(ViewRange, TP_LFT_05_AYoungCaptureStartsBelowZeroWithTheFlipAndAtZeroWithoutIt)
{
    const double viewDuration = 15.0;
    const double delay = 0.085;
    for (double realTime : { 0.0, 0.1, 1.0, 5.0, 10.0, 15.0 }) {
        const double flippedEnd = Main::liveViewEnd(realTime, delay, viewDuration, true);
        const double plainEnd = Main::liveViewEnd(realTime, delay, viewDuration, false);

        // With the flip, the view follows the sound from the first moment. The part before zero is empty.
        EXPECT_DOUBLE_EQ(flippedEnd, realTime - delay);
        EXPECT_LT(flippedEnd - viewDuration, 0.0) << realTime;
        // Without the flip, the graph starts at zero and fills towards the right.
        EXPECT_DOUBLE_EQ(plainEnd, viewDuration) << realTime;
        EXPECT_DOUBLE_EQ(plainEnd - viewDuration, 0.0) << realTime;
    }

    // After the sound fills the width, both views scroll, and the start is not negative.
    EXPECT_DOUBLE_EQ(Main::liveViewEnd(40.0, delay, viewDuration, false), 40.0 - delay);
    EXPECT_GT(Main::liveViewEnd(40.0, delay, viewDuration, true) - viewDuration, 0.0);

    // The view of a past time stays in the length of the session, and a loaded file has a view of the full length at least.
    EXPECT_DOUBLE_EQ(Main::pastViewEnd(false, 100.0, 15.0, 60.0), 60.0);
    EXPECT_DOUBLE_EQ(Main::pastViewEnd(false, 3.0, 15.0, 60.0), 15.0);
    EXPECT_DOUBLE_EQ(Main::pastViewEnd(false, 3.0, 15.0, 8.0), 8.0);
    EXPECT_DOUBLE_EQ(Main::pastViewEnd(true, 3.0, 15.0, 8.0), 15.0);
    EXPECT_DOUBLE_EQ(Main::pastViewEnd(true, 100.0, 15.0, 60.0), 60.0);
}
