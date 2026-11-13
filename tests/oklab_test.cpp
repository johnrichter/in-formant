#include <gtest/gtest.h>
#include "../src/gui/oklab.h"
#include "../src/gui/quietlift.h"
#include <array>

using namespace Gui;

// White and black have the right lightness and no chroma.
TEST(OkLab, WhiteAndBlackHaveTheRightLightnessAndNoChroma)
{
    const Lch white = labToLch(rgbToLab(0xffffff));
    EXPECT_NEAR(white.L, 1.0, 1e-3);
    EXPECT_TRUE(white.C < 1e-3);
    const Lch black = labToLch(rgbToLab(0x000000));
    EXPECT_NEAR(black.L, 0.0, 1e-6);
}

// A colour goes to OKLab and back to the same 8-bit value.
TEST(OkLab, AColourGoesToOKLabAndBackToTheSame8BitValue)
{
    const unsigned int colors[] = { 0x5bcefa, 0xf5a9b8, 0x440154, 0xfde725, 0x123456, 0xff0000, 0x00ff00, 0x0000ff, 0x808080 };
    for (unsigned int c : colors) {
        EXPECT_TRUE(labToRgb(rgbToLab(c)) == c);
    }
}

// A colour outside the gamut keeps its lightness and hue, and loses only chroma.
TEST(OkLab, AColourOutsideTheGamutKeepsItsLightnessAndHueAndLosesOnly)
{
    const Lch wanted { 0.75, 0.40, 150.0 };
    const unsigned int rgb = labToRgb(lchToLab(wanted));
    const Lch got = labToLch(rgbToLab(rgb));
    EXPECT_NEAR(got.L, wanted.L, 0.015);
    EXPECT_TRUE(got.C < wanted.C);
    EXPECT_NEAR(got.h, wanted.h, 4.0);
}

// A blue to pink gradient takes the way through violet, and it keeps its chroma.
TEST(OkLab, ABlueToPinkGradientTakesTheWayThroughVioletAndItKeepsIts)
{
    const Lch blue { 0.60, 0.12, 225.0 };
    const Lch pink { 0.74, 0.10, 5.0 };
    const Lch mid = mixLch(blue, pink, 0.5);
    EXPECT_TRUE(mid.h > 270.0 && mid.h < 340.0);
    EXPECT_TRUE(mid.C > 0.09);
    EXPECT_NEAR(mid.L, 0.67, 1e-9);
}

// A colour with no chroma takes the hue of the other end, so a gradient to grey keeps its hue.
TEST(OkLab, AColourWithNoChromaTakesTheHueOfTheOtherEndSoAGradientToGrey)
{
    const Lch grey { 0.5, 0.0, 0.0 };
    const Lch red { 0.6, 0.2, 30.0 };
    const Lch mid = mixLch(grey, red, 0.5);
    EXPECT_NEAR(mid.h, 30.0, 1e-9);
}

// The distance of equal colours is 0, and black to white is 1.
TEST(OkLab, TheDistanceOfEqualColoursIs0AndBlackToWhiteIs1)
{
    EXPECT_NEAR(labDistance(rgbToLab(0x336699), rgbToLab(0x336699)), 0.0, 1e-12);
    EXPECT_NEAR(labDistance(rgbToLab(0x000000), rgbToLab(0xffffff)), 1.0, 1e-2);
}

// The quiet end of a map that starts at black becomes a dark tint of the map's hue.
// The upper part of the map does not change, and the lightness still rises along the map.
TEST(OkLab, TP_QUI_01_QuietEndLift)
{
    std::array<unsigned int, 256> map;
    for (int i = 0; i < 256; ++i) {
        const double t = i / 255.0;
        // A map from black through purple to yellow.
        map[i] = 0xff000000u | labToRgb(lchToLab(0.9 * t, 0.15 * std::sin(t * 3.14159), 300.0 - 200.0 * t));
    }
    const auto original = map;
    liftQuietEnd(map);
    const Lch quiet = labToLch(rgbToLab(map[0] & 0xffffff));
    EXPECT_TRUE(quiet.L > 0.22);
    EXPECT_TRUE(quiet.L < 0.32);
    const Lch before = labToLch(rgbToLab(original[0] & 0xffffff));
    EXPECT_TRUE(before.L < 0.05);
    // The top of the map is the same.
    for (int i = 140; i < 256; ++i) EXPECT_TRUE(map[i] == original[i]);
    // The lightness never falls along the map.
    double previous = 0.0;
    for (int i = 0; i < 256; ++i) {
        const double L = labToLch(rgbToLab(map[i] & 0xffffff)).L;
        EXPECT_TRUE(L >= previous - 0.01);
        previous = L;
    }
}

// A map that has a light quiet end, like Viridis, does not change.
TEST(OkLab, AMapThatHasALightQuietEndLikeViridisDoesNotChange)
{
    std::array<unsigned int, 256> map;
    for (int i = 0; i < 256; ++i) map[i] = 0xff000000u | labToRgb(lchToLab(0.3 + 0.6 * i / 255.0, 0.12, 280.0));
    const auto original = map;
    liftQuietEnd(map);
    EXPECT_TRUE(map == original);
}

// A grey map stays grey, and it gets a lighter quiet end.
TEST(OkLab, AGreyMapStaysGreyAndItGetsALighterQuietEnd)
{
    std::array<unsigned int, 256> map;
    for (int i = 0; i < 256; ++i) {
        const unsigned v = (unsigned) std::lround(255.0 * i / 255.0);
        map[i] = 0xff000000u | (v << 16) | (v << 8) | v;
    }
    liftQuietEnd(map);
    EXPECT_TRUE(labToLch(rgbToLab(map[0] & 0xffffff)).C < 0.02);
    EXPECT_TRUE(labToLch(rgbToLab(map[0] & 0xffffff)).L > 0.2);
}
