#ifndef GUI_GRIDCOLOR_H
#define GUI_GRIDCOLOR_H

#include <QColor>
#include <algorithm>

// The color of the grid lines. The function has no drawing code, so a test can run it.

namespace Gui {

// The line is a light tint of the quiet color of the theme, or a dark tint when the quiet color is light.
// The line has the hue of the theme, so it fits the picture, and it has a large difference in lightness.
// The strength (0 to 1) and the scale of the line set the opacity.
inline QColor gridLineColor(const QColor &quiet, double strength, double alphaScale)
{
    const bool darkBase = quiet.lightnessF() < 0.5;
    const QColor target = darkBase ? QColor(255, 255, 255) : QColor(0, 0, 0);
    const double mix = 0.75;
    QColor line = QColor::fromRgbF(quiet.redF() * (1 - mix) + target.redF() * mix,
                                   quiet.greenF() * (1 - mix) + target.greenF() * mix,
                                   quiet.blueF() * (1 - mix) + target.blueF() * mix);
    line.setAlphaF(std::clamp(strength * alphaScale, 0.0, 1.0));
    return line;
}

}

#endif // GUI_GRIDCOLOR_H
