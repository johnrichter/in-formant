#ifndef GUI_PITCHAXIS_H
#define GUI_PITCHAXIS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

// The vertical axis of the pitch-only graph. The functions have no drawing code, so a test can run them.

namespace Gui {

// True when the value is not infinite and not NaN. The build uses fast math, and then std::isfinite
// can give a wrong answer, so this function reads the exponent bits and the compiler does not optimize it.
#if defined(__clang__)
#define GUI_NO_FAST_MATH __attribute__((optnone, noinline))
#elif defined(__GNUC__)
#define GUI_NO_FAST_MATH __attribute__((optimize("no-fast-math"), noinline))
#else
#define GUI_NO_FAST_MATH
#endif

GUI_NO_FAST_MATH inline bool isFiniteValue(double v)
{
    std::uint64_t bits;
    std::memcpy(&bits, &v, sizeof bits);
    return ((bits >> 52) & 0x7ff) != 0x7ff;
}

struct PitchAxis {
    double low = 80.0;
    double high = 300.0;
};

constexpr double minPitchAxisSemitones = 12.0;

inline double semitonesBetween(double low, double high)
{
    return 12.0 * std::log2(high / low);
}

// The fixed range of the pitch-only graph. The low edge is at least 0 Hz. The high edge is at most
// 2000 Hz and at least 50 Hz above the low edge. A wrong value gives the default range of 0 to 500 Hz.
inline PitchAxis fixedPitchAxis(double low, double high)
{
    if (!isFiniteValue(low) || !isFiniteValue(high)) return { 0.0, 500.0 };
    PitchAxis axis;
    axis.low = std::clamp(low, 0.0, 1950.0);
    axis.high = std::clamp(high, axis.low + 50.0, 2000.0);
    return axis;
}

// The range that shows the values with a margin. The 5th and the 95th percentile set the edges, so
// a few wrong values, for example an octave error, do not stretch the axis. The range is never
// narrower than 12 semitones. Too few values give the fallback range.
inline PitchAxis fitPitchAxis(std::vector<double> values, const PitchAxis &fallback = PitchAxis())
{
    values.erase(std::remove_if(values.begin(), values.end(), [](double v) { return !isFiniteValue(v) || !(v > 0.0); }),
                 values.end());
    if (values.size() < 5) return fallback;

    std::sort(values.begin(), values.end());
    const size_t last = values.size() - 1;
    double low = values[(size_t) std::floor(0.05 * last)];
    double high = values[(size_t) std::ceil(0.95 * last)];

    double span = semitonesBetween(low, high);
    if (span < minPitchAxisSemitones) {
        // Widen the range around its centre in the log domain.
        const double centre = 0.5 * (std::log2(low) + std::log2(high));
        low = std::exp2(centre - 0.5 * minPitchAxisSemitones / 12.0);
        high = std::exp2(centre + 0.5 * minPitchAxisSemitones / 12.0);
        span = minPitchAxisSemitones;
    }

    // A margin of 15 percent of the span at each side.
    const double margin = 0.15 * span / 12.0;
    PitchAxis axis;
    axis.low = std::clamp(low * std::exp2(-margin), 20.0, 1000.0);
    axis.high = std::clamp(high * std::exp2(margin), axis.low * std::exp2(minPitchAxisSemitones / 12.0), 2000.0);
    return axis;
}

// Move the shown axis towards the fitted axis. A wider range takes effect at once, so no value is
// outside the graph. A narrower range comes slowly, so the axis does not jump while a person speaks.
inline PitchAxis easePitchAxis(const PitchAxis &shown, const PitchAxis &target, double shrinkRate = 0.04)
{
    PitchAxis out;
    out.low = target.low < shown.low ? target.low : std::exp2(std::log2(shown.low) + shrinkRate * (std::log2(target.low) - std::log2(shown.low)));
    out.high = target.high > shown.high ? target.high : std::exp2(std::log2(shown.high) + shrinkRate * (std::log2(target.high) - std::log2(shown.high)));
    return out;
}

}

#endif // GUI_PITCHAXIS_H
