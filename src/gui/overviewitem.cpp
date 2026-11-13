#include "overviewitem.h"

#include <algorithm>
#include <cmath>

#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include "../context/config.h"
#include "../context/datastore.h"
#include "../context/session.h"
#include "../context/sessionwrapper.h"

using namespace Gui;

namespace {

Main::Config *sConfig = nullptr;
Main::DataStore *sDataStore = nullptr;
Main::SessionWrapper *sSession = nullptr;

constexpr int sampleRate = Main::Session::sampleRate;

// The distance from an edge of the frame in which a press takes the edge, in pixels.
constexpr double edgeZone = 6.0;
// The height of the strip under the map, in pixels.
constexpr int scrollStrip = 9;

QString mapTimeText(double seconds)
{
    const long long s = (long long) std::llround(std::max(0.0, seconds));
    const long long h = s / 3600, m = (s / 60) % 60, r = s % 60;
    if (h > 0) return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m).arg(r, 2, 10, QChar('0'));
}

}

OverviewItem::OverviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      mDrag(Drag::None),
      mDragOffset(0),
      mFixedTime(0),
      mCacheBinSamples(0),
      mCachePitchEnd(-1),
      mCacheVersion(0),
      mLastDuration(-1.0),
      mShownDuration(1.0),
      mSoundDirty(true),
      mPeak(0.02f),
      mPitchLow(60.0),
      mPitchHigh(600.0)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);

    // One check for each frame of the screen. The check is cheap, and the map draws only when a value changed.
    mTimer.setInterval(16);
    mTimer.setTimerType(Qt::PreciseTimer);
    connect(&mTimer, &QTimer::timeout, this, &OverviewItem::refresh);
    mTimer.start();
}

void OverviewItem::setSources(Main::Config *config, Main::SessionWrapper *session)
{
    sConfig = config;
    sSession = session;
    sDataStore = session->dataStore();
}

double OverviewItem::shownDuration()
{
    if (sDataStore == nullptr) return 1.0;

    const double duration = sDataStore->session().duration();
    const auto now = std::chrono::steady_clock::now();
    if (duration != mLastDuration) {
        mLastDuration = duration;
        mDurationStamp = now;
    }

    const bool running = sConfig != nullptr && sConfig->getSessionMode() == 0 && !sConfig->isPaused() && sConfig->getViewLive();
    double shown = duration;
    if (running) {
        // The sound arrives in blocks. Between two blocks, the length grows with the clock, up to a short limit.
        shown += std::min(std::chrono::duration<double>(now - mDurationStamp).count(), 0.25);
    }
    mShownDuration = std::max(shown, 1.0);
    return mShownDuration;
}

double OverviewItem::total() const
{
    return mShownDuration;
}

MapView OverviewItem::currentView()
{
    if (mDrag == Drag::Move || mDrag == Drag::LeftEdge || mDrag == Drag::RightEdge) return mDragView;

    const bool live = sConfig->getSessionMode() == 0 && sConfig->getViewLive();
    mView = updateMapView(mState, shownDuration(), sConfig->optionDouble("mapWindow"),
                          sConfig->getViewTimeSpan(), sSession->windowEnd(), live);
    return mView;
}

int OverviewItem::stripHeight() const
{
    // The strip shows when the map shows a part of the session.
    return mView.span < total() - 1e-9 ? scrollStrip : 0;
}

double OverviewItem::timeAt(double x) const
{
    const MapView &view = (mDrag == Drag::None || mDrag == Drag::Scrollbar) ? mView : mDragView;
    const double w = std::max(width(), 1.0);
    return view.start + std::clamp(x / w, 0.0, 1.0) * view.span;
}

double OverviewItem::xAt(double time) const
{
    const MapView &view = (mDrag == Drag::None || mDrag == Drag::Scrollbar) ? mView : mDragView;
    return (time - view.start) / view.span * std::max(width(), 1.0);
}

void OverviewItem::frameEdges(double &left, double &right) const
{
    const double w = width();
    const double end = sSession->windowEnd();
    const double span = sConfig->getViewTimeSpan();
    left = std::clamp(xAt(end - span), 0.0, w - 3.0);
    right = std::clamp(std::max(left + 3.0, xAt(end)), 0.0, w);
}

bool OverviewItem::frameVisible() const
{
    const MapView &view = (mDrag == Drag::None || mDrag == Drag::Scrollbar) ? mView : mDragView;
    const double end = sSession->windowEnd();
    return end > view.start && end - sConfig->getViewTimeSpan() < view.start + view.span;
}

double OverviewItem::pitchEnd() const
{
    sDataStore->beginRead();
    auto &track = sDataStore->getPitchTrack();
    const auto first = track.lower_bound(-1e300);
    const auto end = track.upper_bound(1e300);
    const double last = end == first ? -1.0 : (end - 1)->first;
    sDataStore->endRead();
    return last;
}

void OverviewItem::refresh()
{
    if (sConfig == nullptr || sDataStore == nullptr || !isVisible() || width() < 4) return;

    const MapView view = currentView();
    Key key;
    key.version = sDataStore->session().version();
    key.width = (int) width();
    key.height = (int) height();
    key.start = view.start;
    key.span = view.span;
    key.windowEnd = sSession->windowEnd();
    key.viewSpan = sConfig->getViewTimeSpan();
    key.duration = mShownDuration;
    key.live = sConfig->getViewLive();
    key.follow = mState.follow;
    key.theme = sConfig->optionString("theme") + sConfig->optionString("customPitch");

    // The picture of the sound can be older than the sound. The map then draws again until it is current.
    // The check reads the pitch track under a lock, so it runs a few times in a second only.
    const auto now = std::chrono::steady_clock::now();
    if (now - mSoundCheck > std::chrono::milliseconds(100)) {
        mSoundCheck = now;
        mSoundDirty = key.version != mCacheVersion || pitchEnd() != mCachePitchEnd;
    }

    if (key == mKey && !mSoundDirty) return;
    mKey = key;
    update();
}

void OverviewItem::rebuildCache(const MapView &view, long long binSamples, const MapBins &bins)
{
    auto &session = sDataStore->session();
    session.envelopeBins(bins.first, binSamples, bins.count, mLow, mHigh);

    const double perBin = (double) binSamples / sampleRate;

    // The bins that the map shows. The cache has more bins at both sides, and they must not change the scales.
    const int visibleFirst = (int) std::clamp((long long) std::floor(view.start / perBin) - bins.first, 0LL, (long long) bins.count - 1);
    const int visibleEnd = (int) std::clamp((long long) std::ceil((view.start + view.span) / perBin) - bins.first, (long long) visibleFirst + 1, (long long) bins.count);

    // The scales change with the clock and not with the number of drawings. A louder sound raises the
    // loudness scale at once. A quieter sound lowers it in about a second.
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::clamp(std::chrono::duration<double>(now - mCacheTime).count(), 0.0, 2.0);
    const double blend = 1.0 - std::exp(-elapsed / 1.0);

    float target = 0.02f;
    for (int i = visibleFirst; i < visibleEnd; ++i) target = std::max({ target, mHigh[(size_t) i], -mLow[(size_t) i] });
    mPeak = target >= mPeak ? target : mPeak - (float) blend * (mPeak - target);
    mPitch.assign((size_t) bins.count, 0.0f);
    sDataStore->beginRead();
    auto &track = sDataStore->getPitchTrack();
    mCachePitchEnd = -1;
    if (!track.empty()) {
        mCachePitchEnd = (track.upper_bound(1e300) - 1)->first;
        const auto first = track.lower_bound(-1e300);
        std::vector<double> values;
        std::vector<double> frequency((size_t) bins.count, 0.0);

        for (int i = 0; i < bins.count; ++i) {
            const double t = (double) (bins.first + i) * perBin;
            const long long a = track.lower_bound(t) - first;
            const long long b = track.lower_bound(t + perBin) - first;
            if (b <= a) continue;

            // Look at entries that are evenly spaced in the bin. The spacing is a power of two, and
            // the entries have the places that are multiples of the spacing in the track. A new
            // entry at the end of the track then does not change which old entries the map uses, and
            // the line does not flicker while the sound grows.
            long long step = 1;
            while ((b - a) / step > 24) step *= 2;
            values.clear();
            for (long long k = (a + step - 1) / step * step; k < b; k += step) {
                const auto &entry = *(first + k);
                if (entry.second.has_value() && *entry.second > 0) values.push_back(*entry.second);
            }
            if (values.empty()) continue;

            std::nth_element(values.begin(), values.begin() + values.size() / 2, values.end());
            frequency[(size_t) i] = values[values.size() / 2];
        }

        // The median of each value and its neighbours removes single bins that jump.
        std::vector<double> near;
        double lowest = 1e9, highest = 0.0;
        for (int i = 0; i < bins.count; ++i) {
            if (frequency[(size_t) i] <= 0) continue;
            const bool visible = i >= visibleFirst && i < visibleEnd;

            near.clear();
            for (int j = std::max(0, i - 1); j <= std::min(bins.count - 1, i + 1); ++j) {
                if (frequency[(size_t) j] > 0) near.push_back(frequency[(size_t) j]);
            }
            std::nth_element(near.begin(), near.begin() + near.size() / 2, near.end());
            const double f = near[near.size() / 2];
            mPitch[(size_t) i] = (float) f;
            if (visible) {
                lowest = std::min(lowest, f);
                highest = std::max(highest, f);
            }
        }

        // The scale of the pitch fits the pitch in the map, with a margin. It grows at once and
        // shrinks slowly. It always covers at least one octave.
        if (highest > 0.0) {
            double targetLow = lowest / 1.15;
            double targetHigh = highest * 1.15;
            if (targetHigh / targetLow < 2.0) {
                const double centre = std::sqrt(targetLow * targetHigh);
                targetLow = centre / std::sqrt(2.0);
                targetHigh = centre * std::sqrt(2.0);
            }
            mPitchLow = targetLow < mPitchLow ? targetLow : mPitchLow + blend * (targetLow - mPitchLow);
            mPitchHigh = targetHigh > mPitchHigh ? targetHigh : mPitchHigh + blend * (targetHigh - mPitchHigh);
        }
    }
    sDataStore->endRead();

    mCacheBins = bins;
    mCacheBinSamples = binSamples;
    mCacheVersion = session.version();
    mCacheTime = now;
    mSoundDirty = false;
}

void OverviewItem::paint(QPainter *painter)
{
    const int w = (int) width();
    const int h = (int) height();
    painter->fillRect(0, 0, w, h, QColor(16, 17, 22));
    if (sConfig == nullptr || sDataStore == nullptr || w < 4) return;

    const QString themeKey = sConfig->optionString("theme") + "|" + sConfig->optionString("customColors")
        + "|" + sConfig->optionString("customPitch") + "|" + sConfig->optionString("customFormants");
    if (themeKey != mThemeKey) {
        mThemeKey = themeKey;
        mTheme = resolveTheme(sConfig->optionString("theme"), sConfig->optionString("customColors"),
                              sConfig->optionString("customPitch"), sConfig->optionString("customFormants"));
    }

    const double length = total();
    const MapView view = (mDrag == Drag::None || mDrag == Drag::Scrollbar) ? currentView() : mDragView;
    if (mDrag == Drag::None || mDrag == Drag::Scrollbar) mView = view;

    const int strip = stripHeight();
    const int plotH = h - strip;

    // The bins keep their size while the map scales a little, and the cache has room at both sides of
    // the map. The map then moves and scales without a new cache. A new cache is needed when the size
    // of the bin is far from a pixel, when the map leaves the cache, and when new sound arrives.
    const long long binSamples = mapStableBinSamples(mCacheBinSamples, view.span, w, sampleRate);
    const bool sizeOk = binSamples == mCacheBinSamples;

    const MapBins needed = mapBinRange(view, binSamples, sampleRate);
    const bool inCache = sizeOk && needed.first >= mCacheBins.first
        && needed.first + needed.count <= mCacheBins.first + mCacheBins.count;
    const bool dirty = mSoundDirty && std::chrono::steady_clock::now() - mCacheTime > std::chrono::milliseconds(100);
    if (!inCache || dirty) {
        MapView wide = view;
        wide.start = std::max(0.0, view.start - 0.5 * view.span);
        wide.span = view.span * 2.0;
        MapBins bins = mapBinRange(wide, binSamples, sampleRate);
        // The first bin is a multiple of 16, so the bins keep their place when the cache moves.
        const long long aligned = bins.first / 16 * 16;
        bins.count += (int) (bins.first - aligned);
        bins.first = aligned;
        rebuildCache(view, binSamples, bins);
    }

    const double perBin = (double) mCacheBinSamples / sampleRate;
    const double pixelsPerSecond = w / view.span;
    const double mid = plotH / 2.0;

    // The trace fills the height of the map at the loudest place. A square root shows quiet sound too.
    QColor envelope = mTheme.accent;
    envelope.setAlpha(150);
    auto shape = [this](float v) { return std::copysign(std::sqrt(std::abs(v) / mPeak), v); };
    const int count = (int) std::min<size_t>(mLow.size(), (size_t) mCacheBins.count);
    // Only the bins in the map are drawn.
    const int firstDrawn = (int) std::clamp((long long) std::floor(view.start / perBin) - mCacheBins.first - 1, 0LL, (long long) count);
    const int endDrawn = (int) std::clamp((long long) std::ceil((view.start + view.span) / perBin) - mCacheBins.first + 1, 0LL, (long long) count);
    for (int i = firstDrawn; i < endDrawn; ++i) {
        const double t = (double) (mCacheBins.first + i) * perBin;
        const double x0 = (t - view.start) * pixelsPerSecond;
        const double x1 = (t + perBin - view.start) * pixelsPerSecond;
        if (x1 < 0 || x0 > w) continue;

        const double top = mid - shape(mHigh[(size_t) i]) * (mid - 1);
        const double bottom = mid - shape(mLow[(size_t) i]) * (mid - 1);
        painter->fillRect(QRectF(x0, top, std::max(1.0, x1 - x0), std::max(bottom - top, 1.0)), envelope);
    }

    // The pitch is a line. It breaks where the track has no pitch for more than 70 ms, and where
    // a bin is empty. A bin that has no neighbour draws nothing.
    const int maxGap = std::max(1, (int) std::ceil(0.07 / perBin));
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(mTheme.pitch, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const double logRange = std::log(mPitchHigh / mPitchLow);
    auto pitchPoint = [&](int i) {
        const double t = (double) (mCacheBins.first + i) * perBin + 0.5 * perBin;
        const double y = std::clamp(std::log(mPitch[(size_t) i] / mPitchLow) / logRange, 0.0, 1.0);
        return QPointF((t - view.start) * pixelsPerSecond, (plotH - 3) - y * (plotH - 6));
    };
    const int pitchBins = (int) std::min<size_t>(mPitch.size(), (size_t) endDrawn);
    int previous = -1;
    for (int i = std::max(0, firstDrawn - maxGap - 1); i < pitchBins; ++i) {
        if (mPitch[(size_t) i] <= 0) continue;

        if (previous >= 0 && i - previous <= maxGap) {
            painter->drawLine(pitchPoint(previous), pitchPoint(i));
        }
        previous = i;
    }
    painter->setRenderHint(QPainter::Antialiasing, false);

    // The frame of the main view, with a grip at each edge.
    const bool live = sConfig->getSessionMode() == 0 && sConfig->getViewLive();
    if (frameVisible()) {
        double left, right;
        frameEdges(left, right);
        painter->fillRect(QRectF(left, 0, right - left, plotH), QColor(255, 255, 255, 40));
        painter->setPen(QPen(mTheme.accent, 2));
        painter->drawRoundedRect(QRectF(left + 1, 1, right - left - 2, plotH - 2), 4, 4);
        painter->fillRect(QRectF(left, 0, 4, plotH), mTheme.accent);
        painter->fillRect(QRectF(right - 4, 0, 4, plotH), mTheme.accent);
    }

    // The strip under the map shows which part of the session the map shows.
    if (strip > 0) {
        painter->fillRect(QRectF(0, plotH, w, strip), QColor(255, 255, 255, 22));
        const double thumbLeft = view.start / length * w;
        const double thumbWidth = std::max(16.0, view.span / length * w);
        painter->fillRect(QRectF(thumbLeft, plotH + 1, std::min(thumbWidth, w - thumbLeft), strip - 2), QColor(200, 200, 215, 150));
    }

    // The times at the two ends of the map, and a mark for the live view.
    QFont font = painter->font();
    font.setPixelSize(10);
    painter->setFont(font);
    painter->setPen(QColor(200, 200, 210));
    painter->drawText(QRectF(4, 2, 80, 14), Qt::AlignLeft | Qt::AlignVCenter, mapTimeText(view.start));
    const bool atEnd = view.start + view.span >= length - 1e-6;
    const QString endLabel = (live && atEnd ? QStringLiteral("LIVE  ") : QString()) + mapTimeText(view.start + view.span);
    painter->drawText(QRectF(w - 124, 2, 120, 14), Qt::AlignRight | Qt::AlignVCenter, endLabel);
}

void OverviewItem::moveEnd(double end)
{
    // The last four pixels of the session return to the live view.
    sSession->moveViewEnd(end, mDragView.span * 4.0 / std::max(width(), 1.0));
}

OverviewItem::Drag OverviewItem::hitTest(double x, double y) const
{
    if (stripHeight() > 0 && y >= height() - stripHeight()) return Drag::Scrollbar;

    if (!frameVisible()) return Drag::Move;

    double left, right;
    frameEdges(left, right);
    const double toLeft = std::abs(x - left);
    const double toRight = std::abs(x - right);
    if (toLeft <= edgeZone || toRight <= edgeZone) {
        // A narrow frame has both zones at the same place. The nearer edge wins.
        return toLeft <= toRight ? Drag::LeftEdge : Drag::RightEdge;
    }
    return Drag::Move;
}

void OverviewItem::mousePressEvent(QMouseEvent *event)
{
    if (sConfig == nullptr || sSession == nullptr) return;

    mView = currentView();
    mDragView = mView;
    const double x = event->position().x();
    mDrag = hitTest(x, event->position().y());

    const double span = sConfig->getViewTimeSpan();
    const double end = sSession->windowEnd();

    switch (mDrag) {
    case Drag::Scrollbar: {
        // A press on the thumb keeps its place under the pointer. A press outside puts the middle of the thumb there.
        const double w = std::max(width(), 1.0);
        const double thumbLeft = mView.start / total() * w;
        const double thumbWidth = std::max(16.0, mView.span / total() * w);
        mDragOffset = (x >= thumbLeft && x <= thumbLeft + thumbWidth) ? x - thumbLeft : 0.5 * thumbWidth;
        scrollMap(mState, total(), mView, (x - mDragOffset) / w * total() - mView.start);
        break;
    }
    case Drag::LeftEdge:
        // The right end of the frame stays. The left edge follows the pointer.
        mFixedTime = end;
        break;
    case Drag::RightEdge:
        // The left end of the frame stays. The right edge follows the pointer.
        mFixedTime = end - span;
        break;
    default: {
        // A press inside the frame keeps the place of the frame under the pointer. A press outside
        // puts the middle of the frame at the pointer.
        const double t = timeAt(x);
        mDragOffset = (t >= end - span && t <= end) ? end - t : -0.5 * span;
        mDrag = Drag::Move;
        moveEnd(t + mDragOffset);
        break;
    }
    }
    // The map draws again at once, and does not wait for the next check of the timer.
    update();
    event->accept();
}

void OverviewItem::mouseMoveEvent(QMouseEvent *event)
{
    if (mDrag == Drag::None) return;
    const double x = event->position().x();
    const double t = timeAt(x);

    switch (mDrag) {
    case Drag::Move:
        moveEnd(t + mDragOffset);
        break;
    case Drag::LeftEdge:
        sConfig->setViewTimeSpan(dragViewSpan(mFixedTime - t, total(), sConfig->optionDouble("mapWindow")));
        break;
    case Drag::RightEdge: {
        const double span = dragViewSpan(t - mFixedTime, total(), sConfig->optionDouble("mapWindow"));
        sConfig->setViewTimeSpan(span);
        moveEnd(mFixedTime + span);
        break;
    }
    case Drag::Scrollbar: {
        const double w = std::max(width(), 1.0);
        scrollMap(mState, total(), mView, (x - mDragOffset) / w * total() - mView.start);
        break;
    }
    default:
        break;
    }
    update();
    event->accept();
}

void OverviewItem::mouseReleaseEvent(QMouseEvent *event)
{
    mDrag = Drag::None;
    // The map follows the main view again after a drag of the frame.
    mView = currentView();
    unsetCursor();
    update();
    event->accept();
}

void OverviewItem::hoverMoveEvent(QHoverEvent *event)
{
    if (sConfig == nullptr || sSession == nullptr || mDrag != Drag::None) return;

    const Drag zone = hitTest(event->position().x(), event->position().y());
    if (zone == Drag::LeftEdge || zone == Drag::RightEdge) setCursor(Qt::SizeHorCursor);
    else unsetCursor();
}

void OverviewItem::hoverLeaveEvent(QHoverEvent *)
{
    unsetCursor();
}

void OverviewItem::wheelEvent(QWheelEvent *event)
{
    if (sConfig == nullptr || sSession == nullptr) return;

    const QPoint delta = event->angleDelta();
    const bool horizontal = std::abs(delta.x()) > std::abs(delta.y());
    const double steps = (horizontal ? delta.x() : delta.y()) / 120.0;

    if (horizontal) {
        // A sideways scroll moves the map by a tenth of its width for each step.
        mView = currentView();
        scrollMap(mState, total(), mView, -steps * 0.1 * mView.span);
    }
    else {
        // A turn of one step moves the frame by a tenth of its width.
        sSession->scrollView(-steps);
    }
    update();
    event->accept();
}
