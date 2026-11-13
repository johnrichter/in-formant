#include "qpainterwrapper.h"
#include "ticks.h"
#include "readoutlayout.h"
#include "uiscale.h"
#include "readoutstats.h"
#include "graphlayout.h"
#include "gridlines.h"
#include "gridcolor.h"
#include "bands.h"
#include "../context/timings.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>

using namespace Gui;
using namespace Gui::stats;

namespace {

const QColor outlineColor(0, 0, 0, 255);
const QColor tickColor(255, 255, 255, 235);
const QColor titleColor(200, 200, 210, 230);

// One point of a track in the time and the transformed frequency.
struct TrackPoint {
    double time;
    double value;
    bool valid;
};

}

void QPainterWrapper::updateLayout(const std::vector<std::string> &trackNames)
{
    const GraphLayout layout = computeGraphLayout(mOptions, trackNames, p->devicePixelRatio(), viewport().width(), viewport().height(),
        [this](int size, const std::string &text) { return p->textBoundsOfSize(size, text); });

    mReadout = layout.readout;
    mPlotLeft = layout.plotLeft;
    mPlotRight = layout.plotRight;
    mPlotTop = layout.plotTop;
    mPlotBottom = layout.plotBottom;
    mStripHeight = layout.stripHeight;

    p->setPlotMargins(mPlotLeft, mPlotBottom, mPlotTop, mPlotRight);
    p->setTimeFlipped(mOptions.timeFlipped);
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
    // The ticks and the line of the axis end at the right edge of the graph. A label can reach into the space beyond it.
    const int plotRight = mPlotLeft + plotWidth();
    const int height = viewport().height();

    const double label0Height = p->textBoundsOfSize(4, "0").height();

    // The axis is below the graph. The ticks start at the lower edge of the graph and point down.
    const double tickBase = plotHeight();

    // The widest label has the most digits. Use the time labels at the ends as the estimate.
    const double sampleWidth = std::max<double>(
        p->textBoundsOfSize(4, formatTime(mTimeEnd, 1.0)).width(),
        p->textBoundsOfSize(4, "000").width());

    auto toPixel = [&](double t) { return mapTimeToX(t); };
    const auto ticks = makeTimeTicks(static_cast<TimeTickMode>(mOptions.timeTickMode), mOptions.timeTickStep,
                                     mTimeStart, mTimeEnd, toPixel, sampleWidth, spaceMd * dpr);

    std::vector<float> small, medium;
    std::vector<float> large;

    std::vector<bool> covered(std::max(width, 1), false);

    const double majorLength = tickMajor * dpr;
    const double minorLength = tickMinor * dpr;
    const double xs = spaceXs * dpr;

    for (const Tick &tick : ticks) {
        // The recording starts at time zero. No data exists before that time.
        if (tick.value < -1e-9) continue;

        const double x = mapTimeToX(tick.value);
        if (x < mPlotLeft - 1 || x > plotRight + 1) continue;

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
                p->drawTextRotated(4, start, tickBase + majorLength + xs + label0Height, 0.0f, Qt::white, tick.label, outlineColor);
                for (int tx = std::max(0, (int) std::floor(left)); tx <= std::min(width - 1, (int) std::ceil(right)); ++tx) {
                    covered[tx] = true;
                }
            }
        }
    }

    p->drawSegments(small.data(), (int) small.size() / 4, 1.2f, tickColor);
    p->drawSegments(large.data(), (int) large.size() / 4, 2.0f, tickColor);

    // The line of the axis along the lower edge of the graph.
    const float edge[4] = { (float) mPlotLeft, (float) tickBase, (float) plotRight, (float) tickBase };
    p->drawSegments(edge, 1, 1.6f, tickColor);

    if (mOptions.showAxisTitles) {
        const std::string title = "Time (s)";
        const QRect bounds = p->textBoundsOfSize(1, title);
        const double x = mPlotLeft + 0.5 * plotWidth() - (bounds.x() + bounds.width() / 2.0);
        const double y = tickBase + majorLength + xs + (mOptions.showAxisLabels ? label0Height + xs : 0.0) + bounds.height();
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
    // The colors are in gui/bands.h, so a test can check them.
    auto colorOf = [](int i) { return QColor(zoneColors[i].r, zoneColors[i].g, zoneColors[i].b); };
    const Zone zones[3] = {
        { mOptions.bandEdges[0], mOptions.bandEdges[1], colorOf(0), zoneColors[0].name },
        { mOptions.bandEdges[1], mOptions.bandEdges[2], colorOf(1), zoneColors[1].name },
        { mOptions.bandEdges[2], mOptions.bandEdges[3], colorOf(2), zoneColors[2].name },
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
        // A thin zone has no label, because the label would touch the label of its neighbor.
        if (bottom - top < 3.0 * digit) continue;
        QColor text = zone.color.lighter(150);
        const std::string label = std::string(zone.name) + " zone, " + std::to_string((int) std::lround(zone.low))
            + " to " + std::to_string((int) std::lround(zone.high)) + " Hz";
        p->drawTextRotated(2, mPlotLeft + spaceSm * dpr, top + digit + spaceXs * dpr, 0.0f, text, label, outlineColor);
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

void QPainterWrapper::fillPlotBackground()
{
    if (mOptions.theme.colormap.isEmpty()) return;
    const QColor quiet = QColor::fromRgb(mOptions.theme.colormap[0]);
    p->drawRoundedRect((float) mPlotLeft, 0.0f, (float) plotWidth(), (float) plotHeight(), 0.0f, quiet);
}

std::vector<Gui::Tick> QPainterWrapper::frequencyTicks()
{
    const double digitHeight = p->textBoundsOfSize(axisLabelSize, "0").height();
    auto toPixel = [&](double f) { return mapFrequencyToY(f); };
    return makeFrequencyTicks(static_cast<FrequencyTickMode>(mOptions.freqTickMode), mOptions.freqTickStep,
                              mMinFrequency, mMaxFrequency, toPixel, digitHeight, 0.9 * digitHeight,
                              mFrequencyScale == FrequencyScale::Linear,
                              static_cast<TickDensity>(std::clamp(mOptions.freqTickDensity, 0, 2)));
}

QColor QPainterWrapper::gridColor(double alphaScale) const
{
    const QColor quiet = mOptions.theme.colormap.isEmpty() ? QColor(0, 0, 0) : QColor::fromRgb(mOptions.theme.colormap[0]);
    return gridLineColor(quiet, mOptions.gridStrength, alphaScale);
}

void QPainterWrapper::drawFrequencyGrid()
{
    if (!mOptions.gridMajor && !mOptions.gridMinor) return;

    const auto ticks = frequencyTicks();

    std::vector<float> major, minor;
    const double left = mPlotLeft;
    const double right = mPlotLeft + plotWidth();

    const GridLines rows = makeGridLines(ticks, [this](double f) { return mapFrequencyToY(f); }, plotHeight());
    for (double y : rows.major) major.insert(major.end(), { (float) left, (float) y, (float) right, (float) y });
    for (double y : rows.minor) minor.insert(minor.end(), { (float) left, (float) y, (float) right, (float) y });

    // The minor lines are thinner and fainter than the major lines.
    if (mOptions.gridMinor && !minor.empty()) {
        p->drawSegments(minor.data(), (int) minor.size() / 4, 1.0f, gridColor(0.55));
    }
    if (mOptions.gridMajor && !major.empty()) {
        p->drawSegments(major.data(), (int) major.size() / 4, 1.6f, gridColor(1.0));
    }
}

void QPainterWrapper::drawFrequencyScale()
{
    const double dpr = p->devicePixelRatio();
    const int width = viewport().width();
    const int height = viewport().height();

    const auto ticks = frequencyTicks();

    std::vector<float> small, medium, large;

    const double majorLength = tickMajor * dpr;
    const double mediumLength = tickMedium * dpr;
    const double minorLength = tickMinor * dpr;
    const double xs = spaceXs * dpr;
    // The axis is at the left of the graph. The ticks start at the left edge of the graph and point left.
    const double labelRight = mPlotLeft - majorLength - xs;

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
            // All labels have one size, so the axis looks the same along its length.
            const int size = axisLabelSize;
            const QRect bounds = p->textBoundsOfSize(size, tick.label);
            const double digit = p->textBoundsOfSize(size, "0").height();
            // A label that crosses the top edge, or the lower edge of the graph, is not readable. Skip it.
            if (y - digit / 2.0 < 0 || y + digit / 2.0 > plotHeight()) continue;
            p->drawTextRotated(size, labelRight - (bounds.x() + bounds.width()), y + digit / 2.0, 0.0f,
                               Qt::white, tick.label, outlineColor);
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
        // The title stands at the left of the space that the layout reserves for the numbers. That space has the
        // width of the widest number with 5 digits, so the title keeps its place when the numbers change.
        const QRect reserved = p->textBoundsOfSize(3, "00000");
        const double x = labelRight - (mOptions.showAxisLabels ? reserved.x() + reserved.width() + xs : 0.0);
        // The text runs upward. Its start is at the bottom, and its baseline is on the right.
        const double y = 0.5 * plotHeight() + (bounds.x() + bounds.width() / 2.0);
        p->drawTextRotated(1, x, y, 90.0f, titleColor, title, outlineColor);
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
    return plotContainsPoint(plotFrame(), x, y);
}

double QPainterWrapper::timeAtX(double x) const
{
    return plotXToTime(plotFrame(), x);
}

void QPainterWrapper::drawCursor(double time, const std::vector<CursorValue> &values)
{
    const double dpr = p->devicePixelRatio();
    const double x = std::round(mapTimeToX(time));
    if (x < mPlotLeft || x >= mPlotLeft + plotWidth()) return;

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

    // A label has a padding of the small step at its sides and the smallest step above and below. It stands a
    // medium step from the bar, and the smallest step from the edge of the graph and from the next label.
    const double padX = spaceSm * dpr;
    const double padY = spaceXs * dpr;
    const double gap = spaceMd * dpr;
    const double edge = spaceXs * dpr;
    const double topEdge = spaceXs * dpr;
    const double digit = p->textBoundsOfSize(3, "0").height();
    double widest = 0;
    for (Label &l : labels) {
        const QRect b = p->textBoundsOfSize(3, l.text);
        l.width = b.x() + b.width() + 2.0 * padX;
        l.height = digit + 2.0 * padY;
        widest = std::max(widest, l.width);
    }

    // The labels go to the right of the bar. When they do not fit there, they go to the left.
    const double roomRight = mPlotLeft + plotWidth() - x - gap - edge;
    const double roomLeft = x - mPlotLeft - gap - edge;
    const bool right = roomRight >= widest || roomRight >= roomLeft;

    // Place the labels from the top down, with no overlap, and then pull them back inside the graph.
    std::sort(labels.begin(), labels.end(), [](const Label &a, const Label &b) { return a.anchorY < b.anchorY; });
    double next = topEdge;
    for (Label &l : labels) {
        l.top = std::max(l.anchorY - 0.5 * l.height, next);
        next = l.top + l.height + edge;
    }
    double limit = plotBottom - edge;
    for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        it->top = std::min(it->top, limit - it->height);
        limit = it->top - edge;
    }

    // The panel has an opaque background in a dark tone of the theme, and a border in a light tone of the theme.
    const QColor panel = QColor::fromRgbF(0.07 * accent.redF() + 0.02, 0.07 * accent.greenF() + 0.02,
                                          0.07 * accent.blueF() + 0.03, 1.0);

    for (const Label &l : labels) {
        const double left = right ? x + gap : x - gap - l.width;
        p->drawRoundedRect((float) left, (float) l.top, (float) l.width, (float) l.height, (float) (UiScale::cornerRadius * dpr),
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

void QPainterWrapper::drawHud(const std::vector<HudTrack> &tracks)
{
    const double dpr = p->devicePixelRatio();
    const ReadoutLayout &r = mReadout;
    const double pad = r.pad;

    // The boxes use the origin of the canvas, and the graph uses the origin below the bar at the top.
    p->setOriginY(0);

    const QColor fill(8, 8, 12, 200);
    const QColor border(255, 255, 255, 48);
    const QColor grayText(170, 170, 185);

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
        const float seg[4] = { (float) x, (float) cy, (float) (x + r.swatchWidth - spaceSm * dpr), (float) cy };
        if (style & 2) {
            p->drawSegments(seg, 1, (float) mOptions.lineWidth, t.color);
        }
        if (style & 1) {
            rpm::vector<QPointF> dot(1, QPointF(x + 0.5 * (r.swatchWidth - spaceSm * dpr), cy));
            p->drawScatterWithOutline(dot, (float) mOptions.dotRadius, t.color, outlineColor);
        }
    };

    // The legend is a row of entries in the bar at the top.
    if (r.hasLegend) {
        const BoxPlacement &box = r.legend;
        p->drawRoundedRect(box.x, box.y, box.width, box.height, UiScale::cornerRadius * dpr, fill, border, 1.0f);
        const double entryGap = spaceMd * dpr;
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

        p->drawRoundedRect(box.x, box.y, box.width, box.height, UiScale::cornerRadius * dpr, fill, border, 1.0f);

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

        double base = box.y + pad + r.rowHeight - spaceXs * dpr;
        drawSwatch(box.x + pad, base - 0.5 * digit, *track);
        p->drawTextRotated(3, box.x + pad + r.swatchWidth, base, 0.0f, track->color, track->name, outlineColor);
        if (c.showCurrent) {
            const std::string text = statText(*track, stats, StatCurrent, mOptions.rangeSemitones);
            p->drawTextRotated(3, valueRight - monoRight(text), base, 0.0f, Qt::white, text, outlineColor);
        }

        for (const int bit : c.bits) {
            base += r.rowHeight;
            for (const auto &col : statColumns()) {
                if (col.bit != bit) continue;
                p->drawTextRotated(3, box.x + pad, base, 0.0f, grayText, col.title, outlineColor);
                const std::string text = statText(*track, stats, bit, mOptions.rangeSemitones);
                p->drawTextRotated(3, valueRight - monoRight(text), base, 0.0f, Qt::white, text, outlineColor);
            }
        }
    }

    // The frame timing is a box at the top center of the graph. It stays out of the list of boxes,
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
        p->drawRoundedRect(x, y, r.debugWidth, r.debugHeight, UiScale::cornerRadius * dpr, fill, border, 1.0f);
        p->drawTextRotated(3, x + pad, y + pad + 0.5 * (r.debugHeight - 2.0 * pad) + 0.5 * digit, 0.0f, Qt::white, text, outlineColor);
    }

    p->setOriginY(mPlotTop);
}
