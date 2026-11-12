#include "voice.h"
#include "../analysis/analysis.h"
#include <algorithm>
#include <cmath>

using namespace Synthesis;

namespace {

// The level of each source before the master gain. Both sources go through the filter with unit power gain.
constexpr double glotLevel = 0.25;
constexpr double noiseLevel = 0.055;

// The relative random change of the pitch, and the absolute random change of Rd and Tc, for jitter 1.
constexpr double jitterSize = 0.05;

// The pitch and the shape follow their targets with this factor for each sample.
constexpr double smoothing = 0.997;

// The effective value of the aspirate noise before the filter. The value is a property of the
// noise filter, so it is the same for all voices. The first voice measures it.
double aspirateRms()
{
    static const double rms = [] {
        NoiseState state;
        const auto noise = aspirateNoise(1 << 16, state);
        double sum = 0.0;
        // The first samples hold the start of the filter, so skip them.
        for (size_t i = 256; i < noise.size(); ++i) sum += noise[i] * noise[i];
        return std::sqrt(sum / (double) (noise.size() - 256));
    }();
    return rms;
}

// The effective value of one period of a glottal pulse.
double pulseRms(const LF_State &pulse)
{
    constexpr int points = 64;
    double sum = 0.0;
    for (int i = 0; i < points; ++i) {
        const double v = lfValue(pulse, pulse.T0 * (double) i / points);
        sum += v * v;
    }
    return std::sqrt(sum / points);
}

}

double Synthesis::filterNormalGain(const rpm::vector<std::array<double, 6>> &sos)
{
    if (sos.empty()) return 1.0;

    constexpr int points = 128;
    rpm::vector<std::complex<double>> wn(points);
    for (int k = 0; k < points; ++k) {
        wn[k] = std::polar(1.0, M_PI * (k + 0.5) / points);
    }

    const auto magnitude = Analysis::sosfreqz(sos, wn);
    double power = 0.0;
    for (double m : magnitude) power += m * m;
    power /= (double) points;

    return power > 1e-30 ? 1.0 / std::sqrt(power) : 1.0;
}

Voice::Voice()
    : mJitter(std::random_device{}()),
      mNoiseMemory(20, rpm::vector<double>(4, 0.0)),
      mGlotMemory(20, rpm::vector<double>(4, 0.0)),
      mLowpassMemory(20, rpm::vector<double>(4, 0.0))
{
}

rpm::vector<double> Voice::render(int length, const VoiceParams &p)
{
    rpm::vector<double> out((size_t) std::max(length, 0), 0.0);
    if (length <= 0) return out;

    if (!mStarted) {
        // The voice starts at the target values, so it does not glide from an old value.
        mStarted = true;
        mPitch = p.pitch;
        mRd = p.rd;
        mTc = p.tc;
    }

    const double fs = p.sampleRate;
    std::normal_distribution<> dis(0.0, jitterSize * p.jitter);

    // The values of each sample.
    rpm::vector<double> pitches((size_t) length), rds((size_t) length), tcs((size_t) length);
    for (int i = 0; i < length; ++i) {
        const bool jittered = p.jitter > 0.0;
        mPitch = smoothing * mPitch + (1 - smoothing) * (p.pitch * (1 + (jittered ? dis(mJitter) : 0.0)));
        mRd = smoothing * mRd + (1 - smoothing) * (p.rd + (jittered ? dis(mJitter) : 0.0));
        mTc = smoothing * mTc + (1 - smoothing) * (p.tc + (jittered ? dis(mJitter) : 0.0));
        pitches[(size_t) i] = mPitch;
        rds[(size_t) i] = mRd;
        tcs[(size_t) i] = mTc;
    }

    // The aspirate noise.
    rpm::vector<double> noise;
    if (p.noiseGain > 1e-7) {
        noise = aspirateNoise(length, mNoise);
        for (double &v : noise) v /= aspirateRms();
        noise = sosfilter(p.filter, noise, mNoiseMemory);
    }

    // The glottal source. The phase moves by the pitch for each sample, so the mean pitch is exact.
    // A new period takes the values of its first sample.
    rpm::vector<double> glot((size_t) length, 0.0);
    for (int i = 0; i < length; ++i) {
        if (mPhase >= 1.0) {
            mPhase -= std::floor(mPhase);
            lfSetup(mPulse, pitches[(size_t) i], rds[(size_t) i], tcs[(size_t) i]);
            const double rms = pulseRms(mPulse);
            mPulseScale = rms > 1e-12 ? 1.0 / rms : 1.0;
        }
        glot[(size_t) i] = mPulseScale * lfValue(mPulse, mPhase * mPulse.T0);
        mPhase += pitches[(size_t) i] / fs;
    }

    // Remove the part of the source above the audio band.
    if (mLowpass.empty() || mLowpassRate != fs) {
        mLowpassRate = fs;
        mLowpass = Analysis::butterworthLowpass(10, fs * 0.49, fs);
    }
    glot = sosfilter(mLowpass, glot, mLowpassMemory);
    glot = sosfilter(p.filter, glot, mGlotMemory);

    // The gain moves in a line from the old value to the new value over the block.
    const double gain = filterNormalGain(p.filter);
    if (mFilterGain < 0.0) mFilterGain = gain;
    const double voicedTarget = p.voiced ? 1.0 : 0.0;

    for (int i = 0; i < length; ++i) {
        const double t = (double) (i + 1) / (double) length;
        const double g = mFilterGain + (gain - mFilterGain) * t;
        mVoicedGain += 0.003 * (voicedTarget - mVoicedGain);

        double v = mVoicedGain * glotLevel * p.glotGain * glot[(size_t) i];
        if (!noise.empty()) v += noiseLevel * p.noiseGain * noise[(size_t) i];
        out[(size_t) i] = g * v;
    }
    mFilterGain = gain;

    return out;
}
