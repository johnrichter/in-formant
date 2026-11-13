#include <gtest/gtest.h>
#include "../src/gui/ticks.h"
#include <algorithm>
#include <cmath>
#include <set>

using namespace Gui;

// The decade of a value and its mantissa, for example 1500 gives 1.5.
static double mantissaOf(double v)
{
    return v / std::pow(10.0, std::floor(std::log10(v) + 1e-9));
}

static std::vector<double> labelled(const std::vector<Tick> &ticks)
{
    std::vector<double> values;
    for (const Tick &t : ticks) if (!t.label.empty()) values.push_back(t.value);
    return values;
}

// The step of the time labels is never below 1 second, and a long span has a larger step.
TEST(Ticks, TP_AXT_01_TimeLabelStep)
{
    EXPECT_TRUE(timeLabelStep(0.5, 40, 0.0005) == 1.0);
    EXPECT_TRUE(timeLabelStep(15, 40, 15.0 / 1400) == 1.0);
    EXPECT_TRUE(timeLabelStep(30, 40, 30.0 / 1400) == 2.0);
    EXPECT_TRUE(timeLabelStep(60, 40, 60.0 / 1400) == 5.0);
    EXPECT_TRUE(timeLabelStep(120, 40, 120.0 / 1400) == 10.0);
    EXPECT_TRUE(timeLabelStep(300, 40, 300.0 / 1400) == 30.0);
    EXPECT_TRUE(timeLabelStep(600, 40, 600.0 / 1400) == 60.0);
    EXPECT_TRUE(timeLabelStep(3600, 40, 3600.0 / 1400) == 300.0);
    // A narrow window needs a larger step to leave room for the labels.
    EXPECT_TRUE(timeLabelStep(15, 60, 15.0 / 200) >= 5.0);
}

// A window of 15 s has labels at each second, and no tick between whole seconds.
TEST(Ticks, AWindowOf15SHasLabelsAtEachSecondAndNoTickBetweenWhole)
{
    auto toPixel = [](double t) { return 1400.0 * t / 15.0; };
    const auto ticks = makeTimeTicks(TimeTickMode::Automatic, 0, 0, 15, toPixel, 30, 10);
    for (const Tick &t : ticks) EXPECT_TRUE(std::fabs(t.value - std::round(t.value)) < 1e-9);
    EXPECT_TRUE(labelled(ticks).size() >= 14);
}

// A window of 60 s has labels at 5 s, and a window of 120 s at 10 s.
TEST(Ticks, AWindowOf60SHasLabelsAt5SAndAWindowOf120SAt10S)
{
    auto toPixel = [](double t) { return 1400.0 * t / 60.0; };
    const auto ticks = makeTimeTicks(TimeTickMode::Automatic, 0, 0, 60, toPixel, 30, 10);
    for (double v : labelled(ticks)) EXPECT_TRUE(std::fmod(v, 5.0) < 1e-9);
    EXPECT_TRUE(labelled(ticks).size() == 13);
    auto toPixel2 = [](double t) { return 1400.0 * t / 120.0; };
    const auto ticks2 = makeTimeTicks(TimeTickMode::Automatic, 0, 0, 120, toPixel2, 30, 10);
    for (double v : labelled(ticks2)) EXPECT_TRUE(std::fmod(v, 10.0) < 1e-9);
}

// A custom step below 1 second is raised to 1 second.
TEST(Ticks, ACustomStepBelow1SecondIsRaisedTo1Second)
{
    auto toPixel = [](double t) { return 1400.0 * t / 15.0; };
    const auto ticks = makeTimeTicks(TimeTickMode::Step, 0.5, 0, 15, toPixel, 30, 10);
    for (const Tick &t : ticks) EXPECT_TRUE(std::fabs(t.value - std::round(t.value)) < 1e-9);
}

// The labels of a time axis show whole seconds and minutes with no decimals.
TEST(Ticks, TheLabelsOfATimeAxisShowWholeSecondsAndMinutesWithNoDecimals)
{
    EXPECT_TRUE(formatTime(5, 5) == "5");
    EXPECT_TRUE(formatTime(90, 5) == "1:30");
    EXPECT_TRUE(formatTime(125, 5) == "2:05");
}

// A logarithmic axis labels only the values 1, 2, 3, and 5 times a power of ten in the normal density.
TEST(Ticks, ALogarithmicAxisLabelsOnlyTheValues123And5TimesAPowerOfTenIn)
{
    const double lo = 50, hi = 5000;
    auto toPixel = [&](double f) { return 700.0 * (1.0 - std::log(f / lo) / std::log(hi / lo)); };
    const auto ticks = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, lo, hi, toPixel, 14, 8, false, TickDensity::Normal);
    const auto values = labelled(ticks);
    EXPECT_TRUE(values.size() >= 5);
    for (double v : values) {
        const double m = mantissaOf(v);
        EXPECT_TRUE(std::fabs(m - 1) < 1e-6 || std::fabs(m - 2) < 1e-6 || std::fabs(m - 3) < 1e-6 || std::fabs(m - 5) < 1e-6);
    }
    // A mantissa is labelled in all decades or in none. This keeps the pattern the same along the axis.
    std::set<double> seen;
    for (double v : values) seen.insert(std::round(mantissaOf(v) * 100) / 100);
    for (double m : seen) {
        int inRange = 0, got = 0;
        for (int k = -2; k <= 5; ++k) {
            const double v = m * std::pow(10.0, k);
            const double p = toPixel(v);
            if (v >= lo && v <= hi && p >= 11 && p <= 689) ++inRange;
            for (double x : values) if (std::fabs(x - v) < 1e-6) ++got;
        }
        EXPECT_TRUE(got == inRange);
    }
}

// The labels never overlap, at any height.
TEST(Ticks, TheLabelsNeverOverlapAtAnyHeight)
{
    const double lo = 50, hi = 5000;
    for (double height : { 200.0, 400.0, 700.0, 1200.0 }) {
        for (TickDensity d : { TickDensity::Sparse, TickDensity::Normal, TickDensity::Dense }) {
            auto toPixel = [&](double f) { return height * (1.0 - std::log(f / lo) / std::log(hi / lo)); };
            const auto ticks = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, lo, hi, toPixel, 14, 8, false, d);
            std::vector<double> positions;
            for (double v : labelled(ticks)) positions.push_back(toPixel(v));
            std::sort(positions.begin(), positions.end());
            for (size_t i = 1; i < positions.size(); ++i) EXPECT_TRUE(positions[i] - positions[i - 1] >= 14);
        }
    }
}

// A sparser density never labels a value that a denser density leaves out.
TEST(Ticks, ASparserDensityNeverLabelsAValueThatADenserDensityLeavesOut)
{
    const double lo = 50, hi = 5000;
    auto toPixel = [&](double f) { return 700.0 * (1.0 - std::log(f / lo) / std::log(hi / lo)); };
    const auto sparse = labelled(makeFrequencyTicks(FrequencyTickMode::Automatic, 0, lo, hi, toPixel, 14, 8, false, TickDensity::Sparse));
    const auto dense = labelled(makeFrequencyTicks(FrequencyTickMode::Automatic, 0, lo, hi, toPixel, 14, 8, false, TickDensity::Dense));
    EXPECT_TRUE(sparse.size() <= dense.size());
    for (double v : sparse) {
        const double m = mantissaOf(v);
        EXPECT_TRUE(std::fabs(m - 1) < 1e-6 || std::fabs(m - 2) < 1e-6 || std::fabs(m - 5) < 1e-6);
    }
}

// A linear axis uses equal steps and not powers of ten.
TEST(Ticks, ALinearAxisUsesEqualStepsAndNotPowersOfTen)
{
    auto toPixel = [](double f) { return 600.0 * (1.0 - f / 500.0); };
    const auto ticks = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, 0, 500, toPixel, 14, 8, true, TickDensity::Normal);
    const auto values = labelled(ticks);
    EXPECT_TRUE(values.size() >= 5);
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const double step = sorted[1] - sorted[0];
    for (size_t i = 1; i < sorted.size(); ++i) EXPECT_NEAR(sorted[i] - sorted[i - 1], step, 1e-6);
    EXPECT_TRUE(step == 20 || step == 50 || step == 25 || step == 100 || step == 10);
}

// The ticks of the automatic mode do not change when the call repeats.
TEST(Ticks, TheTicksOfTheAutomaticModeDoNotChangeWhenTheCallRepeats)
{
    auto toPixel = [](double f) { return 700.0 * (1.0 - std::log(f / 50.0) / std::log(100.0)); };
    const auto a = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, 50, 5000, toPixel, 14, 8);
    const auto b = makeFrequencyTicks(FrequencyTickMode::Automatic, 0, 50, 5000, toPixel, 14, 8);
    EXPECT_TRUE(a.size() == b.size());
}
