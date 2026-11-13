#ifndef GUI_READOUTSTATS_H
#define GUI_READOUTSTATS_H

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "renderoptions.h"
#include "statformat.h"
#include "ticks.h"
#include "../context/summary.h"

// The statistics in the table of the readouts. The functions have no drawing code, so a test can run them.

namespace Gui {
namespace stats {

inline std::string format(const char *fmt, double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), fmt, value);
    return buffer;
}

// A value of a statistic in the narrow columns. A value of 1000 or more has no decimals.
inline std::string number(double value)
{
    return format(std::fabs(value) >= 1000.0 ? "%.0f" : "%.1f", value);
}

// The statistics of the values of one track. The standard deviation divides by N - 1, as the report does.
struct TrackStats {
    std::vector<double> sorted;
    double mean = 0.0;
    double sd = 0.0;
    double p5 = 0.0;
    double median = 0.0;
    double p95 = 0.0;
};

inline TrackStats computeStats(const HudTrack &t)
{
    TrackStats s;
    s.sorted = t.values;
    std::sort(s.sorted.begin(), s.sorted.end());
    if (s.sorted.empty()) return s;

    Main::Summary summary;
    summary.frames = t.frames;
    summary.values = s.sorted;
    s.mean = summary.mean();
    s.sd = summary.sd();
    s.p5 = summary.percentile(0.05);
    s.median = summary.median();
    s.p95 = summary.percentile(0.95);
    return s;
}

// The text of one statistic of one track. A track with no value has a dash.
inline std::string statText(const HudTrack &t, const TrackStats &s, int bit, bool rangeSemitones)
{
    if (bit == StatVoiced) {
        return t.frames > 0 ? format("%.0f%%", 100.0 * (t.count > 0 ? t.count : t.values.size()) / t.frames) : "-";
    }
    // A window with frames and no value has a count of 0, and not a dash.
    if (bit == StatCount) {
        return t.frames > 0 || !s.sorted.empty() ? std::to_string(t.count > 0 ? t.count : s.sorted.size()) : "-";
    }
    if (s.sorted.empty()) return "-";

    switch (bit) {
    case StatCurrent: return number(t.values.back());
    case StatMin: return number(s.sorted.front());
    case StatP5: return number(s.p5);
    case StatMean: return number(s.mean);
    case StatMedian: return number(s.median);
    case StatStdDev: return number(s.sd);
    case StatP95: return number(s.p95);
    case StatMax: return number(s.sorted.back());
    case StatRange:
        if (rangeSemitones) return rangeSemitonesText(s.p5, s.p95);
        return number(s.p95 - s.p5);
    case StatCV: return s.mean > 0 ? format("%.1f%%", 100.0 * s.sd / s.mean) : "-";
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

// The start of the window of the statistics, in seconds of the session. A window of 0 follows the
// visible range. Another window ends at the end of the visible range. At the start of a session the
// start can be below 0, and no entry lies there, so the window holds all data that exists.
inline double statsWindowStart(double viewStart, double viewEnd, double windowSeconds)
{
    return windowSeconds > 0 ? viewEnd - windowSeconds : viewStart;
}

}
}

#endif // GUI_READOUTSTATS_H
