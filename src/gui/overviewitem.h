#ifndef GUI_OVERVIEW_ITEM_H
#define GUI_OVERVIEW_ITEM_H

#include <chrono>
#include <vector>

#include <QQuickPaintedItem>
#include <QTimer>

#include "maplayout.h"
#include "theme.h"

namespace Main {
    class Config;
    class DataStore;
    class SessionWrapper;
}

namespace Gui {

    // A map of the session. It shows the loudness and the pitch of a part of the session, and a frame
    // around the part that the main view shows. A click or a drag in the map moves the main view. A
    // drag of an edge of the frame changes the time that the main view shows. A strip under the map
    // scrolls the map when the session is longer than the map.
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
        void hoverMoveEvent(QHoverEvent *event) override;
        void hoverLeaveEvent(QHoverEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

    private:
        // What a press in the map starts.
        enum class Drag { None, Move, LeftEdge, RightEdge, Scrollbar };

        void refresh();
        void rebuildCache(const MapView &view, long long binSamples, const MapBins &bins);

        // The time of the newest entry of the pitch track. The analysis of a file adds entries after the sound is complete.
        double pitchEnd() const;

        // The length of the session, and the part of it that the map shows. A drag keeps the part
        // that the map showed at the press, so the map does not move under the pointer.
        double total() const;
        MapView currentView();
        double timeAt(double x) const;
        double xAt(double time) const;

        // The pixels of the frame, and of the strip under the map.
        void frameEdges(double &left, double &right) const;
        // False when the frame is outside the part of the session that the map shows.
        bool frameVisible() const;
        int stripHeight() const;
        Drag hitTest(double x, double y) const;

        // Move the end of the main view to a time.
        void moveEnd(double end);

        QTimer mTimer;

        // The values that the picture shows. The map draws again when one of them changes.
        QString mKey;

        // The map: the state, the part of the session that it shows now, and a copy during a drag.
        MapState mState;
        MapView mView;
        MapView mDragView;
        Drag mDrag;
        double mDragOffset;
        double mFixedTime;

        // The loudness in each bin, and the pitch in Hz (0 for none).
        std::vector<float> mLow, mHigh, mPitch;
        MapBins mCacheBins;
        long long mCacheBinSamples;
        double mCachePitchEnd;
        unsigned long long mCacheVersion;
        std::chrono::steady_clock::time_point mCacheTime;

        // The scale of the loudness and of the pitch. Both change slowly, so the picture keeps its look
        // while new sound arrives.
        float mPeak;
        double mPitchLow, mPitchHigh;

        QString mThemeKey;
        Theme mTheme;
    };

}

#endif // GUI_OVERVIEW_ITEM_H
