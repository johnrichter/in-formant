#include <gtest/gtest.h>
#include <utility>
#include "../src/gui/uiscale.h"
#include <algorithm>
#include <cmath>

using namespace Gui::UiScale;

namespace {

// The relative luminance of an sRGB colour, and the contrast ratio of two colours (WCAG).
double luminance(unsigned int rgb)
{
    auto channel = [](unsigned int v) {
        const double c = v / 255.0;
        return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel((rgb >> 16) & 0xff) + 0.7152 * channel((rgb >> 8) & 0xff) + 0.0722 * channel(rgb & 0xff);
}

double contrast(unsigned int a, unsigned int b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

}

// TP-UI-01: the values of the type scale and the spacing scale.
TEST(UiScale, TP_UI_01_ScaleValues)
{
    const int sizes[] = { 12, 14, 17, 20, 24 };
    const int lines[] = { 16, 20, 24, 28, 32 };
    ASSERT_EQ(typeSteps.size(), 5u);
    for (size_t i = 0; i < typeSteps.size(); ++i) {
        EXPECT_EQ(typeSteps[i].size, sizes[i]);
        EXPECT_EQ(typeSteps[i].line, lines[i]);
    }
    const int spaces[] = { 4, 8, 12, 16, 24, 32, 48 };
    ASSERT_EQ(spaceSteps.size(), 7u);
    for (size_t i = 0; i < spaceSteps.size(); ++i) EXPECT_EQ(spaceSteps[i].size, spaces[i]);
}

// The type steps grow, and each size follows the ratio from the body, with a floor for the caption.
TEST(UiScale, TypeStepsGrowByTheRatio)
{
    int previous = 0;
    for (const TypeStep &s : typeSteps) {
        EXPECT_GT(s.size, previous);
        previous = s.size;
        EXPECT_GE(s.size, minimumSize);
    }
    double expected = bodySize;
    // The steps above the body.
    for (size_t i = 2; i < typeSteps.size(); ++i) {
        expected *= typeRatio;
        EXPECT_NEAR((double) typeSteps[i].size, expected, 0.5);
    }
    EXPECT_EQ(typeSteps[1].size, bodySize);
    // The caption follows the ratio below the body, and it never goes under the floor.
    EXPECT_EQ(typeSteps[0].size, minimumSize);
    EXPECT_LT(bodySize / typeRatio, minimumSize + 0.5);
}

// Each line height is a multiple of 4 and between 1.3 and 1.5 times the size.
TEST(UiScale, LineHeightsAreOnTheGrid)
{
    for (const TypeStep &s : typeSteps) {
        EXPECT_EQ(s.line % 4, 0);
        EXPECT_GE(s.line, 1.3 * s.size - 1e-9);
        EXPECT_LE(s.line, 1.5 * s.size + 1e-9);
    }
}

// Each space is a multiple of 4, and the steps grow.
TEST(UiScale, SpacesAreOnTheGridAndGrow)
{
    int previous = 0;
    for (const SpaceStep &s : spaceSteps) {
        EXPECT_EQ(s.size % 4, 0);
        EXPECT_GT(s.size, previous);
        previous = s.size;
    }
    EXPECT_EQ(spaceSteps.front().size, 4);
}

// TP-SCR-10: the named steps match the table, the scroll line is on the grid, and the padding of the graph is one of the steps.
TEST(UiScale, TP_SCR_10_NamedStepsAndScrollLine)
{
    EXPECT_TRUE(spaceXs == 4 && spaceSm == 8 && spaceMd == 12 && spaceLg == 16 && spaceXl == 24);
    EXPECT_EQ(gridUnit, 4);
    EXPECT_EQ(scrollLine % gridUnit, 0);
    bool onScale = false;
    for (const SpaceStep &s : spaceSteps) onScale = onScale || s.size == graphPadding;
    EXPECT_TRUE(onScale);
}

// The radii are multiples of 4 and they grow.
TEST(UiScale, RadiiAreOnTheGridAndGrow)
{
    int previous = 0;
    for (const RadiusStep &s : radiusSteps) {
        EXPECT_EQ(s.size % 4, 0);
        EXPECT_GT(s.size, previous);
        previous = s.size;
    }
}

// TP-TYP-02: the scale of the help pages has lines and gaps on the 4-pixel grid, a loose body line, tight heading lines,
// more space above a heading than below it, and a column of about 60 to 80 characters.
TEST(UiScale, TP_TYP_02_ProseScale)
{
    using P = Gui::UiScale::Prose;
    for (int v : { P::bodyLine, P::codeLine, P::h1Line, P::h2Line, P::h3Line, P::paragraphGap, P::listItemGap, P::h1Below,
                   P::h2Above, P::h2Below, P::h3Above, P::h3Below, P::listIndent, P::cellPadding, P::pageMargin }) {
        EXPECT_EQ(v % 4, 0);
    }
    EXPECT_GE(P::bodyLine, 1.6 * P::bodySize);
    EXPECT_GE(P::codeLine, 1.5 * P::codeSize);
    for (auto sizeLine : { std::pair<int, int>(P::h1Size, P::h1Line), std::pair<int, int>(P::h2Size, P::h2Line), std::pair<int, int>(P::h3Size, P::h3Line) }) {
        EXPECT_GE(sizeLine.second, 1.25 * sizeLine.first);
        EXPECT_LE(sizeLine.second, 1.6 * sizeLine.first);
    }
    EXPECT_GT(P::h1Size, P::h2Size);
    EXPECT_GT(P::h2Size, P::h3Size);
    EXPECT_GT(P::h3Size, P::bodySize);
    EXPECT_LT(P::codeSize, P::bodySize);
    // A heading belongs to the text after it: the space above is larger than the space below.
    EXPECT_GE(P::h2Above, 3 * P::h2Below);
    EXPECT_GE(P::h3Above, 3 * P::h3Below);
    EXPECT_GE(P::h2Above, P::h3Above);
    // The gap between two paragraphs is larger than the gap between two list items.
    EXPECT_GT(P::paragraphGap, P::listItemGap);
    // About 0.52 of the size is the mean width of a character of a sans-serif font.
    const double characters = P::measure / (0.52 * P::bodySize);
    EXPECT_TRUE(characters >= 60 && characters <= 80);
}

// TP-SHAPE-01: one corner radius serves all controls and boxes. It is the medium radius.
TEST(UiScale, TP_SHAPE_01_CornerRadiusIsMedium)
{
    EXPECT_EQ(cornerRadius, 8);
    EXPECT_EQ(cornerRadius, radiusSteps[1].size);
}

// TP-UI-13: each text colour reads on the window and on the surface with a contrast of at least 4.5 to 1.
// The primary colour has more contrast than the secondary colour, and the secondary more than the tertiary.
TEST(UiScale, TP_UI_13_TextContrast)
{
    double previous = 1e9;
    for (const ColorToken &c : textColors) {
        EXPECT_GE(contrast(c.rgb, windowBackground), 4.5);
        EXPECT_GE(contrast(c.rgb, surfaceBackground), 4.5);
        EXPECT_LT(contrast(c.rgb, windowBackground), previous);
        previous = contrast(c.rgb, windowBackground);
    }
}

// A line of text has about 70 characters at the body size (the average character is 0.5 times the size).
TEST(UiScale, TextMeasureHasAboutSeventyCharacters)
{
    EXPECT_LE(textMeasure / (0.5 * bodySize), 80.0);
    EXPECT_GE(textMeasure / (0.5 * bodySize), 60.0);
}
