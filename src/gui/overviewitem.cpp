#include "overviewitem.h"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include "../context/config.h"
#include "../context/datastore.h"
#include "../context/sessionwrapper.h"

using namespace Gui;

namespace {

Main::Config *sConfig = nullptr;
Main::DataStore *sDataStore = nullptr;
Main::SessionWrapper *sSession = nullptr;

// The range of the pitch in the map, in Hz.
constexpr double pitchLow = 60.0;
constexpr double pitchHigh = 600.0;

QString formatTime(double seconds)
{
    const long long s = (long long) std::llround(std::max(0.0, seconds));
    const long long h = s / 3600, m = (s / 60) % 60, r = s % 60;
    if (h > 0) return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(r, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m).arg(r, 2, 10, QChar('0'));
}

}

OverviewItem::OverviewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      mCacheWidth(0),
      mCacheTotal(0),
      mCacheVersion(0),
      mDragging(false),
      mDragOffset(0)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    mTimer.setInterval(100);
    connect(&mTimer, &QTimer::timeout, this, &OverviewItem::refresh);
    mTimer.start();
}

void OverviewItem::setSources(Main::Config *config, Main::SessionWrapper *session)
{
    sConfig = config;
    sSession = session;
    sDataStore = session->dataStore();
}

double OverviewItem::total() const
{
    if (sDataStore == nullptr) return 1.0;
    return std::max(sDataStore->session().duration(), 1.0);
}

double OverviewItem::timeAt(double x) const
{
    const double w = std::max(width(), 1.0);
    return std::clamp(x / w, 0.0, 1.0) * total();
}

void OverviewItem::refresh()
{
    if (sConfig == nullptr || sDataStore == nullptr || !isVisible() || width() < 4) return;

    const QString key = QString::number(sDataStore->session().version()) + "|" + QString::number(width()) + "|"
        + QString::number(height()) + "|" + QString::number(sConfig->getViewEnd(), 'f', 3) + "|"
        + QString::number(sConfig->getViewTimeSpan()) + "|" + QString::number(sConfig->getViewLive()) + "|"
        + sConfig->optionString("theme") + sConfig->optionString("customPitch");
    if (key == mKey) return;
    mKey = key;
    update();
}

void OverviewItem::rebuildCache(int width, double length)
{
    auto &session = sDataStore->session();
    session.envelope(0.0, length, width, mLow, mHigh);

    mPitch.assign((size_t) width, -1.0f);
    sDataStore->beginRead();
    auto &track = sDataStore->getPitchTrack();
    if (!track.empty()) {
        const double perBin = length / width;
        std::vector<double> values;
        for (int i = 0; i < width; ++i) {
            const auto begin = track.lower_bound(i * perBin);
            const auto end = track.upper_bound((i + 1) * perBin);
            const long long count = end - begin;
            if (count <= 0) continue;

            // Look at up to eight entries that are evenly spaced in the bin.
            values.clear();
            const long long step = std::max(1LL, count / 8);
            for (long long k = 0; k < count; k += step) {
                const auto &entry = *(begin + k);
                if (entry.second.has_value() && *entry.second > 0) values.push_back(*entry.second);
            }
            if (values.size() * 3 < (size_t) std::min(8LL, count)) continue;

            std::nth_element(values.begin(), values.begin() + values.size() / 2, values.end());
            const double f = values[values.size() / 2];
            mPitch[(size_t) i] = (float) std::clamp(std::log(f / pitchLow) / std::log(pitchHigh / pitchLow), 0.0, 1.0);
        }
    }
    sDataStore->endRead();

    mCacheWidth = width;
    mCacheTotal = length;
    mCacheVersion = session.version();
    mCacheTime = std::chrono::steady_clock::now();
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

    // The data of the map changes when the sound grows, but a new picture for each change is not needed.
    auto &session = sDataStore->session();
    const double length = total();
    const bool sizeChanged = w != mCacheWidth;
    const bool soundChanged = session.version() != mCacheVersion || length != mCacheTotal;
    if (sizeChanged || (soundChanged && std::chrono::steady_clock::now() - mCacheTime > std::chrono::milliseconds(250))) {
        rebuildCache(w, length);
    }

    const double mid = h / 2.0;
    QColor envelope = mTheme.accent;
    envelope.setAlpha(150);
    painter->setPen(envelope);
    const int bins = (int) std::min<size_t>(mLow.size(), (size_t) w);

    // The trace fills the height of the map at the loudest place. A square root shows quiet sound too.
    float peak = 0.02f;
    for (int x = 0; x < bins; ++x) peak = std::max({peak, mHigh[(size_t) x], -mLow[(size_t) x]});
    auto shape = [peak](float v) { return std::copysign(std::sqrt(std::abs(v) / peak), v); };
    for (int x = 0; x < bins; ++x) {
        const double top = mid - shape(mHigh[(size_t) x]) * (mid - 1);
        const double bottom = mid - shape(mLow[(size_t) x]) * (mid - 1);
        painter->drawLine(QPointF(x + 0.5, top), QPointF(x + 0.5, std::max(bottom, top + 1.0)));
    }

    painter->setPen(mTheme.pitch);
    const int pitchBins = (int) std::min<size_t>(mPitch.size(), (size_t) w);
    for (int x = 0; x < pitchBins; ++x) {
        if (mPitch[(size_t) x] < 0) continue;
        const double y = (h - 3) - mPitch[(size_t) x] * (h - 6);
        painter->fillRect(QRectF(x, y - 1, 2, 2), mTheme.pitch);
    }

    // The frame of the main view.
    const double span = sConfig->getViewTimeSpan();
    const bool live = sConfig->getSessionMode() == 0 && sConfig->getViewLive();
    const double end = sSession->windowEnd();
    const double left = std::min(std::max(0.0, end - span) / length * w, w - 3.0);
    const double right = std::min(std::max(left + 3.0, end / length * w), (double) w);

    QColor fill = QColor(255, 255, 255, 40);
    painter->fillRect(QRectF(left, 0, right - left, h), fill);
    painter->setPen(QPen(mTheme.accent, 2));
    painter->drawRect(QRectF(left + 1, 1, right - left - 2, h - 2));

    // The start and the end of the sound, and a mark for the live view.
    QFont font = painter->font();
    font.setPixelSize(10);
    painter->setFont(font);
    painter->setPen(QColor(200, 200, 210));
    painter->drawText(QRectF(4, 2, 80, 14), Qt::AlignLeft | Qt::AlignVCenter, "0:00");
    const QString endLabel = (live ? QStringLiteral("LIVE  ") : QString()) + formatTime(session.duration());
    painter->drawText(QRectF(w - 124, 2, 120, 14), Qt::AlignRight | Qt::AlignVCenter, endLabel);
}

void OverviewItem::moveEnd(double end)
{
    // The last four pixels of the map return to the live view.
    sSession->moveViewEnd(end, total() * 4.0 / std::max(width(), 1.0));
}

void OverviewItem::mousePressEvent(QMouseEvent *event)
{
    if (sConfig == nullptr || sSession == nullptr) return;

    const double span = sConfig->getViewTimeSpan();
    const double end = sSession->windowEnd();
    const double t = timeAt(event->position().x());

    // A press inside the frame keeps the place of the frame under the pointer. A press outside
    // puts the middle of the frame at the pointer.
    mDragOffset = (t >= end - span && t <= end) ? end - t : -0.5 * span;
    mDragging = true;
    moveEnd(t + mDragOffset);
    event->accept();
}

void OverviewItem::mouseMoveEvent(QMouseEvent *event)
{
    if (!mDragging) return;
    moveEnd(timeAt(event->position().x()) + mDragOffset);
    event->accept();
}

void OverviewItem::mouseReleaseEvent(QMouseEvent *event)
{
    mDragging = false;
    event->accept();
}

void OverviewItem::wheelEvent(QWheelEvent *event)
{
    if (sConfig == nullptr || sSession == nullptr) return;

    // A turn of one step moves the frame by a tenth of its width.
    const QPoint delta = event->angleDelta();
    const double steps = (std::abs(delta.x()) > std::abs(delta.y()) ? delta.x() : delta.y()) / 120.0;
    sSession->scrollView(-steps);
    event->accept();
}
