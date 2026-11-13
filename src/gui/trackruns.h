#ifndef GUI_TRACKRUNS_H
#define GUI_TRACKRUNS_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

// The runs of a track. A line joins the points of one run, and it never crosses the gap between two runs. The code
// has no dependency on Qt, so a test can run it.

namespace Gui {

    // One point of a track in the time and the transformed frequency.
    struct TrackPoint {
        double time;
        double value;
        bool valid;
    };

    // The semitone jump between two frames that the line still joins. An octave error of a pitch solver is 12
    // semitones, and the voice does not move 8 semitones in one frame of 10 to 20 ms.
    constexpr double kTrackJumpSemitones = 8.0;

    // The largest jump in pixels between two frames that a line still joins. At least a tenth of the height of the
    // graph, because a view of a wide range (the spectrogram) has a small jump in pixels for a big jump in Hz. In
    // the graph of the pitch alone, the range is small, and a tenth of its height is only one or two semitones. A
    // normal glide is larger than that, and a limit of a tenth of the height cut the line into short pieces. The
    // pixels of the semitone jump are the other bound, and the larger one wins.
    inline double trackJumpLimit(double plotHeight, double pixelsOfJumpSemitones)
    {
        const double fraction = 0.1 * plotHeight;
        if (!std::isfinite(pixelsOfJumpSemitones) || pixelsOfJumpSemitones < 0.0) return fraction;
        return std::max(fraction, pixelsOfJumpSemitones);
    }

    // The height in pixels of a jump of kTrackJumpSemitones that starts at the lowest frequency of the view. On a
    // logarithmic axis the height is the same everywhere. On a linear axis a semitone is smallest at the low end,
    // so the low end gives the smallest height. The limit then never joins a jump that is larger than the border at
    // any frequency of the view. The result is -1 when the range is not valid (a minimum of 0 Hz, or no range).
    inline double trackJumpPixels(const std::function<double(double)> &frequencyToY, double minFrequency, double maxFrequency)
    {
        if (!(minFrequency > 0.0) || !(maxFrequency > minFrequency) || !std::isfinite(maxFrequency)) return -1.0;
        const double above = minFrequency * std::pow(2.0, kTrackJumpSemitones / 12.0);
        const double pixels = std::fabs(frequencyToY(above) - frequencyToY(minFrequency));
        return std::isfinite(pixels) ? pixels : -1.0;
    }

    // Split the points into runs of indices. A run has no frame without a value. A run also ends at a time gap of
    // more than three times the median gap, and at a jump of more than maxJump pixels. A line across a jump shows a
    // change that never happened, for example an octave error of the pitch.
    inline std::vector<std::vector<size_t>> splitTrackRuns(const std::vector<TrackPoint> &points,
                                                           const std::function<double(double)> &toY,
                                                           double maxJump)
    {
        std::vector<std::vector<size_t>> runs;
        std::vector<size_t> current;

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
        return runs;
    }

}

#endif // GUI_TRACKRUNS_H
