#include "qpainterwrapper.h"
#include "ticks.h"
#include "readoutlayout.h"
#include "../context/timings.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>

using namespace Gui;

namespace {

const QColor outlineColor(0, 0, 0, 255);
const QColor tickColor(255, 255, 255, 235);
const QColor titleColor(200, 200, 210, 230);

std::string format(const char *fmt, double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), fmt, value);
    return buffer;
}

// A value of a statistic in the narrow columns. A value of 1000 or more has no decimals.
std::string number(double value)
{
    return format(std::fabs(value) >= 1000.0 ? "%.0f" : "%.1f", value);
}

double percentile(const std::vector<double> &sorted, double fraction)
{
    if (sorted.empty()) return 0.0;
    const double position = fraction * (sorted.size() - 1);
    const size_t lo = (size_t) std::floor(position);
    const size_t hi = std::min(lo + 1, sorted.size() - 1);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * (position - lo);
}

// One point of a track in the time and the transformed frequency.
struct TrackPoint {
    double time;
    double value;
    bool valid;
};

// One statistic. The pattern is the widest text of its value, with digits as place holders.
struct StatColumn {
    int bit;
    const char *title;
    const char *pattern;
};

const StatColumn statColumns[] = {
    { StatCurrent, "Now", "0000.0" }, { StatMin, "Min", "0000.0" }, { StatP5, "P5", "0000.0" },
    { StatMean, "Mean", "0000.0" }, { StatMedian, "Median", "0000.0" }, { StatStdDev, "SD", "0000.0" },
    { StatP95, "P95", "0000.0" }, { StatMax, "Max", "0000.0" }, { StatRange, "Range", "0000.0" },
    { StatCV, "CV", "000.0%" }, { StatVoiced, "Voiced", "000%" }, { StatCount, "N", "000000" },
    { StatNote, "Note", "G#4 -00c" },
};

// The index of a track in the options: 0 is the pitch, and 1 to 4 are the formants F1 to F4.
// The formants after F4 use the index of F4.
int trackIndex(const std::string &name)
{
    if (name == "Pitch") return 0;
    const int n = name.size() > 1 ? std::atoi(name.c_str() + 1) : 1;
    return std::clamp(n, 1, 4);
}

std::string trackKey(const std::string &name)
{
    std::string key = name;
    for (char &c : key) c = (char) std::tolower((unsigned char) c);
    return key;
}

const double readoutPad = 7.0;
// The space at the left and the right of the strip, and between the boxes of the strip.
const double stripMargin = 10.0;
const double stripGap = 8.0;

}

void QPainterWrapper::updateLayout(const std::vector<std::string> &trackNames)
{
    const double dpr = p->devicePixelRatio();
    const double majorLength = 9.0 * dpr;
    const double labelHeight = p->textBoundsOfSize(4, "0").height();

    // The bottom margin holds the ticks, the numbers and the title of the time axis.
    double bottom = majorLength + 6.0 * dpr;
    if (mOptions.showAxisLabels) bottom += labelHeight + 4.0 * dpr;
    if (mOptions.showAxisTitles) bottom += labelHeight + 10.0 * dpr;

    // The left margin holds the ticks, the numbers and the title of the frequency axis. The widest number has 5 digits.
    double left = majorLength + 6.0 * dpr;
    if (mOptions.showAxisLabels) {
        const QRect widest = p->textBoundsOfSize(3, "00000");
        left += widest.x() + widest.width() + 4.0 * dpr;
    }
    if (mOptions.showAxisTitles) left += labelHeight + 10.0 * dpr;

    // The width of a text, and the width of the widest text that has the same pattern. The width of
    // a digit is not the same for all digits in a proportional font, so the widest digit sets the width.
    auto textWidth = [&](const std::string &text) {
        const QRect b = p->textBoundsOfSize(0, text);
        return (double) (b.x() + b.width());
    };
    char widestDigit = '0';
    double widestWidth = 0.0;
    for (char c = '0'; c <= '9'; ++c) {
        const double w = textWidth(std::string(1, c));
        if (w > widestWidth) {
            widestWidth = w;
            widestDigit = c;
        }
    }
    // The boxes of the tracks and the frame times use the monospace font, so each digit has the same width.
    auto monoWidth = [&](const std::string &text) {
        const QRect b = p->textBoundsOfSize(3, text);
        return (double) (b.x() + b.width());
    };
    auto fixedWidth = [&](const char *pattern) { return monoWidth(fixedWidthSample(pattern, '0')); };

    const double pad = readoutPad * dpr;
    const double gap = stripGap * dpr;
    const double margin = stripMargin * dpr;
    const double digit = p->textBoundsOfSize(0, "0").height();

    ReadoutLayout &r = mReadout;
    r = ReadoutLayout();
    r.pad = pad;
    r.rowHeight = digit + 7.0 * dpr;
    r.swatchWidth = 22.0 * dpr;

    // The size of each box depends on the settings and on the fonts. It never depends on the values,
    // so a box keeps its size when the values change.
    std::vector<BoxSize> sizes;
    for (const std::string &key : normalizeReadoutOrder(mOptions.readoutOrder)) {
        BoxSize size;
        size.key = key;

        // The legend and the frame timing stand in the bar at the top of the graph.
        if (key == "legend") continue;
        else {
            if (!mOptions.showStats || mOptions.statsMask == 0) continue;
            const std::string *track = nullptr;
            for (const auto &name : trackNames) {
                if (trackKey(name) == key) track = &name;
            }
            if (track == nullptr || !mOptions.trackBoxShown[trackIndex(*track)]) continue;

            BoxContent content;
            const bool isPitch = key == "pitch";
            for (const auto &c : statColumns) {
                if (!(mOptions.statsMask & c.bit)) continue;
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
            const double header = r.swatchWidth + content.nameWidth + (content.showCurrent ? 10.0 * dpr + content.headerValueWidth : 0.0);
            const double body = content.bits.empty() ? 0.0 : content.labelWidth + 14.0 * dpr + content.valueWidth;
            size.width = 2.0 * pad + std::max(header, body);
            size.height = 2.0 * pad + (1 + content.bits.size()) * r.rowHeight - 3.0 * dpr;
            r.content[key] = content;
        }
        sizes.push_back(size);
    }

    // The box with the frame timing has one fixed size.
    r.debugWidth = 2.0 * pad + fixedWidth("Render 00.00 ms    Update 00.00 ms    Frame 000.0 ms (000 fps)");
    r.debugHeight = 2.0 * pad + std::max(digit + 6.0 * dpr, 14.0 * dpr) - 2.0 * dpr;

    // The legend is one row of entries. It has one size for the names of the tracks.
    BoxSize legendSize;
    legendSize.key = "legend";
    r.hasLegend = mOptions.showLegend && !trackNames.empty();
    if (r.hasLegend) {
        const double entryGap = 14.0 * dpr;
        double width = 0.0;
        for (const auto &name : trackNames) width += r.swatchWidth + textWidth(name) + entryGap;
        width -= entryGap;
        legendSize.width = 2.0 * pad + width;
        legendSize.height = 2.0 * pad + std::max(digit + 6.0 * dpr, 14.0 * dpr) - 2.0 * dpr;
    }
    // The bar keeps the space of the frame timing while a recording hides the box, so the graph does not move.
    r.hasDebug = mOptions.showDebug;

    // The bar at the top holds the legend and the frame timing. The two boxes stand side by side,
    // centred over the graph. The bar has a gap above it and a gap below it.
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
    const double barTotal = barHeight > 0.0 ? barHeight + 2.0 * gap : 0.0;
    const int top = (int) std::ceil(barTotal);

    const double viewWidth = viewport().width();
    const double viewHeight = viewport().height();

    mPlotLeft = (int) std::ceil(left);

    if (!mOptions.readoutLeft) {
        // The boxes stand in a row below the time axis. The row is as high as the highest box.
        double strip = 0.0;
        for (const BoxSize &b : sizes) strip = std::max(strip, b.height);
        // The strip has a gap above it and a margin below it.
        if (strip > 0.0) strip += 2.0 * gap;
        mStripHeight = (int) std::ceil(strip);
        mPlotBottom = (int) std::ceil(bottom) + mStripHeight;
        r.boxes = placeInRow(sizes, margin, viewHeight - mStripHeight + gap, viewWidth - margin, gap);
    }
    else {
        // The boxes stand in a column at the left of the axis, below the bar. A column that is too high wraps.
        // The column goes from the top to the bottom in the order of the tracks in the graph. The graph
        // shows the highest formant at the top and the pitch at the bottom, so the order is reversed.
        std::reverse(sizes.begin(), sizes.end());
        mStripHeight = 0;
        mPlotBottom = (int) std::ceil(bottom);
        double total = 0.0;
        r.boxes = placeInColumns(sizes, margin, barTotal + gap, viewHeight - mPlotBottom - barTotal - 2.0 * gap, gap, total);
        mPlotLeft = (int) std::ceil(left + (sizes.empty() ? 0.0 : total + 2.0 * margin));
    }
    mPlotTop = top;

    // The bar is centred over the graph. A bar that is wider than the graph starts at the left edge of the graph.
    if (barHeight > 0.0) {
        const double plotWide = viewWidth - mPlotLeft;
        double x = mPlotLeft + std::max(0.0, 0.5 * (plotWide - barWidth));
        if (r.hasLegend) {
            r.legend.key = "legend";
            r.legend.width = legendSize.width;
            r.legend.height = legendSize.height;
            r.legend.x = x;
            r.legend.y = gap + 0.5 * (barHeight - legendSize.height);
            x += legendSize.width + gap;
        }
        if (r.hasDebug) {
            r.debugX = x;
            r.debugY = gap + 0.5 * (barHeight - r.debugHeight);
        }
    }

    p->setPlotMargins(mPlotLeft, mPlotBottom, mPlotTop);
    // The code of the graph counts from the top of the graph.
    p->setOriginY(mPlotTop);
}

void QPainterWrapper::beginPlot()
{
    p->setClip(mPlotLeft, 0, plotWidth(), plotHeight());
}

void QPainterWrapper::endPlot()
{
    p->clearClip();
}

void QPainterWrapper::drawTimeAxis()
{
    const double dpr = p->devicePixelRatio();
    const int width = viewport().width();
    const int height = viewport().height();

    const double label0Height = p->textBoundsOfSize(4, "0").height();

    // The axis is below the graph. The ticks start at the lower edge of the graph and point down.
    const double tickBase = plotHeight();

    // The widest label has the most digits. Use the time labels at the ends as the estimate.
    const double sampleWidth = std::max<double>(
        p->textBoundsOfSize(4, formatTime(mTimeEnd, 0.001)).width(),
        p->textBoundsOfSize(4, "00.000").width());

    auto toPixel = [&](double t) { return mapTimeToX(t); };
    const auto ticks = makeTimeTicks(static_cast<TimeTickMode>(mOptions.timeTickMode), mOptions.timeTickStep,
                                     mTimeStart, mTimeEnd, toPixel, sampleWidth * 0.8, 10.0 * dpr);

    std::vector<float> small, medium;
    std::vector<float> large;

    std::vector<bool> covered(std::max(width, 1), false);

    const double majorLength = 9.0 * dpr;
    const double minorLength = 4.0 * dpr;

    for (const Tick &tick : ticks) {
        // The recording starts at time zero. No data exists before that time.
        if (tick.value < -1e-9) continue;

        const double x = mapTimeToX(tick.value);
        if (x < mPlotLeft - 1 || x > width + 1) continue;

        const bool labeled = !tick.label.empty();
        auto &list = labeled ? large : small;
        list.insert(list.end(), { (float) x, (float) tickBase, (float) x, (float) (tickBase + (labeled ? majorLength : minorLength)) });

        if (labeled && mOptions.showAxisLabels) {
            const QRect bounds = p->textBoundsOfSize(4, tick.label);
            const double start = x - (bounds.x() + bounds.width() / 2.0);
            const double left = start + bounds.x();
            const double right = left + bounds.width();

            // A label that crosses the edge of the view is not readable. Skip it.
            bool free = left >= 0 && right <= width;
            for (int tx = std::max(0, (int) std::floor(left) - 4); free && tx <= std::min(width - 1, (int) std::ceil(right) + 4); ++tx) {
                if (covered[tx]) { free = false; break; }
            }
            if (free) {
                p->drawTextRotated(4, start, tickBase + majorLength + 4.0 * dpr + label0Height, 0.0f, Qt::white, tick.label, outlineColor);
                for (int tx = std::max(0, (int) std::floor(left)); tx <= std::min(width - 1, (int) std::ceil(right)); ++tx) {
                    covered[tx] = true;
                }
            }
        }
    }

    p->drawSegments(small.data(), (int) small.size() / 4, 1.2f, tickColor);
    p->drawSegments(large.data(), (int) large.size() / 4, 2.0f, tickColor);

    // The line of the axis along the lower edge of the graph.
    const float edge[4] = { (float) mPlotLeft, (float) tickBase, (float) width, (float) tickBase };
    p->drawSegments(edge, 1, 1.6f, tickColor);

    if (mOptions.showAxisTitles) {
        const std::string title = "Time (s)";
        const QRect bounds = p->textBoundsOfSize(1, title);
        const double x = mPlotLeft + 0.5 * plotWidth() - (bounds.x() + bounds.width() / 2.0);
        const double y = tickBase + majorLength + 4.0 * dpr + (mOptions.showAxisLabels ? label0Height + 6.0 * dpr : 0.0) + label0Height + 4.0 * dpr;
        p->drawTextRotated(1, x, y, 0.0f, titleColor, title, outlineColor);
    }
}

void QPainterWrapper::drawFrequencyBands()
{
    if (!mOptions.showBands) return;

    struct Zone {
        double low;
        double high;
        QColor color;
        const char *name;
    };
    // The middle zone has a yellow-green colour, so it is not on the pink and blue axis.
    const Zone zones[3] = {
        { mOptions.bandEdges[0], mOptions.bandEdges[1], QColor(74, 144, 226), "Masculine" },
        { mOptions.bandEdges[1], mOptions.bandEdges[2], QColor(190, 205, 70), "Androgynous" },
        { mOptions.bandEdges[2], mOptions.bandEdges[3], QColor(232, 111, 164), "Feminine" },
    };

    const double dpr = p->devicePixelRatio();
    const double height = plotHeight();
    const double width = plotWidth();

    for (const Zone &zone : zones) {
        const double top = std::clamp(mapFrequencyToY(zone.high), 0.0, height);
        const double bottom = std::clamp(mapFrequencyToY(zone.low), 0.0, height);
        if (bottom - top < 1.0) continue;

        QColor fill = zone.color;
        fill.setAlpha(38);
        p->drawRoundedRect((float) mPlotLeft, (float) top, (float) width, (float) (bottom - top), 0.0f, fill);

        // The name of the zone and its range, at the left, below the upper edge of the zone.
        const double digit = p->textBoundsOfSize(2, "0").height();
        // A thin zone has no label, because the label would touch the label of its neighbour.
        if (bottom - top < 3.0 * digit) continue;
        QColor text = zone.color.lighter(150);
        const std::string label = std::string(zone.name) + " zone, " + std::to_string((int) std::lround(zone.low))
            + " to " + std::to_string((int) std::lround(zone.high)) + " Hz";
        p->drawTextRotated(2, mPlotLeft + 8.0 * dpr, top + digit + 4.0 * dpr, 0.0f, text, label, outlineColor);
    }

    // A thin line at each edge.
    std::vector<float> lines;
    for (int i = 0; i < 4; ++i) {
        const double y = mapFrequencyToY(mOptions.bandEdges[i]);
        if (y < 0.0 || y > height) continue;
        lines.insert(lines.end(), { (float) mPlotLeft, (float) y, (float) (mPlotLeft + width), (float) y });
    }
    if (!lines.empty()) {
        p->drawSegments(lines.data(), (int) lines.size() / 4, 1.0f, QColor(255, 255, 255, 70));
    }
}

void QPainterWrapper::drawFrequencyScale()
{
    const double dpr = p->devicePixelRatio();
    const int width = viewport().width();
    const int height = viewport().height();

    const double digitHeight = p->textBoundsOfSize(3, "0").height();

    auto toPixel = [&](double f) { return mapFrequencyToY(f); };
    const auto ticks = makeFrequencyTicks(static_cast<FrequencyTickMode>(mOptions.freqTickMode), mOptions.freqTickStep,
                                          mMinFrequency, mMaxFrequency, toPixel, digitHeight, 0.9 * digitHeight);

    std::vector<float> small, medium, large;

    const double majorLength = 9.0 * dpr;
    const double mediumLength = 6.0 * dpr;
    const double minorLength = 4.0 * dpr;
    // The axis is at the left of the graph. The ticks start at the left edge of the graph and point left.
    const double labelRight = mPlotLeft - majorLength - 5.0 * dpr;

    double widestLabel = 0.0;

    for (const Tick &tick : ticks) {
        const double y = mapFrequencyToY(tick.value);
        if (y < -1 || y > plotHeight() + 1) continue;

        const bool labeled = !tick.label.empty();
        if (labeled) {
            large.insert(large.end(), { (float) (mPlotLeft - majorLength), (float) y, (float) mPlotLeft, (float) y });
        }
        else if (tick.level == 1) {
            medium.insert(medium.end(), { (float) (mPlotLeft - mediumLength), (float) y, (float) mPlotLeft, (float) y });
        }
        else {
            small.insert(small.end(), { (float) (mPlotLeft - minorLength), (float) y, (float) mPlotLeft, (float) y });
        }

        if (labeled && mOptions.showAxisLabels) {
            const int size = tick.level == 0 ? 3 : 4;
            const QRect bounds = p->textBoundsOfSize(size, tick.label);
            const double digit = p->textBoundsOfSize(size, "0").height();
            // A label that crosses the top edge, or the lower edge of the graph, is not readable. Skip it.
            if (y - digit / 2.0 < 0 || y + digit / 2.0 > plotHeight()) continue;
            p->drawTextRotated(size, labelRight - (bounds.x() + bounds.width()), y + digit / 2.0, 0.0f,
                               Qt::white, tick.label, outlineColor);
            widestLabel = std::max<double>(widestLabel, bounds.width() + bounds.x());
        }
    }

    p->drawSegments(small.data(), (int) small.size() / 4, 1.2f, tickColor);
    p->drawSegments(medium.data(), (int) medium.size() / 4, 1.6f, tickColor);
    p->drawSegments(large.data(), (int) large.size() / 4, 2.0f, tickColor);

    // The line of the axis along the left edge of the graph.
    const float edge[4] = { (float) mPlotLeft, 0.0f, (float) mPlotLeft, (float) plotHeight() };
    p->drawSegments(edge, 1, 1.6f, tickColor);

    if (mOptions.showAxisTitles) {
        std::string title = "Frequency (Hz";
        switch (mFrequencyScale) {
        case FrequencyScale::Linear: title += ", linear"; break;
        case FrequencyScale::Logarithmic: title += ", log"; break;
        case FrequencyScale::Mel: title += ", mel"; break;
        case FrequencyScale::ERB: title += ", ERB"; break;
        }
        title += ")";

        const QRect bounds = p->textBoundsOfSize(1, title);
        const double ascent = p->textBoundsOfSize(1, "0").height();
        const double x = labelRight - (mOptions.showAxisLabels ? widestLabel : 0.0) - 10.0 * dpr;
        // The text runs upward. Its start is at the bottom, and its baseline is on the right.
        const double y = 0.5 * plotHeight() + (bounds.x() + bounds.width() / 2.0);
        p->drawTextRotated(1, x + ascent * 0.5 - 0.0, y, 90.0f, titleColor, title, outlineColor);
    }
}

namespace {

// Smooth the values with a gaussian filter. sigma is in samples.
void smoothValues(std::vector<double> &values, double sigma)
{
    if (sigma < 0.05 || values.size() < 3) return;

    const int radius = std::max(1, (int) std::ceil(3.0 * sigma));
    std::vector<double> kernel(2 * radius + 1);
    for (int k = -radius; k <= radius; ++k) {
        kernel[k + radius] = std::exp(-0.5 * k * k / (sigma * sigma));
    }

    std::vector<double> out(values.size());
    for (int i = 0; i < (int) values.size(); ++i) {
        double sum = 0, weight = 0;
        for (int k = -radius; k <= radius; ++k) {
            const int j = i + k;
            if (j < 0 || j >= (int) values.size()) continue;
            sum += kernel[k + radius] * values[j];
            weight += kernel[k + radius];
        }
        out[i] = sum / weight;
    }
    values.swap(out);
}

// A point on the Catmull-Rom curve between b and c.
QPointF catmullRom(const QPointF &a, const QPointF &b, const QPointF &c, const QPointF &d, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return 0.5 * ((2.0 * b) + (-a + c) * t + (2.0 * a - 5.0 * b + 4.0 * c - d) * t2 + (-a + 3.0 * b - 3.0 * c + d) * t3);
}

}

namespace {

template <typename Iterator, typename Value>
void collectPoints(Iterator begin, Iterator end, Value valueOf, std::vector<TrackPoint> &out,
                   const std::function<double(double)> &transform)
{
    for (auto it = begin; it != end; ++it) {
        double v;
        if (valueOf(*it, v)) {
            out.push_back({ it->first, transform(v), true });
        }
        else {
            out.push_back({ it->first, 0.0, false });
        }
    }
}

}

namespace {

void drawTrackPoints(Gui::CanvasRenderer *p, const Gui::RenderOptions &o,
                     const std::vector<TrackPoint> &points,
                     const std::function<double(double)> &toX,
                     const std::function<double(double)> &toY,
                     double maxJump,
                     const QColor &color,
                     int style)
{
    // Split the track into runs. A run has no frame without a value.
    std::vector<std::vector<size_t>> runs;
    std::vector<size_t> current;
    // A run also ends at a long time gap and at a jump of the value. A line across a jump
    // shows a change that never happened, for example an octave error of the pitch.
    std::vector<double> gaps;
    for (size_t i = 1; i < points.size(); ++i) {
        gaps.push_back(std::fabs(points[i].time - points[i - 1].time));
    }
    double medianGap = 0.0;
    if (!gaps.empty()) {
        std::nth_element(gaps.begin(), gaps.begin() + gaps.size() / 2, gaps.end());
        medianGap = gaps[gaps.size() / 2];
    }
    const double maxGap = medianGap > 0 ? 3.0 * medianGap : 1e9;

    for (size_t i = 0; i < points.size(); ++i) {
        if (points[i].valid) {
            if (!current.empty()) {
                const size_t prev = current.back();
                if (std::fabs(points[i].time - points[prev].time) > maxGap
                    || std::fabs(toY(points[i].value) - toY(points[prev].value)) > maxJump) {
                    runs.push_back(std::move(current));
                    current.clear();
                }
            }
            current.push_back(i);
        }
        else if (!current.empty()) {
            runs.push_back(std::move(current));
            current.clear();
        }
    }
    if (!current.empty()) runs.push_back(std::move(current));

    const bool drawLines = (style & 2) != 0;
    const bool drawDots = (style & 1) != 0;

    rpm::vector<QPointF> dots;

    for (const auto &run : runs) {
        std::vector<double> values;
        values.reserve(run.size());
        for (size_t i : run) values.push_back(points[i].value);

        // The dots are on the measured values. The line follows the smoothed values.
        if (drawDots) {
            for (size_t i = 0; i < run.size(); ++i) {
                dots.emplace_back(toX(points[run[i]].time), toY(values[i]));
            }
        }

        if (!drawLines || run.size() < 2) continue;

        smoothValues(values, o.lineSmoothness * 2.5);

        std::vector<QPointF> pixels;
        pixels.reserve(run.size());
        for (size_t i = 0; i < run.size(); ++i) {
            pixels.emplace_back(toX(points[run[i]].time), toY(values[i]));
        }

        // A curve through the points. More subdivisions give a smoother curve. Skip them where
        // the points are closer than a pixel, because the curve has no detail there.
        const double span = std::fabs(pixels.back().x() - pixels.front().x());
        const bool dense = span < 0.7 * pixels.size();
        const int subdivisions = (o.lineSmoothness > 0.01 && !dense) ? 1 + (int) std::lround(o.lineSmoothness * 7.0) : 1;

        rpm::vector<QPointF> line;
        if (subdivisions == 1) {
            for (const auto &pt : pixels) line.push_back(pt);
        }
        else {
            for (size_t i = 0; i + 1 < pixels.size(); ++i) {
                const QPointF &a = pixels[i == 0 ? 0 : i - 1];
                const QPointF &b = pixels[i];
                const QPointF &c = pixels[i + 1];
                const QPointF &d = pixels[std::min(i + 2, pixels.size() - 1)];
                for (int k = 0; k < subdivisions; ++k) {
                    line.push_back(catmullRom(a, b, c, d, double(k) / subdivisions));
                }
            }
            line.push_back(pixels.back());
        }

        p->drawPolyline(line, (float) o.lineWidth, color, outlineColor);
    }

    if (drawDots && !dots.empty()) {
        p->drawScatterWithOutline(dots, (float) o.dotRadius, color, outlineColor);
    }
}

}

void QPainterWrapper::drawFrequencyTrack(
            const TimeTrack<double>::const_iterator& begin,
            const TimeTrack<double>::const_iterator& end,
            const QColor &color,
            int style)
{
    std::vector<TrackPoint> points;
    collectPoints(begin, end, [](const auto &item, double &v) { v = item.second; return true; }, points,
                  [this](double f) { return f; });

    drawTrackPoints(p, mOptions, points,
                    [this](double t) { return mapTimeToX(t); },
                    [this](double f) { return mapFrequencyToY(f); },
                    0.1 * plotHeight(),
                    color,
                    style);
}

void QPainterWrapper::drawFrequencyTrack(
            const OptionalTimeTrack<double>::const_iterator& begin,
            const OptionalTimeTrack<double>::const_iterator& end,
            const QColor &color,
            int style)
{
    std::vector<TrackPoint> points;
    collectPoints(begin, end, [](const auto &item, double &v) {
                      if (!item.second.has_value()) return false;
                      v = *item.second;
                      return true;
                  }, points, [](double f) { return f; });

    // The values stay in Hz until the mapping to pixels, so the filter works on the value of the view scale.
    for (auto &pt : points) {
        if (pt.valid) pt.value = transformFrequency(pt.value);
    }

    drawTrackPoints(p, mOptions, points,
                    [this](double t) { return mapTimeToX(t); },
                    [this](double v) {
                        // v is on the frequency scale of the view already.
                        const double vmin = transformFrequency(mMinFrequency);
                        const double vmax = transformFrequency(mMaxFrequency);
                        return plotHeight() * (1.0 - (v - vmin) / (vmax - vmin));
                    },
                    0.1 * plotHeight(),
                    color,
                    style);
}

bool QPainterWrapper::plotContains(double x, double y) const
{
    // The point is in pixels of the canvas. The graph starts below the bar at the top.
    return x >= mPlotLeft && x <= viewport().width() && y >= mPlotTop && y <= mPlotTop + plotHeight();
}

double QPainterWrapper::timeAtX(double x) const
{
    return mTimeStart + (x - mPlotLeft) / std::max(plotWidth(), 1) * (mTimeEnd - mTimeStart);
}

void QPainterWrapper::drawCursor(double time, const std::vector<CursorValue> &values)
{
    const double dpr = p->devicePixelRatio();
    const double x = std::round(mapTimeToX(time));
    if (x < mPlotLeft || x > viewport().width()) return;

    const double plotBottom = plotHeight();
    const QColor accent = mOptions.theme.accent;

    // The bar has a dark casing and a core in a light tone of the theme, so it shows on any picture.
    const QColor core = QColor::fromRgbF(0.5 * accent.redF() + 0.5, 0.5 * accent.greenF() + 0.5,
                                         0.5 * accent.blueF() + 0.5, 1.0);
    const float bar[4] = { (float) x, 0.0f, (float) x, (float) plotBottom };
    p->drawSegments(bar, 1, (float) (7.0 * dpr), QColor(0, 0, 0, 235));
    p->drawSegments(bar, 1, (float) (3.0 * dpr), core);

    // One label for the time, and one for each track that has a value at the moment.
    struct Label {
        std::string text;
        QColor color;
        double anchorY;
        double width, height, top;
        bool dot;
    };
    std::vector<Label> labels;

    labels.push_back({ format("%.3f", time) + " s", Qt::white, 0.0, 0, 0, 0, false });
    for (const CursorValue &v : values) {
        std::string text = v.name + "  " + number(v.frequency) + " Hz";
        if (v.isPitch) {
            double cents = 0;
            const std::string note = noteName(v.frequency, &cents);
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "  %s %+.0fc", note.c_str(), cents);
            text += buffer;
        }
        labels.push_back({ text, v.color, std::clamp<double>(mapFrequencyToY(v.frequency), 0.0, plotBottom), 0, 0, 0, true });
    }

    const double padX = 8.0 * dpr;
    const double padY = 5.0 * dpr;
    const double gap = 12.0 * dpr;
    const double edge = 4.0 * dpr;
    // The top of the canvas lies a little under the toolbar, so the first label keeps off the edge.
    const double topEdge = 14.0 * dpr;
    const double digit = p->textBoundsOfSize(3, "0").height();
    double widest = 0;
    for (Label &l : labels) {
        const QRect b = p->textBoundsOfSize(3, l.text);
        l.width = b.x() + b.width() + 2.0 * padX;
        l.height = digit + 2.0 * padY;
        widest = std::max(widest, l.width);
    }

    // The labels go to the right of the bar. When they do not fit there, they go to the left.
    const double roomRight = viewport().width() - x - gap - edge;
    const double roomLeft = x - mPlotLeft - gap - edge;
    const bool right = roomRight >= widest || roomRight >= roomLeft;

    // Place the labels from the top down, with no overlap, and then pull them back inside the graph.
    std::sort(labels.begin(), labels.end(), [](const Label &a, const Label &b) { return a.anchorY < b.anchorY; });
    double next = topEdge;
    for (Label &l : labels) {
        l.top = std::max(l.anchorY - 0.5 * l.height, next);
        next = l.top + l.height + 3.0 * dpr;
    }
    double limit = plotBottom - edge;
    for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        it->top = std::min(it->top, limit - it->height);
        limit = it->top - 3.0 * dpr;
    }

    // The panel has an opaque background in a dark tone of the theme, and a border in a light tone of the theme.
    const QColor panel = QColor::fromRgbF(0.07 * accent.redF() + 0.02, 0.07 * accent.greenF() + 0.02,
                                          0.07 * accent.blueF() + 0.03, 1.0);

    for (const Label &l : labels) {
        const double left = right ? x + gap : x - gap - l.width;
        p->drawRoundedRect((float) left, (float) l.top, (float) l.width, (float) l.height, 5.0f * (float) dpr,
                           panel, core, (float) (1.4 * dpr));
        const double baseline = l.top + 0.5 * l.height + 0.5 * digit;
        p->drawTextRotated(3, (float) (left + padX), (float) baseline, 0.0f, l.color, l.text, outlineColor);
    }

    // The dots lie on the bar at the value of each track.
    for (const Label &l : labels) {
        if (!l.dot) continue;
        rpm::vector<QPointF> dot(1, QPointF(x, l.anchorY));
        p->drawScatterWithOutline(dot, (float) (6.0 * dpr), l.color, QColor(255, 255, 255, 255));
    }
}

namespace {

// The statistics of the values of one track.
struct TrackStats {
    std::vector<double> sorted;
    double mean = 0.0;
    double sd = 0.0;
    double p5 = 0.0;
    double p95 = 0.0;
};

TrackStats computeStats(const HudTrack &t)
{
    TrackStats s;
    s.sorted = t.values;
    std::sort(s.sorted.begin(), s.sorted.end());
    if (s.sorted.empty()) return s;

    s.mean = std::accumulate(s.sorted.begin(), s.sorted.end(), 0.0) / s.sorted.size();
    double variance = 0;
    for (double v : s.sorted) variance += (v - s.mean) * (v - s.mean);
    s.sd = s.sorted.size() > 1 ? std::sqrt(variance / (s.sorted.size() - 1)) : 0.0;
    s.p5 = percentile(s.sorted, 0.05);
    s.p95 = percentile(s.sorted, 0.95);
    return s;
}

// The text of one statistic of one track. A track with no value has a dash.
std::string statText(const HudTrack &t, const TrackStats &s, int bit, bool rangeSemitones)
{
    if (bit == StatVoiced) {
        return t.frames > 0 ? format("%.0f%%", 100.0 * (t.count > 0 ? t.count : t.values.size()) / t.frames) : "-";
    }
    if (s.sorted.empty()) return "-";

    switch (bit) {
    case StatCurrent: return number(t.values.back());
    case StatMin: return number(s.sorted.front());
    case StatP5: return number(s.p5);
    case StatMean: return number(s.mean);
    case StatMedian: return number(percentile(s.sorted, 0.5));
    case StatStdDev: return number(s.sd);
    case StatP95: return number(s.p95);
    case StatMax: return number(s.sorted.back());
    case StatRange:
        if (rangeSemitones) return s.p5 > 0 && s.p95 >= s.p5 ? format("%.1fst", 12.0 * std::log2(s.p95 / s.p5)) : "-";
        return number(s.p95 - s.p5);
    case StatCV: return s.mean > 0 ? format("%.1f%%", 100.0 * s.sd / s.mean) : "-";
    case StatCount: return std::to_string(t.count > 0 ? t.count : s.sorted.size());
    case StatNote:
        if (t.isPitch) {
            double cents = 0;
            const std::string note = noteName(s.mean, &cents);
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%s %+.0fc", note.c_str(), cents);
            return buffer;
        }
        break;
    }
    return "-";
}

}

void QPainterWrapper::drawHud(const std::vector<HudTrack> &tracks)
{
    const double dpr = p->devicePixelRatio();
    const ReadoutLayout &r = mReadout;
    const double pad = r.pad;

    // The boxes use the origin of the canvas, and the graph uses the origin below the bar at the top.
    p->setOriginY(0);

    const QColor fill(8, 8, 12, 200);
    const QColor border(255, 255, 255, 48);
    const QColor greyText(170, 170, 185);

    const double digit = p->textBoundsOfSize(0, "0").height();
    auto textRight = [&](const std::string &text) {
        const QRect b = p->textBoundsOfSize(0, text);
        return (double) (b.x() + b.width());
    };
    auto monoRight = [&](const std::string &text) {
        const QRect b = p->textBoundsOfSize(3, text);
        return (double) (b.x() + b.width());
    };

    // The swatch of a track in the style of the track: a line, a dot, or both.
    auto drawSwatch = [&](double x, double cy, const HudTrack &t) {
        const int style = mOptions.trackStyle[trackIndex(t.name)];
        const float seg[4] = { (float) x, (float) cy, (float) (x + r.swatchWidth - 6.0 * dpr), (float) cy };
        if (style & 2) {
            p->drawSegments(seg, 1, (float) mOptions.lineWidth, t.color);
        }
        if (style & 1) {
            rpm::vector<QPointF> dot(1, QPointF(x + 0.5 * (r.swatchWidth - 6.0 * dpr), cy));
            p->drawScatterWithOutline(dot, (float) mOptions.dotRadius, t.color, outlineColor);
        }
    };

    // The legend is a row of entries in the bar at the top.
    if (r.hasLegend) {
        const BoxPlacement &box = r.legend;
        p->drawRoundedRect(box.x, box.y, box.width, box.height, 6.0 * dpr, fill, border, 1.0f);
        const double entryGap = 14.0 * dpr;
        const double cy = box.y + 0.5 * box.height;
        double ex = box.x + pad;
        for (const auto &t : tracks) {
            drawSwatch(ex, cy, t);
            p->drawTextRotated(0, ex + r.swatchWidth, cy + 0.5 * digit, 0.0f, Qt::white, t.name, outlineColor);
            ex += r.swatchWidth + textRight(t.name) + entryGap;
        }
    }

    for (const BoxPlacement &box : r.boxes) {
        if (!box.visible) continue;

        p->drawRoundedRect(box.x, box.y, box.width, box.height, 6.0 * dpr, fill, border, 1.0f);

        // A box of one track: a header with the name and the live value, and a line for each statistic.
        const auto content = r.content.find(box.key);
        if (content == r.content.end()) continue;
        const BoxContent &c = content->second;

        const HudTrack *track = nullptr;
        for (const auto &t : tracks) {
            if (trackKey(t.name) == box.key) track = &t;
        }
        if (track == nullptr) continue;

        const TrackStats stats = computeStats(*track);
        const double valueRight = box.x + box.width - pad;

        double base = box.y + pad + r.rowHeight - 4.0 * dpr;
        drawSwatch(box.x + pad, base - 0.5 * digit, *track);
        p->drawTextRotated(3, box.x + pad + r.swatchWidth, base, 0.0f, track->color, track->name, outlineColor);
        if (c.showCurrent) {
            const std::string text = statText(*track, stats, StatCurrent, mOptions.rangeSemitones);
            p->drawTextRotated(3, valueRight - monoRight(text), base, 0.0f, Qt::white, text, outlineColor);
        }

        for (const int bit : c.bits) {
            base += r.rowHeight;
            for (const auto &col : statColumns) {
                if (col.bit != bit) continue;
                p->drawTextRotated(3, box.x + pad, base, 0.0f, greyText, col.title, outlineColor);
                const std::string text = statText(*track, stats, bit, mOptions.rangeSemitones);
                p->drawTextRotated(3, valueRight - monoRight(text), base, 0.0f, Qt::white, text, outlineColor);
            }
        }
    }

    // The frame timing is a box at the top centre of the graph. It stays out of the list of boxes,
    // and it has one size, so it does not move when the numbers change.
    if (r.hasDebug && !mOptions.hideDebug) {
        static QElapsedTimer clock;
        static double smoothedFrame = 16.7;
        if (clock.isValid()) {
            const double frame = clock.nsecsElapsed() * 1e-6;
            smoothedFrame += 0.05 * (std::min(frame, 250.0) - smoothedFrame);
        }
        clock.restart();

        // Each number has a fixed width, so the text keeps its shape in the monospace font.
        const std::string text = "Render " + format("%5.2f", (double) timings::render) + " ms    Update "
            + format("%5.2f", (double) timings::update) + " ms    Frame " + format("%5.1f", smoothedFrame)
            + " ms (" + format("%3.0f", 1000.0 / std::max(smoothedFrame, 0.1)) + " fps)";

        const double x = r.debugX;
        const double y = r.debugY;
        p->drawRoundedRect(x, y, r.debugWidth, r.debugHeight, 6.0 * dpr, fill, border, 1.0f);
        p->drawTextRotated(3, x + pad, y + pad + 0.5 * (r.debugHeight - 2.0 * pad) + 0.5 * digit, 0.0f, Qt::white, text, outlineColor);
    }

    p->setOriginY(mPlotTop);
}
