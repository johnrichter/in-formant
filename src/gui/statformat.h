#ifndef GUI_STATFORMAT_H
#define GUI_STATFORMAT_H

#include <cmath>
#include <cstdio>
#include <string>

namespace Gui {

    // The Range statistic in semitones: 12 times the base-2 logarithm of P95 divided by P5.
    // The text is a dash when the ratio has no meaning (P5 of 0 or less, or P95 below P5).
    inline std::string rangeSemitonesText(double p5, double p95)
    {
        if (!(p5 > 0.0) || !(p95 >= p5)) return "-";
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.1fst", 12.0 * std::log2(p95 / p5));
        return buffer;
    }

}

#endif // GUI_STATFORMAT_H
