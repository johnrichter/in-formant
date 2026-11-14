#include "pitch.h"
#include "../util/util.h"
#include <algorithm>

using Analysis::PitchResult;
using namespace Analysis::Pitch;

inline int
pow2roundup (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

Yin::Yin(double threshold)
    : mThreshold(threshold),
      mFFT(nullptr)
{
}

PitchResult Yin::solve(const double *data, int length, int sampleRate) 
{
    // The difference function d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2 uses a fixed window of W
    // samples, so the lags 0 to W-1 need the samples 0 to 2W-2.
    const int W = length / 2;
    if (W < 4) {
        return {0.0, false};
    }

    // The two energy terms come from a cumulative sum of squares.
    // e(tau) is the energy of the window that starts at tau.
    mCumulative.resize(length + 1);
    mCumulative[0] = 0.0;
    for (int i = 0; i < length; ++i) {
        mCumulative[i + 1] = mCumulative[i] + data[i] * data[i];
    }
    const double energyZero = mCumulative[W];
    if (energyZero <= 0.0) {
        return {0.0, false};
    }

    // The cross term r_W(tau) = sum_{j=0}^{W-1} x[j] x[j+tau] is a cross-correlation of the first
    // window with the whole frame. The FFT is long enough that no lag in 0..W-1 wraps around.
    int nfft = pow2roundup(length);

    if (!mFFT || mFFT->getLength() != nfft) {
        mFFT = std::make_shared<ComplexFFT>(nfft);
    }

    for (int i = 0; i < nfft; ++i) {
        mFFT->data(i) = (i < W) ? data[i] : 0.0;
    }
    mFFT->computeForward();
    mWindowSpectrum.resize(nfft);
    for (int i = 0; i < nfft; ++i) {
        mWindowSpectrum[i] = mFFT->data(i);
    }

    for (int i = 0; i < nfft; ++i) {
        mFFT->data(i) = (i < length) ? data[i] : 0.0;
    }
    mFFT->computeForward();
    for (int i = 0; i < nfft; ++i) {
        mFFT->data(i) = (conj(mWindowSpectrum[i]) * mFFT->data(i)) / (double) nfft;
    }
    mFFT->computeBackward();

    mDifference.resize(W);
    mDifference[0] = 0.0;
    for (int tau = 1; tau < W; ++tau) {
        const double energyTau = mCumulative[tau + W] - mCumulative[tau];
        const double cross = mFFT->data(tau).real();
        // Rounding can give a small negative value.
        mDifference[tau] = std::max(0.0, energyZero + energyTau - 2.0 * cross);
    }

    mCMND.resize(W);
    double runningSum = 0.0;
    mCMND[0] = 1.0;
    for (int tau = 1; tau < W; ++tau) {
        runningSum += mDifference[tau];
        mCMND[tau] = (runningSum > 0.0) ? (tau * mDifference[tau]) / runningSum : 1.0;
    }

    int k;
    for (k = 2; k < W; ++k) {
        if (mCMND[k] < mThreshold) {
            while (k + 1 < W && mCMND[k + 1] < mCMND[k])
                k++;
            break;
        }
    }

    if (k >= W || mCMND[k] >= mThreshold) {
        return {0.0, false};
    }

    // Parabolic interpolation of the chosen lag. It needs a neighbor on each side.
    double lag = k;
    if (k + 1 < W) {
        lag = Analysis::parabolicInterpolation(mCMND, k).first;
    }

    double pitch = (double) sampleRate / lag;
    return {pitch, true};
}
