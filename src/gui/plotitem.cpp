#include "plotitem.h"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>

#include "../context/config.h"
#include "ticks.h"

using namespace Gui;

struct PlotItem::Frame {
    PlotSnapshot snapshot;
    QColor colors[2];
    QColor grid;
};

namespace {

PlotProvider *sProviders[PlotItem::KindCount] = {};
Main::Config *sConfig = nullptr;

// Values of 1, 2, and 5 times a power of ten between the two limits.
std::vector<double> niceTicks(double lo, double hi, int maxCount)
{
    std::vector<double> ticks;
    if (!(hi > lo) || maxCount < 2) return ticks;

    const double rough = (hi - lo) / maxCount;
    const double base = std::pow(10.0, std::floor(std::log10(rough)));
    double step = 10.0 * base;
    for (double m : { 1.0, 2.0, 5.0, 10.0 }) {
        if (m * base >= rough) {
            step = m * base;
            break;
        }
    }

    for (double v = std::ceil(lo / step - 1e-9) * step; v <= hi + 1e-9 * step; v += step) {
        ticks.push_back(std::fabs(v) < 1e-12 * step ? 0.0 : v);
    }
    return ticks;
}

// The values of 1, 2, and 5 times a power of ten for a logarithmic axis.
std::vector<double> logTicks(double lo, double hi)
{
    std::vector<double> ticks;
    if (!(lo > 0) || !(hi > lo)) return ticks;

    for (double decade = std::pow(10.0, std::floor(std::log10(lo))); decade <= hi; decade *= 10.0) {
        for (double m : { 1.0, 2.0, 5.0 }) {
            const double v = m * decade;
            if (v >= lo * (1 - 1e-9) && v <= hi * (1 + 1e-9)) ticks.push_back(v);
        }
    }
    return ticks;
}

QString tickLabel(double value, bool logAxis)
{
    if (logAxis && value >= 1000.0) {
        return QString::fromStdString(Gui::formatNumber(value / 1000.0)) + "k";
    }
    return QString::fromStdString(Gui::formatNumber(value));
}

}

PlotItem::PlotItem(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      mKind(Scope),
      mWatching(false),
      mShownVersion(0)
{
    // The default image target paints with the raster engine, and it needs no OpenGL context for
    // the item. A framebuffer target created a new vsync thread for each frame.
    setAntialiasing(true);
    setOpaquePainting(true);

    // Each swapped frame starts the next refresh, so the plot follows the display. The slow timer
    // starts the chain again after a pause of the data.
    mTimer.setInterval(100);
    connect(&mTimer, &QTimer::timeout, this, &PlotItem::refresh);
}

PlotItem::~PlotItem()
{
    if (mWatching && sProviders[mKind]) {
        sProviders[mKind]->changeWatchers(mKind, -1);
    }
}

void PlotItem::setProvider(int kind, PlotProvider *provider)
{
    if (kind >= 0 && kind < KindCount) sProviders[kind] = provider;
}

void PlotItem::setConfig(Main::Config *config)
{
    sConfig = config;
}

void PlotItem::setKind(int kind)
{
    if (kind == mKind || kind < 0 || kind >= KindCount) return;

    const bool wasWatching = mWatching;
    if (wasWatching) {
        mWatching = false;
        if (sProviders[mKind]) sProviders[mKind]->changeWatchers(mKind, -1);
    }
    mKind = kind;
    mShownVersion = 0;
    emit kindChanged();
    if (wasWatching) updateWatching();
}

void PlotItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    // The base class sets the new window first. The code below then sees the new window.
    QQuickPaintedItem::itemChange(change, value);

    if (change == ItemSceneChange) {
        QObject::disconnect(mWindowConnection);
        QObject::disconnect(mFrameConnection);
        if (mWatching) {
            // The old window is gone. Start again with the new window.
            mWatching = false;
            if (sProviders[mKind]) sProviders[mKind]->changeWatchers(mKind, -1);
            mTimer.stop();
        }
        if (value.window) {
            mWindowConnection = connect(value.window, &QWindow::visibleChanged, this, [this] { updateWatching(); });
        }
        updateWatching();
    }
    else if (change == ItemVisibleHasChanged) {
        updateWatching();
    }
}

void PlotItem::updateWatching()
{
    const bool want = isVisible() && window() != nullptr && window()->isVisible();
    if (want == mWatching) return;

    mWatching = want;
    if (sProviders[mKind]) {
        sProviders[mKind]->changeWatchers(mKind, want ? 1 : -1);
    }

    QObject::disconnect(mFrameConnection);
    if (want) {
        mFrameConnection = connect(window(), &QQuickWindow::frameSwapped, this, &PlotItem::refresh, Qt::QueuedConnection);
        mTimer.start();
        refresh();
    }
    else {
        mTimer.stop();
    }
}

void PlotItem::refresh()
{
    PlotProvider *provider = sProviders[mKind];
    if (!mWatching || provider == nullptr || sConfig == nullptr) return;

    PlotRequest request;
    request.scopeWindowMs = sConfig->optionDouble("scopeWindow");
    request.showGif = sConfig->optionBool("scopeShowGif");

    auto frame = std::make_shared<Frame>();
    if (!provider->fillPlot(mKind, request, frame->snapshot)) return;
    if (frame->snapshot.version == mShownVersion) return;
    mShownVersion = frame->snapshot.version;

    const QString themeKey = sConfig->optionString("theme") + "|" + sConfig->optionString("customColors")
        + "|" + sConfig->optionString("customPitch") + "|" + sConfig->optionString("customFormants") + "|" + sConfig->optionString("colorSaturation") + "|" + sConfig->optionString("minVolumeBlack");
    if (themeKey != mThemeKey) {
        mThemeKey = themeKey;
        mTheme = Gui::resolveTheme(sConfig->optionString("theme"), sConfig->optionString("customColors"),
                                   sConfig->optionString("customPitch"), sConfig->optionString("customFormants"),
                                   sConfig->optionDouble("colorSaturation"), sConfig->optionBool("minVolumeBlack"));
    }

    switch (mKind) {
    case Scope:
        frame->colors[0] = mTheme.oscilloscope;
        frame->colors[1] = mTheme.accent;
        break;
    case Filter:
        frame->colors[0] = mTheme.synthFilter;
        frame->colors[1] = mTheme.accent;
        break;
    default:
        frame->colors[0] = mTheme.synthSource;
        frame->colors[1] = mTheme.accent;
        break;
    }
    frame->grid = mTheme.oscilloscopeGrid;

    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mFrame = std::move(frame);
    }
    update();
}

void PlotItem::paint(QPainter *painter)
{
    std::shared_ptr<const Frame> frame;
    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        frame = mFrame;
    }

    const double w = width();
    const double h = height();
    painter->fillRect(QRectF(0, 0, w, h), QColor(20, 22, 28));
    if (!frame || w < 80 || h < 60) return;

    const PlotSnapshot &s = frame->snapshot;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    QFont font = painter->font();
    font.setPixelSize(11);
    painter->setFont(font);
    const QFontMetricsF metrics(font);

    // The area of the lines, inside the margins for the labels.
    const double left = 12 + metrics.horizontalAdvance("-000") + 14;
    const double right = 14;
    const double top = 28;
    const double bottom = 36;
    const QRectF area(left, top, w - left - right, h - top - bottom);
    if (area.width() < 20 || area.height() < 20) return;

    const bool logX = s.logX && s.xMin > 0 && s.xMax > s.xMin;
    const double lx0 = logX ? std::log10(s.xMin) : s.xMin;
    const double lx1 = logX ? std::log10(s.xMax) : s.xMax;
    auto mapX = [&](double x) {
        const double v = logX ? std::log10(std::max(x, 1e-12)) : x;
        return area.left() + (v - lx0) / (lx1 - lx0) * area.width();
    };
    auto mapY = [&](double y) {
        return area.bottom() - (y - s.yMin) / (s.yMax - s.yMin) * area.height();
    };

    // The grid, the labels, the title, and the legend change only when the data axes change. They
    // go to an image, and each frame draws the image and then the lines.
    auto drawStatic = [&](QPainter *painter) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setFont(font);

        painter->fillRect(area, QColor(27, 29, 37));

        QColor gridColor = frame->grid;
        gridColor.setAlpha(70);
        QColor textColor(200, 204, 214);
        QPen gridPen(gridColor, 1.0);

        // Horizontal axis.
        const auto xTicks = logX ? logTicks(s.xMin, s.xMax)
                                 : niceTicks(s.xMin, s.xMax, std::max(2, (int) (area.width() / 70)));
        double lastLabelRight = -1e9;
        for (double t : xTicks) {
            const double x = mapX(t);
            painter->setPen(gridPen);
            painter->drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));

            const QString label = tickLabel(t, logX);
            const double lw = metrics.horizontalAdvance(label);
            if (x - lw / 2 > lastLabelRight + 6 && x - lw / 2 >= 0 && x + lw / 2 <= w) {
                painter->setPen(textColor);
                painter->drawText(QPointF(x - lw / 2, area.bottom() + 15), label);
                lastLabelRight = x + lw / 2;
            }
        }

        // Vertical axis.
        const auto yTicks = niceTicks(s.yMin, s.yMax, std::max(2, (int) (area.height() / 38)));
        for (double t : yTicks) {
            const double y = mapY(t);
            painter->setPen(gridPen);
            painter->drawLine(QPointF(area.left(), y), QPointF(area.right(), y));

            const QString label = tickLabel(t, false);
            painter->setPen(textColor);
            painter->drawText(QPointF(area.left() - 6 - metrics.horizontalAdvance(label), y + metrics.ascent() / 2 - 1), label);
        }

        // The frame of the area.
        painter->setPen(QPen(QColor(90, 94, 108), 1.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(area);

        // The title and the units.
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(QColor(235, 238, 245));
        painter->drawText(QPointF(area.left(), 17), s.title);
        font.setBold(false);
        painter->setFont(font);
        painter->setPen(QColor(150, 154, 168));
        if (!s.xUnit.isEmpty()) {
            const double uw = metrics.horizontalAdvance(s.xUnit);
            painter->drawText(QPointF(area.center().x() - uw / 2, h - 5), s.xUnit);
        }
        if (!s.yUnit.isEmpty()) {
            painter->drawText(QPointF(6, 17), s.yUnit);
        }

        // The legend of the lines, when the plot has more than one.
        if (s.series.size() > 1) {
            double x = area.right() - 6;
            for (auto it = s.series.rbegin(); it != s.series.rend(); ++it) {
                const double lw = metrics.horizontalAdvance(it->name);
                painter->setPen(textColor);
                painter->drawText(QPointF(x - lw, 17), it->name);
                painter->setPen(QPen(frame->colors[std::clamp(it->colorIndex, 0, 1)], 2.5));
                painter->drawLine(QPointF(x - lw - 24, 13), QPointF(x - lw - 6, 13));
                x -= lw + 36;
            }
        }
    };

    const qreal ratio = painter->device()->devicePixelRatioF();
    QString key = QString::asprintf("%.0f|%.0f|%.2f|%.6g|%.6g|%.6g|%.6g|%d|", w, h, ratio, s.xMin, s.xMax, s.yMin, s.yMax, (int) logX)
        + s.title + "|" + s.xUnit + "|" + s.yUnit
        + "|" + QString::number(frame->grid.rgba(), 16)
        + "|" + QString::number(frame->colors[0].rgba(), 16) + "|" + QString::number(frame->colors[1].rgba(), 16);
    for (const PlotSeries &series : s.series) key += "|" + series.name + QString::number(series.colorIndex);

    if (mStatic.isNull() || key != mStaticKey) {
        mStatic = QImage(QSize((int) std::ceil(w * ratio), (int) std::ceil(h * ratio)), QImage::Format_ARGB32_Premultiplied);
        mStatic.setDevicePixelRatio(ratio);
        mStatic.fill(QColor(20, 22, 28));
        QPainter staticPainter(&mStatic);
        drawStatic(&staticPainter);
        mStaticKey = key;
    }
    painter->drawImage(QPointF(0, 0), mStatic);

    // The lines.
    painter->save();
    painter->setClipRect(area.adjusted(1, 1, -1, -1));
    for (const PlotSeries &series : s.series) {
        if (series.points.size() < 2) continue;

        QPolygonF polygon;
        polygon.reserve(series.points.size());
        for (const QPointF &p : series.points) {
            if (logX && p.x() <= 0) continue;
            polygon << QPointF(mapX(p.x()), mapY(p.y()));
        }

        // A cosmetic line of one pixel uses a fast path of the raster engine. A wide line goes
        // through the general stroker, and that took most of the time of the frame. Two lines with
        // a small offset give a line of about two pixels.
        QPen pen(frame->colors[std::clamp(series.colorIndex, 0, 1)], 1.0);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->translate(0, -0.45);
        painter->drawPolyline(polygon);
        painter->translate(0, 0.9);
        painter->drawPolyline(polygon);
        painter->translate(0, -0.45);
    }
    painter->restore();

}
