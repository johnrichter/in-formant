#ifndef GUI_PLOT_ITEM_H
#define GUI_PLOT_ITEM_H

#include <memory>
#include <mutex>

#include <QImage>
#include <QQuickPaintedItem>
#include <QTimer>

#include "plotsnapshot.h"
#include "theme.h"

namespace Main {
    class Config;
}

namespace Gui {

    // The settings of the plots, from the options of the user.
    struct PlotRequest {
        double scopeWindowMs = 30.0;
        bool showGif = true;
    };

    // A source of plot data. The kinds are PlotItem::Scope, Filter, Source, and Spectrum.
    class PlotProvider {
    public:
        virtual ~PlotProvider() = default;

        // Fill the snapshot with the newest data. Return false when there is no data yet.
        virtual bool fillPlot(int kind, const PlotRequest &request, PlotSnapshot &out) = 0;

        // The number of plots on the screen for this kind changed by delta. A provider makes data
        // only while a plot of its kind is on the screen.
        virtual void changeWatchers(int kind, int delta) = 0;
    };

    // A line plot with axes for the oscilloscope and the synthesizer. It draws with one fast
    // paint call for each frame, and it updates only while it is on the screen.
    class PlotItem : public QQuickPaintedItem {
        Q_OBJECT
        Q_PROPERTY(int kind READ kind WRITE setKind NOTIFY kindChanged)

    public:
        enum Kind { Scope = 0, Filter = 1, Source = 2, Spectrum = 3, KindCount = 4 };

        PlotItem(QQuickItem *parent = nullptr);
        ~PlotItem() override;

        int kind() const { return mKind; }
        void setKind(int kind);

        void paint(QPainter *painter) override;

        // The provider of the data of one kind, and the options. The caller keeps the objects alive.
        static void setProvider(int kind, PlotProvider *provider);
        static void setConfig(Main::Config *config);

    signals:
        void kindChanged();

    protected:
        void itemChange(ItemChange change, const ItemChangeData &value) override;

    private:
        void updateWatching();
        void refresh();

        int mKind;
        bool mWatching;
        QTimer mTimer;

        QString mThemeKey;
        Theme mTheme;

        // The data and the colors of one frame.
        struct Frame;
        std::mutex mFrameMutex;
        std::shared_ptr<const Frame> mFrame;
        unsigned long long mShownVersion;
        QMetaObject::Connection mWindowConnection;
        QMetaObject::Connection mFrameConnection;

        // The image of the parts that change rarely, and the values that it shows.
        QImage mStatic;
        QString mStaticKey;
    };

}

#endif // GUI_PLOT_ITEM_H
