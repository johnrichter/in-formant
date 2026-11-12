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

// A column of the table of statistics. The sample text has the widest usual value of the column.
struct StatColumn {
    int bit;
    const char *title;
    const char *sample;
};

const StatColumn statColumns[] = {
    { StatCurrent, "Now", "0000.0" }, { StatMin, "Min", "0000.0" }, { StatP5, "P5", "0000.0" },
    { StatMean, "Mean", "0000.0" }, { StatMedian, "Median", "0000.0" }, { StatStdDev, "SD", "0000.0" },
    { StatP95, "P95", "0000.0" }, { StatMax, "Max", "0000.0" }, { StatRange, "Range", "0000.0" },
    { StatCV, "CV", "000.0%" }, { StatVoiced, "Voiced", "100%" }, { StatCount, "N", "00000" },
    { StatNote, "Note", "A#4 +50c" },
};

// The gap between two columns of the table, and the space before the first column, in device-independent pixels.
const double statColumnGap = 12.0;
const double statNameGap = 16.0;
// The space around a readout box, and between the boxes and the axis.
const double readoutPad = 7.0;
const double readoutMargin = 10.0;

}

void QPainterWrapper::updateLayout()
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

    // The readout boxes have their own column at the far left, left of the frequency axis. The
    // width has the widest box. The column takes at most 45% of the window.
    double readout = 0.0;
    if (mOptions.showDebug) {
        readout = std::max<double>(readout, p->textBoundsOfSize(1, "Frame: 000.0 ms (000 fps)").width());
    }
    if (mOptions.showLegend) {
        readout = std::max<double>(readout, p->textBoundsOfSize(1, "Pitch").width() + 22.0 * dpr);
    }
    if (mOptions.showStats && mOptions.statsMask != 0) {
        double table = p->textBoundsOfSize(1, "Pitch").width() + statNameGap * dpr;
        for (const StatColumn &c : statColumns) {
            if (!(mOptions.statsMask & c.bit)) continue;
            table += std::max<double>(p->textBoundsOfSize(2, c.title).width(), p->textBoundsOfSize(1, c.sample).width())
                     + statColumnGap * dpr;
        }
        readout = std::max(readout, table - statColumnGap * dpr);
    }
    if (readout > 0.0) {
        readout += 2.0 * readoutPad * dpr + 2.0 * readoutMargin * dpr;
        readout = std::min(readout, 0.45 * viewport().width());
    }
    mReadoutWidth = (int) std::ceil(readout);

    mPlotLeft = (int) std::ceil(left) + mReadoutWidth;
    mPlotBottom = (int) std::ceil(bottom);
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
                    0.1 * viewport().height(),
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
                        return viewport().height() * (1.0 - (v - vmin) / (vmax - vmin));
                    },
                    0.1 * viewport().height(),
                    color);
}

namespace {

struct HudLine {
    std::string text;
    QColor color;
    int size;                 // 0 normal, 1 small, 2 smaller
    int swatch;               // 0 none, 1 swatch of a track
    QColor swatchColor;
};

}

void QPainterWrapper::drawHud(const std::vector<HudTrack> &tracks)
{
    const double dpr = p->devicePixelRatio();
    // The panels have their own column at the left of the frequency axis.
    const double margin = readoutMargin * dpr;
    const double pad = readoutPad * dpr;

    double y = 10.0 * dpr;

    auto drawPanel = [&](const std::vector<HudLine> &lines) {
        if (lines.empty()) return;

        const double swatchWidth = 22.0 * dpr;
        double width = 0;
        double height = 0;
        for (const auto &l : lines) {
            const QRect b = p->textBoundsOfSize(l.size, l.text);
            width = std::max<double>(width, b.x() + b.width() + (l.swatch ? swatchWidth : 0.0));
            height += std::max<double>(p->textBoundsOfSize(l.size, "0").height() + 6.0 * dpr, l.swatch ? 14.0 * dpr : 0.0);
        }

        p->drawRoundedRect(margin, y, width + 2 * pad, height + 2 * pad - 2.0 * dpr, 6.0 * dpr,
                           QColor(8, 8, 12, 165), QColor(255, 255, 255, 36), 1.0f);

        double ly = y + pad;
        for (const auto &l : lines) {
            const double digit = p->textBoundsOfSize(l.size, "0").height();
            const double rowHeight = std::max<double>(digit + 6.0 * dpr, l.swatch ? 14.0 * dpr : 0.0);
            const double baseline = ly + 0.5 * rowHeight + 0.5 * digit;
            double x = margin + pad;
            if (l.swatch) {
                const double cy = ly + 0.5 * rowHeight;
                const float seg[4] = { (float) x, (float) cy, (float) (x + swatchWidth - 6.0 * dpr), (float) cy };
                if (mOptions.trackStyle != 0) {
                    p->drawSegments(seg, 1, (float) mOptions.lineWidth, l.swatchColor);
                }
                if (mOptions.trackStyle != 1) {
                    rpm::vector<QPointF> dot(1, QPointF(x + 0.5 * (swatchWidth - 6.0 * dpr), cy));
                    p->drawScatterWithOutline(dot, (float) mOptions.dotRadius, l.swatchColor, outlineColor);
                }
                x += swatchWidth;
            }
            p->drawTextRotated(l.size, x, baseline, 0.0f, l.color, l.text, outlineColor);
            ly += rowHeight;
        }

        y += height + 2 * pad + 6.0 * dpr;
    };

    // The debug readout.
    if (mOptions.showDebug) {
        static QElapsedTimer clock;
        static double smoothedFrame = 16.7;
        if (clock.isValid()) {
            const double frame = clock.nsecsElapsed() * 1e-6;
            smoothedFrame += 0.05 * (std::min(frame, 250.0) - smoothedFrame);
        }
        clock.restart();

        std::vector<HudLine> lines;
        lines.push_back({ "Render: " + format("%.2f", (double) timings::render) + " ms", Qt::white, 1, 0, QColor() });
        lines.push_back({ "Update: " + format("%.2f", (double) timings::update) + " ms", Qt::white, 1, 0, QColor() });
        lines.push_back({ "Frame: " + format("%.1f", smoothedFrame) + " ms (" + format("%.0f", 1000.0 / std::max(smoothedFrame, 0.1)) + " fps)", Qt::white, 1, 0, QColor() });
        drawPanel(lines);
    }

    // The legend.
    if (mOptions.showLegend && !tracks.empty()) {
        std::vector<HudLine> lines;
        lines.push_back({ "Legend", QColor(180, 180, 195), 2, 0, QColor() });
        for (const auto &t : tracks) {
            lines.push_back({ t.name, Qt::white, 1, 1, t.color });
        }
        drawPanel(lines);
    }

    // The statistics.
    if (mOptions.showStats && !tracks.empty() && mOptions.statsMask != 0) {
        // A column that does not fit in the readout column is not shown.
        const double available = mReadoutWidth - 2.0 * margin - 2.0 * pad;
        std::vector<const StatColumn *> active;
        double needed = p->textBoundsOfSize(1, "Pitch").width() + statNameGap * dpr;
        for (const auto &c : statColumns) {
            if (!(mOptions.statsMask & c.bit)) continue;
            needed += std::max<double>(p->textBoundsOfSize(2, c.title).width(), p->textBoundsOfSize(1, c.sample).width())
                      + statColumnGap * dpr;
            if (!active.empty() && needed - statColumnGap * dpr > available) break;
            active.push_back(&c);
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
                    case StatCurrent: text = format("%.1f", t.values.back()); break;
                    case StatMin: text = format("%.1f", sorted.front()); break;
                    case StatP5: text = format("%.1f", p5); break;
                    case StatMean: text = format("%.1f", mean); break;
                    case StatMedian: text = format("%.1f", percentile(sorted, 0.5)); break;
                    case StatStdDev: text = format("%.1f", sd); break;
                    case StatP95: text = format("%.1f", p95); break;
                    case StatMax: text = format("%.1f", sorted.back()); break;
                    case StatRange: text = format("%.1f", p95 - p5); break;
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
        const double gap = statColumnGap * dpr;
        std::vector<double> colWidth(active.size(), 0.0);
        double nameWidth = p->textBoundsOfSize(1, "Pitch").width();
        for (size_t c = 0; c < active.size(); ++c) {
            colWidth[c] = std::max<double>(p->textBoundsOfSize(2, active[c]->title).width() + p->textBoundsOfSize(2, active[c]->title).x(),
                                           p->textBoundsOfSize(1, active[c]->sample).width());
            for (size_t r = 0; r < rows; ++r) {
                const QRect b = p->textBoundsOfSize(1, cells[r][c]);
                colWidth[c] = std::max<double>(colWidth[c], b.width() + b.x());
            }
        }
        for (const auto &t : tracks) {
            const QRect b = p->textBoundsOfSize(1, t.name);
            nameWidth = std::max<double>(nameWidth, b.width() + b.x());
        }

        const double rowHeight = p->textBoundsOfSize(1, "0").height() + 7.0 * dpr;
        const double headerHeight = p->textBoundsOfSize(2, "0").height() + 7.0 * dpr;
        double width = nameWidth + statNameGap * dpr;
        for (double w : colWidth) width += w + gap;

        const double height = headerHeight + rows * rowHeight;
        p->drawRoundedRect(margin, y, width + 2 * pad - gap, height + 2 * pad - 3.0 * dpr, 6.0 * dpr,
                           QColor(8, 8, 12, 165), QColor(255, 255, 255, 36), 1.0f);

        const double textTop = y + pad;
        double x = margin + pad + nameWidth + statNameGap * dpr;
        const double headerBase = textTop + headerHeight - 3.5 * dpr;
        for (size_t c = 0; c < active.size(); ++c) {
            p->drawTextRotated(2, x, headerBase, 0.0f, QColor(170, 170, 185), active[c]->title, outlineColor);
            x += colWidth[c] + gap;
        }

        for (size_t r = 0; r < rows; ++r) {
            const double base = textTop + headerHeight + (r + 1) * rowHeight - 4.0 * dpr;
            p->drawTextRotated(1, margin + pad, base, 0.0f, tracks[r].color, tracks[r].name, outlineColor);
            x = margin + pad + nameWidth + statNameGap * dpr;
            for (size_t c = 0; c < active.size(); ++c) {
                p->drawTextRotated(1, x, base, 0.0f, Qt::white, cells[r][c], outlineColor);
                x += colWidth[c] + gap;
            }
        }

        y += height + 2 * pad + 6.0 * dpr;
    }
}
