#include <gtest/gtest.h>
#include "analysis_signals.h"
#include "../src/analysis/invglot/invglot.h"
#include "../src/analysis/filter/filter.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

using namespace Analysis;
using namespace AnalysisSignals;

namespace {

// A voiced frame: a harmonic sum at 120 Hz with a little noise, so the LPC problems are well posed.
std::vector<double> voicedFrame(double sampleRate, int length)
{
    auto x = harmonics(120.0, sampleRate, length, 12, 0.4);
    const auto noise = whiteNoise(length, 0.005, 7);
    for (int i = 0; i < length; ++i)
        x[i] += noise[i];
    return x;
}

// The output must be finite, and its largest absolute value must be 1.
void expectNormalized(const InvglotResult &r, int length, double sampleRate)
{
    EXPECT_DOUBLE_EQ(r.sampleRate, sampleRate);
    ASSERT_EQ((int) r.glotSig.size(), length);
    double peak = 0.0;
    for (double v : r.glotSig) {
        ASSERT_TRUE(std::isfinite(v));
        peak = std::max(peak, std::fabs(v));
    }
    EXPECT_NEAR(peak, 1.0, 1e-9);
}


// A reference of the IAIF and GFM-IAIF algorithms. It builds the high-pass filter and the Hann window for each call
// from the sample rate, so it holds no state. The solvers must give the same output.
rpm::vector<double> referenceLpc(const rpm::vector<double> &x, const rpm::vector<double> &w, int len, int order)
{
    rpm::vector<double> in(len);
    for (int i = 0; i < len; ++i)
        in[i] = w[i] * x[x.size() / 2 - len / 2 + i];
    LP::Burg burg;
    double gain;
    auto a = burg.solve(in.data(), len, order, &gain);
    a.insert(a.begin(), 1.0);
    return a;
}

rpm::vector<double> hann(int points)
{
    rpm::vector<double> w(points);
    for (int i = 0; i < points; ++i)
        w[i] = 0.5 - 0.5 * std::cos((2.0 * pi * i) / (double) (points - 1));
    return w;
}

rpm::vector<double> dropFront(const rpm::vector<double> &x, int n)
{
    return rpm::vector<double>(x.begin() + n, x.end());
}

rpm::vector<double> normalize(rpm::vector<double> g)
{
    double peak = 1e-10;
    for (double v : g)
        peak = std::max(peak, std::fabs(v));
    for (auto &v : g)
        v /= peak;
    return g;
}

rpm::vector<double> referenceIaif(const std::vector<double> &xData, double rate, double d)
{
    const int length = (int) xData.size();
    const int p_gl = 2;
    const int p_vt = 2 * std::round(rate / 2000) + 4;
    const int lpW = std::round(0.015 * rate);
    const rpm::vector<double> one({1.0});
    const rpm::vector<double> oneMinusD({1.0, -d});
    const auto window = hann(lpW);
    const auto hp = butterworthHighpass(8, 70.0, rate);

    const int preflt = p_vt + 1;
    rpm::vector<double> pre(preflt + length);
    for (int i = 0; i < preflt; ++i)
        pre[i] = 2.0 * ((double) i / (double) (preflt - 1) - 0.5) * xData[0];
    for (int i = 0; i < length; ++i)
        pre[preflt + i] = xData[i];
    pre = sosfilter(hp, pre);
    const auto x = dropFront(pre, preflt);

    auto Hg1 = referenceLpc(x, window, lpW, 1);
    auto y1 = dropFront(filter(Hg1, one, pre), preflt);
    auto Hvt1 = referenceLpc(y1, window, lpW, p_vt);
    auto g1 = dropFront(filter(one, oneMinusD, filter(Hvt1, one, pre)), preflt);
    auto Hg2 = referenceLpc(g1, window, lpW, p_gl);
    auto y = dropFront(filter(one, oneMinusD, filter(Hg2, one, pre)), preflt);
    auto Hvt2 = referenceLpc(y, window, lpW, p_vt);
    return normalize(dropFront(filter(one, oneMinusD, filter(Hvt2, one, pre)), preflt));
}

rpm::vector<double> convolve(const rpm::vector<double> &x, const rpm::vector<double> &y)
{
    rpm::vector<double> w(x.size() + y.size() - 1, 0.0);
    for (size_t i = 0; i < x.size(); ++i)
        for (size_t j = 0; j < y.size(); ++j)
            w[i + j] += x[i] * y[j];
    return w;
}

rpm::vector<double> referenceGfmIaif(const std::vector<double> &xData, double rate, double d)
{
    const int length = (int) xData.size();
    const int ng = 3;
    const int nv = std::round(rate / 1000);
    const int Lpf = nv + 1;
    const int lpW = std::round(0.015 * rate);
    const rpm::vector<double> one({1.0});
    const rpm::vector<double> oneMinusD({1.0, -d});
    const auto window = hann(lpW);
    const auto hp = butterworthHighpass(10, 70.0, rate);

    rpm::vector<double> s_gvl(xData.begin(), xData.end());
    s_gvl = sosfilter(hp, s_gvl);
    rpm::vector<double> x_gvl(Lpf + length);
    for (int i = 0; i < Lpf; ++i)
        x_gvl[i] = 2.0 * ((double) i / (double) (Lpf - 1) - 0.5) * s_gvl[0];
    for (int i = 0; i < length; ++i)
        x_gvl[Lpf + i] = s_gvl[i];

    auto s_gv = filter(one, oneMinusD, s_gvl);
    auto x_gv = filter(one, oneMinusD, x_gvl);
    auto ag1 = referenceLpc(s_gv, window, lpW, 1);
    for (int i = 1; i < ng; ++i) {
        auto s_v1x = dropFront(filter(ag1, x_gv), Lpf);
        ag1 = convolve(ag1, referenceLpc(s_v1x, window, lpW, 1));
    }
    auto s_v1 = dropFront(filter(ag1, x_gv), Lpf);
    auto av1 = referenceLpc(s_v1, window, lpW, nv);
    auto s_g1 = dropFront(filter(av1, x_gv), Lpf);
    auto ag = referenceLpc(s_g1, window, lpW, ng);
    auto s_v = dropFront(filter(ag, x_gv), Lpf);
    auto av = referenceLpc(s_v, window, lpW, nv);
    return normalize(dropFront(filter(av, x_gv), Lpf));
}

template <class Solver>
rpm::vector<double> reference(const std::vector<double> &x, double rate);
template <>
rpm::vector<double> reference<Invglot::IAIF>(const std::vector<double> &x, double rate)
{
    return referenceIaif(x, rate, 0.99);
}
template <>
rpm::vector<double> reference<Invglot::GFM_IAIF>(const std::vector<double> &x, double rate)
{
    return referenceGfmIaif(x, rate, 0.99);
}

template <class Solver>
void expectSameAsReference(Solver &solver, double rate, int length)
{
    SCOPED_TRACE("rate " + std::to_string(rate) + ", length " + std::to_string(length));
    const auto x = voicedFrame(rate, length);
    const auto got = solver.solve(x.data(), length, rate).glotSig;
    const auto want = reference<Solver>(x, rate);
    ASSERT_EQ(got.size(), want.size());
    for (size_t i = 0; i < got.size(); ++i)
        ASSERT_NEAR(got[i], want[i], 1e-9) << "sample " << i;
}

struct Frame {
    double rate;
    int length;
};

template <class Solver>
void checkTwoRates()
{
    Solver solver(0.99);
    const Frame frames[] = {{16000.0, 1024}, {48000.0, 3072}, {16000.0, 1024}, {48000.0, 3072}};
    for (const Frame &f : frames) {
        SCOPED_TRACE("rate " + std::to_string(f.rate));
        const auto x = voicedFrame(f.rate, f.length);
        expectNormalized(solver.solve(x.data(), f.length, f.rate), f.length, f.rate);
    }
}

// A solver that saw other rates and lengths must give the output of a reference that holds no state.
template <class Solver>
void checkNoStateFromEarlierCalls()
{
    Solver solver(0.99);
    const Frame frames[] = {{16000.0, 1024}, {48000.0, 3072}, {44100.0, 2048}, {16000.0, 1024}, {48000.0, 3072}};
    for (const Frame &f : frames)
        expectSameAsReference(solver, f.rate, f.length);
}

// A change of the frame length at one rate must not crash and must give the output of the reference.
template <class Solver>
void checkFrameLengthChange()
{
    const double rate = 16000.0;
    Solver solver(0.99);
    for (int length : {1024, 512, 4096, 700, 1024}) {
        const auto x = voicedFrame(rate, length);
        expectNormalized(solver.solve(x.data(), length, rate), length, rate);
        expectSameAsReference(solver, rate, length);
    }
}

// Two solvers on two threads, with different rates, must give the output that each gives alone.
template <class Solver>
void checkTwoThreads()
{
    auto run = [](double rate, int length, std::vector<double> &out) {
        Solver solver(0.99);
        const auto x = voicedFrame(rate, length);
        for (int i = 0; i < 30; ++i)
            out = solver.solve(x.data(), length, rate).glotSig;
    };
    std::vector<double> outA, outB;
    std::thread ta(run, 16000.0, 1024, std::ref(outA));
    std::thread tb(run, 48000.0, 3072, std::ref(outB));
    ta.join();
    tb.join();
    const auto refA = reference<Solver>(voicedFrame(16000.0, 1024), 16000.0);
    const auto refB = reference<Solver>(voicedFrame(48000.0, 3072), 48000.0);
    ASSERT_EQ(outA.size(), refA.size());
    ASSERT_EQ(outB.size(), refB.size());
    for (size_t i = 0; i < outA.size(); ++i)
        ASSERT_NEAR(outA[i], refA[i], 1e-9) << "sample " << i;
    for (size_t i = 0; i < outB.size(); ++i)
        ASSERT_NEAR(outB[i], refB[i], 1e-9) << "sample " << i;
}

} // namespace

TEST(Invglot, TP_ANA_18_IaifGivesAFiniteNormalizedOutputAtTwoSampleRates)
{
    checkTwoRates<Invglot::IAIF>();
}

TEST(Invglot, TP_ANA_19_IaifKeepsNoFilterOrWindowFromEarlierSampleRates)
{
    checkNoStateFromEarlierCalls<Invglot::IAIF>();
}

TEST(Invglot, TP_ANA_20_IaifSurvivesAChangeOfTheFrameLength)
{
    checkFrameLengthChange<Invglot::IAIF>();
}

TEST(Invglot, TP_ANA_21_IaifSolversOnTwoThreadsDoNotShareBuffers)
{
    checkTwoThreads<Invglot::IAIF>();
}

TEST(Invglot, TP_ANA_22_GfmIaifGivesAFiniteNormalizedOutputAtTwoSampleRates)
{
    checkTwoRates<Invglot::GFM_IAIF>();
}

TEST(Invglot, TP_ANA_23_GfmIaifKeepsNoFilterOrWindowFromEarlierSampleRates)
{
    checkNoStateFromEarlierCalls<Invglot::GFM_IAIF>();
}

TEST(Invglot, TP_ANA_24_GfmIaifSurvivesAChangeOfTheFrameLength)
{
    checkFrameLengthChange<Invglot::GFM_IAIF>();
}

TEST(Invglot, TP_ANA_25_GfmIaifSolversOnTwoThreadsDoNotShareBuffers)
{
    checkTwoThreads<Invglot::GFM_IAIF>();
}

TEST(Invglot, TP_ANA_30_AFrameShorterThanTheLpcWindowGivesSilenceAndNoOverflow)
{
    // The LPC window is 15 ms, 240 samples at 16 kHz. A frame of 100 samples is shorter.
    const auto x = voicedFrame(16000, 100);
    Invglot::IAIF iaif(0.99);
    Invglot::GFM_IAIF gfm(0.99);
    for (InvglotSolver *solver : {static_cast<InvglotSolver *>(&iaif), static_cast<InvglotSolver *>(&gfm)}) {
        const InvglotResult r = solver->solve(x.data(), (int) x.size(), 16000);
        EXPECT_DOUBLE_EQ(r.sampleRate, 16000);
        ASSERT_EQ(r.glotSig.size(), x.size());
        for (double v : r.glotSig)
            EXPECT_EQ(v, 0.0);
    }
    // A normal frame after the short one still works.
    const auto y = voicedFrame(16000, 1024);
    expectNormalized(iaif.solve(y.data(), (int) y.size(), 16000), 1024, 16000);
    expectNormalized(gfm.solve(y.data(), (int) y.size(), 16000), 1024, 16000);
}
