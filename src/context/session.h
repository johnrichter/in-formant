#ifndef MAIN_CONTEXT_SESSION_H
#define MAIN_CONTEXT_SESSION_H

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <QString>
#include <QTemporaryFile>

#include "rpcxx.h"
#include "../modules/audio/resampler/resampler.h"

namespace Main {

    // The sound of one session: from the start of the capture, or from an audio file. The sound
    // goes to a temporary file on the disk, so a long session does not fill the memory. The time
    // of a sample is its index divided by the sample rate.
    class Session {
    public:
        // The sample rate of the stored sound, in Hz.
        static constexpr int sampleRate = 32000;

        // The longest session, in seconds. The session ignores the sound after this time.
        static constexpr double maxDuration = 86400.0;

        Session();
        ~Session();

        Session(const Session &) = delete;
        Session &operator=(const Session &) = delete;

        // Delete the sound, and start again.
        void clear();

        // Delete the sound files of earlier runs that ended in a crash. When keepFolder is not
        // empty, the sound of each file goes there as a WAV file first.
        static void recoverOrphans(const QString &keepFolder);

        // Add samples of any sample rate. The caller can use any thread.
        void append(const double *samples, int count, double fs);

        // Add samples that already have the stored sample rate.
        void appendStored(const float *samples, size_t count);

        // The length of the sound in seconds.
        double duration() const;

        // The value changes each time that the sound changes.
        unsigned long long version() const { return mVersion.load(); }

        // The samples from t0 to t1 seconds. The result has the length that fits the time span.
        // The values outside the sound are zero. The result is false when the file fails.
        bool read(double t0, double t1, rpm::vector<float> &out) const;

        // The lowest and the highest sample value (from -1 to 1) in each of the bins, from t0 to t1.
        void envelope(double t0, double t1, int bins, std::vector<float> &low, std::vector<float> &high) const;

        // Write the sound from t0 to t1 as a 16-bit WAV file. The result is false when it fails.
        bool exportWav(const QString &path, double t0, double t1) const;

    private:
        // The number of samples in one envelope block.
        static constexpr int blockSize = 256;

        bool readSamples(long long first, long long count, std::vector<int16_t> &out) const;
        void appendSamples(const std::vector<int16_t> &samples);

        mutable std::mutex mMutex;
        std::unique_ptr<QTemporaryFile> mFile;
        long long mCount;

        // The lowest and the highest value of each full block, and of the block in progress.
        std::vector<int16_t> mBlocks;
        int16_t mPartialLow, mPartialHigh;
        int mPartialCount;

        Module::Audio::Resampler mResampler;
        int mInputRate;

        std::atomic<unsigned long long> mVersion;
    };

}

#endif // MAIN_CONTEXT_SESSION_H
