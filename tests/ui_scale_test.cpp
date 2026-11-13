#include "check.h"
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

int main()
{
    // The type steps grow, and each size follows the ratio from the body, with a floor for the caption.
    {
        int previous = 0;
        for (const TypeStep &s : typeSteps) {
            CHECK(s.size > previous);
            previous = s.size;
            CHECK(s.size >= minimumSize);
        }
        double expected = bodySize;
        // The steps above the body.
        for (size_t i = 2; i < typeSteps.size(); ++i) {
            expected *= typeRatio;
            CHECK_NEAR((double) typeSteps[i].size, expected, 0.5);
        }
        CHECK(typeSteps[1].size == bodySize);
        // The caption follows the ratio below the body, and it never goes under the floor.
        CHECK(typeSteps[0].size == minimumSize);
        CHECK(bodySize / typeRatio < minimumSize + 0.5);
    }

    // Each line height is a multiple of 4 and between 1.3 and 1.5 times the size.
    for (const TypeStep &s : typeSteps) {
        CHECK(s.line % 4 == 0);
        CHECK(s.line >= 1.3 * s.size - 1e-9);
        CHECK(s.line <= 1.5 * s.size + 1e-9);
    }

    // Each space is a multiple of 4, and the steps grow.
    {
        int previous = 0;
        for (const SpaceStep &s : spaceSteps) {
            CHECK(s.size % 4 == 0);
            CHECK(s.size > previous);
            previous = s.size;
        }
        CHECK(spaceSteps.front().size == 4);
    }

    // The named steps match the table, and the padding of the graph is one of the steps.
    {
        CHECK(spaceXs == 4 && spaceSm == 8 && spaceMd == 12 && spaceLg == 16 && spaceXl == 24);
        CHECK(gridUnit == 4);
        bool onScale = false;
        for (const SpaceStep &s : spaceSteps) onScale = onScale || s.size == graphPadding;
        CHECK(onScale);
    }

    // The radii are multiples of 4 and they grow.
    {
        int previous = 0;
        for (const RadiusStep &s : radiusSteps) {
            CHECK(s.size % 4 == 0);
            CHECK(s.size > previous);
            previous = s.size;
        }
    }

    // The scale of the help pages: lines and gaps on the 4-pixel grid, a loose body line, tight heading lines,
    // more space above a heading than below it, and a column of about 60 to 80 characters.
    {
        using P = Gui::UiScale::Prose;
        for (int v : { P::bodyLine, P::codeLine, P::h1Line, P::h2Line, P::h3Line, P::paragraphGap, P::listItemGap, P::h1Below,
                       P::h2Above, P::h2Below, P::h3Above, P::h3Below, P::listIndent, P::cellPadding, P::pageMargin }) {
            CHECK(v % 4 == 0);
        }
        CHECK(P::bodyLine >= 1.6 * P::bodySize);
        CHECK(P::codeLine >= 1.5 * P::codeSize);
        for (auto sizeLine : { std::pair<int, int>(P::h1Size, P::h1Line), std::pair<int, int>(P::h2Size, P::h2Line), std::pair<int, int>(P::h3Size, P::h3Line) }) {
            CHECK(sizeLine.second >= 1.25 * sizeLine.first);
            CHECK(sizeLine.second <= 1.6 * sizeLine.first);
        }
        CHECK(P::h1Size > P::h2Size);
        CHECK(P::h2Size > P::h3Size);
        CHECK(P::h3Size > P::bodySize);
        CHECK(P::codeSize < P::bodySize);
        // A heading belongs to the text after it: the space above is larger than the space below.
        CHECK(P::h2Above >= 3 * P::h2Below);
        CHECK(P::h3Above >= 3 * P::h3Below);
        CHECK(P::h2Above >= P::h3Above);
        // The gap between two paragraphs is larger than the gap between two list items.
        CHECK(P::paragraphGap > P::listItemGap);
        // About 0.52 of the size is the mean width of a character of a sans-serif font.
        const double characters = P::measure / (0.52 * P::bodySize);
        CHECK(characters >= 60 && characters <= 80);
    }

    // One corner radius serves all controls and boxes. It is the medium radius.
    CHECK(cornerRadius == 8);

    // Each text colour reads on the window and on the surface with a contrast of at least 4.5 to 1.
    // The primary colour has more contrast than the secondary colour, and the secondary more than the tertiary.
    {
        double previous = 1e9;
        for (const ColorToken &c : textColors) {
            CHECK(contrast(c.rgb, windowBackground) >= 4.5);
            CHECK(contrast(c.rgb, surfaceBackground) >= 4.5);
            CHECK(contrast(c.rgb, windowBackground) < previous);
            previous = contrast(c.rgb, windowBackground);
        }
    }

    // A line of text has about 70 characters at the body size (the average character is 0.5 times the size).
    CHECK(textMeasure / (0.5 * bodySize) <= 80.0);
    CHECK(textMeasure / (0.5 * bodySize) >= 60.0);

    return TEST_RESULT();
}
