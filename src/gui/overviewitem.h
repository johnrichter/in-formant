#ifndef GUI_OVERVIEW_ITEM_H
#define GUI_OVERVIEW_ITEM_H

#include <chrono>
#include <vector>

#include <QQuickPaintedItem>
#include <QTimer>

#include "theme.h"

namespace Main {
    class Config;
    class DataStore;
    class SessionWrapper;
}

namespace Gui {

    // A map of the whole session. It shows the loudness and the pitch from the start to the end of the
    // sound, and a frame around the part that the main view shows. A click or a drag in the map moves
    // the main view to another time of the session.
    class OverviewItem : public QQuickPaintedItem {
        Q_OBJECT

    public:
        OverviewItem(QQuickItem *parent = nullptr);

        void paint(QPainter *painter) override;

        // The objects that the map reads. The caller keeps them alive.
        static void setSources(Main::Config *config, Main::SessionWrapper *session);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

    private:
        void refresh();
        void rebuildCache(int width, double total);

        // The time of the newest entry of the pitch track. The analysis of a file adds entries after the sound is complete.
        double pitchEnd() const;

        // The length of the session shown by the map, and the time of a position in the map.
        double total() const;
        double timeAt(double x) const;

        // Move the end of the main view to a time.
        void moveEnd(double end);

        QTimer mTimer;

        // The values that the picture shows. The map draws again when one of them changes.
        QString mKey;

        // The loudness in each pixel column, and the pitch from 0 (low) to 1 (high), or less than 0 for none.
        std::vector<float> mLow, mHigh, mPitch;
        int mCacheWidth;
        double mCacheTotal;
        double mCachePitchEnd;
        unsigned long long mCacheVersion;
        std::chrono::steady_clock::time_point mCacheTime;

        QString mThemeKey;
        Theme mTheme;

        // The offset between the pointer and the end of the main view during a drag.
        bool mDragging;
        double mDragOffset;
    };

}

#endif // GUI_OVERVIEW_ITEM_H
