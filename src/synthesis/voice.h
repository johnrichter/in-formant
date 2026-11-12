#ifndef SYNTHESIS_VOICE_H
#define SYNTHESIS_VOICE_H

#include "synthesis.h"
#include <array>
#include <random>

// The voice of the synthesizer: a glottal source and aspirate noise, both through the filter of the
// vocal tract. The class has no audio device and no thread, so a test can run it.

namespace Synthesis
{
    struct VoiceParams {
        double sampleRate = 16000.0;
        double pitch = 170.0;       // Hz
        double rd = 1.7;
        double tc = 1.0;
        // The random change of the pitch, Rd and Tc. 0 gives none. 1 gives the amount of the first version.
        double jitter = 0.0;
        double noiseGain = 0.0;
        double glotGain = 1.0;
        bool voiced = true;
        // The filter of the vocal tract, as second-order sections. An empty filter passes the sound.
        rpm::vector<std::array<double, 6>> filter;
    };

    // The factor that gives a filter a power gain of 1 for white noise. The level of the voice
    // follows the filter and not the sound, so it stays the same while the sound repeats.
    double filterNormalGain(const rpm::vector<std::array<double, 6>> &sos);

    class Voice {
    public:
        Voice();

        // Make length samples. The level does not depend on the sound that the call makes.
        rpm::vector<double> render(int length, const VoiceParams &params);

    private:
        NoiseState mNoise;
        std::mt19937_64 mJitter;

        // The pitch, Rd and Tc follow their targets with a short smoothing time.
        bool mStarted = false;
        double mPitch = 0.0;
        double mRd = 0.0;
        double mTc = 0.0;

        // The glottal pulse of the current period, its scale, and the position in the period (0 to 1).
        LF_State mPulse {};
        double mPulseScale = 1.0;
        double mPhase = 1.0;

        rpm::vector<rpm::vector<double>> mNoiseMemory;
        rpm::vector<rpm::vector<double>> mGlotMemory;
        rpm::vector<rpm::vector<double>> mLowpassMemory;
        rpm::vector<std::array<double, 6>> mLowpass;
        double mLowpassRate = 0.0;

        double mVoicedGain = 0.0;
        double mFilterGain = -1.0;
    };
}

#endif // SYNTHESIS_VOICE_H
