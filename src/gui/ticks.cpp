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

// The rank of a candidate. A round value has a small rank, so the algorithm labels it first.
// The value is m * 10^k, with m from 1 to 99.
struct Candidate {
    double value;
    int digits;       // the number of significant digits of m: 1 or 2
    int rank;         // the position of m in the order of preference
};

int mantissaRank(int m)
{
    if (m < 10) {
        static const int order[10] = { 99, 0, 2, 3, 4, 1, 5, 6, 7, 8 };
        return order[m];
    }
    // Two digits: the multiples of 5 first, then the even values, then the rest.
    if (m % 5 == 0) return m % 10 == 0 ? 0 : 1;
    if (m % 2 == 0) return 2;
    return 3;
}

std::vector<Candidate> roundCandidates(double lo, double hi)
{
    std::vector<Candidate> list;
    if (!(hi > lo) || hi <= 0) {
        return list;
    }

    const double lowPositive = std::max(lo, 1e-6);
    const int k0 = (int) std::floor(std::log10(lowPositive)) - 1;
    const int k1 = (int) std::ceil(std::log10(hi)) + 1;

    for (int k = k0; k <= k1; ++k) {
        const double unit = std::pow(10.0, k);
        for (int m = 1; m <= 99; ++m) {
            if (m % 10 == 0) continue;     // 10 * 10^k is 1 * 10^(k+1)
            const double value = m * unit;
            if (value < lo - 1e-12 || value > hi + 1e-12) continue;
            list.push_back({ value, m < 10 ? 1 : 2, mantissaRank(m) });
        }
    }

    // The zero value is a candidate when the range includes zero.
    if (lo <= 0 && hi >= 0) {
        list.push_back({ 0.0, 1, 0 });
    }

    std::sort(list.begin(), list.end(), [](const Candidate &a, const Candidate &b) {
        if (a.digits != b.digits) return a.digits < b.digits;
        if (a.rank != b.rank) return a.rank < b.rank;
        return a.value < b.value;
    });
    return list;
}

}

std::vector<Tick> Gui::makeFrequencyTicks(FrequencyTickMode mode, double step,
                                          double minFrequency, double maxFrequency,
                                          const ToPixel &toPixel,
                                          double labelSize, double labelGap)
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

    // Automatic: label the round values first, and then use the free space for less round values.
    const auto candidates = roundCandidates(lo, hi);
    for (const auto &c : candidates) {
        const int level = c.digits == 1 && c.rank <= 1 ? 0 : (c.digits == 1 ? 1 : 2);
        if (!tryLabel(c.value, level, formatNumber(c.value))) {
            tryMark(c.value, level == 0 ? 1 : 2);
        }
    }
    return ticks;
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
        labelStep = step;
    }
    else {
        // The smallest value of 1, 2, 5 times a power of ten that leaves enough space for each label.
        const double needed = (labelWidth + labelGap) * secondsPerPixel;
        static const double mantissas[] = { 1, 2, 5, 10 };
        const double base = std::pow(10.0, std::floor(std::log10(needed)));
        labelStep = 10.0 * base;
        for (double m : mantissas) {
            if (m * base >= needed) {
                labelStep = m * base;
                break;
            }
        }

        // For a long time span, use a step that is a multiple of a minute or an hour.
        static const double clockSteps[] = { 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200, 14400, 21600, 43200, 86400 };
        if (labelStep > 10.0) {
            for (double c : clockSteps) {
                if (c >= needed) {
                    labelStep = c;
                    break;
                }
            }
        }
    }

    // The step between small ticks.
    double fineStep = labelStep / 5.0;
    if (labelStep >= 60.0) {
        fineStep = labelStep / 3.0 >= 60.0 ? labelStep / 3.0 : labelStep / 2.0;
    }
    else {
        const double mantissa = labelStep / std::pow(10.0, std::floor(std::log10(labelStep) + 1e-9));
        if (std::fabs(mantissa - 2.0) < 1e-6) fineStep = labelStep / 4.0;
    }
    if (fineStep / secondsPerPixel < 4.0) {
        fineStep = labelStep;
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
