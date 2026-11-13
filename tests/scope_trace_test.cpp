#include <gtest/gtest.h>
#include "../src/context/scopetrace.h"

#include <cmath>

using namespace Main;

namespace {

std::vector<double> sine(double hz, double fs, int count, double amplitude = 1.0, double offset = 0.0)
{
    std::vector<double> s((size_t) count);
    for (int i = 0; i < count; ++i) s[(size_t) i] = offset + amplitude * std::sin(2.0 * M_PI * hz * i / fs);
    return s;
}

}

TEST(ScopeTrace, TP_SCP_01_TheSpanHasLimits)
{
    // 30 ms at 32000 Hz is 960 samples. The ring has 12800 samples.
    const ScopeSpan a = scopeSpan(30.0, 32000.0, 12800);
    EXPECT_EQ(a.window, 960);
    EXPECT_EQ(a.maxBack, 800);
    EXPECT_EQ(a.needed, 960 + 800 + 8);

    // A very short time gives 32 samples. A very long time gives half of the ring.
    EXPECT_EQ(scopeSpan(0.0, 32000.0, 12800).window, 32);
    EXPECT_EQ(scopeSpan(5000.0, 32000.0, 12800).window, 6400);
    EXPECT_EQ(scopeSpan(std::nan(""), 32000.0, 12800).window, 32);

    // A small ring gives no negative search distance.
    const ScopeSpan small = scopeSpan(30.0, 32000.0, 100);
    EXPECT_GE(small.maxBack, 0);
    EXPECT_GE(small.needed, small.window);
}

TEST(ScopeTrace, TP_SCP_02_NotEnoughSamplesGiveNoTrace)
{
    const ScopeSpan span = scopeSpan(30.0, 32000.0, 12800);
    EXPECT_FALSE(computeScopeTrace(std::vector<double>((size_t) span.needed - 1, 0.3), span).ok);
    EXPECT_FALSE(computeScopeTrace({}, span).ok);
    EXPECT_TRUE(computeScopeTrace(std::vector<double>((size_t) span.needed, 0.3), span).ok);
}

// The view starts at a rising zero crossing, so each frame of a steady tone has the same shape.
TEST(ScopeTrace, TP_SCP_03_AToneStartsAtARisingZeroCrossing)
{
    const double fs = 32000.0;
    const ScopeSpan span = scopeSpan(30.0, fs, 12800);
    const ScopeTrace trace = computeScopeTrace(sine(200.0, fs, 12800), span);
    ASSERT_TRUE(trace.ok);
    ASSERT_EQ((long long) trace.y.size(), span.window);
    EXPECT_NEAR(trace.y.front(), 0.0, 0.1);
    EXPECT_GT(trace.y[3], trace.y.front());

    // Two frames that differ by a part of a period give nearly the same picture.
    const ScopeTrace other = computeScopeTrace(sine(200.0, fs, 12800 - 37), span);
    ASSERT_TRUE(other.ok);
    double worst = 0;
    for (size_t k = 0; k < 200; ++k) worst = std::max(worst, std::fabs(trace.y[k] - other.y[k]));
    EXPECT_LT(worst, 0.2);
}

// The offset of the sound is removed, and the peak is 1.
TEST(ScopeTrace, TP_SCP_04_TheOffsetIsRemovedAndThePeakIsOne)
{
    const double fs = 32000.0;
    const ScopeSpan span = scopeSpan(30.0, fs, 12800);
    const ScopeTrace trace = computeScopeTrace(sine(150.0, fs, 12800, 0.02, 0.3), span);
    ASSERT_TRUE(trace.ok);
    double peak = 0, mean = 0;
    for (double y : trace.y) {
        peak = std::max(peak, std::fabs(y));
        mean += y;
    }
    mean /= (double) trace.y.size();
    EXPECT_NEAR(peak, 1.0, 1e-9);
    EXPECT_LT(std::fabs(mean), 0.2);
}

// Silence gives a flat trace and no value that is not a number.
TEST(ScopeTrace, TP_SCP_05_SilenceGivesAFlatTrace)
{
    const ScopeSpan span = scopeSpan(30.0, 32000.0, 12800);
    const ScopeTrace trace = computeScopeTrace(std::vector<double>(12800, 0.0), span);
    ASSERT_TRUE(trace.ok);
    for (double y : trace.y) {
        EXPECT_TRUE(std::isfinite(y));
        EXPECT_EQ(y, 0.0);
    }
}
