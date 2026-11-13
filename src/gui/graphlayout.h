#ifndef GUI_GRAPHLAYOUT_H
#define GUI_GRAPHLAYOUT_H

#include <QRect>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "readoutlayout.h"
#include "renderoptions.h"
#include "uiscale.h"

// The layout of the graph: the margins of the axes and the places of the readout boxes. The code has no
// drawing code and no window. The caller gives the text measure, so a test can use a fake font.

namespace Gui {

// All spacing of the graph comes from the spacing scale of the windows (gui/uiscale.h). The steps are in
// pixels of the layout. Each use multiplies a step by the pixel ratio of the screen.
constexpr double spaceXs = UiScale::spaceXs;
constexpr double spaceSm = UiScale::spaceSm;
constexpr double spaceMd = UiScale::spaceMd;
constexpr double spaceXl = UiScale::spaceXl;
constexpr double graphPadding = UiScale::graphPadding;

// The length of the tick marks of the axes. A mark is a shape and not a space, so it has no step.
constexpr double tickMajor = 9.0;
constexpr double tickMedium = 6.0;
constexpr double tickMinor = 4.0;

// One statistic. The pattern is the widest text of its value, with digits as place holders.
struct StatColumn {
    int bit;
    const char *title;
    const char *pattern;
};

inline const std::vector<StatColumn> &statColumns()
{
    static const std::vector<StatColumn> columns = {
        { StatCurrent, "Now", "0000.0" }, { StatMin, "Min", "0000.0" }, { StatP5, "P5", "0000.0" },
        { StatMean, "Mean", "0000.0" }, { StatMedian, "Median", "0000.0" }, { StatStdDev, "SD", "0000.0" },
        { StatP95, "P95", "0000.0" }, { StatMax, "Max", "0000.0" }, { StatRange, "Range", "0000.0" },
        { StatCV, "CV", "000.0%" }, { StatVoiced, "Voiced", "000%" }, { StatCount, "N", "0000000" },
        { StatNote, "Note", "G#4 -00c" },
    };
    return columns;
}

// The index of a track in the options: 0 is the pitch, and 1 to 4 are the formants F1 to F4.
// The formants after F4 use the index of F4.
inline int trackIndex(const std::string &name)
{
    if (name == "Pitch") return 0;
    const int n = name.size() > 1 ? std::atoi(name.c_str() + 1) : 1;
    return std::clamp(n, 1, 4);
}

inline std::string trackKey(const std::string &name)
{
    std::string key = name;
    for (char &c : key) c = (char) std::tolower((unsigned char) c);
    return key;
}

// What a box of one track shows, and the sizes of its parts. The sizes come from the settings and
// the fonts only, so they stay the same when the values change.
struct BoxContent {
    std::vector<int> bits;          // the statistics that have a line, in the order of the lines
    bool showCurrent = false;       // the header shows the live value
    double labelWidth = 0.0;
    double valueWidth = 0.0;
    double nameWidth = 0.0;
    double headerValueWidth = 0.0;
};

// The layout of the readout boxes of the frame.
struct ReadoutLayout {
    std::vector<BoxPlacement> boxes;
    std::map<std::string, BoxContent> content;
    double pad = 0.0;
    double rowHeight = 0.0;
    double swatchWidth = 0.0;
    double debugWidth = 0.0;
    double debugHeight = 0.0;
    // The bar at the top holds the legend and the frame timing, centered over the graph.
    BoxPlacement legend;
    bool hasLegend = false;
    double debugX = 0.0;
    double debugY = 0.0;
    bool hasDebug = false;
};

// The margins of the graph in pixels, and the boxes.
struct GraphLayout {
    ReadoutLayout readout;
    int plotLeft = 0;
    int plotRight = 0;
    int plotTop = 0;
    int plotBottom = 0;
    // The height of the row of readout boxes below the time axis. The row is part of the bottom margin.
    int stripHeight = 0;

    int plotWidth(int viewWidth) const { return viewWidth - plotLeft - plotRight; }
    int plotHeight(int viewHeight) const { return viewHeight - plotBottom - plotTop; }
};

// Shrink two margins, the far one first, until they leave room for the graph.
inline void shrinkMargins(int &near, int &far, int room)
{
    int excess = near + far - room;
    if (excess <= 0) return;
    const int fromFar = std::min(far, excess);
    far -= fromFar;
    excess -= fromFar;
    near = std::max(0, near - excess);
}

// The bounds of a text in a font size. The font size is an index, as in the canvas renderer.
using TextBounds = std::function<QRect(int size, const std::string &text)>;

inline GraphLayout computeGraphLayout(const RenderOptions &options, const std::vector<std::string> &trackNames,
                                      double dpr, int viewWidth, int viewHeight, const TextBounds &textBounds)
{
    GraphLayout layout;
    const double majorLength = tickMajor * dpr;
    const double labelHeight = textBounds(4, "0").height();
    const double titleHeight = textBounds(1, "0").height();
    const double xs = spaceXs * dpr;

    // The bottom margin holds the ticks, the numbers and the title of the time axis. The smallest step
    // separates each part from the next part.
    double bottom = majorLength + xs;
    if (options.showAxisLabels) bottom += labelHeight + xs;
    if (options.showAxisTitles) bottom += titleHeight + xs;

    // The left margin holds the ticks, the numbers and the title of the frequency axis. The widest number has 5 digits.
    double left = majorLength + xs;
    if (options.showAxisLabels) {
        const QRect widest = textBounds(3, "00000");
        left += widest.x() + widest.width() + xs;
    }
    if (options.showAxisTitles) left += titleHeight;

    // The width of a text, and the width of the widest text that has the same pattern. The width of
    // a digit is not the same for all digits in a proportional font, so the widest digit sets the width.
    auto textWidth = [&](const std::string &text) {
        const QRect b = textBounds(0, text);
        return (double) (b.x() + b.width());
    };
    // The boxes of the tracks and the frame times use the monospace font, so each digit has the same width.
    auto monoWidth = [&](const std::string &text) {
        const QRect b = textBounds(3, text);
        return (double) (b.x() + b.width());
    };
    // The widest digit of the monospace font sets the width of a pattern.
    char widestDigit = '0';
    double widestWidth = 0.0;
    for (char c = '0'; c <= '9'; ++c) {
        const double w = monoWidth(std::string(1, c));
        if (w > widestWidth) {
            widestWidth = w;
            widestDigit = c;
        }
    }
    auto fixedWidth = [&](const char *pattern) { return monoWidth(fixedWidthSample(pattern, widestDigit)); };

    // A box has a padding of the small step, and two boxes have a gap of the small step. A row of text is
    // as high as a digit plus the smallest step, rounded up to the grid of the scale.
    const double pad = spaceSm * dpr;
    const double gap = spaceSm * dpr;
    const double padding = graphPadding * dpr;
    const double digit = textBounds(0, "0").height();
    const double grid = UiScale::gridUnit * dpr;

    ReadoutLayout &r = layout.readout;
    r.pad = pad;
    r.rowHeight = std::ceil((digit + xs) / grid) * grid;
    r.swatchWidth = spaceXl * dpr;

    // The size of each box depends on the settings and on the fonts. It never depends on the values,
    // so a box keeps its size when the values change.
    std::vector<BoxSize> sizes;
    for (const std::string &key : normalizeReadoutOrder(options.readoutOrder)) {
        BoxSize size;
        size.key = key;

        // The legend and the frame timing stand in the bar at the top of the graph.
        if (key == "legend") continue;
        if (!options.showStats || options.statsMask == 0) continue;
        const std::string *track = nullptr;
        for (const auto &name : trackNames) {
            if (trackKey(name) == key) track = &name;
        }
        if (track == nullptr || !options.trackBoxShown[trackIndex(*track)]) continue;

        BoxContent content;
        const bool isPitch = key == "pitch";
        for (const auto &c : statColumns()) {
            if (!(options.statsMask & c.bit)) continue;
            if (c.bit == StatCurrent) {
                content.showCurrent = true;
                content.headerValueWidth = fixedWidth(c.pattern);
            }
            else if (c.bit != StatNote || isPitch) {
                content.bits.push_back(c.bit);
                content.labelWidth = std::max(content.labelWidth, monoWidth(c.title));
                content.valueWidth = std::max(content.valueWidth, fixedWidth(c.pattern));
            }
        }
        if (!content.showCurrent && content.bits.empty()) continue;

        content.nameWidth = monoWidth(*track);
        const double header = r.swatchWidth + content.nameWidth + (content.showCurrent ? gap + content.headerValueWidth : 0.0);
        const double body = content.bits.empty() ? 0.0 : content.labelWidth + spaceMd * dpr + content.valueWidth;
        size.width = 2.0 * pad + std::max(header, body);
        size.height = 2.0 * pad + (1 + content.bits.size()) * r.rowHeight;
        r.content[key] = content;
        sizes.push_back(size);
    }

    // The box with the frame timing has one fixed size.
    r.debugWidth = 2.0 * pad + fixedWidth("Render 00.00 ms    Update 00.00 ms    Frame 000.0 ms (000 fps)");
    r.debugHeight = 2.0 * pad + r.rowHeight;

    // The legend is one row of entries. It has one size for the names of the tracks.
    BoxSize legendSize;
    legendSize.key = "legend";
    r.hasLegend = options.showLegend && !trackNames.empty();
    if (r.hasLegend) {
        const double entryGap = spaceMd * dpr;
        double width = 0.0;
        for (const auto &name : trackNames) width += r.swatchWidth + textWidth(name) + entryGap;
        width -= entryGap;
        legendSize.width = 2.0 * pad + width;
        legendSize.height = 2.0 * pad + r.rowHeight;
    }
    // The bar keeps the space of the frame timing while a recording hides the box, so the graph does not move.
    r.hasDebug = options.showDebug;

    // The bar at the top holds the legend and the frame timing. The two boxes stand side by side,
    // centered over the graph. The padding of the graph lies above the bar, and a gap lies below it.
    double barHeight = 0.0;
    double barWidth = 0.0;
    if (r.hasLegend) {
        barHeight = std::max(barHeight, legendSize.height);
        barWidth += legendSize.width;
    }
    if (r.hasDebug) {
        barHeight = std::max(barHeight, r.debugHeight);
        barWidth += r.debugWidth + (r.hasLegend ? gap : 0.0);
    }
    const double barTop = padding;

    // The graph has the padding of the scale on all four sides. The padding at the top lies above the bar.
    layout.plotTop = (int) std::ceil(barTop + (barHeight > 0.0 ? barHeight + gap : 0.0));
    layout.plotRight = (int) std::ceil(padding);

    if (!options.readoutLeft) {
        // The boxes stand in a row below the time axis. The row is as high as the highest box, with a gap above it.
        double rowHeight = 0.0;
        for (const BoxSize &b : sizes) rowHeight = std::max(rowHeight, b.height);
        layout.stripHeight = (int) std::ceil(rowHeight > 0.0 ? rowHeight + gap : 0.0);
        layout.plotBottom = (int) std::ceil(bottom + padding) + layout.stripHeight;
        layout.plotLeft = (int) std::ceil(padding + left);
        r.boxes = placeInRow(sizes, padding, viewHeight - padding - rowHeight, viewWidth - padding, gap);
    }
    else {
        // The boxes stand in a column at the left of the axis, from the top of the graph. A column that is too high
        // wraps. The column goes from the top to the bottom in the order of the tracks in the graph. The graph
        // shows the highest formant at the top and the pitch at the bottom, so the order is reversed.
        std::reverse(sizes.begin(), sizes.end());
        layout.stripHeight = 0;
        layout.plotBottom = (int) std::ceil(bottom + padding);
        double total = 0.0;
        r.boxes = placeInColumns(sizes, padding, layout.plotTop, viewHeight - layout.plotBottom - layout.plotTop, gap, total);
        // The padding separates the columns from the title of the frequency axis, as it separates the graph from the edge.
        layout.plotLeft = (int) std::ceil(padding + (sizes.empty() ? 0.0 : total + padding) + left);
    }

    // The bar is centered over the graph. A bar that is wider than the graph starts at the left edge of the graph.
    if (barHeight > 0.0) {
        const double plotWide = viewWidth - layout.plotLeft - layout.plotRight;
        double x = layout.plotLeft + std::max(0.0, 0.5 * (plotWide - barWidth));
        if (r.hasLegend) {
            r.legend.key = "legend";
            r.legend.width = legendSize.width;
            r.legend.height = legendSize.height;
            r.legend.x = x;
            r.legend.y = barTop + 0.5 * (barHeight - legendSize.height);
            x += legendSize.width + gap;
        }
        if (r.hasDebug) {
            r.debugX = x;
            r.debugY = barTop + 0.5 * (barHeight - r.debugHeight);
        }
    }

    // A small window leaves no room for the margins. The graph keeps at least one pixel in each direction, and
    // the boxes go away, because they would stand on the graph.
    const int roomX = std::max(viewWidth - 1, 0);
    const int roomY = std::max(viewHeight - 1, 0);
    if (layout.plotLeft + layout.plotRight > roomX || layout.plotTop + layout.plotBottom > roomY) {
        shrinkMargins(layout.plotLeft, layout.plotRight, roomX);
        shrinkMargins(layout.plotTop, layout.plotBottom, roomY);
        layout.stripHeight = std::min(layout.stripHeight, layout.plotBottom);
        for (BoxPlacement &box : r.boxes) box.visible = false;
        r.hasLegend = r.hasLegend && layout.plotWidth(viewWidth) >= r.legend.width;
        r.hasDebug = r.hasDebug && layout.plotWidth(viewWidth) >= r.debugWidth;
    }
    return layout;
}

}

#endif // GUI_GRAPHLAYOUT_H
