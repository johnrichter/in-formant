#ifndef GUI_READOUTLAYOUT_H
#define GUI_READOUTLAYOUT_H

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// The pure logic of the readout boxes: the order, the fixed text widths and the placement. The
// functions have no drawing code, so a test can run them.

namespace Gui {

// The keys of the boxes in the default order. A track box has the lower case name of its track.
inline const std::vector<std::string> &readoutKeys()
{
    static const std::vector<std::string> keys = { "legend", "pitch", "f1", "f2", "f3", "f4" };
    return keys;
}

// Read an order from text with keys separated by commas. An unknown key and a repeated key are
// dropped. A key that the text omits goes to the end, in the default order. The result always has
// each key one time.
inline std::vector<std::string> normalizeReadoutOrder(const std::string &text)
{
    std::vector<std::string> order;
    std::string word;
    auto flush = [&]() {
        std::string key;
        for (char c : word) {
            if (!std::isspace((unsigned char) c)) key += (char) std::tolower((unsigned char) c);
        }
        word.clear();
        const auto &keys = readoutKeys();
        if (std::find(keys.begin(), keys.end(), key) != keys.end()
            && std::find(order.begin(), order.end(), key) == order.end()) {
            order.push_back(key);
        }
    };
    for (char c : text) {
        if (c == ',') flush();
        else word += c;
    }
    flush();
    for (const auto &key : readoutKeys()) {
        if (std::find(order.begin(), order.end(), key) == order.end()) order.push_back(key);
    }
    return order;
}

inline std::string joinReadoutOrder(const std::vector<std::string> &order)
{
    std::string text;
    for (const auto &key : order) {
        if (!text.empty()) text += ',';
        text += key;
    }
    return text;
}

// Replace each digit of a pattern with the widest digit of the font. The width of the result is
// the width of the widest text that has the same pattern, so a box that has this width never
// changes its size when the digits change.
inline std::string fixedWidthSample(const std::string &pattern, char widestDigit)
{
    std::string sample = pattern;
    for (char &c : sample) {
        if (c >= '0' && c <= '9') c = widestDigit;
    }
    return sample;
}

struct BoxSize {
    std::string key;
    double width = 0.0;
    double height = 0.0;
};

struct BoxPlacement {
    std::string key;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    bool visible = true;
};

// Place the boxes in one row from x0, with the top at y. A box that does not fit before the
// right edge is not visible, and the next box can still use the space.
inline std::vector<BoxPlacement> placeInRow(const std::vector<BoxSize> &boxes, double x0, double y,
                                            double right, double gap)
{
    std::vector<BoxPlacement> result;
    double x = x0;
    for (const BoxSize &b : boxes) {
        BoxPlacement p;
        p.key = b.key;
        p.width = b.width;
        p.height = b.height;
        p.y = y;
        if (x + b.width <= right) {
            p.x = x;
            x += b.width + gap;
        }
        else {
            p.visible = false;
        }
        result.push_back(p);
    }
    return result;
}

// Place the boxes in columns from the top at y0. A column ends when the next box does not fit
// in maxHeight, and a new column starts at the right of it. totalWidth is the width of all columns
// and the gaps between them.
inline std::vector<BoxPlacement> placeInColumns(const std::vector<BoxSize> &boxes, double x0, double y0,
                                                double maxHeight, double gap, double &totalWidth)
{
    std::vector<BoxPlacement> result;
    double columnX = x0;
    double columnWidth = 0.0;
    double y = y0;
    totalWidth = 0.0;
    bool columnHasBox = false;

    for (const BoxSize &b : boxes) {
        if (columnHasBox && y + b.height > y0 + maxHeight) {
            columnX += columnWidth + gap;
            columnWidth = 0.0;
            y = y0;
            columnHasBox = false;
        }
        BoxPlacement p;
        p.key = b.key;
        p.x = columnX;
        p.y = y;
        p.width = b.width;
        p.height = b.height;
        result.push_back(p);
        y += b.height + gap;
        columnWidth = std::max(columnWidth, b.width);
        columnHasBox = true;
        totalWidth = (columnX - x0) + columnWidth;
    }
    return result;
}

}

#endif // GUI_READOUTLAYOUT_H
