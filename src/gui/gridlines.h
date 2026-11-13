#ifndef GUI_GRIDLINES_H
#define GUI_GRIDLINES_H

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

#include "ticks.h"

// The horizontal grid lines of the graph. The functions have no drawing code, so a test can run them.

namespace Gui {

// The rows of the grid lines, in pixels from the top of the graph.
struct GridLines {
    std::vector<double> major;     // at the ticks with a label
    std::vector<double> minor;     // at the ticks without a label
};

// The lines for a list of ticks. A tick that is outside the graph has no line. A pixel row holds one line at
// most: when two ticks fall on the same row, the major line wins, and the first line wins among equal lines.
// So a graph that is very short does not draw a line two times, which would make it brighter.
inline GridLines makeGridLines(const std::vector<Tick> &ticks, const ToPixel &toY, double height)
{
    GridLines lines;
    std::set<long> used;
    auto row = [](double y) { return (long) std::floor(y + 0.5); };

    // The labeled ticks first, so a major line takes its row before a minor line.
    for (const Tick &tick : ticks) {
        const double y = toY(tick.value);
        if (y < 0.0 || y > height || tick.label.empty()) continue;
        if (used.insert(row(y)).second) lines.major.push_back(y);
    }
    for (const Tick &tick : ticks) {
        const double y = toY(tick.value);
        if (y < 0.0 || y > height || !tick.label.empty()) continue;
        if (used.insert(row(y)).second) lines.minor.push_back(y);
    }
    return lines;
}

}

#endif // GUI_GRIDLINES_H
