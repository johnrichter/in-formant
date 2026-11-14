#ifndef MAIN_CONTEXT_DATA_STORE_H
#define MAIN_CONTEXT_DATA_STORE_H

#include "rpcxx.h"
#include "../timetrack.h"
#include "../analysis/analysis.h"
#include "session.h"
#include <array>
#include <chrono>
#include <functional>
#include <QReadWriteLock>

enum class FrequencyScale : unsigned int {
    Linear      = 0,
    Logarithmic = 1,
    Mel         = 2,
    ERB         = 3,
};

namespace Main {

    struct SpectrogramCoefs {
        // The power of each frequency bin. A float is enough for the picture, and it halves the memory.
        rpm::vector<float> magnitudes;
        double sampleRate;
    };

    class DataStore {
    public:
        DataStore();

        void beginWrite();
        void endWrite();

        void beginRead();
        void endRead();

        double getTime() const;
        void setTime(double t);

        TimeTrack<SpectrogramCoefs>& getSpectrogram();

        OptionalTimeTrack<double>& getPitchTrack();

        OptionalTimeTrack<double>& getFormantTrack(int i);
        int getFormantTrackCount() const;
        void setFormantTrackCount(int n);

        TimeTrack<rpm::vector<double>>& getSoundTrack();
        TimeTrack<rpm::vector<double>>& getGifTrack();

        // The sound of the session. The tracks above hold the results of the analysis for it.
        Session& session();

        // Delete all tracks. The caller holds the write lock.
        void clearTracks();
    
    private:
        int mTrackLength;

        QReadWriteLock mLock;

        volatile double mTime;

        TimeTrack<SpectrogramCoefs> mSpectrogram;
        
        OptionalTimeTrack<double> mPitchTrack;
        rpm::vector<OptionalTimeTrack<double>> mFormantTracks;

        TimeTrack<rpm::vector<double>> mSoundTrack;
        TimeTrack<rpm::vector<double>> mGifTrack;

        Session mSession;
    };

}

#endif // MAIN_CONTEXT_DATA_STORE_H

