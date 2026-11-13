#include <gtest/gtest.h>

#include <cmath>

#include "../src/gui/readoutstats.h"

using namespace Gui;
using namespace Gui::stats;

namespace {

HudTrack track(std::vector<double> values, size_t frames = 0, bool isPitch = true)
{
    HudTrack t;
    t.name = isPitch ? "Pitch" : "F1";
    t.isPitch = isPitch;
    t.values = std::move(values);
    t.frames = frames ? frames : t.values.size();
    return t;
}

}

// The report and the live table use the standard deviation with N minus 1.
TEST(ReadoutStats, TP_STA_01_TheReportAndTheLiveTableUseNMinusOne)
{
    // The values 2, 4, 4, 4, 5, 5, 7, 9 have a population SD of 2 and a sample SD of sqrt(32 / 7).
    const std::vector<double> values = { 9, 4, 5, 2, 7, 4, 5, 4 };
    const double sample = std::sqrt(32.0 / 7.0);

    Main::Summary summary;
    summary.frames = values.size();
    summary.values = values;
    std::sort(summary.values.begin(), summary.values.end());
    EXPECT_NEAR(summary.sd(), sample, 1e-12);

    const HudTrack t = track(values);
    const TrackStats s = computeStats(t);
    EXPECT_NEAR(s.sd, sample, 1e-12);
    EXPECT_NE(s.sd, 2.0);
    EXPECT_EQ(s.sd, summary.sd());
    EXPECT_EQ(statText(t, s, StatStdDev, false), "2.1");

    // One value has no spread.
    EXPECT_EQ(computeStats(track({ 5.0 })).sd, 0.0);
}

// The percentiles for 1, 2, 3, and 1000 values, and for equal values.
TEST(ReadoutStats, TP_STA_02_ThePercentilesForFewAndManyValues)
{
    auto check = [](const std::vector<double> &values, double p5, double median, double p95) {
        const TrackStats s = computeStats(track(values));
        EXPECT_NEAR(s.p5, p5, 1e-9) << values.size();
        EXPECT_NEAR(s.median, median, 1e-9) << values.size();
        EXPECT_NEAR(s.p95, p95, 1e-9) << values.size();
        // The percentiles lie between the smallest and the largest value.
        EXPECT_GE(s.p5, s.sorted.front());
        EXPECT_LE(s.p95, s.sorted.back());
    };

    check({ 7.0 }, 7.0, 7.0, 7.0);
    check({ 10.0, 20.0 }, 10.5, 15.0, 19.5);
    check({ 3.0, 1.0, 2.0 }, 1.1, 2.0, 2.9);
    std::vector<double> many;
    for (int i = 1000; i >= 1; --i) many.push_back(i);
    check(many, 50.95, 500.5, 950.05);
    check({ 7.0, 7.0, 7.0, 7.0 }, 7.0, 7.0, 7.0);
}

// A window with no value has a dash, and a count of 0 when it has frames.
TEST(ReadoutStats, TP_STA_03_CvVoicedShareAndCountForAWindowWithNoValues)
{
    // Frames with no value: the share is 0, the count is 0, and the other statistics have a dash.
    const HudTrack silent = track({}, 50);
    const TrackStats s = computeStats(silent);
    EXPECT_EQ(statText(silent, s, StatVoiced, false), "0%");
    EXPECT_EQ(statText(silent, s, StatCount, false), "0");
    for (int bit : { StatCurrent, StatMin, StatP5, StatMean, StatMedian, StatStdDev, StatP95, StatMax, StatRange, StatCV, StatNote }) {
        EXPECT_EQ(statText(silent, s, bit, false), "-") << bit;
    }
    EXPECT_EQ(statText(silent, s, StatRange, true), "-");

    // No frame at all: every statistic has a dash.
    const HudTrack none = track({}, 0);
    const TrackStats n = computeStats(none);
    EXPECT_EQ(n.sd, 0.0);
    EXPECT_EQ(statText(none, n, StatVoiced, false), "-");
    EXPECT_EQ(statText(none, n, StatCount, false), "-");
    EXPECT_EQ(statText(none, n, StatCV, false), "-");

    // A mean of 0 or below gives no CV. The share and the count follow the frames.
    const HudTrack zero = track({ 0.0, 0.0, 0.0 }, 6);
    EXPECT_EQ(statText(zero, computeStats(zero), StatCV, false), "-");
    EXPECT_EQ(statText(zero, computeStats(zero), StatVoiced, false), "50%");
    EXPECT_EQ(statText(zero, computeStats(zero), StatCount, false), "3");

    // A long window goes through a reduction. The count then comes from the field count.
    HudTrack reduced = track({ 100.0, 110.0 }, 1000);
    reduced.count = 400;
    EXPECT_EQ(statText(reduced, computeStats(reduced), StatCount, false), "400");
    EXPECT_EQ(statText(reduced, computeStats(reduced), StatVoiced, false), "40%");
}

// The start of the window of the statistics, in the first seconds of a session.
TEST(ReadoutStats, TP_STA_04_TheWindowOfTheStatisticsAtTheStartOfASession)
{
    // The visible window (0) follows the view. Another window ends at the end of the view.
    EXPECT_EQ(statsWindowStart(0.0, 15.0, 0.0), 0.0);
    EXPECT_EQ(statsWindowStart(40.0, 55.0, 0.0), 40.0);
    EXPECT_EQ(statsWindowStart(40.0, 55.0, 5.0), 50.0);
    EXPECT_EQ(statsWindowStart(40.0, 55.0, 10.0), 45.0);
    EXPECT_EQ(statsWindowStart(40.0, 55.0, 30.0), 25.0);
    EXPECT_EQ(statsWindowStart(40.0, 55.0, 60.0), -5.0);

    // At the start of a session less data exists than the window. The start is below 0, and no entry lies
    // there, so the statistics use all data that exists: each window of 5 s or longer holds the first 3 s.
    for (double window : { 5.0, 10.0, 30.0, 60.0 }) {
        const double start = statsWindowStart(0.0, 3.0, window);
        EXPECT_LT(start, 0.0) << window;
        EXPECT_LE(start, 0.0);
    }
    // The visible window at the start of a live view starts at 0 too.
    EXPECT_EQ(statsWindowStart(0.0, 3.0, 0.0), 0.0);
}

// The note name and the cents of a pitch.
TEST(ReadoutStats, TP_STA_05_TheNoteNameAndTheCents)
{
    double cents = 99.0;
    EXPECT_EQ(noteName(440.0, &cents), "A4");
    EXPECT_NEAR(cents, 0.0, 1e-9);

    // 435 Hz is 19.8 cents below A4.
    EXPECT_EQ(noteName(435.0, &cents), "A4");
    EXPECT_NEAR(cents, 1200.0 * std::log2(435.0 / 440.0), 1e-9);
    const HudTrack t = track({ 435.0 });
    EXPECT_EQ(statText(t, computeStats(t), StatNote, false), "A4 -20c");
    const HudTrack exact = track({ 440.0 });
    EXPECT_EQ(statText(exact, computeStats(exact), StatNote, false), "A4 +0c");

    // The lowest and the highest pitch of the tracker.
    EXPECT_EQ(noteName(65.406, &cents), "C2");
    EXPECT_NEAR(cents, 0.0, 0.1);
    EXPECT_EQ(noteName(20.0, &cents), "D#0");
    EXPECT_EQ(noteName(4186.01, &cents), "C8");
    EXPECT_EQ(noteName(8000.0, &cents), "B8");
    EXPECT_EQ(noteName(261.626, &cents), "C4");

    // A value of 0 or below, or a value that is not a number, has no note.
    cents = 99.0;
    EXPECT_EQ(noteName(0.0, &cents), "");
    EXPECT_EQ(cents, 0.0);
    EXPECT_EQ(noteName(-100.0, &cents), "");
    EXPECT_EQ(noteName(std::nan(""), &cents), "");
    // A value that is not a pitch (a formant) has no note.
    const HudTrack formant = track({ 500.0 }, 0, false);
    EXPECT_EQ(statText(formant, computeStats(formant), StatNote, false), "-");
    // A tiny pitch does not crash, and the cents stay in range.
    noteName(1e-9, &cents);
    EXPECT_LE(std::fabs(cents), 50.0 + 1e-6);
}

// The number format changes at 1000, so a value fits the width of its column.
TEST(ReadoutStats, TP_STA_06_TheNumberFormatAt999_95And1000)
{
    EXPECT_EQ(number(0.0), "0.0");
    EXPECT_EQ(number(123.456), "123.5");
    EXPECT_EQ(number(999.94), "999.9");
    // The value 999.95 rounds up to 1000.0. This text has 6 characters, as wide as the pattern 0000.0.
    EXPECT_EQ(number(999.95), "1000.0");
    EXPECT_EQ(number(999.95).size(), std::string("0000.0").size());
    EXPECT_EQ(number(1000.0), "1000");
    EXPECT_EQ(number(1000.4), "1000");
    EXPECT_EQ(number(5499.6), "5500");
    EXPECT_EQ(number(-250.25), "-250.2");
    // The widest value that the table shows has no more than the 6 characters of the pattern.
    for (double v : { 0.0, 9.99, 99.99, 999.94, 999.95, 1000.0, 5500.0, 9999.4 }) {
        EXPECT_LE(number(v).size(), std::string("0000.0").size()) << v;
    }
}
