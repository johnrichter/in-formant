#ifndef GUI_BANDS_H
#define GUI_BANDS_H

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>

// The pitch zones that the graph can show. Four edges in Hz make three zones. The functions have no
// drawing code, so a test can run them.

namespace Gui {

using BandEdges = std::array<double, 4>;

// The default edges: 60 to 150 Hz, 150 to 180 Hz, and 180 to 300 Hz.
constexpr BandEdges defaultBandEdges = { 60.0, 150.0, 180.0, 300.0 };

// Read four edges from a text of numbers that commas or spaces separate. A text that has not exactly
// four values, or has values that do not rise, or has a value below 20 Hz or above 2000 Hz, gives the
// default edges.
inline BandEdges parseBandEdges(const std::string &text)
{
    BandEdges edges {};
    int count = 0;
    const char *p = text.c_str();
    while (*p != '\0') {
        while (*p == ',' || *p == ' ' || *p == ';' || *p == '\t') ++p;
        if (*p == '\0') break;
        char *end = nullptr;
        const double value = std::strtod(p, &end);
        if (end == p || count >= 4) return defaultBandEdges;
        edges[count++] = value;
        p = end;
    }

    if (count != 4) return defaultBandEdges;
    for (int i = 0; i < 4; ++i) {
        if (!std::isfinite(edges[i]) || edges[i] < 20.0 || edges[i] > 2000.0) return defaultBandEdges;
        if (i > 0 && edges[i] <= edges[i - 1]) return defaultBandEdges;
    }
    return edges;
}

}

#endif // GUI_BANDS_H
