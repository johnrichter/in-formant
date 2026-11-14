#include <gtest/gtest.h>
#include "analysis_signals.h"
#include "../src/analysis/linpred/linpred.h"
#include "../src/analysis/util/aberth.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <thread>
#include <vector>

using namespace Analysis;
using namespace AnalysisSignals;

namespace {

// An AR(2) process with a pole pair, driven by a fixed noise. Its LPC solution is a known polynomial.
std::vector<double> arProcess(int length, double a1, double a2, unsigned seed)
{
    const auto e = whiteNoise(length, 0.1, seed);
    std::vector<double> x(length, 0.0);
    for (int i = 0; i < length; ++i) {
        x[i] = e[i] + (i > 0 ? a1 * x[i - 1] : 0.0) + (i > 1 ? a2 * x[i - 2] : 0.0);
    }
    return x;
}

// A polynomial with the given roots, highest power first.
std::vector<double> polyFromRoots(const std::vector<std::complex<double>> &roots)
{
    std::vector<std::complex<double>> p{1.0};
    for (const auto &r : roots) {
        std::vector<std::complex<double>> next(p.size() + 1, 0.0);
        for (size_t i = 0; i < p.size(); ++i) {
            next[i] += p[i];
            next[i + 1] -= r * p[i];
        }
        p = next;
    }
    std::vector<double> out;
    for (const auto &c : p)
        out.push_back(c.real());
    return out;
}

// True when each expected root has a found root within the tolerance.
bool hasAllRoots(const std::vector<std::complex<double>> &found, const std::vector<std::complex<double>> &expected,
                 double tolerance)
{
    for (const auto &e : expected) {
        const bool near = std::any_of(found.begin(), found.end(),
                                      [&](const std::complex<double> &f) { return std::abs(f - e) < tolerance; });
        if (!near)
            return false;
    }
    return true;
}

} // namespace

TEST(AnalysisState, TP_ANA_26_CovarSolversOnTwoThreadsDoNotShareWorkBuffers)
{
    auto run = [](double a1, double a2, int length, int order, std::vector<double> &out) {
        LP::Covar covar;
        const auto x = arProcess(length, a1, a2, 99);
        for (int i = 0; i < 300; ++i)
            out = covar.solve(x.data(), length, order, nullptr);
    };
    std::vector<double> refA, refB, outA, outB;
    run(1.2, -0.6, 1000, 2, refA);
    run(0.3, -0.2, 777, 6, refB);
    std::thread ta(run, 1.2, -0.6, 1000, 2, std::ref(outA));
    std::thread tb(run, 0.3, -0.2, 777, 6, std::ref(outB));
    ta.join();
    tb.join();
    EXPECT_EQ(outA, refA);
    EXPECT_EQ(outB, refB);
    // The first solver finds the process: the LPC sign convention gives -a1 and -a2.
    ASSERT_EQ(refA.size(), 2u);
    EXPECT_NEAR(refA[0], -1.2, 0.1);
    EXPECT_NEAR(refA[1], 0.6, 0.1);
}

TEST(AnalysisState, TP_ANA_27_AberthRootsFollowEachPolynomialNotTheFirstOne)
{
    // The first polynomial has small roots, and the second one has large roots. The start values come from the
    // bounds of the polynomial at hand.
    const std::vector<std::complex<double>> small{{0.3, 0.2}, {0.3, -0.2}, {-0.5, 0.0}};
    const std::vector<std::complex<double>> large{{40.0, 30.0}, {40.0, -30.0}, {-70.0, 0.0}, {5.0, 0.0}};
    for (int round = 0; round < 5; ++round) {
        EXPECT_TRUE(hasAllRoots(aberthRoots(polyFromRoots(small)), small, 1e-6));
        EXPECT_TRUE(hasAllRoots(aberthRoots(polyFromRoots(large)), large, 1e-4));
    }
}

TEST(AnalysisState, TP_ANA_28_AberthRootsOnTwoThreadsFindTheirRoots)
{
    const std::vector<std::complex<double>> a{{0.9, 0.3}, {0.9, -0.3}, {-0.4, 0.5}, {-0.4, -0.5}};
    const std::vector<std::complex<double>> b{{0.5, 0.7}, {0.5, -0.7}, {0.1, 0.0}};
    auto run = [](const std::vector<std::complex<double>> &roots, int &failures) {
        const auto poly = polyFromRoots(roots);
        for (int i = 0; i < 300; ++i) {
            if (!hasAllRoots(aberthRoots(poly), roots, 1e-6))
                ++failures;
        }
    };
    int failuresA = 0, failuresB = 0;
    std::thread ta(run, std::cref(a), std::ref(failuresA));
    std::thread tb(run, std::cref(b), std::ref(failuresB));
    ta.join();
    tb.join();
    EXPECT_EQ(failuresA, 0);
    EXPECT_EQ(failuresB, 0);
}

TEST(AnalysisState, TP_ANA_29_AberthRootsAroundAnInitialPointUseThePointOfEachCall)
{
    // The roots of this polynomial lie at 0.4 rad and at 2.0 rad on a circle of radius 0.9. One cluster has the
    // mean of all four roots, whatever the start. A call that starts near 2.0 rad must give this mean, also after
    // a call that started near 0.4 rad.
    const std::vector<std::complex<double>> roots{std::polar(0.9, 0.4), std::polar(0.9, -0.4), std::polar(0.9, 2.0),
                                                  std::polar(0.9, -2.0)};
    const double mean = 0.9 * (std::cos(0.4) + std::cos(2.0)) / 2.0;
    const auto poly = polyFromRoots(roots);
    for (int round = 0; round < 3; ++round) {
        for (double phi : {0.4, 2.0, 0.4}) {
            const auto picked = aberthRootsAroundInitial(poly, 0.9, phi, 1);
            ASSERT_EQ(picked.size(), 1u);
            EXPECT_NEAR(picked[0].real(), mean, 1e-6);
            EXPECT_NEAR(picked[0].imag(), 0.0, 1e-6);
        }
    }
}

TEST(AnalysisState, TP_ANA_32_AberthRootsAroundAnInitialPointNeverGiveNan)
{
    // Four clusters for four roots. The k-means start can pick one root twice, so one cluster stays empty.
    // An empty cluster must keep its centroid, not divide by zero.
    const std::vector<std::complex<double>> roots{std::polar(0.9, 0.4), std::polar(0.9, -0.4), std::polar(0.9, 2.0),
                                                  std::polar(0.9, -2.0)};
    const auto poly = polyFromRoots(roots);
    for (int round = 0; round < 50; ++round) {
        const auto picked = aberthRootsAroundInitial(poly, 0.9, 0.4, 4);
        ASSERT_EQ(picked.size(), 4u);
        for (const auto &z : picked) {
            ASSERT_TRUE(std::isfinite(z.real())) << "round " << round;
            ASSERT_TRUE(std::isfinite(z.imag())) << "round " << round;
        }
    }
}
