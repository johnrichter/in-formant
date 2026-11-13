#ifndef DATAVIS_WRAPPER_H
#define DATAVIS_WRAPPER_H

#include <atomic>
#include <mutex>
#include <vector>

#include <QObject>

#include "rpcxx.h"
#include "../gui/plotitem.h"

namespace Main {

    class Config;
    class Session;

    // The data of the oscilloscope: the newest sound, and the estimate of the glottal flow. The
    // pipeline writes the data, and the plot of the oscilloscope reads it.
    class DataVisWrapper : public QObject, public Gui::PlotProvider {
        Q_OBJECT

    public:
        DataVisWrapper();

        // Add the newest samples of the sound. The pipeline thread calls this for each block.
        void pushAudio(const double *data, int count, double fs);

        // Store the newest estimate of the glottal flow. It ends at the newest sample of the sound.
        void setGif(const rpm::vector<double>& signal, double fs);

        // The settings and the stored sound. They give the picture of a paused capture and of a loaded file.
        void setReview(Config *config, Session *session);

        // True while an oscilloscope plot is on the screen.
        bool watched() const { return mWatchers.load() > 0; }

        bool fillPlot(int kind, const Gui::PlotRequest &request, Gui::PlotSnapshot &out) override;
        void changeWatchers(int kind, int delta) override;

    private:
        bool fillLive(const Gui::PlotRequest &request, Gui::PlotSnapshot &out);
        bool fillStored(const Gui::PlotRequest &request, Gui::PlotSnapshot &out);

        Config *mConfig = nullptr;
        Session *mSession = nullptr;

        std::mutex mMutex;

        // The newest 400 ms of the sound in a ring. mTotal counts all samples since the start.
        std::vector<double> mRing;
        unsigned long long mTotal;
        double mFs;

        std::vector<double> mGif;
        double mGifFs;
        unsigned long long mGifEnd;
        unsigned long long mGifVersion;

        std::atomic<int> mWatchers;
    };

}

#endif
