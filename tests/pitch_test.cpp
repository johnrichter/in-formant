#include <gtest/gtest.h>
#include "analysis_signals.h"
#include "../src/analysis/pitch/pitch.h"
#include <cmath>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

using namespace Analysis;
using namespace AnalysisSignals;

namespace {

struct Rate {
    int sampleRate;
    int length; // The frame length. About 128 ms.
};

const Rate rates[] = {{16000, 2048}, {48000, 6144}};
const double pitches[] = {100.0, 220.0, 440.0};

// The pitch must be within 1 percent.
void expectPitch(PitchSolver &solver, const std::vector<double> &x, int sampleRate, double expected)
{
    const PitchResult r = solver.solve(x.data(), (int) x.size(), sampleRate);
    ASSERT_TRUE(r.voiced);
    EXPECT_NEAR(r.pitch, expected, expected * 0.01);
}

// Run a solver on the sines and on the harmonic sums of the test plan.
void checkTones(PitchSolver &solver)
{
    for (const Rate &rate : rates) {
        for (double f : pitches) {
            SCOPED_TRACE("pitch " + std::to_string(f) + " Hz, rate " + std::to_string(rate.sampleRate));
            {
                SCOPED_TRACE("sine");
                expectPitch(solver, sine(f, rate.sampleRate, rate.length), rate.sampleRate, f);
            }
            {
                SCOPED_TRACE("harmonic sum");
                expectPitch(solver, harmonics(f, rate.sampleRate, rate.length, 6), rate.sampleRate, f);
            }
        }
    }
}

void checkUnvoiced(PitchSolver &solver)
{
    for (const Rate &rate : rates) {
        SCOPED_TRACE("rate " + std::to_string(rate.sampleRate));
        const auto quiet = silence(rate.length);
        EXPECT_FALSE(solver.solve(quiet.data(), rate.length, rate.sampleRate).voiced);
        for (unsigned seed : {1u, 2u, 3u, 4u, 5u}) {
            const auto noise = whiteNoise(rate.length, 0.3, seed);
            const PitchResult r = solver.solve(noise.data(), rate.length, rate.sampleRate);
            EXPECT_FALSE(r.voiced) << "seed " << seed << " gave " << r.pitch << " Hz";
        }
    }
}

} // namespace

TEST(Pitch, TP_ANA_07_YinFindsSinesAndHarmonicSumsWithin1Percent)
{
    Pitch::Yin yin(0.15);
    checkTones(yin);
}

TEST(Pitch, TP_ANA_08_YinFollowsASignalWhoseLevelRises)
{
    // The difference function uses a fixed window, so a rise of the level over the frame does not move the minimum.
    Pitch::Yin yin(0.15);
    for (const Rate &rate : rates) {
        for (double f : pitches) {
            SCOPED_TRACE("pitch " + std::to_string(f) + " Hz, rate " + std::to_string(rate.sampleRate));
            auto x = harmonics(f, rate.sampleRate, rate.length, 3);
            for (int i = 0; i < rate.length; ++i)
                x[i] *= 0.3 + 0.7 * i / (double) rate.length;
            expectPitch(yin, x, rate.sampleRate, f);
        }
    }
}

TEST(Pitch, TP_ANA_09_YinGivesNoVoicedResultForSilenceAndNoise)
{
    Pitch::Yin yin(0.15);
    checkUnvoiced(yin);
}

TEST(Pitch, TP_ANA_10_YinInterpolatesALagBetweenTwoSamples)
{
    // At 16 kHz, 440 Hz has a period of 36.36 samples. A whole lag of 36 gives 444.4 Hz, which is 1.01 percent high.
    // The parabolic interpolation must bring the result closer than that.
    Pitch::Yin yin(0.15);
    const auto x = sine(440.0, 16000, 2048);
    const PitchResult r = yin.solve(x.data(), (int) x.size(), 16000);
    ASSERT_TRUE(r.voiced);
    EXPECT_LT(std::fabs(r.pitch - 440.0), 2.0);
}

TEST(Pitch, TP_ANA_11_YinKeepsNoStateBetweenFrameLengths)
{
    Pitch::Yin reused(0.15);
    const auto a = harmonics(220.0, 16000, 2048, 4);
    const auto b = harmonics(220.0, 16000, 1536, 4);
    reused.solve(a.data(), (int) a.size(), 16000);
    const PitchResult r1 = reused.solve(b.data(), (int) b.size(), 16000);
    Pitch::Yin fresh(0.15);
    const PitchResult r2 = fresh.solve(b.data(), (int) b.size(), 16000);
    EXPECT_EQ(r1.voiced, r2.voiced);
    EXPECT_DOUBLE_EQ(r1.pitch, r2.pitch);
}

TEST(Pitch, TP_ANA_12_MpmFindsSinesAndHarmonicSumsWithin1Percent)
{
    Pitch::MPM mpm;
    checkTones(mpm);
}

TEST(Pitch, TP_ANA_13_MpmGivesNoVoicedResultForSilenceAndNoise)
{
    Pitch::MPM mpm;
    checkUnvoiced(mpm);
}

TEST(Pitch, TP_ANA_14_TwoMpmSolversWithDifferentFrameLengthsAnswerInTurn)
{
    Pitch::MPM first;
    Pitch::MPM second;
    const auto a = harmonics(220.0, 16000, 2048, 4);
    const auto b = harmonics(100.0, 48000, 6144, 4);
    const auto c = harmonics(440.0, 16000, 1024, 4);
    for (int round = 0; round < 3; ++round) {
        SCOPED_TRACE("round " + std::to_string(round));
        expectPitch(first, a, 16000, 220.0);
        expectPitch(second, b, 48000, 100.0);
        expectPitch(first, c, 16000, 440.0);
        expectPitch(second, a, 16000, 220.0);
    }
}

TEST(Pitch, TP_ANA_15_MpmSolversOnTwoThreadsDoNotShareAnFft)
{
    // Each thread owns its solver, and the two frame lengths differ. A shared FFT object breaks this test.
    auto run = [](double pitch, int sampleRate, int length, int &failures) {
        Pitch::MPM mpm;
        const auto x = harmonics(pitch, sampleRate, length, 4);
        for (int i = 0; i < 2000; ++i) {
            const PitchResult r = mpm.solve(x.data(), length, sampleRate);
            if (!r.voiced || std::fabs(r.pitch - pitch) > pitch * 0.01)
                ++failures;
        }
    };
    int failuresA = 0, failuresB = 0;
    std::thread ta(run, 220.0, 16000, 2048, std::ref(failuresA));
    std::thread tb(run, 100.0, 48000, 6144, std::ref(failuresB));
    ta.join();
    tb.join();
    EXPECT_EQ(failuresA, 0);
    EXPECT_EQ(failuresB, 0);
}

TEST(Pitch, TP_ANA_16_RaptComputesTheLpcOrderAgainWhenTheSampleRateChanges)
{
    Pitch::RAPT rapt;
    const auto a = harmonics(150.0, 16000, 1024, 4);
    const auto b = harmonics(150.0, 48000, 3072, 4);
    rapt.solve(a.data(), (int) a.size(), 16000);
    EXPECT_EQ(rapt.lpcOrder, 2 + 16);
    rapt.solve(b.data(), (int) b.size(), 48000);
    EXPECT_EQ(rapt.lpcOrder, 2 + 48);
    rapt.solve(a.data(), (int) a.size(), 16000);
    EXPECT_EQ(rapt.lpcOrder, 2 + 16);
}

TEST(Pitch, TP_ANA_17_RaptSolversOnTwoThreadsDoNotShareTheirPathBuffers)
{
    // The program uses one frame in the path (nbFrames is 1). The frames differ in their candidates.
    auto run = [](double pitch, int sampleRate, int length, std::vector<double> &path) {
        Pitch::RAPT rapt;
        const auto x = harmonics(pitch, sampleRate, length, 4);
        for (int round = 0; round < 200; ++round) {
            rapt.computeFrame(x.data(), length, sampleRate);
            path = rapt.computePath();
        }
    };
    std::vector<double> refA, refB, pathA, pathB;
    run(150.0, 16000, 1024, refA);
    run(300.0, 48000, 3072, refB);
    ASSERT_EQ(refA.size(), 1u);
    ASSERT_EQ(refB.size(), 1u);
    EXPECT_NEAR(refA[0], 150.0, 1.5);
    EXPECT_NEAR(refB[0], 300.0, 3.0);
    std::thread ta(run, 150.0, 16000, 1024, std::ref(pathA));
    std::thread tb(run, 300.0, 48000, 3072, std::ref(pathB));
    ta.join();
    tb.join();
    EXPECT_EQ(pathA, refA);
    EXPECT_EQ(pathB, refB);
}

TEST(Pitch, TP_ANA_31_RaptWithAPathOfFiveFramesFollowsASteadyTone)
{
    // The program uses one frame. A path of more frames reads the spectrum of the frame before, and the dynamic
    // programming adds the cost of each transition to the cost of the path so far.
    Pitch::RAPT rapt;
    rapt.nbFrames = 5;
    const auto x = harmonics(150.0, 16000, 1024, 4);
    std::vector<double> path;
    for (int round = 0; round < 10; ++round) {
        rapt.computeFrame(x.data(), (int) x.size(), 16000);
        path = rapt.computePath();
    }
    ASSERT_EQ(path.size(), 5u);
    for (double p : path) {
        ASSERT_TRUE(std::isfinite(p));
        EXPECT_NEAR(p, 150.0, 3.0);
    }
}
