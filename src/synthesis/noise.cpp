#include "synthesis.h"

Synthesis::NoiseState::NoiseState()
    : gen(std::random_device{}())
{
}

rpm::vector<double> Synthesis::whiteNoise(int length, NoiseState &state)
{
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    rpm::vector<double> out(length);
    for (int i = 0; i < length; ++i) {
        out[i] = dis(state.gen);
    }
    return out;
}

rpm::vector<double> Synthesis::aspirateNoise(int length, NoiseState &state)
{
    constexpr double alpha = -1.5;

    // The coefficients are the same for all users, and nobody changes them after the first call.
    static const rpm::vector<double> filter = [] {
        rpm::vector<double> f(64);
        f[0] = 1.0;
        for (int k = 1; k <= 63; ++k) {
            f[k] = (k - 1.0 - alpha / 2.0) * f[k - 1] / (double) k;
        }
        return f;
    }();

    return Synthesis::filter(filter, {1.0}, whiteNoise(length, state), state.zf);
}
