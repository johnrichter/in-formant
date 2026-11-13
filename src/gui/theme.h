#ifndef GUI_THEME_H
#define GUI_THEME_H

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <vector>

namespace Gui {

    // A color theme sets the colors of the spectrogram, the tracks, the oscilloscope, and the
    // synthesizer. Each theme works on a dark background.
    struct Theme {
        QString name;
        QString group;

        // 256 colors from silence to the loudest sound.
        QVector<QRgb> colormap;

        QColor pitch;
        QColor formants[5];

        QColor accent;
        QColor oscilloscope;
        QColor oscilloscopeGrid;
        QColor synthSource;
        QColor synthFilter;

        QColor formant(int i) const { return formants[i < 5 ? i : 4]; }
    };

    // The names of all themes, in the order of the theme list. The last name is "Custom".
    QStringList themeNames();

    // The group of a theme: "Scientific" or "Pride".
    QString themeGroup(const QString &name);

    // The theme with this name. An unknown name gives the default theme. The custom theme uses the
    // three strings: the colors of the spectrogram from low to high, the color of the pitch track,
    // and the colors of the formant tracks. A color has the form #rrggbb. Commas separate colors.
    // The saturation scales the chroma of the color map. A value of 1 leaves the colors as they are.
    // When minVolumeBlack is false, the quiet end of the map has a dark tint of the map's hue and not black.
    Theme resolveTheme(const QString &name,
                       const QString &customColors = QString(),
                       const QString &customPitch = QString(),
                       const QString &customFormants = QString(),
                       double saturation = 1.0,
                       bool minVolumeBlack = true);

    // The theme with its original color map, where the quietest color is black or as the theme defines it.
    Theme resolveThemeBlack(const QString &name, const QString &customColors, const QString &customPitch,
                            const QString &customFormants, double saturation);

    // Some evenly spaced colors of the color map, for a preview.
    QVariantList themePreview(const QString &name, int count, double saturation = 1.0, bool minVolumeBlack = true);

    // The colors of the theme for the QML windows.
    QVariantMap themeColorMap(const Theme &theme);

}

#endif // GUI_THEME_H
