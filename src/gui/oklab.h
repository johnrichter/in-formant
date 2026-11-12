#ifndef GUI_OKLAB_H
#define GUI_OKLAB_H

#include <algorithm>
#include <cmath>

// Colour maths in the OKLab space. Equal steps in OKLab look like equal steps of colour, so a
// gradient has no grey patch and no jump of brightness. The functions have no Qt types, so a test
// can run them.

namespace Gui {

struct Lab {
    double L = 0.0;
    double a = 0.0;
    double b = 0.0;
};

// The cylindrical form of OKLab: lightness, chroma, and hue in degrees.
struct Lch {
    double L = 0.0;
    double C = 0.0;
    double h = 0.0;
};

inline double srgbToLinear(double c)
{
    return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

inline double linearToSrgb(double c)
{
    return c <= 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
}

// rgb is 0xrrggbb.
inline Lab rgbToLab(unsigned int rgb)
{
    const double r = srgbToLinear(((rgb >> 16) & 0xff) / 255.0);
    const double g = srgbToLinear(((rgb >> 8) & 0xff) / 255.0);
    const double bl = srgbToLinear((rgb & 0xff) / 255.0);

    const double l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * bl);
    const double m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * bl);
    const double s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * bl);

    Lab lab;
    lab.L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    lab.a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    lab.b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
    return lab;
}

// The linear sRGB value of a colour. A value outside 0 to 1 means the colour is outside the gamut.
inline void labToLinear(const Lab &lab, double out[3])
{
    const double l_ = lab.L + 0.3963377774 * lab.a + 0.2158037573 * lab.b;
    const double m_ = lab.L - 0.1055613458 * lab.a - 0.0638541728 * lab.b;
    const double s_ = lab.L - 0.0894841775 * lab.a - 1.2914855480 * lab.b;
    const double l = l_ * l_ * l_;
    const double m = m_ * m_ * m_;
    const double s = s_ * s_ * s_;
    out[0] = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    out[1] = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    out[2] = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;
}

inline bool labInGamut(const Lab &lab)
{
    double c[3];
    labToLinear(lab, c);
    const double eps = 1e-4;
    return c[0] >= -eps && c[0] <= 1 + eps && c[1] >= -eps && c[1] <= 1 + eps && c[2] >= -eps && c[2] <= 1 + eps;
}

// A colour outside the gamut loses chroma until it fits. The lightness and the hue stay the same.
inline unsigned int labToRgb(Lab lab)
{
    if (!labInGamut(lab)) {
        double low = 0.0;
        double high = 1.0;
        const Lab full = lab;
        for (int i = 0; i < 24; ++i) {
            const double mid = 0.5 * (low + high);
            Lab t = full;
            t.a *= mid;
            t.b *= mid;
            if (labInGamut(t)) low = mid;
            else high = mid;
        }
        lab.a = full.a * low;
        lab.b = full.b * low;
    }

    double c[3];
    labToLinear(lab, c);
    unsigned int out = 0;
    for (int i = 0; i < 3; ++i) {
        const double v = std::clamp(linearToSrgb(std::clamp(c[i], 0.0, 1.0)), 0.0, 1.0);
        out = (out << 8) | (unsigned int) std::lround(v * 255.0);
    }
    return out;
}

inline Lch labToLch(const Lab &lab)
{
    Lch lch;
    lch.L = lab.L;
    lch.C = std::hypot(lab.a, lab.b);
    lch.h = lch.C > 1e-9 ? std::atan2(lab.b, lab.a) * 180.0 / M_PI : 0.0;
    if (lch.h < 0.0) lch.h += 360.0;
    return lch;
}

inline Lab lchToLab(const Lch &lch)
{
    const double rad = lch.h * M_PI / 180.0;
    return { lch.L, lch.C * std::cos(rad), lch.C * std::sin(rad) };
}

inline Lab lchToLab(double L, double C, double h)
{
    return lchToLab(Lch { L, C, h });
}

// The colour at position u (0 to 1) between two colours. The lightness and the chroma change in a
// straight line. The hue takes the shorter way round the circle, so a blue to pink gradient goes
// through violet and not through grey. A colour with almost no chroma takes the hue of the other end.
inline Lch mixLch(const Lch &a, const Lch &b, double u)
{
    constexpr double neutral = 0.01;
    double ha = a.h;
    double hb = b.h;
    if (a.C < neutral) ha = hb;
    if (b.C < neutral) hb = ha;

    double delta = hb - ha;
    if (delta > 180.0) delta -= 360.0;
    if (delta < -180.0) delta += 360.0;

    Lch out;
    out.L = a.L + (b.L - a.L) * u;
    out.C = a.C + (b.C - a.C) * u;
    out.h = ha + delta * u;
    if (out.h < 0.0) out.h += 360.0;
    if (out.h >= 360.0) out.h -= 360.0;
    return out;
}

// The distance of two colours. A distance below 0.1 is a weak contrast.
inline double labDistance(const Lab &x, const Lab &y)
{
    return std::sqrt((x.L - y.L) * (x.L - y.L) + (x.a - y.a) * (x.a - y.a) + (x.b - y.b) * (x.b - y.b));
}

}

#endif // GUI_OKLAB_H
