#ifndef GUI_PLOTMAP_H
#define GUI_PLOTMAP_H

#include <algorithm>

// The map between the time and the pixel of the graph, and the test for a point in the graph. The functions
// have no drawing code and no window, so a test can run them.

namespace Gui {

// The graph in pixels of the canvas, and the time range that it shows. With the flip, the newest time is at the left.
struct PlotFrame {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    double timeStart = 0.0;
    double timeEnd = 1.0;
    bool flipped = false;
};

// The pixel column of a time. The column is a fraction when the time is between two pixels.
inline double plotTimeToX(const PlotFrame &f, double time)
{
    const double offset = f.width * (time - f.timeStart) / (f.timeEnd - f.timeStart);
    return f.left + (f.flipped ? f.width - offset : offset);
}

// The time at a pixel column. A graph with no width counts as one pixel wide.
inline double plotXToTime(const PlotFrame &f, double x)
{
    const double fraction = (x - f.left) / std::max(f.width, 1);
    return f.timeStart + (f.flipped ? 1.0 - fraction : fraction) * (f.timeEnd - f.timeStart);
}

// True when the point is inside the graph, and not in a margin. The first row and the first column are inside.
// The row and the column after the last pixel of the graph are in the margin.
inline bool plotContainsPoint(const PlotFrame &f, double x, double y)
{
    return x >= f.left && x < f.left + f.width && y >= f.top && y < f.top + f.height;
}

}

#endif // GUI_PLOTMAP_H
