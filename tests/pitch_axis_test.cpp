#include "check.h"
#include "../src/gui/pitchaxis.h"
#include <cstdint>
#include <cstring>

using namespace Gui;

// A NaN from its bits. The build uses fast math, so the NAN macro is not reliable.
static double makeNaN()
{
    // A volatile read stops the compiler from folding the value at build time.
    volatile std::uint64_t volatileBits = 0x7ff8000000000000ULL;
    const std::uint64_t bits = volatileBits;
    double v;
    std::memcpy(&v, &bits, sizeof v);
    return v;
}

int main()
{
    // Too few values give the fallback.
    {
        const PitchAxis a = fitPitchAxis({ 100.0, 110.0 }, { 70.0, 250.0 });
        CHECK(a.low == 70.0 && a.high == 250.0);
    }

    // The range has a margin and covers the values.
    {
        std::vector<double> v;
        for (int i = 0; i < 100; ++i) v.push_back(100.0 + i);
        const PitchAxis a = fitPitchAxis(v);
        CHECK(a.low < 105.0);
        CHECK(a.high > 194.0);
        CHECK(semitonesBetween(a.low, a.high) > 12.0);
    }

    // A narrow range widens to 12 semitones at least.
    {
        std::vector<double> v(50, 200.0);
        const PitchAxis a = fitPitchAxis(v);
        CHECK(semitonesBetween(a.low, a.high) >= 12.0 - 1e-6);
        CHECK(a.low < 200.0 && a.high > 200.0);
    }

    // A few octave errors do not stretch the axis.
    {
        std::vector<double> v;
        for (int i = 0; i < 100; ++i) v.push_back(150.0 + 0.5 * i);
        v.push_back(60.0);
        v.push_back(900.0);
        const PitchAxis a = fitPitchAxis(v);
        CHECK(a.low > 100.0);
        CHECK(a.high < 300.0);
    }

    // Invalid values go away.
    {
        std::vector<double> v(20, 180.0);
        v.push_back(-5.0);
        v.push_back(0.0);
        v.push_back(makeNaN());
        const PitchAxis a = fitPitchAxis(v);
        CHECK(a.low > 20.0 && a.high < 2000.0);
    }

    // A wider target takes effect at once. A narrower target comes slowly.
    {
        const PitchAxis shown { 100.0, 200.0 };
        const PitchAxis wide = easePitchAxis(shown, { 80.0, 300.0 });
        CHECK(wide.low == 80.0 && wide.high == 300.0);
        const PitchAxis narrow = easePitchAxis(shown, { 120.0, 160.0 });
        CHECK(narrow.low > 100.0 && narrow.low < 120.0);
        CHECK(narrow.high < 200.0 && narrow.high > 160.0);
    }

    // The fixed range keeps the edges that make sense, and a wrong value gives 0 to 500 Hz.
    {
        const PitchAxis a = fixedPitchAxis(0.0, 600.0);
        CHECK(a.low == 0.0 && a.high == 600.0);
        const PitchAxis b = fixedPitchAxis(-50.0, 400.0);
        CHECK(b.low == 0.0 && b.high == 400.0);
        const PitchAxis c = fixedPitchAxis(300.0, 310.0);
        CHECK(c.low == 300.0 && c.high == 350.0);
        const PitchAxis d = fixedPitchAxis(100.0, 9000.0);
        CHECK(d.high == 2000.0);
        const PitchAxis e = fixedPitchAxis(makeNaN(), 600.0);
        CHECK(e.low == 0.0 && e.high == 500.0);
    }

    return TEST_RESULT();
}
