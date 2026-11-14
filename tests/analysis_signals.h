#ifndef TESTS_ANALYSIS_SIGNALS_H
#define TESTS_ANALYSIS_SIGNALS_H

// Synthetic signals for the tests of the analysis code. They are plain functions with no state.

#include <cmath>
#include <random>
#include <vector>

namespace AnalysisSignals {

constexpr double pi = 3.14159265358979323846;

// A sum of harmonics. The amplitude of harmonic h is 1/h. The peak of the sum is about 0.5 times the amplitude.
inline std::vector<double> harmonics(double pitch, double sampleRate, int length, int count, double amplitude = 0.5,
                                     double startPhase = 0.3)
{
    std::vector<double> x(length, 0.0);
    double norm = 0.0;
    for (int h = 1; h <= count; ++h)
        norm += 1.0 / h;
    for (int i = 0; i < length; ++i) {
        const double t = i / sampleRate;
        double v = 0.0;
        for (int h = 1; h <= count; ++h)
            v += std::sin(2.0 * pi * pitch * h * t + startPhase * h) / h;
        x[i] = amplitude * v / norm;
    }
    return x;
}

inline std::vector<double> sine(double pitch, double sampleRate, int length, double amplitude = 0.5)
{
    return harmonics(pitch, sampleRate, length, 1, amplitude);
}

// White noise from a fixed seed, so a run always gives the same samples.
inline std::vector<double> whiteNoise(int length, double amplitude = 0.3, unsigned seed = 12345)
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, amplitude);
    std::vector<double> x(length);
    for (auto &v : x)
        v = dist(gen);
    return x;
}

inline std::vector<double> silence(int length)
{
    return std::vector<double>(length, 0.0);
}

} // namespace AnalysisSignals

#endif // TESTS_ANALYSIS_SIGNALS_H
