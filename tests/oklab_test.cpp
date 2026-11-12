#include "check.h"
#include "../src/gui/oklab.h"

using namespace Gui;

int main()
{
    // White and black have the right lightness and no chroma.
    {
        const Lch white = labToLch(rgbToLab(0xffffff));
        CHECK_NEAR(white.L, 1.0, 1e-3);
        CHECK(white.C < 1e-3);
        const Lch black = labToLch(rgbToLab(0x000000));
        CHECK_NEAR(black.L, 0.0, 1e-6);
    }

    // A colour goes to OKLab and back to the same 8-bit value.
    {
        const unsigned int colors[] = { 0x5bcefa, 0xf5a9b8, 0x440154, 0xfde725, 0x123456, 0xff0000, 0x00ff00, 0x0000ff, 0x808080 };
        for (unsigned int c : colors) {
            CHECK(labToRgb(rgbToLab(c)) == c);
        }
    }

    // A colour outside the gamut keeps its lightness and hue, and loses only chroma.
    {
        const Lch wanted { 0.75, 0.40, 150.0 };
        const unsigned int rgb = labToRgb(lchToLab(wanted));
        const Lch got = labToLch(rgbToLab(rgb));
        CHECK_NEAR(got.L, wanted.L, 0.015);
        CHECK(got.C < wanted.C);
        CHECK_NEAR(got.h, wanted.h, 4.0);
    }

    // A blue to pink gradient takes the way through violet, and it keeps its chroma.
    {
        const Lch blue { 0.60, 0.12, 225.0 };
        const Lch pink { 0.74, 0.10, 5.0 };
        const Lch mid = mixLch(blue, pink, 0.5);
        CHECK(mid.h > 270.0 && mid.h < 340.0);
        CHECK(mid.C > 0.09);
        CHECK_NEAR(mid.L, 0.67, 1e-9);
    }

    // A colour with no chroma takes the hue of the other end, so a gradient to grey keeps its hue.
    {
        const Lch grey { 0.5, 0.0, 0.0 };
        const Lch red { 0.6, 0.2, 30.0 };
        const Lch mid = mixLch(grey, red, 0.5);
        CHECK_NEAR(mid.h, 30.0, 1e-9);
    }

    // The distance of equal colours is 0, and black to white is 1.
    {
        CHECK_NEAR(labDistance(rgbToLab(0x336699), rgbToLab(0x336699)), 0.0, 1e-12);
        CHECK_NEAR(labDistance(rgbToLab(0x000000), rgbToLab(0xffffff)), 1.0, 1e-2);
    }

    return TEST_RESULT();
}
