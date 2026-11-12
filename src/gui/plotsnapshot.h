#ifndef GUI_PLOT_SNAPSHOT_H
#define GUI_PLOT_SNAPSHOT_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <vector>

namespace Gui {

    // One line of a plot.
    struct PlotSeries {
        QString name;

        // Which colour of the theme draws the line: 0 is the main colour, 1 is the second colour.
        int colorIndex = 0;

        QVector<QPointF> points;
    };

    // All data that a plot needs to draw one frame. The provider of the data fills it in the GUI
    // thread, and the renderer reads it in the render thread.
    struct PlotSnapshot {
        QString title;
        QString xUnit;
        QString yUnit;

        // The horizontal axis uses a logarithmic scale.
        bool logX = false;

        double xMin = 0, xMax = 1;
        double yMin = -1, yMax = 1;

        std::vector<PlotSeries> series;

        // The value changes when the data changes.
        unsigned long long version = 0;
    };

}

#endif // GUI_PLOT_SNAPSHOT_H
