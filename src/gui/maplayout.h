#ifndef GUI_MAPLAYOUT_H
#define GUI_MAPLAYOUT_H

#include <algorithm>
#include <cmath>
#include <vector>

// The pure logic of the session map: which part of the session the map shows, and the bins of its
// picture. The functions have no drawing code, so a test can run them.

namespace Gui {

// The part of the session that the map shows.
struct MapView {
    double start = 0.0;
    double span = 1.0;
};

// What the map remembers between two frames.
struct MapState {
    double start = 0.0;
    // True when the map keeps the newest sound in view during a live capture.
    bool follow = true;
    double lastEnd = -1.0;
    bool wasLive = false;
};

// The map shows mapWindow seconds. A session that is shorter has a map of its whole length. A frame
// that is longer than the window widens the map, so that the frame has room at both sides.
inline double mapSpanFor(double duration, double mapWindow, double viewSpan)
{
    return std::min(duration, std::max(mapWindow, 1.25 * viewSpan));
}

// Update the state and return the part of the session that the map shows.
// live is true when the main view follows the newest sound. frameEnd is the end of the main view.
inline MapView updateMapView(MapState &state, double duration, double mapWindow, double viewSpan,
                             double frameEnd, bool live)
{
    duration = std::max(duration, 1.0);

    if (live && !state.wasLive) state.follow = true;
    state.wasLive = live;

    MapView view;
    if (duration <= mapWindow) {
        // The whole session fits in the map.
        state.start = 0.0;
        state.follow = true;
        state.lastEnd = frameEnd;
        view.start = 0.0;
        view.span = duration;
        return view;
    }

    view.span = mapSpanFor(duration, mapWindow, viewSpan);
    const double maxStart = duration - view.span;

    if (live && state.follow) {
        state.start = maxStart;
    }
    else if (!live && frameEnd != state.lastEnd) {
        // The frame moved. Scroll the map only when the frame is outside of it.
        const double frameStart = frameEnd - viewSpan;
        if (frameEnd > state.start + view.span) state.start = frameEnd - 0.9 * view.span;
        else if (frameStart < state.start) state.start = frameStart - 0.1 * view.span;
    }

    state.start = std::clamp(state.start, 0.0, maxStart);
    state.lastEnd = frameEnd;
    view.start = state.start;
    return view;
}

// Move the map by a time. The map follows the live capture again when it reaches the end.
inline void scrollMap(MapState &state, double duration, const MapView &view, double delta)
{
    const double maxStart = std::max(0.0, std::max(duration, 1.0) - view.span);
    state.start = std::clamp(view.start + delta, 0.0, maxStart);
    state.follow = state.start >= maxStart - 1e-6;
}

// The number of samples in one bin of the picture. A bin is a little wider than a pixel. The
// width comes from a short list of round values, so it stays the same while the window changes a
// little. The bins start at multiples of this number, so a bin keeps its samples when the map
// scrolls or when new sound arrives.
inline long long mapBinSamples(double span, int widthPixels, int sampleRate)
{
    const double ideal = std::max(1.0, span * sampleRate / std::max(widthPixels, 1));
    const double magnitude = std::pow(10.0, std::floor(std::log10(ideal)));
    static const double ladder[] = { 1.0, 1.25, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0 };
    for (double step : ladder) {
        if (step * magnitude >= ideal - 1e-9) return std::max(1LL, (long long) std::llround(step * magnitude));
    }
    return std::max(1LL, (long long) std::llround(10.0 * magnitude));
}

// The size of the bin for a picture that scales while the map shows a longer or a shorter time. The
// current size stays while it is between one and two and a half pixels wide. A new size is a power
// of two, and it is between one and two pixels wide. A new size is then rare, so the map does not
// need a new picture for each frame.
inline long long mapStableBinSamples(long long current, double span, int widthPixels, int sampleRate)
{
    const double ideal = std::max(1.0, span * sampleRate / std::max(widthPixels, 1));
    if (current > 0 && (double) current >= ideal && (double) current <= 2.5 * ideal) return current;

    long long size = 1;
    while ((double) size < ideal) size *= 2;
    return size;
}

// The first bin and the number of bins that cover the part of the map from start to start + span.
struct MapBins {
    long long first = 0;
    int count = 0;
};

inline MapBins mapBinRange(const MapView &view, long long binSamples, int sampleRate)
{
    MapBins bins;
    const double perBin = (double) binSamples / sampleRate;
    bins.first = (long long) std::floor(view.start / perBin);
    const long long last = (long long) std::ceil((view.start + view.span) / perBin);
    bins.count = (int) std::max(1LL, last - bins.first);
    return bins;
}

// The limits that a frame edge can take while the person drags it, in seconds of view span.
constexpr double minViewSpan = 0.5;
constexpr double maxViewSpan = 86400.0;

inline double clampViewSpan(double span)
{
    return std::clamp(span, minViewSpan, maxViewSpan);
}

// The span that a drag of a frame edge sets. The span has one decimal, so the View field shows a
// short number. The span cannot be longer than the longer of the session and the map time.
inline double dragViewSpan(double span, double duration, double mapWindow)
{
    const double limit = std::max(std::max(duration, mapWindow), minViewSpan);
    return std::clamp(std::round(span * 10.0) / 10.0, minViewSpan, std::min(limit, maxViewSpan));
}

// The length of the session that the map shows. The sound arrives in blocks, and the length of the session grows in
// steps. Between two blocks, the shown length grows with the clock, up to a short limit, so the map moves smoothly.
// The clock is a number of seconds from any start, so a test can use a fake clock.
struct MapClock {
    double lastDuration = -1.0;     // the length of the session at the last call
    double stamp = 0.0;             // the time of the last change of the length
    double shown = 1.0;             // the length that the map shows
};

constexpr double mapSmoothLimit = 0.25;

inline double updateShownDuration(MapClock &clock, double duration, double now, bool running)
{
    // A length that is shorter than before means that the session started again (a reset or a new file).
    const bool restarted = duration < clock.lastDuration;
    if (duration != clock.lastDuration) {
        clock.lastDuration = duration;
        clock.stamp = now;
    }

    double shown = duration;
    if (running) {
        shown += std::min(std::max(now - clock.stamp, 0.0), mapSmoothLimit);
        // The length never goes back while the capture runs. A late block of sound would else move the map back.
        if (!restarted) shown = std::max(shown, clock.shown);
    }
    clock.shown = std::max(shown, 1.0);
    return clock.shown;
}

// The scale of the loudness of the map. A louder sound raises the scale at once, so no sound is cut off. A quieter
// sound lowers it with a time constant of one second. The change follows the clock, and not the number of drawings,
// so ten drawings in a second give the same scale as two drawings. The elapsed time is limited to 2 seconds.
constexpr float mapMinimumPeak = 0.02f;

inline float updateMapPeak(float peak, float target, double elapsedSeconds)
{
    target = std::max(target, mapMinimumPeak);
    const double elapsed = std::clamp(elapsedSeconds, 0.0, 2.0);
    const double blend = 1.0 - std::exp(-elapsed / 1.0);
    return target >= peak ? target : peak - (float) blend * (peak - target);
}

// The scale of the pitch line of the map, in Hz. The scale fits the pitch in the map with a margin of 15 percent.
// It grows at once and shrinks slowly. It always covers at least one octave.
struct MapPitchScale {
    double low = 60.0;
    double high = 600.0;
};

inline MapPitchScale updateMapPitchScale(MapPitchScale scale, double lowest, double highest, double elapsedSeconds)
{
    if (!(highest > 0.0) || !(lowest > 0.0)) return scale;
    const double blend = 1.0 - std::exp(-std::clamp(elapsedSeconds, 0.0, 2.0) / 1.0);

    double targetLow = lowest / 1.15;
    double targetHigh = highest * 1.15;
    if (targetHigh / targetLow < 2.0) {
        const double centre = std::sqrt(targetLow * targetHigh);
        targetLow = centre / std::sqrt(2.0);
        targetHigh = centre * std::sqrt(2.0);
    }
    scale.low = targetLow < scale.low ? targetLow : scale.low + blend * (targetLow - scale.low);
    scale.high = targetHigh > scale.high ? targetHigh : scale.high + blend * (targetHigh - scale.high);
    return scale;
}

// The pitch line breaks where the track has no pitch for more than 70 ms, and where a bin is empty. The number is
// the largest distance in bins between two bins that the line joins. A bin that is wider than 70 ms joins its
// neighbour only.
inline int mapMaxGapBins(double secondsPerBin)
{
    return std::max(1, (int) std::ceil(0.07 / secondsPerBin));
}

// The pieces of the pitch line. Each piece is a pair of bins that the line joins. The pitch of a bin is 0 when the
// bin has no pitch. The bins from first to end (not including end) are in view.
struct MapPitchPiece {
    int from;
    int to;
};

inline std::vector<MapPitchPiece> mapPitchPieces(const std::vector<float> &pitch, int first, int end, int maxGap)
{
    std::vector<MapPitchPiece> pieces;
    const int count = (int) std::min<size_t>(pitch.size(), (size_t) std::max(end, 0));
    int previous = -1;
    for (int i = std::max(0, first - maxGap - 1); i < count; ++i) {
        if (pitch[(size_t) i] <= 0) continue;
        if (previous >= 0 && i - previous <= maxGap) pieces.push_back({ previous, i });
        previous = i;
    }
    return pieces;
}

}

#endif // GUI_MAPLAYOUT_H
