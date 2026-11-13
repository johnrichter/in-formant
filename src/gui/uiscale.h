#ifndef GUI_UISCALE_H
#define GUI_UISCALE_H

#include <array>

// The type scale and the spacing scale of all windows. One place holds the numbers, so a test can check
// the rules, and the windows read the same values through the context property "ui".
//
// The design of InFormant:
// - The app is a dense tool with dark windows, so the body size is 14 pixels and not 16.
// - The type scale is a minor third (a ratio of 1.2) from the body size: 14, 16.8, 20.2, and 24.2.
//   The values round to 17, 20, and 24. The step below the body is 11.7, and nothing is smaller than
//   12 pixels, so the caption is 12.
// - The spacing scale is a multiple of 4 pixels. The steps go 1, 2, 3, 4, 6, 8, and 12 units.
// - Each line height is a multiple of 4 pixels, between 1.3 and 1.5 times the size.
// - A space inside a group is smaller than the space between groups, so a person sees the groups.
// - The hierarchy has three levels in each window: a heading (17 pixels, bold), a field label (14 pixels,
//   medium), and a hint (12 pixels, regular, in the secondary colour). Size, weight, and colour all
//   change from one level to the next, so the levels read at a glance.
// - A line of text has at most about 70 characters (560 pixels at 14 pixels), because longer lines tire the eye.
// - Each text colour has a contrast of at least 4.5 to 1 against the window and the surface colours.

namespace Gui::UiScale {

struct TypeStep {
    const char *name;
    int size;
    int line;
};

// From the smallest to the largest.
inline constexpr std::array<TypeStep, 5> typeSteps = { {
    { "caption", 12, 16 },   // hints, units, small notes
    { "body", 14, 20 },      // labels, controls, paragraphs
    { "subtitle", 17, 24 },  // headings of the sections in a window
    { "title", 20, 28 },     // the title of a window or a dialog
    { "headline", 24, 32 },  // a number or a short phrase that stands alone
} };

struct SpaceStep {
    const char *name;
    int size;
};

inline constexpr std::array<SpaceStep, 7> spaceSteps = { {
    { "xs", 4 },     // between a control and its hint
    { "sm", 8 },     // between a label and its control
    { "md", 12 },    // between two controls of a group
    { "lg", 16 },    // the margin of a small window, and the space above a heading
    { "xl", 24 },    // the margin of a large window, and the space between groups
    { "xxl", 32 },   // the space between large parts
    { "xxxl", 48 },  // the space around an empty state
} };

struct RadiusStep {
    const char *name;
    int size;
};

inline constexpr std::array<RadiusStep, 3> radiusSteps = { {
    { "sm", 4 },
    { "md", 8 },
    { "lg", 12 },
} };

// The corner radius of every control and every box. The windows and the graph use the same value.
inline constexpr int cornerRadius = radiusSteps[1].size;

struct ColorToken {
    const char *name;
    unsigned int rgb;
};

// The colours of the windows (the dark theme of Qt Material) and the colours of the text on them.
inline constexpr unsigned int windowBackground = 0x1C1B1F;
inline constexpr unsigned int surfaceBackground = 0x2B2930;

inline constexpr std::array<ColorToken, 3> textColors = { {
    { "primary", 0xE6E1E5 },    // headings, labels, values
    { "secondary", 0xCAC4D0 },  // hints and units
    { "tertiary", 0xA8A2AE },   // the least important text, for example a table header
} };

// The longest line of a paragraph, in pixels, at the body size.
inline constexpr int textMeasure = 560;

// The scale of the help pages. A page is long text, so it has its own scale. The body is 16 pixels with a
// line of 28 pixels (1.75 times the size), because long text needs more air than a window of controls.
// The sizes of the headings go up by a ratio of about 1.2 (16, 18, 22, 28). A heading has a tight line
// (1.3 times the size), because a short line of large text needs less air. The rules come from three sources:
// the Tailwind typography plugin (a body line of 1.75, a paragraph gap of 1.25 em, a list gap of 0.5 em),
// "Refactoring UI" (more space above a heading than below it, so the heading belongs to the text after it),
// and the habit of long-form blogs (a line of 60 to 75 characters).
//
// Qt adds the bottom margin of a block and the top margin of the next block. It does not merge them.
// So the page uses a bottom margin for a normal gap, and a top margin only to widen the gap above a heading.
struct Prose {
    static constexpr int bodySize = 16;
    static constexpr int bodyLine = 28;
    static constexpr int codeSize = 14;      // code, and the cells of a table
    static constexpr int codeLine = 24;
    static constexpr int h1Size = 28;
    static constexpr int h1Line = 36;
    static constexpr int h2Size = 22;
    static constexpr int h2Line = 32;
    static constexpr int h3Size = 18;
    static constexpr int h3Line = 28;

    static constexpr int paragraphGap = 20;  // below a paragraph, a list, a table, and a code block
    static constexpr int listItemGap = 8;    // between two items of a list
    static constexpr int h1Below = 24;
    static constexpr int h2Above = 48;       // the total space above, with the gap of the block before it
    static constexpr int h2Below = 12;
    static constexpr int h3Above = 32;
    static constexpr int h3Below = 8;
    static constexpr int listIndent = 24;
    static constexpr int cellPadding = 8;
    static constexpr int measure = 640;      // the width of the column: about 75 characters at 16 pixels
    static constexpr int pageMargin = 32;    // the space between the column and the window
};

// The ratio between the steps of the type scale, and the size of the body.
inline constexpr double typeRatio = 1.2;
inline constexpr int bodySize = 14;
inline constexpr int minimumSize = 12;

}

#endif // GUI_UISCALE_H
