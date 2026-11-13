#include <gtest/gtest.h>

#include <QElapsedTimer>
#include <algorithm>
#include <cmath>

#include "../src/gui/gridcolor.h"
#include "../src/gui/oklab.h"
#include "../src/gui/quietlift.h"
#include "../src/gui/theme.h"

using namespace Gui;

namespace {

double luminance(unsigned rgb)
{
    auto channel = [](int v) { const double c = v / 255.0; return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel((rgb >> 16) & 0xff) + 0.7152 * channel((rgb >> 8) & 0xff) + 0.0722 * channel(rgb & 0xff);
}

// The contrast ratio of two colors as WCAG defines it.
double contrast(unsigned a, unsigned b)
{
    const double x = luminance(a), y = luminance(b);
    return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
}

unsigned rgbOf(QRgb c) { return c & 0xffffff; }

// The color of a line after it is drawn over a background.
unsigned blend(const QColor &line, unsigned background)
{
    const double a = line.alphaF();
    auto mix = [&](int l, int b) { return (unsigned) std::lround(l * a + b * (1.0 - a)); };
    return (mix(line.red(), (background >> 16) & 0xff) << 16) | (mix(line.green(), (background >> 8) & 0xff) << 8)
         | mix(line.blue(), background & 0xff);
}

std::vector<QString> prideThemes()
{
    std::vector<QString> names;
    for (const QString &name : themeNames()) {
        if (themeGroup(name) == "Pride") names.push_back(name);
    }
    return names;
}

double meanChroma(const QVector<QRgb> &map)
{
    double sum = 0.0;
    for (QRgb c : map) sum += labToLch(rgbToLab(rgbOf(c))).C;
    return sum / map.size();
}

}

// The 16 pride themes have a lightness that rises with the volume, and no stop is white.
TEST(Colors, TP_COL_01_ThePrideThemesRiseInLightnessAndHaveNoWhiteStop)
{
    const auto names = prideThemes();
    ASSERT_EQ(names.size(), 16u);
    for (const QString &name : names) {
        const Theme t = resolveThemeBlack(name, "", "", "", 1.0);
        ASSERT_EQ(t.colormap.size(), 256);

        double previous = -1.0;
        double top = 0.0;
        for (int i = 0; i < 256; ++i) {
            const double lightness = rgbToLab(rgbOf(t.colormap[i])).L;
            // A step down of 0.005 is the rounding of the 8-bit colors.
            EXPECT_GE(lightness, previous - 0.005) << name.toStdString() << " entry " << i;
            previous = std::max(previous, lightness);
            top = std::max(top, lightness);

            const unsigned rgb = rgbOf(t.colormap[i]);
            const int smallest = std::min({ (int) (rgb >> 16) & 0xff, (int) (rgb >> 8) & 0xff, (int) rgb & 0xff });
            EXPECT_LT(smallest, 245) << name.toStdString() << " entry " << i << " is near white";
        }
        EXPECT_LE(top, 0.90) << name.toStdString();
        // The quiet end is dark, and the loud end is much lighter.
        EXPECT_LT(rgbToLab(rgbOf(t.colormap[0])).L, 0.12) << name.toStdString();
        EXPECT_GT(rgbToLab(rgbOf(t.colormap[255])).L, 0.70) << name.toStdString();
    }
}

// The top three stops of "Trans (Dark)" have a chroma of 0.12 or more and a pink hue.
TEST(Colors, TP_COL_02_TheTopStopsOfTransDarkHaveChromaAndAPinkHue)
{
    const Theme t = resolveThemeBlack("Trans (Dark)", "", "", "", 1.0);
    // The three top stops are at 76, 90, and 100 percent of the map.
    for (double position : { 0.76, 0.90, 1.00 }) {
        const int index = (int) std::lround(position * 255.0);
        const Lch c = labToLch(rgbToLab(rgbOf(t.colormap[index])));
        EXPECT_GE(c.C, 0.12) << "position " << position;
        EXPECT_GE(c.h, 340.0) << "position " << position;
        EXPECT_LE(c.h, 360.0) << "position " << position;
    }
}

// The table of each theme has 256 entries, and each entry is a solid color in the sRGB gamut.
TEST(Colors, TP_COL_03_EachTableHas256SolidColorsInTheGamut)
{
    for (const QString &name : themeNames()) {
        for (double saturation : { 0.5, 1.0, 1.5 }) {
            for (bool black : { true, false }) {
                const Theme t = resolveTheme(name, "", "", "", saturation, black);
                ASSERT_EQ(t.colormap.size(), 256) << name.toStdString();
                for (int i = 0; i < 256; ++i) {
                    EXPECT_EQ(t.colormap[i] >> 24, 0xffu) << name.toStdString() << " entry " << i;
                    // Each channel is a byte, so a color that went through the conversion is in the gamut.
                    // The test also checks that the conversion to OKLab and back keeps the color.
                    const Lab lab = rgbToLab(rgbOf(t.colormap[i]));
                    EXPECT_TRUE(labInGamut(lab)) << name.toStdString() << " entry " << i;
                    EXPECT_EQ(labToRgb(lab), rgbOf(t.colormap[i])) << name.toStdString() << " entry " << i;
                }
            }
        }
    }
}

// A track color keeps a distance from every color of its table. The tracks have an outline, so a distance
// above 0.025 (about the smallest difference that a person sees) is the floor.
TEST(Colors, TP_COL_04_EachTrackColorKeepsADistanceFromItsTable)
{
    constexpr double floor = 0.025;
    for (const QString &name : themeNames()) {
        const Theme t = resolveThemeBlack(name, "", "", "", 1.0);
        const QColor tracks[6] = { t.pitch, t.formants[0], t.formants[1], t.formants[2], t.formants[3], t.formants[4] };
        for (int track = 0; track < 6; ++track) {
            const Lab lab = rgbToLab(tracks[track].rgb() & 0xffffff);
            double nearest = 1e9;
            for (int i = 0; i < 256; ++i) nearest = std::min(nearest, labDistance(lab, rgbToLab(rgbOf(t.colormap[i]))));
            EXPECT_GT(nearest, floor) << name.toStdString() << " track " << track;
        }
    }
}

// The saturation from 50 to 150 percent scales the chroma and keeps the lightness.
TEST(Colors, TP_COL_06_TheSaturationScalesTheChromaAndKeepsTheLightness)
{
    for (const char *name : { "Bisexual (Dark)", "Viridis", "Rainbow (Dark)" }) {
        const Theme base = resolveThemeBlack(name, "", "", "", 1.0);
        const Theme low = resolveThemeBlack(name, "", "", "", 0.5);
        const Theme high = resolveThemeBlack(name, "", "", "", 1.5);

        const double c1 = meanChroma(base.colormap);
        EXPECT_NEAR(meanChroma(low.colormap) / c1, 0.5, 0.08) << name;
        // The gamut limits a larger chroma, so the rise is less than 1.5 and more than 1.1.
        EXPECT_GT(meanChroma(high.colormap) / c1, 1.1) << name;
        EXPECT_LE(meanChroma(high.colormap) / c1, 1.5 + 0.05) << name;

        for (int i = 0; i < 256; ++i) {
            const double l = rgbToLab(rgbOf(base.colormap[i])).L;
            EXPECT_NEAR(rgbToLab(rgbOf(low.colormap[i])).L, l, 0.04) << name << " entry " << i;
            EXPECT_NEAR(rgbToLab(rgbOf(high.colormap[i])).L, l, 0.06) << name << " entry " << i;
        }
        // The order of the saturation shows in the chroma.
        EXPECT_LT(meanChroma(low.colormap), c1);
        EXPECT_LT(c1, meanChroma(high.colormap));
    }
}

// A custom theme with a wrong color, a missing color, or a very long list falls back to a safe value.
TEST(Colors, TP_COL_09_ACustomThemeWithWrongValuesFallsBackToSafeValues)
{
    const Theme magma = resolveThemeBlack("Magma", "", "", "", 1.0);

    // Nothing valid, or only one valid color: the map of the default theme.
    for (const char *colors : { "", "   ", "#zzzzzz,#nothex", "banana", "#336699" }) {
        const Theme t = resolveThemeBlack("Custom", colors, "", "", 1.0);
        EXPECT_EQ(t.colormap, magma.colormap) << "colors: " << colors;
        EXPECT_EQ(t.pitch, magma.pitch);
        EXPECT_EQ(t.formants[2], magma.formants[2]);
        EXPECT_EQ(t.name, "Custom");
    }

    // A wrong color among the good ones is skipped.
    const Theme gray = resolveThemeBlack("Custom", "#000000,bad,#ffffff", "", "", 1.0);
    ASSERT_EQ(gray.colormap.size(), 256);
    EXPECT_EQ(rgbOf(gray.colormap[0]), 0x000000u);
    EXPECT_EQ(rgbOf(gray.colormap[255]), 0xffffffu);

    // A wrong pitch color keeps the default pitch. A good one replaces it.
    EXPECT_EQ(resolveThemeBlack("Custom", "", "not a color", "", 1.0).pitch, magma.pitch);
    EXPECT_EQ(resolveThemeBlack("Custom", "", "#112233", "", 1.0).pitch, QColor("#112233"));
    // Fewer formant colors than five: the last color repeats. A wrong color is skipped.
    const Theme formants = resolveThemeBlack("Custom", "", "", "#112233,wrong,#445566", 1.0);
    EXPECT_EQ(formants.formants[0], QColor("#112233"));
    EXPECT_EQ(formants.formants[1], QColor("#445566"));
    EXPECT_EQ(formants.formants[4], QColor("#445566"));
    EXPECT_EQ(resolveThemeBlack("Custom", "", "", "wrong", 1.0).formants[0], magma.formants[0]);

    // A very long list gives a table of 256 entries in a short time.
    QStringList many;
    for (int i = 0; i < 20000; ++i) many << QString("#%1").arg((i * 2654435761u) & 0xffffff, 6, 16, QChar('0'));
    QElapsedTimer timer;
    timer.start();
    const Theme big = resolveThemeBlack("Custom", many.join(','), "", many.join(','), 1.0);
    EXPECT_EQ(big.colormap.size(), 256);
    // The plan limit is 2 s. The test allows three times this, because a busy machine is slower.
    EXPECT_LT(timer.elapsed(), 6000);

    // A value of the saturation that is far outside the range still gives a table of 256 entries in the gamut.
    for (double saturation : { 0.0, -1.0, 100.0 }) {
        const Theme extreme = resolveThemeBlack("Custom", "#000000,#ff0000,#ffff00", "", "", saturation);
        ASSERT_EQ(extreme.colormap.size(), 256) << saturation;
        for (QRgb c : extreme.colormap) EXPECT_EQ(c >> 24, 0xffu);
    }
}

// The grid color is a light tint for a dark quiet color, and a dark tint for a light quiet color.
TEST(Colors, TP_GRD_11_TheGridColorIsALightTintOnDarkAndADarkTintOnLight)
{
    const QColor dark = gridLineColor(QColor(10, 10, 40), 0.6, 1.0);
    EXPECT_GT(dark.lightnessF(), 0.5);
    EXPECT_GT(dark.lightnessF(), QColor(10, 10, 40).lightnessF() + 0.3);
    // The tint keeps the hue of the quiet color: blue stays the largest channel.
    EXPECT_GT(dark.blue(), dark.red());

    const QColor light = gridLineColor(QColor(240, 235, 200), 0.6, 1.0);
    EXPECT_LT(light.lightnessF(), 0.5);
    EXPECT_GT(QColor(240, 235, 200).lightnessF() - light.lightnessF(), 0.3);
    EXPECT_GT(light.red(), light.blue());

    // The opacity is the strength times the scale, in the range 0 to 1.
    EXPECT_NEAR(gridLineColor(QColor(0, 0, 0), 0.6, 1.0).alphaF(), 0.6, 1e-4);
    EXPECT_NEAR(gridLineColor(QColor(0, 0, 0), 0.6, 0.55).alphaF(), 0.33, 1e-3);
    EXPECT_DOUBLE_EQ(gridLineColor(QColor(0, 0, 0), 2.0, 1.0).alphaF(), 1.0);
    EXPECT_DOUBLE_EQ(gridLineColor(QColor(0, 0, 0), -1.0, 1.0).alphaF(), 0.0);
    // The medium color at the border of 0.5 lightness goes to one of the two tints.
    EXPECT_GT(gridLineColor(QColor(100, 100, 100), 0.6, 1.0).lightnessF(), 0.5);
}

// The lift of the quiet end changes nothing in Viridis, and it adds no chroma to a gray map.
TEST(Colors, TP_QUI_02_TheLiftChangesNothingInViridisAndAddsNoChromaToGray)
{
    QVector<QRgb> viridis = resolveThemeBlack("Viridis", "", "", "", 1.0).colormap;
    const QVector<QRgb> before = viridis;
    liftQuietEnd(viridis);
    EXPECT_EQ(viridis, before);

    QVector<QRgb> gray = resolveThemeBlack("Grayscale", "", "", "", 1.0).colormap;
    const QVector<QRgb> grayBefore = gray;
    liftQuietEnd(gray);
    EXPECT_NE(gray, grayBefore);
    EXPECT_GE(rgbToLab(rgbOf(gray[0])).L, quietLightness - 0.01);
    for (QRgb c : gray) EXPECT_LT(labToLch(rgbToLab(rgbOf(c))).C, 0.01);
    // The upper part of the map keeps its exact colors.
    for (int i = 128; i < 256; ++i) EXPECT_EQ(gray[i], grayBefore[i]) << i;

    // A map with a tint gets the hue of its quiet end.
    QVector<QRgb> tinted = resolveThemeBlack("Ocean", "", "", "", 1.0).colormap;
    liftQuietEnd(tinted);
    EXPECT_GT(labToLch(rgbToLab(rgbOf(tinted[0]))).C, 0.04);
    // A second lift moves each channel by two steps at most. The first lift ends at the target lightness up to the
    // rounding of the 8-bit colors, so the second lift is tiny.
    QVector<QRgb> again = tinted;
    liftQuietEnd(again);
    for (int i = 0; i < 256; ++i) {
        for (int shift : { 0, 8, 16 }) {
            EXPECT_LE(std::abs((int) ((again[i] >> shift) & 0xff) - (int) ((tinted[i] >> shift) & 0xff)), 2) << i;
        }
    }

    // An array with less than 2 colors is not changed.
    QVector<QRgb> one = { 0xff000000 };
    liftQuietEnd(one);
    EXPECT_EQ(one[0], 0xff000000u);
}

// The grid color and each track color have a contrast of 3 to 1 against the quiet color, in each theme.
TEST(Colors, TP_QUI_06_TheGridAndTheTracksHaveAContrastOfThreeToOneAgainstTheQuietColor)
{
    for (const QString &name : themeNames()) {
        for (bool black : { true, false }) {
            const Theme t = resolveTheme(name, "", "", "", 1.0, black);
            const unsigned quiet = rgbOf(t.colormap[0]);
            const std::string where = name.toStdString() + (black ? " black" : " lifted");

            // The grid has the default strength of 0.6. The major lines are the strongest.
            const QColor grid = gridLineColor(QColor::fromRgb(t.colormap[0]), 0.6, 1.0);
            EXPECT_GE(contrast(blend(grid, quiet), quiet), 3.0) << where << " grid";

            const QColor tracks[6] = { t.pitch, t.formants[0], t.formants[1], t.formants[2], t.formants[3], t.formants[4] };
            for (int i = 0; i < 6; ++i) {
                EXPECT_GE(contrast(tracks[i].rgb() & 0xffffff, quiet), 3.0) << where << " track " << i;
            }
        }
    }
}
