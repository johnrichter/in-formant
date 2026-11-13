#ifndef GUI_QUIETLIFT_H
#define GUI_QUIETLIFT_H

#include "oklab.h"
#include <algorithm>
#include <cmath>

// The quietest color of a color map is often black. Black is the color of the window around the
// graph, so a quiet sound has no contrast with the window, and the loud parts look cut out. The
// Viridis map has a dark purple at the quiet end. The purple frames the graph and shows every
// quiet detail. This function gives each map the same property: it lifts the low end to a dark
// tint of the map's own hue, and it leaves the upper part of the map as it was.
//
// The map is an array of 256 colors (0xAARRGGBB or 0x00RRGGBB). The function changes the array in place.

namespace Gui {

// The lightness of the quiet end after the lift. Viridis has about this value.
constexpr double quietLightness = 0.27;
// The chroma of the quiet end. Viridis has a purple of about this chroma.
constexpr double quietChroma = 0.12;
// The part of the map (0 to 1) over which the lift fades out.
constexpr double quietReach = 0.45;

inline double quietWeight(double t)
{
    const double u = std::clamp(t / quietReach, 0.0, 1.0);
    const double smooth = u * u * (3.0 - 2.0 * u);
    return 1.0 - smooth;
}

template<typename Array>
void liftQuietEnd(Array &map)
{
    const int count = (int) map.size();
    if (count < 2) return;

    const Lch first = labToLch(rgbToLab(map[0] & 0xffffff));
    const double lift = std::max(0.0, quietLightness - first.L);
    if (lift <= 0.0) return;     // The quiet end is already light enough, for example in Viridis.

    // The hue of the quiet end: the first color of the map that has a clear hue.
    double hue = 0.0;
    double chroma = 0.0;
    bool found = false;
    for (int i = 0; i < count && i < (int) (0.5 * count); ++i) {
        const Lch c = labToLch(rgbToLab(map[i] & 0xffffff));
        if (c.C > 0.04) {
            hue = c.h;
            chroma = quietChroma;
            found = true;
            break;
        }
    }

    for (int i = 0; i < count; ++i) {
        const double t = i / double(count - 1);
        const double w = quietWeight(t);
        if (w <= 0.0) continue;     // The upper part of the map keeps its exact colors.
        Lch c = labToLch(rgbToLab(map[i] & 0xffffff));

        c.L += lift * w;
        if (found) {
            // A color with little chroma takes the hue of the quiet end, and the chroma rises to the target.
            const double own = std::clamp(c.C / 0.04, 0.0, 1.0);
            double delta = c.h - hue;
            if (delta > 180.0) delta -= 360.0;
            if (delta < -180.0) delta += 360.0;
            c.h = hue + delta * own;
            if (c.h < 0.0) c.h += 360.0;
            c.C = std::max(c.C, chroma * w);
        }
        map[i] = (map[i] & 0xff000000u) | labToRgb(lchToLab(c));
    }
}

}

#endif // GUI_QUIETLIFT_H
