#ifndef GUI_THEME_H
#define GUI_THEME_H

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <vector>

namespace Gui {

    // A colour theme sets the colours of the spectrogram, the tracks, the oscilloscope, and the
    // synthesizer. Each theme works on a dark background.
    struct Theme {
        QString name;
        QString group;

        // 256 colours from silence to the loudest sound.
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
    // three strings: the colours of the spectrogram from low to high, the colour of the pitch track,
    // and the colours of the formant tracks. A colour has the form #rrggbb. Commas separate colours.
    // The saturation scales the chroma of the colour map. A value of 1 leaves the colours as they are.
    Theme resolveTheme(const QString &name,
                       const QString &customColors = QString(),
                       const QString &customPitch = QString(),
                       const QString &customFormants = QString(),
                       double saturation = 1.0);

    // Some evenly spaced colours of the colour map, for a preview.
    QVariantList themePreview(const QString &name, int count, double saturation = 1.0);

    // The colours of the theme for the QML windows.
    QVariantMap themeColorMap(const Theme &theme);

}

#endif // GUI_THEME_H
