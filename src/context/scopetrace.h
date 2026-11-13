#ifndef MAIN_SCOPE_TRACE_H
#define MAIN_SCOPE_TRACE_H

#include <algorithm>
#include <cmath>
#include <vector>

namespace Main {

    // How many samples the oscilloscope needs. The view shows `window` samples. The search for a trigger goes
    // back `maxBack` samples before the view. `ringSize` is the number of samples that the source can give.
    struct ScopeSpan {
        long long window = 32;
        long long maxBack = 0;
        long long needed = 40;
    };

    inline ScopeSpan scopeSpan(double windowMs, double fs, long long ringSize)
    {
        ScopeSpan span;
        const long long wanted = std::isfinite(windowMs * fs) ? std::llround(windowMs * fs / 1000.0) : 32;
        span.window = std::clamp<long long>(wanted, 32, std::max<long long>(32, ringSize / 2));
        span.maxBack = std::max<long long>(0, std::min<long long>(std::llround(0.025 * fs), ringSize - span.window - 1));
        span.needed = span.window + span.maxBack + 8;
        return span;
    }

    // The trace of the oscilloscope. `start` is the index of the first sample of the view in the input.
    struct ScopeTrace {
        bool ok = false;
        long long start = 0;
        std::vector<double> y;   // the samples of the view, without the offset, from -1 to 1
    };

    // The view starts at the last rising zero crossing before the newest window. A periodic sound then has the
    // same shape in each frame, like the trigger of an oscilloscope. The input holds the newest samples, the
    // last one is the newest. It needs at least span.needed samples.
    inline ScopeTrace computeScopeTrace(const std::vector<double> &s, const ScopeSpan &span)
    {
        ScopeTrace trace;
        const long long total = (long long) s.size();
        if (total < span.needed) return trace;

        const long long window = span.window;
        const long long maxBack = span.maxBack;
        auto at = [&](long long index) { return s[(size_t) index]; };

        // The offset of the sound. The trigger and the view remove it.
        double dc = 0;
        for (long long i = total - window - maxBack; i < total; ++i) dc += at(i);
        dc /= (double) (window + maxBack);

        double rough = 0;
        for (long long i = total - window - maxBack; i < total; ++i) rough = std::max(rough, std::fabs(at(i) - dc));
        const double threshold = 0.05 * rough;

        long long start = total - window;
        if (rough > 1e-6) {
            for (long long c = total - window; c > total - window - maxBack; --c) {
                if (at(c - 1) - dc < 0 && at(c) - dc >= 0 && at(c + 3) - dc > threshold) {
                    start = c;
                    break;
                }
            }
        }

        double peak = 1e-9;
        for (long long i = start; i < start + window; ++i) peak = std::max(peak, std::fabs(at(i) - dc));

        trace.y.reserve((size_t) window);
        for (long long k = 0; k < window; ++k) trace.y.push_back((at(start + k) - dc) / peak);
        trace.start = start;
        trace.ok = true;
        return trace;
    }

}

#endif // MAIN_SCOPE_TRACE_H
