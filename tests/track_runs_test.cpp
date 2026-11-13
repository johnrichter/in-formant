#include <gtest/gtest.h>
#include "gui/trackruns.h"

#include <cmath>
#include <functional>
#include <limits>
#include <vector>

using namespace Gui;

namespace {

// The pitch-only graph: one octave (12 semitones) over 300 pixels on a logarithmic axis, from 100 to 200 Hz.
constexpr double kHeight = 300.0;
constexpr double kLow = 100.0;
constexpr double kHigh = 200.0;

double pitchY(double hz)
{
    return kHeight * (1.0 - std::log(hz / kLow) / std::log(kHigh / kLow));
}

// The pixels of a jump of the limit in semitones on this axis.
double limitPixels()
{
    return trackJumpPixels(pitchY, kLow, kHigh);
}

double hzAfter(double hz, double semitones)
{
    return hz * std::pow(2.0, semitones / 12.0);
}

// A track with one frame every 10 ms. The values are in Hz. A value of 0 means no value.
std::vector<TrackPoint> track(const std::vector<double> &hz)
{
    std::vector<TrackPoint> out;
    for (size_t i = 0; i < hz.size(); ++i) out.push_back({ 0.01 * (double) i, hz[i], hz[i] > 0.0 });
    return out;
}

}

// A linear axis from 60 to 500 Hz over 300 pixels.
double linearY(double hz)
{
    return kHeight * (1.0 - (hz - 60.0) / (500.0 - 60.0));
}

// The pixels of the jump limit follow from the semitones, and the line never gets less than a tenth of the height.
TEST(TrackRuns, TP_PITCH_07_TheJumpLimitIsTheLargerOfATenthOfTheHeightAndTheSemitoneJump)
{
    EXPECT_DOUBLE_EQ(limitPixels(), kHeight * 8.0 / 12.0);
    EXPECT_DOUBLE_EQ(trackJumpLimit(kHeight, limitPixels()), 200.0);

    // A wide view, such as the spectrogram, has a small jump in pixels. The tenth of the height then applies.
    EXPECT_DOUBLE_EQ(trackJumpLimit(kHeight, 12.0), 30.0);

    // No value for the jump gives the tenth of the height. This covers an axis with a minimum of 0 Hz.
    EXPECT_DOUBLE_EQ(trackJumpLimit(kHeight, -1.0), 30.0);
    EXPECT_DOUBLE_EQ(trackJumpLimit(kHeight, std::numeric_limits<double>::quiet_NaN()), 30.0);
    EXPECT_DOUBLE_EQ(trackJumpLimit(kHeight, std::numeric_limits<double>::infinity()), 30.0);
    EXPECT_DOUBLE_EQ(trackJumpLimit(0.0, 0.0), 0.0);
}

// The fault: a glide of 3 semitones in 10 ms is 75 pixels on this axis. The old limit of a tenth of the height was
// 30 pixels, so the line broke at each normal step of the voice, and only the dots stayed.
TEST(TrackRuns, TP_PITCH_07_AGlideThatTheVoiceMakesStaysOneRunInThePitchOnlyGraph)
{
    std::vector<double> hz;
    double value = 120.0;
    for (int i = 0; i < 20; ++i) {
        hz.push_back(value);
        value = hzAfter(value, 3.0 * (i % 2 == 0 ? 1.0 : -1.0)); // up 3 semitones, then down 3, and so on
    }
    const auto points = track(hz);

    // The old limit splits the track at each step. This shows the fault.
    EXPECT_EQ(splitTrackRuns(points, pitchY, 0.1 * kHeight).size(), points.size());

    // The new limit keeps one run.
    const auto runs = splitTrackRuns(points, pitchY, trackJumpLimit(kHeight, limitPixels()));
    ASSERT_EQ(runs.size(), 1u);
    EXPECT_EQ(runs[0].size(), points.size());
}

// A jump of an octave is an error of the pitch solver, and a line over it shows a change that never happened.
TEST(TrackRuns, TP_PITCH_07_AnOctaveJumpStillBreaksTheLine)
{
    const auto points = track({ 130.0, 131.0, 132.0, hzAfter(132.0, 12.0), hzAfter(133.0, 12.0), 134.0, 135.0 });
    const auto runs = splitTrackRuns(points, pitchY, trackJumpLimit(kHeight, limitPixels()));
    ASSERT_EQ(runs.size(), 3u);
    EXPECT_EQ(runs[0].size(), 3u);
    EXPECT_EQ(runs[1].size(), 2u);
    EXPECT_EQ(runs[2].size(), 2u);
}

// The border of the limit: 7.9 semitones join, and 8.1 semitones split.
TEST(TrackRuns, TP_PITCH_07_TheBorderIsEightSemitones)
{
    const double limit = trackJumpLimit(kHeight, limitPixels());
    EXPECT_EQ(splitTrackRuns(track({ 110.0, hzAfter(110.0, 7.9) }), pitchY, limit).size(), 1u);
    EXPECT_EQ(splitTrackRuns(track({ 110.0, hzAfter(110.0, -7.9) }), pitchY, limit).size(), 1u);
    EXPECT_EQ(splitTrackRuns(track({ 110.0, hzAfter(110.0, 8.1) }), pitchY, limit).size(), 2u);
    EXPECT_EQ(splitTrackRuns(track({ 140.0, hzAfter(140.0, -8.1) }), pitchY, limit).size(), 2u);
}

// A frame with no value ends a run, and a time gap of more than three median gaps ends a run.
TEST(TrackRuns, TP_PITCH_07_AFrameWithNoValueAndALongGapEndARun)
{
    const double limit = trackJumpLimit(kHeight, limitPixels());

    const auto holes = splitTrackRuns(track({ 120.0, 121.0, 0.0, 122.0, 123.0, 124.0 }), pitchY, limit);
    ASSERT_EQ(holes.size(), 2u);
    EXPECT_EQ(holes[0].size(), 2u);
    EXPECT_EQ(holes[1].size(), 3u);

    auto gap = track({ 120.0, 121.0, 122.0, 123.0, 124.0, 125.0 });
    for (size_t i = 3; i < gap.size(); ++i) gap[i].time += 0.5;
    const auto gaps = splitTrackRuns(gap, pitchY, limit);
    ASSERT_EQ(gaps.size(), 2u);
    EXPECT_EQ(gaps[0].size(), 3u);
    EXPECT_EQ(gaps[1].size(), 3u);

    EXPECT_TRUE(splitTrackRuns({}, pitchY, limit).empty());
    EXPECT_TRUE(splitTrackRuns(track({ 0.0, 0.0 }), pitchY, limit).empty());
    EXPECT_EQ(splitTrackRuns(track({ 120.0 }), pitchY, limit).size(), 1u);
}

// On a linear axis (the fixed axis of the pitch-only graph) a semitone is smaller at low frequencies. The limit
// is measured at the low end, so an octave error of 100 to 200 Hz still splits the line. The first version
// measured in the middle of the range, and the limit then joined this error.
TEST(TrackRuns, TP_PITCH_07_AnOctaveErrorAtTheLowEndOfALinearAxisStillBreaksTheLine)
{
    const double pixels = trackJumpPixels(linearY, 60.0, 500.0);
    const double limit = trackJumpLimit(kHeight, pixels);

    // The jump of 8 semitones at 60 Hz is 35 Hz, so the tenth of the height (44 Hz of the range) applies.
    EXPECT_NEAR(pixels, kHeight * (60.0 * (std::pow(2.0, 8.0 / 12.0) - 1.0)) / 440.0, 1e-9);
    EXPECT_DOUBLE_EQ(limit, 0.1 * kHeight);

    // An octave from 100 to 200 Hz is 100 Hz, which is 68 pixels. It splits.
    EXPECT_EQ(splitTrackRuns(track({ 100.0, 101.0, 200.0, 201.0 }), linearY, limit).size(), 2u);

    // A glide of 3 semitones at 120 Hz is 21 Hz, which is 14 pixels. It stays.
    EXPECT_EQ(splitTrackRuns(track({ 120.0, hzAfter(120.0, 3.0), 120.0 }), linearY, limit).size(), 1u);

    // A range with a minimum of 0 Hz, no range, or a value that is not a number gives no height.
    EXPECT_EQ(trackJumpPixels(linearY, 0.0, 500.0), -1.0);
    EXPECT_EQ(trackJumpPixels(linearY, 500.0, 60.0), -1.0);
    EXPECT_EQ(trackJumpPixels(linearY, 60.0, 60.0), -1.0);
    EXPECT_EQ(trackJumpPixels(linearY, std::numeric_limits<double>::quiet_NaN(), 500.0), -1.0);
    EXPECT_EQ(trackJumpPixels(linearY, 60.0, std::numeric_limits<double>::infinity()), -1.0);
}
