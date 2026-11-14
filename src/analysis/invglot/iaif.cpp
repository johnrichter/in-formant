#include "invglot.h"
#include "../filter/filter.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace Analysis::Invglot;
using Analysis::InvglotResult;

IAIF::IAIF(double d)
    : d(d)
{
    lpc = std::make_unique<LP::Burg>();
}

static rpm::vector<double> calculateLPC(const rpm::vector<double>& x, const rpm::vector<double>& w, int len, int order, std::unique_ptr<Analysis::LinpredSolver>& lpc, rpm::vector<double>& lpcIn)
{
    double gain;

    lpcIn.resize(len);
    for (int i = 0; i < len; ++i) {
        lpcIn[i] = w[i] * x[x.size() / 2 - len / 2 + i];
    }
    auto a = lpc->solve(lpcIn.data(), len, order, &gain);
    a.insert(a.begin(), 1.0);
    return a;
}

static rpm::vector<double> removePreRamp(const rpm::vector<double>& x, int preflt)
{
    return rpm::vector<double>(std::next(x.begin(), preflt), x.end());
}

InvglotResult IAIF::solve(const double *xData, int length, double sampleRate)
{
    const int p_gl = 2;
    const int p_vt = 2 * std::round(sampleRate / 2000) + 4;

    const int lpW = std::round(0.015 * sampleRate);

    // The LPC window takes 15 ms from the middle of the frame. A shorter frame has no flow estimate.
    if (lpW < 2 || length < lpW) {
        return {sampleRate, rpm::vector<double>(std::max(length, 0), 0.0)};
    }

    rpm::vector<double> one({1.0});
    rpm::vector<double> oneMinusD({1.0, -d});

    if ((int) window.size() != lpW) {
        window.resize(lpW);
        for (int i = 0; i < lpW; ++i) {
            window[i] = 0.5 - 0.5 * cos((2.0 * M_PI * i) / (double) (lpW - 1));
        }
    }

    rpm::vector<double> x(xData, xData + length);
   
    if (hpfilt.empty() || hpfiltRate != sampleRate) {
        hpfilt = Analysis::butterworthHighpass(8, 70.0, sampleRate);
        hpfiltRate = sampleRate;
    }

    int preflt = p_vt + 1;

    rpm::vector<double> xWithPreRamp(preflt + length);
    for (int i = 0; i < preflt; ++i) {
        xWithPreRamp[i] = 2.0 * ((double) i / (double) (preflt - 1) - 0.5) * x[0];
    }
    for (int i = 0; i < length; ++i) {
        xWithPreRamp[preflt + i] = x[i];
    }

    xWithPreRamp = sosfilter(hpfilt, xWithPreRamp);
    x = removePreRamp(xWithPreRamp, preflt);

    auto Hg1 = calculateLPC(x, window, lpW, 1, lpc, lpcIn);
    auto y1 = removePreRamp(filter(Hg1, one, xWithPreRamp), preflt);

    auto Hvt1 = calculateLPC(y1, window, lpW, p_vt, lpc, lpcIn);
    auto g1 = removePreRamp(filter(one, oneMinusD, filter(Hvt1, one, xWithPreRamp)), preflt);

    auto Hg2 = calculateLPC(g1, window, lpW, p_gl, lpc, lpcIn);
    auto y = removePreRamp(filter(one, oneMinusD, filter(Hg2, one, xWithPreRamp)), preflt);

    auto Hvt2 = calculateLPC(y, window, lpW, p_vt, lpc, lpcIn);
    auto g = removePreRamp(filter(one, oneMinusD, filter(Hvt2, one, xWithPreRamp)), preflt);

    double gMax = 1e-10;
    for (int i = 0; i < length; ++i) {
        double absG = fabs(g[i]);
        if (absG > gMax)
            gMax = absG;
    }
    for (int i = 0; i < length; ++i) {
        g[i] /= gMax;
    }

    return {sampleRate, g};
}
