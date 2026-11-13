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

        // The length of the session for the map. While a capture runs, the length grows with the clock
        // between two blocks of sound, so the map moves in small steps and not in one step for each block.
        double shownDuration();

        // The time of the newest entry of the pitch track. The analysis of a file adds entries after the sound is complete.
        double pitchEnd() const;

        // The length of the session, and the part of it that the map shows. A drag keeps the part
        // that the map showed at the press, so the map does not move under the pointer.
        double total() const;
        // The end of the frame of the main view, in the time of the map.
        double frameEnd() const;
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
        struct Key {
            unsigned long long version = 0;
            int width = 0, height = 0;
            double start = 0, span = 0, windowEnd = 0, viewSpan = 0, duration = 0;
            bool live = false, follow = false;
            QString theme;
            bool operator==(const Key &other) const {
                return version == other.version && width == other.width && height == other.height && start == other.start
                    && span == other.span && windowEnd == other.windowEnd && viewSpan == other.viewSpan
                    && duration == other.duration && live == other.live && follow == other.follow && theme == other.theme;
            }
        };
        Key mKey;

        // The time of the last block of sound, and the length that the map shows now.
        double mShownDuration;
        Gui::MapClock mClock;

        // True when the sound or the pitch is newer than the cache. The check runs a few times in a second.
        bool mSoundDirty;
        std::chrono::steady_clock::time_point mSoundCheck;

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
