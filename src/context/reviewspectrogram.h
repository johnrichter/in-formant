#ifndef MAIN_CONTEXT_REVIEW_SPECTROGRAM_H
#define MAIN_CONTEXT_REVIEW_SPECTROGRAM_H

#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "datastore.h"
#include "session.h"

namespace Main {

    // The settings of one spectrogram of the stored sound.
    struct ReviewRequest {
        // The time span in seconds, and the number of columns that cover it.
        double t0 = 0, t1 = 1;
        int columns = 1000;

        // The sample rate of the analysis, and the length of the FFT window in samples.
        int sampleRate = 11000;
        int fftSamples = 1024;

        bool operator==(const ReviewRequest &o) const {
            return t0 == o.t0 && t1 == o.t1 && columns == o.columns && sampleRate == o.sampleRate && fftSamples == o.fftSamples;
        }
    };

    struct ReviewResult {
        // The number of the result. Each result has a new number.
        unsigned long long id = 0;
        ReviewRequest request;

        // The picture in the layout of the texture: one row for each frequency bin, and one value
        // for each column in a row. The thread of the analysis fills it, so the thread of the
        // display only uploads it.
        int columns = 0;
        int rows = 0;
        rpm::vector<float> data;
        std::array<int, 4096> sliceSizes{};
        std::array<float, 4096> sampleRates{};

        // The level that normalizes each column, and the length of the stored sound at the time of
        // the analysis. A later result takes the columns that did not change.
        rpm::vector<float> holds;
        double sessionEnd = 0;
    };

    // Makes the spectrogram of a span of the stored sound in a thread. The analysis of the view
    // uses this for views that are too long for the live spectrogram, and for a view of a past
    // time. A new request replaces a request that waits.
    class ReviewSpectrogram {
    public:
        explicit ReviewSpectrogram(const Session *session);
        ~ReviewSpectrogram();

        // Ask for a spectrogram. The call returns at once.
        void request(const ReviewRequest &request);

        // The newest finished result, or no result at all.
        std::shared_ptr<const ReviewResult> latest() const;

    private:
        void run();
        void compute(const ReviewRequest &request, ReviewResult &result);

        const Session *mSession;

        mutable std::mutex mMutex;
        std::condition_variable mWake;
        bool mHasRequest;
        bool mStop;
        ReviewRequest mRequest;
        std::shared_ptr<const ReviewResult> mLatest;
        unsigned long long mNextId;

        std::thread mThread;
    };

}

#endif // MAIN_CONTEXT_REVIEW_SPECTROGRAM_H
