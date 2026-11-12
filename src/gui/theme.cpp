#include "theme.h"
#include "qpainterwrapper.h"
#include <algorithm>
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

// Make 256 colours from the stops. The colours change in a straight line between the stops.
QVector<QRgb> buildColormap(const std::vector<Stop> &stops)
{
    QVector<QRgb> map(256);
    for (int i = 0; i < 256; ++i) {
        const double t = i / 255.0;

        size_t k = 1;
        while (k < stops.size() - 1 && stops[k].position < t) {
            ++k;
        }

        const Stop &a = stops[k - 1];
        const Stop &b = stops[k];
        const double span = std::max(b.position - a.position, 1e-9);
        const double u = std::clamp((t - a.position) / span, 0.0, 1.0);

        auto channel = [&](int shift) {
            const double ca = (a.rgb >> shift) & 0xff;
            const double cb = (b.rgb >> shift) & 0xff;
            return (int) std::lround(ca + (cb - ca) * u);
        };

        map[i] = qRgb(channel(16), channel(8), channel(0));
    }
    return map;
}

struct Definition {
    const char *name;
    const char *group;
    std::vector<Stop> stops;
    unsigned int pitch;
    unsigned int formants[5];
    unsigned int accent;
};

// The colours of the pride themes are darker than the flag colours. A flag colour at full brightness
// is too bright for a dark window, so each theme keeps the hue and lowers the brightness.
const std::vector<Definition> &definitions()
{
    static const std::vector<Definition> defs = {
        // Scientific themes. The first one uses the colour map of the original application.
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

        // Pride themes.
        { "Trans (Dark)", "Pride",
            { {0.00, 0x05070d}, {0.22, 0x102a45}, {0.42, 0x2f6a93}, {0.62, 0xb36e8c}, {0.80, 0xdcaebf}, {1.00, 0xefe9ec} },
            0xd889a6, { 0x5aa5c8, 0xd889a6, 0xd9d2d6, 0x7f9fb8, 0xb87a95 }, 0x5aa5c8 },
        { "Transfeminine (Dark)", "Pride",
            { {0.00, 0x0a0609}, {0.25, 0x4a1a38}, {0.50, 0xa85a82}, {0.75, 0xd8a0b8}, {1.00, 0xf6eaf0} },
            0xd889a6, { 0xe0a0b8, 0xc8789a, 0xf0d0dc, 0xa85a82, 0xe8b8c8 }, 0xc8789a },
        { "Transmasculine (Dark)", "Pride",
            { {0.00, 0x05070d}, {0.25, 0x0e3a66}, {0.50, 0x3f86ab}, {0.75, 0x9cc8de}, {1.00, 0xeef4f6} },
            0x5aa5c8, { 0x7fb0d0, 0xc8dde8, 0x3f86ab, 0xa0c0d8, 0xe0eef4 }, 0x3f86ab },
        { "Rainbow (Dark)", "Pride",
            { {0.00, 0x06060c}, {0.15, 0x3b1d5a}, {0.33, 0x2f4f9c}, {0.50, 0x2f8f5a}, {0.66, 0xc9b23a},
              {0.83, 0xd8742e}, {1.00, 0xf0e6dc} },
            0xe05a5a, { 0xe8873a, 0xd6c34a, 0x4fae6e, 0x5a7fd0, 0x9a6bc0 }, 0x9a6bc0 },
        { "Progress Pride (Dark)", "Pride",
            { {0.00, 0x060509}, {0.14, 0x4e3322}, {0.30, 0x3f86ab}, {0.46, 0xbf7592}, {0.62, 0xd8cfd3},
              {0.78, 0xc9b23a}, {1.00, 0xf4efe0} },
            0xe05a5a, { 0x5aa5c8, 0xd889a6, 0xe8873a, 0x4fae6e, 0x9a6bc0 }, 0xd889a6 },
        { "Bisexual (Dark)", "Pride",
            { {0.00, 0x070510}, {0.22, 0x1c2a6e}, {0.45, 0x5b3d8e}, {0.68, 0xb0306f}, {0.85, 0xd98ab4}, {1.00, 0xf2e6ee} },
            0xd9609a, { 0x4f78d0, 0x9a6bc0, 0xd9609a, 0x7aa0e0, 0xc58ad0 }, 0x9a6bc0 },
        { "Pansexual (Dark)", "Pride",
            { {0.00, 0x05080e}, {0.22, 0x164a78}, {0.45, 0x3a8fc4}, {0.68, 0xc93c80}, {0.85, 0xd9b04a}, {1.00, 0xf6eed2} },
            0xe0508f, { 0x3a9fd4, 0xd9b04a, 0xe0508f, 0x6ac0e8, 0xe8c870 }, 0xc93c80 },
        { "Lesbian (Dark)", "Pride",
            { {0.00, 0x0a0508}, {0.20, 0x6e1d12}, {0.38, 0xb24a1c}, {0.55, 0xd98a5a}, {0.72, 0xd8c6c0},
              {0.86, 0xb8639a}, {1.00, 0xf0e4ec} },
            0xe0905a, { 0xc0507e, 0xd8c6c0, 0xb24a1c, 0xa0457e, 0xe8a070 }, 0xb24a1c },
        { "Gay MLM (Dark)", "Pride",
            { {0.00, 0x04070a}, {0.20, 0x0a4a44}, {0.40, 0x1f9a80}, {0.58, 0x7ac4a8}, {0.75, 0x8fb4d8},
              {0.90, 0x5a56b8}, {1.00, 0xeef2f4} },
            0x3fbf9f, { 0x7aaad8, 0x8a7ad8, 0xcfe8dc, 0x5fd0b0, 0x6a62c8 }, 0x1f9a80 },
        { "Non-binary (Dark)", "Pride",
            { {0.00, 0x050505}, {0.20, 0x2a1a44}, {0.42, 0x7a4aa8}, {0.62, 0xb8a0d0}, {0.80, 0xcfc84a}, {1.00, 0xf4f2d8} },
            0xd6cc3a, { 0xa070d0, 0xe0e0e0, 0x8a8a8a, 0xc8b0e0, 0xe8e070 }, 0x7a4aa8 },
        { "Asexual (Dark)", "Pride",
            { {0.00, 0x040404}, {0.25, 0x2a2a2a}, {0.45, 0x5a2a6a}, {0.65, 0x8a8a8a}, {0.85, 0xbbb0c4}, {1.00, 0xf2f0f4} },
            0xb070c0, { 0xa0a0a0, 0x7a3a8a, 0xd0d0d0, 0xc090d0, 0x707070 }, 0x5a2a6a },
        { "Aromantic (Dark)", "Pride",
            { {0.00, 0x040604}, {0.22, 0x1c4a22}, {0.42, 0x3d8a44}, {0.62, 0x98c070}, {0.80, 0xb8b8b8}, {1.00, 0xf2f4ee} },
            0x5fb860, { 0xa7d379, 0xcfcfcf, 0x3d8a44, 0x8aa070, 0xe0e8d8 }, 0x3d8a44 },
        { "Genderfluid (Dark)", "Pride",
            { {0.00, 0x040408}, {0.20, 0x1c2268}, {0.40, 0x6a2a8a}, {0.60, 0xc95a80}, {0.80, 0xd8b8c8}, {1.00, 0xf4eef2} },
            0xd06a92, { 0x6a72d0, 0xb050c0, 0xe0d0d8, 0x8a2a9a, 0x4a52b0 }, 0x6a2a8a },
        { "Genderqueer (Dark)", "Pride",
            { {0.00, 0x05060a}, {0.20, 0x264012}, {0.42, 0x3f7020}, {0.62, 0x9a78c0}, {0.82, 0xd0c4e0}, {1.00, 0xf4f2f6} },
            0xa888d0, { 0x5a9a38, 0xe0d8e8, 0x7a5aa0, 0x8fbf6a, 0xc0a8e0 }, 0x7a5aa0 },
        { "Agender (Dark)", "Pride",
            { {0.00, 0x040404}, {0.20, 0x2a2a2a}, {0.40, 0x6a6a6a}, {0.60, 0x7ab060}, {0.80, 0xd0d0d0}, {1.00, 0xf2f2f2} },
            0x7ab060, { 0xb0b0b0, 0x5a8a48, 0xe0e0e0, 0x8a8a8a, 0xa0d080 }, 0x5a8a48 },
        { "Intersex (Dark)", "Pride",
            { {0.00, 0x05030a}, {0.25, 0x3a0f66}, {0.50, 0x7a2aa8}, {0.72, 0xc9a42a}, {0.88, 0xe8d060}, {1.00, 0xf8f0d0} },
            0xd9b030, { 0x9a50c8, 0xe8d060, 0x6a2a98, 0xc8a8e0, 0xf0e0a0 }, 0x7a2aa8 },
    };
    return defs;
}

Theme fromDefinition(const Definition &d)
{
    Theme t;
    t.name = d.name;
    t.group = d.group;

    if (d.stops.empty()) {
        t.colormap = QPainterWrapper::defaultColorMap();
    }
    else {
        t.colormap = buildColormap(d.stops);
    }

    t.pitch = hex(d.pitch);
    for (int i = 0; i < 5; ++i) {
        t.formants[i] = hex(d.formants[i]);
    }
    t.accent = hex(d.accent);

    // The oscilloscope and the synthesizer use the pitch colour and the first formant colour, so that
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
                        const QString &customPitch, const QString &customFormants)
{
    if (name == "Custom") {
        // The base is the default theme. Each custom value that is valid replaces the default value.
        Theme t = fromDefinition(definitions().front());
        t.name = "Custom";
        t.group = "Custom";

        const auto colors = parseColors(customColors);
        if (colors.size() >= 2) {
            std::vector<Stop> stops;
            for (int i = 0; i < colors.size(); ++i) {
                stops.push_back({ i / double(colors.size() - 1), (unsigned) (colors[i].rgb() & 0xffffff) });
            }
            t.colormap = buildColormap(stops);
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
            return fromDefinition(d);
        }
    }
    return fromDefinition(definitions().front());
}

QVariantList Gui::themePreview(const QString &name, int count)
{
    const Theme t = resolveTheme(name);
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
