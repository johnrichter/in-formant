#include "check.h"
#include "../src/synthesis/voice.h"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace Synthesis;

namespace {

constexpr double sampleRate = 16000.0;
constexpr int seconds = 8;

// Render the voice in blocks of 1024 samples, as the audio callback does.
std::vector<double> renderSeconds(Voice &voice, const VoiceParams &params)
{
    std::vector<double> out;
    const int total = (int) sampleRate * seconds;
    while ((int) out.size() < total) {
        const auto block = voice.render(1024, params);
        out.insert(out.end(), block.begin(), block.end());
    }
    out.resize(total);
    return out;
}

// The RMS of each second, without the first second.
std::vector<double> secondLevels(const std::vector<double> &x)
{
    std::vector<double> levels;
    const int n = (int) sampleRate;
    for (int s = 1; s < seconds; ++s) {
        double sum = 0.0;
        for (int i = s * n; i < (s + 1) * n; ++i) sum += x[i] * x[i];
        levels.push_back(std::sqrt(sum / n));
    }
    return levels;
}

// The times of the sharp negative peak of each glottal pulse, in seconds.
std::vector<double> pulseTimes(const std::vector<double> &x)
{
    const int start = (int) (0.5 * sampleRate);
    double lowest = 0.0;
    for (int i = start; i < (int) x.size(); ++i) lowest = std::min(lowest, x[i]);

    std::vector<double> times;
    const double threshold = 0.5 * lowest;
    int i = start;
    while (i < (int) x.size()) {
        if (x[i] < threshold) {
            int best = i;
            while (i < (int) x.size() && x[i] < threshold) {
                if (x[i] < x[best]) best = i;
                ++i;
            }
            times.push_back(best / sampleRate);
        }
        else {
            ++i;
        }
    }
    return times;
}

}

int main()
{
    // The level of a voice with no filter stays the same from one second to the next.
    {
        Voice voice;
        VoiceParams params;
        params.sampleRate = sampleRate;
        params.pitch = 150.0;

        const auto x = renderSeconds(voice, params);
        const auto levels = secondLevels(x);
        const double low = *std::min_element(levels.begin(), levels.end());
        const double high = *std::max_element(levels.begin(), levels.end());
        CHECK(low > 0.01);
        CHECK((high - low) / high < 0.01);

        // The mean pitch has an error of less than 1 cent when the jitter is 0.
        const auto times = pulseTimes(x);
        CHECK(times.size() > 100);
        if (times.size() > 100) {
            const double pitch = (times.size() - 1) / (times.back() - times.front());
            const double cents = 1200.0 * std::log2(pitch / params.pitch);
            CHECK(std::fabs(cents) < 1.0);
        }
    }

    // The same holds for another pitch.
    {
        Voice voice;
        VoiceParams params;
        params.sampleRate = sampleRate;
        params.pitch = 233.3;

        const auto times = pulseTimes(renderSeconds(voice, params));
        CHECK(times.size() > 100);
        if (times.size() > 100) {
            const double pitch = (times.size() - 1) / (times.back() - times.front());
            CHECK(std::fabs(1200.0 * std::log2(pitch / params.pitch)) < 1.0);
        }
    }

    // The level with a formant filter and aspirate noise stays the same from one second to the next.
    {
        Voice voice;
        VoiceParams params;
        params.sampleRate = sampleRate;
        params.pitch = 170.0;
        params.noiseGain = 0.3;
        params.filter = frequencyShiftFilter({ {500, 60}, {1500, 90}, {2500, 120}, {3500, 150} }, sampleRate, 1.0);

        const auto levels = secondLevels(renderSeconds(voice, params));
        const double low = *std::min_element(levels.begin(), levels.end());
        const double high = *std::max_element(levels.begin(), levels.end());
        CHECK(low > 0.01);
        CHECK((high - low) / high < 0.02);
    }

    // The jitter gives a sound with no invalid sample.
    {
        Voice voice;
        VoiceParams params;
        params.sampleRate = sampleRate;
        params.jitter = 1.0;
        for (double v : renderSeconds(voice, params)) CHECK(std::isfinite(v));
    }

    return TEST_RESULT();
}
