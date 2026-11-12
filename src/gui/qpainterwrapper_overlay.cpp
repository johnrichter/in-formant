#include "qpainterwrapper.h"
#include "ticks.h"
#include "../context/timings.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <cstdio>
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

// A column of the table of statistics. The column has one statistic for each track.
struct StatColumn {
    int bit;
    const char *title;
};

const StatColumn statColumns[] = {
    { StatCurrent, "Now" }, { StatMin, "Min" }, { StatP5, "P5" }, { StatMean, "Mean" },
    { StatMedian, "Median" }, { StatStdDev, "SD" }, { StatP95, "P95" }, { StatMax, "Max" },
    { StatRange, "Range" }, { StatCV, "CV" }, { StatVoiced, "Voiced" }, { StatCount, "N" },
    { StatNote, "Note" },
};

const double readoutPad = 7.0;
// The space at the left and the right of the strip, and between the boxes of the strip.
const double stripMargin = 10.0;
const double stripGap = 8.0;

}

void QPainterWrapper::updateLayout(int trackCount)
{
    const double dpr = p->devicePixelRatio();
    const double majorLength = 9.0 * dpr;
    const double labelHeight = p->textBoundsOfSize(1, "0").height();

    // The bottom margin holds the ticks, the numbers and the title of the time axis.
    double bottom = majorLength + 6.0 * dpr;
    if (mOptions.showAxisLabels) bottom += labelHeight + 4.0 * dpr;
    if (mOptions.showAxisTitles) bottom += labelHeight + 10.0 * dpr;

    // The left margin holds the ticks, the numbers and the title of the frequency axis. The widest number has 5 digits.
    double left = majorLength + 6.0 * dpr;
    if (mOptions.showAxisLabels) {
        const QRect widest = p->textBoundsOfSize(0, "00000");
        left += widest.x() + widest.width() + 4.0 * dpr;
    }
    if (mOptions.showAxisTitles) left += labelHeight + 10.0 * dpr;

    // The readout boxes stand in a row below the time axis. The row is as high as the highest box.
    // The legend and the debug box have one line. The table of statistics has a line for each track.
    const double pad = readoutPad * dpr;
    const double digit = p->textBoundsOfSize(0, "0").height();
    const double lineHeight = std::max<double>(digit + 6.0 * dpr, 14.0 * dpr) + 2.0 * pad - 2.0 * dpr;
    double strip = 0.0;
    if (mOptions.showDebug) strip = std::max(strip, lineHeight);
    if (mOptions.showLegend && trackCount > 0) strip = std::max(strip, lineHeight);
    if (mOptions.showStats && mOptions.statsMask != 0 && trackCount > 0) {
        const double rowHeight = digit + 7.0 * dpr;
        const double headerHeight = p->textBoundsOfSize(1, "0").height() + 7.0 * dpr;
        strip = std::max(strip, headerHeight + trackCount * rowHeight + 2.0 * pad - 3.0 * dpr);
    }
    // The strip has a gap above it and a margin below it.
    if (strip > 0.0) strip += 2.0 * stripGap * dpr;
    mStripHeight = (int) std::ceil(strip);

    mPlotLeft = (int) std::ceil(left);
    mPlotBottom = (int) std::ceil(bottom) + mStripHeight;
    p->setPlotMargins(mPlotLeft, mPlotBottom);
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

    const double label0Height = p->textBoundsOfSize(1, "0").height();

    // The axis is below the graph. The ticks start at the lower edge of the graph and point down.
    const double tickBase = plotHeight();

    // The widest label has the most digits. Use the time labels at the ends as the estimate.
    const double sampleWidth = std::max<double>(
        p->textBoundsOfSize(1, formatTime(mTimeEnd, 0.001)).width(),
        p->textBoundsOfSize(1, "00.000").width());

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
            const QRect bounds = p->textBoundsOfSize(1, tick.label);
            const double start = x - (bounds.x() + bounds.width() / 2.0);
            const double left = start + bounds.x();
            const double right = left + bounds.width();

            // A label that crosses the edge of the view is not readable. Skip it.
            bool free = left >= 0 && right <= width;
            for (int tx = std::max(0, (int) std::floor(left) - 4); free && tx <= std::min(width - 1, (int) std::ceil(right) + 4); ++tx) {
                if (covered[tx]) { free = false; break; }
            }
            if (free) {
                p->drawTextRotated(1, start, tickBase + majorLength + 4.0 * dpr + label0Height, 0.0f, Qt::white, tick.label, outlineColor);
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

void QPainterWrapper::drawFrequencyScale()
{
    const double dpr = p->devicePixelRatio();
    const int width = viewport().width();
    const int height = viewport().height();

    const double digitHeight = p->textBoundsOfSize(0, "0").height();

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
            const int size = tick.level == 0 ? 0 : 1;
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
                     const QColor &color)
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

    const bool drawLines = o.trackStyle == 1 || o.trackStyle == 2;
    const bool drawDots = o.trackStyle == 0 || o.trackStyle == 2;

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
            const QColor &color)
{
    std::vector<TrackPoint> points;
    collectPoints(begin, end, [](const auto &item, double &v) { v = item.second; return true; }, points,
                  [this](double f) { return f; });

    drawTrackPoints(p, mOptions, points,
                    [this](double t) { return mapTimeToX(t); },
                    [this](double f) { return mapFrequencyToY(f); },
                    0.1 * plotHeight(),
                    color);
}

void QPainterWrapper::drawFrequencyTrack(
            const OptionalTimeTrack<double>::const_iterator& begin,
            const OptionalTimeTrack<double>::const_iterator& end,
            const QColor &color)
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
                    color);
}

bool QPainterWrapper::plotContains(double x, double y) const
{
    return x >= mPlotLeft && x <= viewport().width() && y >= 0 && y <= plotHeight();
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
    const double digit = p->textBoundsOfSize(0, "0").height();
    double widest = 0;
    for (Label &l : labels) {
        const QRect b = p->textBoundsOfSize(0, l.text);
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
        p->drawTextRotated(0, (float) (left + padX), (float) baseline, 0.0f, l.color, l.text, outlineColor);
    }

    // The dots lie on the bar at the value of each track.
    for (const Label &l : labels) {
        if (!l.dot) continue;
        rpm::vector<QPointF> dot(1, QPointF(x, l.anchorY));
        p->drawScatterWithOutline(dot, (float) (6.0 * dpr), l.color, QColor(255, 255, 255, 255));
    }
}

void QPainterWrapper::drawHud(const std::vector<HudTrack> &tracks)
{
    const double dpr = p->devicePixelRatio();
    const double pad = readoutPad * dpr;
    const double gap = stripGap * dpr;
    const double right = viewport().width() - stripMargin * dpr;

    // The boxes stand in a row below the time axis. They start at the top of the strip.
    const double top = viewport().height() - mStripHeight + stripGap * dpr;
    double x = stripMargin * dpr;

    const QColor fill(8, 8, 12, 200);
    const QColor border(255, 255, 255, 48);

    // The statistics.
    if (mOptions.showStats && !tracks.empty() && mOptions.statsMask != 0) {
        std::vector<const StatColumn *> active;
        for (const auto &c : statColumns) {
            if (mOptions.statsMask & c.bit) active.push_back(&c);
        }

        const size_t rows = tracks.size();
        std::vector<std::vector<std::string>> cells(rows, std::vector<std::string>(active.size()));

        for (size_t r = 0; r < rows; ++r) {
            const HudTrack &t = tracks[r];

            std::vector<double> sorted = t.values;
            std::sort(sorted.begin(), sorted.end());

            const bool has = !sorted.empty();
            const double mean = has ? std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size() : 0.0;
            double variance = 0;
            for (double v : sorted) variance += (v - mean) * (v - mean);
            const double sd = sorted.size() > 1 ? std::sqrt(variance / (sorted.size() - 1)) : 0.0;
            const double p5 = percentile(sorted, 0.05);
            const double p95 = percentile(sorted, 0.95);

            for (size_t c = 0; c < active.size(); ++c) {
                std::string text = "-";
                if (has) {
                    switch (active[c]->bit) {
                    case StatCurrent: text = number(t.values.back()); break;
                    case StatMin: text = number(sorted.front()); break;
                    case StatP5: text = number(p5); break;
                    case StatMean: text = number(mean); break;
                    case StatMedian: text = number(percentile(sorted, 0.5)); break;
                    case StatStdDev: text = number(sd); break;
                    case StatP95: text = number(p95); break;
                    case StatMax: text = number(sorted.back()); break;
                    case StatRange: text = number(p95 - p5); break;
                    case StatCV: text = mean > 0 ? format("%.1f%%", 100.0 * sd / mean) : "-"; break;
                    case StatCount: text = std::to_string(t.count > 0 ? t.count : sorted.size()); break;
                    case StatNote:
                        if (t.isPitch) {
                            double cents = 0;
                            const std::string note = noteName(mean, &cents);
                            char buffer[32];
                            std::snprintf(buffer, sizeof(buffer), "%s %+.0fc", note.c_str(), cents);
                            text = buffer;
                        }
                        break;
                    }
                }
                if (active[c]->bit == StatVoiced) {
                    text = t.frames > 0 ? format("%.0f%%", 100.0 * (t.count > 0 ? t.count : t.values.size()) / t.frames) : "-";
                }
                cells[r][c] = text;
            }
        }

        // The width of each column.
        const double columnGap = 12.0 * dpr;
        std::vector<double> colWidth(active.size(), 0.0);
        double nameWidth = 0;
        for (size_t c = 0; c < active.size(); ++c) {
            const QRect title = p->textBoundsOfSize(1, active[c]->title);
            colWidth[c] = title.width() + title.x();
            for (size_t r = 0; r < rows; ++r) {
                const QRect b = p->textBoundsOfSize(0, cells[r][c]);
                colWidth[c] = std::max<double>(colWidth[c], b.width() + b.x());
            }
        }
        for (const auto &t : tracks) {
            const QRect b = p->textBoundsOfSize(0, t.name);
            nameWidth = std::max<double>(nameWidth, b.width() + b.x());
        }

        // A column that does not fit in the row is not shown.
        double width = nameWidth + 16.0 * dpr;
        size_t shown = 0;
        for (; shown < active.size(); ++shown) {
            if (shown > 0 && x + width + colWidth[shown] + 2.0 * pad > right) break;
            width += colWidth[shown] + columnGap;
        }
        width -= columnGap;

        const double rowHeight = p->textBoundsOfSize(0, "0").height() + 7.0 * dpr;
        const double headerHeight = p->textBoundsOfSize(1, "0").height() + 7.0 * dpr;
        const double height = headerHeight + rows * rowHeight;
        p->drawRoundedRect(x, top, width + 2.0 * pad, height + 2.0 * pad - 3.0 * dpr, 6.0 * dpr, fill, border, 1.0f);

        const double textTop = top + pad;
        double cx = x + pad + nameWidth + 16.0 * dpr;
        const double headerBase = textTop + headerHeight - 3.5 * dpr;
        for (size_t c = 0; c < shown; ++c) {
            p->drawTextRotated(1, cx, headerBase, 0.0f, QColor(170, 170, 185), active[c]->title, outlineColor);
            cx += colWidth[c] + columnGap;
        }

        for (size_t r = 0; r < rows; ++r) {
            const double base = textTop + headerHeight + (r + 1) * rowHeight - 4.0 * dpr;
            p->drawTextRotated(0, x + pad, base, 0.0f, tracks[r].color, tracks[r].name, outlineColor);
            cx = x + pad + nameWidth + 16.0 * dpr;
            for (size_t c = 0; c < shown; ++c) {
                p->drawTextRotated(0, cx, base, 0.0f, Qt::white, cells[r][c], outlineColor);
                cx += colWidth[c] + columnGap;
            }
        }

        x += width + 2.0 * pad + gap;
    }

    // The legend is one row. Each entry has a swatch and a name.
    if (mOptions.showLegend && !tracks.empty()) {
        const double swatchWidth = 22.0 * dpr;
        const double entryGap = 14.0 * dpr;
        double width = 0;
        for (const auto &t : tracks) {
            const QRect b = p->textBoundsOfSize(0, t.name);
            width += swatchWidth + b.x() + b.width() + entryGap;
        }
        width -= entryGap;

        if (x + width + 2.0 * pad <= right) {
            const double digit = p->textBoundsOfSize(0, "0").height();
            const double rowHeight = std::max<double>(digit + 6.0 * dpr, 14.0 * dpr);
            p->drawRoundedRect(x, top, width + 2.0 * pad, rowHeight + 2.0 * pad - 2.0 * dpr, 6.0 * dpr, fill, border, 1.0f);

            const double cy = top + pad + 0.5 * rowHeight;
            double ex = x + pad;
            for (const auto &t : tracks) {
                const float seg[4] = { (float) ex, (float) cy, (float) (ex + swatchWidth - 6.0 * dpr), (float) cy };
                if (mOptions.trackStyle != 0) {
                    p->drawSegments(seg, 1, (float) mOptions.lineWidth, t.color);
                }
                if (mOptions.trackStyle != 1) {
                    rpm::vector<QPointF> dot(1, QPointF(ex + 0.5 * (swatchWidth - 6.0 * dpr), cy));
                    p->drawScatterWithOutline(dot, (float) mOptions.dotRadius, t.color, outlineColor);
                }
                p->drawTextRotated(0, ex + swatchWidth, cy + 0.5 * digit, 0.0f, Qt::white, t.name, outlineColor);
                const QRect b = p->textBoundsOfSize(0, t.name);
                ex += swatchWidth + b.x() + b.width() + entryGap;
            }
            x += width + 2.0 * pad + gap;
        }
    }

    // The debug readout is one row.
    if (mOptions.showDebug) {
        static QElapsedTimer clock;
        static double smoothedFrame = 16.7;
        if (clock.isValid()) {
            const double frame = clock.nsecsElapsed() * 1e-6;
            smoothedFrame += 0.05 * (std::min(frame, 250.0) - smoothedFrame);
        }
        clock.restart();

        const std::string text = "Render " + format("%.2f", (double) timings::render) + " ms    Update "
            + format("%.2f", (double) timings::update) + " ms    Frame " + format("%.1f", smoothedFrame)
            + " ms (" + format("%.0f", 1000.0 / std::max(smoothedFrame, 0.1)) + " fps)";
        // The text changes at each frame, so the box has the width of the widest text.
        const QRect b = p->textBoundsOfSize(0, "Render 00.00 ms    Update 00.00 ms    Frame 000.0 ms (000 fps)");
        const double width = b.x() + b.width();

        if (x + width + 2.0 * pad <= right) {
            const double digit = p->textBoundsOfSize(0, "0").height();
            const double rowHeight = std::max<double>(digit + 6.0 * dpr, 14.0 * dpr);
            p->drawRoundedRect(x, top, width + 2.0 * pad, rowHeight + 2.0 * pad - 2.0 * dpr, 6.0 * dpr, fill, border, 1.0f);
            p->drawTextRotated(0, x + pad, top + pad + 0.5 * rowHeight + 0.5 * digit, 0.0f, Qt::white, text, outlineColor);
            x += width + 2.0 * pad + gap;
        }
    }
}
