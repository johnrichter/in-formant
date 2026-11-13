#include "ticks.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace Gui;

std::string Gui::formatNumber(double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
    return buffer;
}

std::string Gui::formatTime(double seconds, double step)
{
    const bool negative = seconds < 0;
    double t = std::fabs(seconds);

    char buffer[64];
    if (t >= 3600.0) {
        const long total = (long) std::floor(t + 1e-9);
        std::snprintf(buffer, sizeof(buffer), "%ld:%02ld:%02ld", total / 3600, (total / 60) % 60, total % 60);
    }
    else if (t >= 60.0 && step >= 1.0 - 1e-9) {
        const long total = (long) std::floor(t + 1e-9);
        std::snprintf(buffer, sizeof(buffer), "%ld:%02ld", total / 60, total % 60);
    }
    else if (t >= 60.0) {
        const long minutes = (long) std::floor(t / 60.0);
        const double rest = t - 60.0 * minutes;
        const int decimals = std::clamp((int) std::ceil(-std::log10(step) - 1e-9), 0, 6);
        std::snprintf(buffer, sizeof(buffer), "%ld:%0*.*f", minutes, decimals > 0 ? decimals + 3 : 2, decimals, rest);
    }
    else {
        const int decimals = std::clamp((int) std::ceil(-std::log10(step) - 1e-9), 0, 6);
        std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, t);
    }
    return std::string(negative ? "-" : "") + buffer;
}

std::string Gui::noteName(double frequency, double *cents)
{
    static const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (!(frequency > 0)) {
        if (cents) *cents = 0;
        return "";
    }

    const double midi = 69.0 + 12.0 * std::log2(frequency / 440.0);
    const long nearest = std::lround(midi);
    if (cents) {
        *cents = (midi - nearest) * 100.0;
    }

    const int pitchClass = (int) (((nearest % 12) + 12) % 12);
    const long octave = (long) std::floor(nearest / 12.0) - 1;
    return std::string(names[pitchClass]) + std::to_string(octave);
}

namespace {

constexpr double minTickGap = 4.0;

struct Occupancy {
    // The used intervals along the axis, as pairs of positions.
    std::vector<std::pair<double, double>> used;

    bool free(double lo, double hi) const {
        for (const auto &u : used) {
            if (hi > u.first && lo < u.second) return false;
        }
        return true;
    }

    void add(double lo, double hi) { used.emplace_back(lo, hi); }
};

// The mantissas of the labels for each density, in the order of preference. The first one gets its label first.
const std::vector<double> &labelMantissas(TickDensity density)
{
    static const std::vector<double> sparse = { 1, 5, 2 };
    static const std::vector<double> normal = { 1, 5, 2, 3 };
    static const std::vector<double> dense = { 1, 5, 2, 3, 4, 1.5 };
    switch (density) {
    case TickDensity::Sparse: return sparse;
    case TickDensity::Dense: return dense;
    default: return normal;
    }
}

// All values of m * 10^k in the range, for one mantissa m.
std::vector<double> valuesOfMantissa(double m, double lo, double hi)
{
    std::vector<double> values;
    if (!(hi > lo) || hi <= 0) return values;
    const int k0 = (int) std::floor(std::log10(std::max(lo, 1e-6))) - 1;
    const int k1 = (int) std::ceil(std::log10(hi)) + 1;
    for (int k = k0; k <= k1; ++k) {
        const double value = std::round(m * 1000.0) / 1000.0 * std::pow(10.0, k);
        if (value >= lo - 1e-12 && value <= hi + 1e-12) values.push_back(value);
    }
    return values;
}

}

std::vector<Tick> Gui::makeFrequencyTicks(FrequencyTickMode mode, double step,
                                          double minFrequency, double maxFrequency,
                                          const ToPixel &toPixel,
                                          double labelSize, double labelGap,
                                          bool linearScale, TickDensity density)
{
    std::vector<Tick> ticks;
    const double lo = std::min(minFrequency, maxFrequency);
    const double hi = std::max(minFrequency, maxFrequency);
    if (!(hi > lo)) {
        return ticks;
    }

    Occupancy labels;     // the space of the labels
    Occupancy marks;      // the space of all ticks

    auto tryLabel = [&](double value, int level, const std::string &text) {
        const double p = toPixel(value);
        const double half = 0.5 * labelSize + 0.5 * labelGap;
        if (!labels.free(p - half, p + half)) {
            return false;
        }
        labels.add(p - half, p + half);
        marks.add(p - 0.5 * minTickGap, p + 0.5 * minTickGap);
        ticks.push_back({ value, level, text });
        return true;
    };

    auto tryMark = [&](double value, int level) {
        const double p = toPixel(value);
        if (!marks.free(p - 0.5 * minTickGap, p + 0.5 * minTickGap)) {
            return false;
        }
        marks.add(p - 0.5 * minTickGap, p + 0.5 * minTickGap);
        ticks.push_back({ value, level, std::string() });
        return true;
    };

    if (mode == FrequencyTickMode::Notes || mode == FrequencyTickMode::Octaves) {
        struct Note { double frequency; int priority; std::string label; };
        std::vector<Note> notes;
        for (int midi = 0; midi < 140; ++midi) {
            const double f = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
            if (f < lo || f > hi) continue;

            const int pitchClass = midi % 12;
            int priority;
            if (pitchClass == 0) priority = 0;                            // C
            else if (pitchClass == 9) priority = 1;                       // A, the tuning note
            else if (pitchClass == 4 || pitchClass == 7) priority = 2;    // E and G
            else priority = 3;

            if (mode == FrequencyTickMode::Octaves && priority != 0) continue;

            std::string label = noteName(f);
            if (priority <= 1) {
                label += " " + formatNumber(std::round(f * 10.0) / 10.0);
            }
            notes.push_back({ f, priority, label });
        }

        std::stable_sort(notes.begin(), notes.end(), [](const Note &a, const Note &b) { return a.priority < b.priority; });

        for (const auto &n : notes) {
            if (n.priority <= 2) {
                if (!tryLabel(n.frequency, n.priority, n.label)) {
                    tryMark(n.frequency, 1);
                }
            }
            else {
                tryMark(n.frequency, 2);
            }
        }
        return ticks;
    }

    if (mode == FrequencyTickMode::Step && step > 0) {
        // The number of ticks has an upper limit, so a very small step cannot slow down the drawing.
        const double first = std::ceil(lo / step) * step;
        const long count = (long) std::floor((hi - first) / step) + 1;
        if (count > 0 && count < 5000) {
            // Label each n-th tick, where n is the smallest of 1, 2, 5, 10, 20, 50 and so on that fits.
            std::vector<double> values;
            for (long i = 0; i < count; ++i) values.push_back(first + i * step);

            long every = 1;
            for (int round = 0; round < 12; ++round) {
                Occupancy test;
                bool fits = true;
                for (long i = 0; i < (long) values.size() && fits; ++i) {
                    if (i % every != 0) continue;
                    const double p = toPixel(values[i]);
                    const double half = 0.5 * labelSize + 0.5 * labelGap;
                    if (!test.free(p - half, p + half)) fits = false;
                    else test.add(p - half, p + half);
                }
                if (fits) break;
                every = (round % 3 == 0) ? every * 2 : (round % 3 == 1 ? every * 5 / 2 : every * 2);
                if (every < 1) every = 1;
            }

            for (long i = 0; i < (long) values.size(); ++i) {
                if (i % every == 0) {
                    tryLabel(values[i], 0, formatNumber(values[i]));
                }
            }
            for (long i = 0; i < (long) values.size(); ++i) {
                if (i % every != 0) tryMark(values[i], 1);
            }
        }
        return ticks;
    }

    const double pLow = std::min(toPixel(lo), toPixel(hi));
    const double pHigh = std::max(toPixel(lo), toPixel(hi));
    const double half = 0.5 * labelSize + 0.5 * labelGap;
    // A label that would cross an end of the axis cannot show, so it does not count.
    auto inside = [&](double value) {
        const double p = toPixel(value);
        return p - half >= pLow - 1e-9 && p + half <= pHigh + 1e-9;
    };

    if (linearScale) {
        // Equal steps. The step is the smallest of 1, 2, 5 times a power of ten that leaves room for each label.
        // A sparse axis needs more room for each label than a dense axis.
        static const double room[3] = { 2.0, 1.4, 1.0 };
        const double pixelsPerUnit = (pHigh - pLow) / (hi - lo);
        const double needed = (labelSize + labelGap) * room[(int) density] / std::max(pixelsPerUnit, 1e-9);
        const double base = std::pow(10.0, std::floor(std::log10(needed)));
        double labelStep = 10.0 * base;
        for (double m : { 1.0, 2.0, 5.0, 10.0 }) {
            if (m * base >= needed - 1e-12) { labelStep = m * base; break; }
        }

        // Four small steps between two labels, or five.
        const double mantissa = labelStep / std::pow(10.0, std::floor(std::log10(labelStep) + 1e-9));
        const double fineStep = std::fabs(mantissa - 2.0) < 1e-6 ? labelStep / 4.0 : labelStep / 5.0;
        const bool fine = fineStep * pixelsPerUnit >= 6.0;

        const double firstLabel = std::ceil(lo / labelStep - 1e-9);
        const double lastLabel = std::floor(hi / labelStep + 1e-9);
        for (double i = firstLabel; i <= lastLabel; i += 1.0) {
            const double value = i * labelStep;
            if (value > lo - 1e-9 && value < hi + 1e-9) {
                if (inside(value)) tryLabel(value, 0, formatNumber(value));
                else tryMark(value, 1);
            }
        }
        if (fine) {
            const double firstFine = std::ceil(lo / fineStep - 1e-9);
            const double lastFine = std::floor(hi / fineStep + 1e-9);
            for (double i = firstFine; i <= lastFine; i += 1.0) {
                const double ratio = i * fineStep / labelStep;
                if (std::fabs(ratio - std::round(ratio)) < 1e-6) continue;
                tryMark(i * fineStep, 2);
            }
        }
        return ticks;
    }

    // Powers of ten. A group of values with the same mantissa gets labels in all decades, or in none.
    // This keeps the pattern of the labels the same along the axis. The mantissa 1 comes first, then 5, 2, 3, 4, and 1.5.
    std::vector<double> labeled;
    for (double m : labelMantissas(density)) {
        std::vector<double> values;
        for (double v : valuesOfMantissa(m, lo, hi)) {
            if (inside(v)) values.push_back(v);
        }
        if (values.empty()) continue;

        Occupancy test = labels;
        bool fits = true;
        for (double v : values) {
            const double p = toPixel(v);
            if (!test.free(p - half, p + half)) { fits = false; break; }
            test.add(p - half, p + half);
        }
        if (!fits) continue;

        for (double v : values) {
            tryLabel(v, 0, formatNumber(v));
            labeled.push_back(v);
        }
    }

    // The other values of 1 to 9 (and 1.5) get small ticks, when they have room.
    for (double m : { 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 }) {
        for (double v : valuesOfMantissa(m, lo, hi)) {
            if (std::find_if(labeled.begin(), labeled.end(), [&](double x) { return std::fabs(x - v) < 1e-9 * std::max(1.0, v); }) != labeled.end()) continue;
            tryMark(v, 2);
        }
    }
    return ticks;
}

double Gui::timeLabelStep(double span, double pixelsPerLabel, double secondsPerPixel)
{
    // The steps that people read at once: seconds, then 5, 10, 15, 30 seconds, then minutes and hours.
    static const double steps[] = { 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200, 14400, 21600, 43200, 86400 };
    const double forSpace = pixelsPerLabel * secondsPerPixel;
    const double forSpan = span / 15.0;
    const double needed = std::max(1.0, std::max(forSpace, forSpan));
    for (double step : steps) {
        if (step >= needed - 1e-9) return step;
    }
    return 86400.0 * std::ceil(needed / 86400.0);
}

std::vector<Tick> Gui::makeTimeTicks(TimeTickMode mode, double step,
                                     double minTime, double maxTime,
                                     const ToPixel &toPixel,
                                     double labelWidth, double labelGap)
{
    std::vector<Tick> ticks;
    const double lo = std::min(minTime, maxTime);
    const double hi = std::max(minTime, maxTime);
    if (!(hi > lo)) {
        return ticks;
    }

    // The width of a pixel in seconds, from the positions of the two ends of the axis.
    const double pixels = std::fabs(toPixel(hi) - toPixel(lo));
    if (pixels < 1.0) {
        return ticks;
    }
    const double secondsPerPixel = (hi - lo) / pixels;

    double labelStep;     // the step between two labels

    if (mode == TimeTickMode::Step && step > 0) {
        // Half seconds carry no meaning, so the step is at least 1 second.
        labelStep = std::max(step, 1.0);
    }
    else {
        labelStep = timeLabelStep(hi - lo, labelWidth + labelGap, secondsPerPixel);
    }

    // The step between small ticks. It is a whole number of seconds, and it is at least 1 second.
    double fineStep = labelStep;
    for (double parts : { 5.0, 3.0, 4.0, 2.0 }) {
        const double candidate = labelStep / parts;
        if (candidate >= 1.0 - 1e-9 && std::fabs(candidate - std::round(candidate)) < 1e-9 && candidate / secondsPerPixel >= 4.0) {
            fineStep = candidate;
            break;
        }
    }

    const long first = (long) std::ceil(lo / fineStep - 1e-9);
    const long last = (long) std::floor(hi / fineStep + 1e-9);
    if (last - first > 20000) {
        return ticks;
    }

    for (long i = first; i <= last; ++i) {
        const double t = i * fineStep;
        const double ratio = t / labelStep;
        const bool isLabel = std::fabs(ratio - std::round(ratio)) < 1e-6;
        ticks.push_back({ t, isLabel ? 0 : 2, isLabel ? formatTime(t, labelStep) : std::string() });
    }
    return ticks;
}
