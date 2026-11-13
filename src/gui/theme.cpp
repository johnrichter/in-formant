#include "theme.h"
#include "oklab.h"
#include "quietlift.h"
#include "qpainterwrapper.h"
#include <algorithm>
#include <array>
#include <cmath>

using namespace Gui;

namespace {

struct Stop {
    double position;
    unsigned int rgb;
};

QColor hex(unsigned int rgb)
{
    return QColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

// Make 256 colors from the stops. The colors change in OKLab, so a gradient has no gray patch.
// The saturation scales the chroma of each color. A value of 1 leaves the colors as they are.
QVector<QRgb> buildColormap(const std::vector<Stop> &stops, double saturation = 1.0)
{
    std::vector<Lch> lch;
    for (const Stop &stop : stops) {
        lch.push_back(labToLch(rgbToLab(stop.rgb)));
    }

    std::vector<Lch> wanted(256);
    for (int i = 0; i < 256; ++i) {
        const double t = i / 255.0;

        size_t k = 1;
        while (k < stops.size() - 1 && stops[k].position < t) {
            ++k;
        }

        const double span = std::max(stops[k].position - stops[k - 1].position, 1e-9);
        const double u = std::clamp((t - stops[k - 1].position) / span, 0.0, 1.0);

        wanted[i] = mixLch(lch[k - 1], lch[k], u);
        wanted[i].C *= saturation;
    }

    // A color that is outside the gamut loses chroma, and the loss changes with the hue. A stop
    // that fits keeps all its chroma, and the colors next to it do not. The stop then shows as a
    // thin line that is too vivid. The chroma of the map must change smoothly, so a stop never has
    // more chroma than the average of its neighbors.
    std::vector<double> feasible(256);
    for (int i = 0; i < 256; ++i) {
        feasible[i] = labToLch(rgbToLab(labToRgb(lchToLab(wanted[i])))).C;
    }

    constexpr int reach = 8;
    QVector<QRgb> map(256);
    for (int i = 0; i < 256; ++i) {
        double sum = 0.0;
        int count = 0;
        for (int j = std::max(0, i - reach); j <= std::min(255, i + reach); ++j) {
            sum += feasible[j];
            ++count;
        }
        Lch c = wanted[i];
        c.C = std::min(c.C, sum / count);
        map[i] = 0xff000000u | labToRgb(lchToLab(c));
    }
    return map;
}

// Make the colors of a table that is not in OKLab, for example the first theme, more or less saturated.
void saturate(QVector<QRgb> &map, double saturation)
{
    if (std::fabs(saturation - 1.0) < 1e-6) return;
    for (QRgb &c : map) {
        Lch lch = labToLch(rgbToLab(c & 0xffffff));
        lch.C *= saturation;
        c = 0xff000000u | labToRgb(lchToLab(lch));
    }
}

// A theme of seven stops. The lightness goes up from 7 percent to 86 percent in fixed steps. It rises fast at first, because most of a voice sound is in the lower half of the map. Each
// stop sets the chroma and the hue. The top stop keeps a hue, and no stop is white.
struct Tone {
    double chroma;
    double hue;
};

std::vector<Stop> ramp(const std::array<Tone, 7> &tones, const std::array<double, 7> &lightness = { 0.07, 0.28, 0.48, 0.64, 0.75, 0.82, 0.86 })
{
    static const double positions[7] = { 0.00, 0.18, 0.38, 0.58, 0.76, 0.90, 1.00 };
    std::vector<Stop> stops;
    for (int i = 0; i < 7; ++i) {
        stops.push_back({ positions[i], labToRgb(lchToLab(lightness[i], tones[i].chroma, tones[i].hue)) });
    }
    return stops;
}

struct Definition {
    const char *name;
    const char *group;
    std::vector<Stop> stops;
    unsigned int pitch;
    unsigned int formants[5];
    unsigned int accent;
};

// The pride themes use the flag colors at full chroma. The lightness goes up in steps from near black
// to a light tint, and no stop is white. A loud sound then keeps a hue, and a quiet sound is dark.
const std::vector<Definition> &definitions()
{
    static const std::vector<Definition> defs = {
        // Scientific themes. The first one uses the color map of the original application.
        { "Magma", "Scientific", {}, 0x00ffff,
            { 0x00ff00, 0x91ee91, 0xff0000, 0xff99ff, 0xffcc00 }, 0x7c4dff },
        { "Viridis", "Scientific",
            { {0.00, 0x440154}, {0.11, 0x482878}, {0.22, 0x3e4989}, {0.33, 0x31688e}, {0.44, 0x26828e},
              {0.55, 0x1f9e89}, {0.66, 0x35b779}, {0.77, 0x6ece58}, {0.88, 0xb5de2b}, {1.00, 0xfde725} },
            0xff7a59, { 0xffb347, 0xff6b9d, 0xffffff, 0x9d8cff, 0xff9d6b }, 0x35b779 },
        { "Inferno", "Scientific",
            { {0.00, 0x000004}, {0.11, 0x1b0c41}, {0.22, 0x4a0c6b}, {0.33, 0x781c6d}, {0.44, 0xa52c60},
              {0.55, 0xcf4446}, {0.66, 0xed6925}, {0.77, 0xfb9b06}, {0.88, 0xf7d13d}, {1.00, 0xfcffa4} },
            0x00e5ff, { 0x7cffb2, 0x6fa8ff, 0xffffff, 0xc58bff, 0x7cffe9 }, 0xed6925 },
        { "Grayscale", "Scientific",
            { {0.00, 0x000000}, {1.00, 0xffffff} },
            0x00e5ff, { 0xffb000, 0xff5c8a, 0x7cff6b, 0xb28bff, 0xff8c42 }, 0x9e9e9e },
        { "Ocean", "Scientific",
            { {0.00, 0x000814}, {0.18, 0x001d3d}, {0.38, 0x003566}, {0.56, 0x0a5a8a}, {0.72, 0x1b85b8},
              {0.86, 0x5bb8d4}, {1.00, 0xf0fbff} },
            0xffb347, { 0xff7a7a, 0x9dff8a, 0xffe66d, 0xd1a3ff, 0xff9d6b }, 0x1b85b8 },
        { "Ember", "Scientific",
            { {0.00, 0x050000}, {0.20, 0x3d0a00}, {0.42, 0x8a1c00}, {0.62, 0xd94a00}, {0.80, 0xff9a1f},
              {0.92, 0xffe08a}, {1.00, 0xfff8e0} },
            0x4dd0e1, { 0x7cff9a, 0x82b1ff, 0xffffff, 0xe1a8ff, 0xb0ff7c }, 0xd94a00 },

        // Pride themes. A pitch color of 0 means that the tracks take their colors from the table.
        { "Trans (Dark)", "Pride", ramp({ { {0.03, 260}, {0.09, 255}, {0.14, 245}, {0.11, 225}, {0.19, 355}, {0.20, 350}, {0.17, 348} } },
                                                    // The top stops stay darker than the other themes, because a pink that is vivid needs less light.
                                                    { 0.07, 0.28, 0.48, 0.64, 0.72, 0.75, 0.79 }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Transfeminine (Dark)", "Pride", ramp({ { {0.03, 340}, {0.09, 340}, {0.15, 350}, {0.14, 358}, {0.1, 5}, {0.06, 10}, {0.03, 350} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Transmasculine (Dark)", "Pride", ramp({ { {0.03, 260}, {0.1, 262}, {0.15, 255}, {0.12, 240}, {0.09, 225}, {0.05, 225}, {0.03, 230} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Rainbow (Dark)", "Pride", ramp({ { {0.03, 300}, {0.1, 305}, {0.12, 265}, {0.13, 150}, {0.15, 100}, {0.12, 60}, {0.04, 60} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Progress Pride (Dark)", "Pride", ramp({ { {0.02, 40}, {0.06, 45}, {0.12, 255}, {0.13, 355}, {0.14, 95}, {0.1, 60}, {0.03, 80} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Bisexual (Dark)", "Pride", ramp({ { {0.03, 275}, {0.12, 275}, {0.17, 300}, {0.17, 350}, {0.12, 350}, {0.07, 340}, {0.03, 340} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Pansexual (Dark)", "Pride", ramp({ { {0.03, 250}, {0.1, 250}, {0.14, 235}, {0.17, 355}, {0.15, 95}, {0.09, 95}, {0.03, 95} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Lesbian (Dark)", "Pride", ramp({ { {0.03, 30}, {0.1, 30}, {0.15, 40}, {0.14, 55}, {0.05, 40}, {0.11, 350}, {0.04, 350} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Gay MLM (Dark)", "Pride", ramp({ { {0.03, 200}, {0.07, 195}, {0.1, 190}, {0.12, 170}, {0.1, 230}, {0.08, 265}, {0.03, 265} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Non-binary (Dark)", "Pride", ramp({ { {0.01, 300}, {0.07, 300}, {0.14, 305}, {0.07, 300}, {0.03, 300}, {0.12, 100}, {0.05, 100} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Asexual (Dark)", "Pride", ramp({ { {0.0, 0}, {0.0, 0}, {0.0, 0}, {0.13, 310}, {0.14, 310}, {0.08, 310}, {0.04, 310} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Aromantic (Dark)", "Pride", ramp({ { {0.02, 150}, {0.07, 150}, {0.13, 145}, {0.15, 140}, {0.12, 130}, {0.05, 130}, {0.03, 130} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Genderfluid (Dark)", "Pride", ramp({ { {0.03, 265}, {0.1, 265}, {0.16, 285}, {0.15, 330}, {0.13, 355}, {0.07, 355}, {0.03, 355} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Genderqueer (Dark)", "Pride", ramp({ { {0.03, 145}, {0.08, 145}, {0.12, 145}, {0.14, 140}, {0.1, 305}, {0.06, 305}, {0.02, 305} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Agender (Dark)", "Pride", ramp({ { {0.0, 140}, {0.02, 140}, {0.05, 140}, {0.14, 145}, {0.14, 140}, {0.08, 140}, {0.03, 140} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
        { "Intersex (Dark)", "Pride", ramp({ { {0.03, 300}, {0.1, 305}, {0.17, 305}, {0.14, 300}, {0.15, 100}, {0.11, 100}, {0.04, 100} } }), 0, { 0, 0, 0, 0, 0 }, 0 },
    };
    return defs;
}

// The colors of the tracks for a table with no hand-picked colors. The pitch has the hue that is
// opposite to the loud part of the table. The five formants have hues that differ from the pitch
// and from the loud part, and the lightness alternates, so that neighbors differ in two ways.
void autoTracks(Theme &t)
{
    double a = 0.0;
    double b = 0.0;
    for (int i = 150; i < 235; ++i) {
        const Lab lab = rgbToLab(t.colormap[i] & 0xffffff);
        a += lab.a;
        b += lab.b;
    }
    double hue = 250.0;
    if (std::hypot(a, b) / 85.0 > 0.015) {
        hue = std::atan2(b, a) * 180.0 / M_PI;
    }

    const double pitchHue = hue + 180.0;
    const double offsets[5] = { 120.0, -120.0, 60.0, -60.0, 150.0 };
    const double lightness[5] = { 0.80, 0.86, 0.76, 0.84, 0.78 };

    auto color = [](double L, double C, double h) {
        const Lab lab = lchToLab(L, C, h);
        const unsigned int rgb = labToRgb(lab);
        return QColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
    };

    t.pitch = color(0.88, 0.14, pitchHue);
    for (int i = 0; i < 5; ++i) {
        t.formants[i] = color(lightness[i], 0.17, pitchHue + offsets[i]);
    }
    t.accent = QColor::fromRgb(t.colormap[100]);
}

Theme fromDefinition(const Definition &d, double saturation)
{
    Theme t;
    t.name = d.name;
    t.group = d.group;

    if (d.stops.empty()) {
        t.colormap = QPainterWrapper::defaultColorMap();
        saturate(t.colormap, saturation);
    }
    else {
        t.colormap = buildColormap(d.stops, saturation);
    }

    if (d.pitch == 0) {
        autoTracks(t);
    }
    else {
        t.pitch = hex(d.pitch);
        for (int i = 0; i < 5; ++i) {
            t.formants[i] = hex(d.formants[i]);
        }
        t.accent = hex(d.accent);
    }

    // The oscilloscope and the synthesizer use the pitch color and the first formant color, so that
    // each window matches the spectrogram.
    t.oscilloscope = t.pitch;
    t.oscilloscopeGrid = QColor(255, 255, 255, 40);
    t.synthSource = t.pitch;
    t.synthFilter = t.formants[0];
    return t;
}

QList<QColor> parseColors(const QString &text)
{
    QList<QColor> colors;
    const auto parts = text.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QColor c(part.trimmed());
        if (c.isValid()) {
            colors.push_back(c);
        }
    }
    return colors;
}

}

QStringList Gui::themeNames()
{
    QStringList names;
    for (const auto &d : definitions()) {
        names << d.name;
    }
    names << "Custom";
    return names;
}

QString Gui::themeGroup(const QString &name)
{
    for (const auto &d : definitions()) {
        if (name == d.name) {
            return d.group;
        }
    }
    return "Custom";
}

Theme Gui::resolveTheme(const QString &name, const QString &customColors,
                        const QString &customPitch, const QString &customFormants, double saturation,
                        bool minVolumeBlack)
{
    Theme theme = resolveThemeBlack(name, customColors, customPitch, customFormants, saturation);
    if (!minVolumeBlack) {
        liftQuietEnd(theme.colormap);
    }
    return theme;
}

Theme Gui::resolveThemeBlack(const QString &name, const QString &customColors,
                             const QString &customPitch, const QString &customFormants, double saturation)
{
    if (name == "Custom") {
        // The base is the default theme. Each custom value that is valid replaces the default value.
        Theme t = fromDefinition(definitions().front(), saturation);
        t.name = "Custom";
        t.group = "Custom";

        const auto colors = parseColors(customColors);
        if (colors.size() >= 2) {
            std::vector<Stop> stops;
            for (int i = 0; i < colors.size(); ++i) {
                stops.push_back({ i / double(colors.size() - 1), (unsigned) (colors[i].rgb() & 0xffffff) });
            }
            t.colormap = buildColormap(stops, saturation);
        }

        const QColor pitch(customPitch.trimmed());
        if (pitch.isValid()) {
            t.pitch = pitch;
            t.oscilloscope = pitch;
            t.synthSource = pitch;
        }

        const auto formants = parseColors(customFormants);
        for (int i = 0; i < 5 && !formants.isEmpty(); ++i) {
            t.formants[i] = formants[std::min<int>(i, formants.size() - 1)];
        }
        t.synthFilter = t.formants[0];
        return t;
    }

    for (const auto &d : definitions()) {
        if (name == d.name) {
            return fromDefinition(d, saturation);
        }
    }
    return fromDefinition(definitions().front(), saturation);
}

QVariantList Gui::themePreview(const QString &name, int count, double saturation, bool minVolumeBlack)
{
    const Theme t = resolveTheme(name, QString(), QString(), QString(), saturation, minVolumeBlack);
    QVariantList list;
    for (int i = 0; i < count; ++i) {
        const int index = count > 1 ? (i * 255) / (count - 1) : 0;
        list << QColor::fromRgb(t.colormap[index]);
    }
    return list;
}

QVariantMap Gui::themeColorMap(const Theme &theme)
{
    QVariantMap m;
    m["name"] = theme.name;
    m["accent"] = theme.accent;
    m["pitch"] = theme.pitch;
    QVariantList formants;
    for (int i = 0; i < 5; ++i) {
        formants << theme.formants[i];
    }
    m["formants"] = formants;
    m["oscilloscope"] = theme.oscilloscope;
    m["oscilloscopeGrid"] = theme.oscilloscopeGrid;
    m["synthSource"] = theme.synthSource;
    m["synthFilter"] = theme.synthFilter;
    return m;
}
