#ifndef SYNTHESIS_H
#define SYNTHESIS_H

#include "rpcxx.h"
#include "../analysis/filter/filter.h"
#include "../analysis/formant/formant.h"
#include <complex>
#include <random>

namespace Synthesis
{
    // The state of a noise source: the random generator and the memory of the shaping filter. Each
    // thread that makes noise needs its own state, so that two threads never share a filter memory.
    struct NoiseState {
        NoiseState();
        std::mt19937_64 gen;
        rpm::vector<double> zf;
    };

    rpm::vector<double> whiteNoise(int length, NoiseState &state);
    rpm::vector<double> aspirateNoise(int length, NoiseState &state);

    struct LF_State {
        double T0;
        double Tc;
        double Rd;
        double te, tp, ta;
        double alpha, eps;
    };

    void lfRd2tetpta(LF_State& state);
    void lfEpsAlpha(LF_State& state);

    rpm::vector<double> lfGenFrame(double f0, double Fs, double Rd, double tc);

    // One pulse of the glottal source. Set it up for the values of one period, and then read its
    // value at any time inside the period.
    void lfSetup(LF_State& state, double f0, double Rd, double tc);
    double lfValue(const LF_State& state, double t);

    rpm::vector<double> createPolynomialFromRoots(const rpm::vector<std::complex<double>>& z);

    rpm::vector<std::array<double, 6>> frequencyShiftFilter(const rpm::vector<Analysis::FormantData>& formants, double Fs, double factor);

    rpm::vector<double> filter(
                    const rpm::vector<double>& b,
                    const rpm::vector<double>& a,
                    const rpm::vector<double>& x,
                    rpm::vector<double>& zf);

    rpm::vector<double> sosfilter(
                    const rpm::vector<std::array<double, 6>>& sos,
                    const rpm::vector<double>& x,
                    rpm::vector<rpm::vector<double>>& zf);
}

#endif // SYNTHESIS_H
