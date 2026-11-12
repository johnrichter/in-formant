#ifndef MAIN_CONTEXT_FILE_ANALYZER_H
#define MAIN_CONTEXT_FILE_ANALYZER_H

#include <atomic>
#include <functional>
#include <thread>

#include "config.h"
#include "datastore.h"

namespace Main {

    // Runs the pitch analysis and the formant analysis over the sound of the session, in a thread,
    // faster than the sound plays. The results go to the tracks of the data store, as for a live capture.
    class FileAnalyzer {
    public:
        FileAnalyzer(Config *config, DataStore *dataStore, int sampleRate);
        ~FileAnalyzer();

        // Start the analysis of the sound that the session has now. The callback runs in the
        // thread of the analysis when it ends. A running analysis stops first.
        void start(std::function<void(bool completed)> finished);

        // Stop the analysis, and wait for the thread.
        void cancel();

        bool running() const { return mRunning.load(); }

        // The part of the sound that the analysis finished, from 0 to 1.
        double progress() const { return mProgress.load(); }

    private:
        void run(double duration, std::function<void(bool)> finished);

        Config *mConfig;
        DataStore *mDataStore;
        int mSampleRate;

        std::thread mThread;
        std::atomic<bool> mRunning;
        std::atomic<bool> mCancel;
        std::atomic<double> mProgress;
    };

}

#endif // MAIN_CONTEXT_FILE_ANALYZER_H
