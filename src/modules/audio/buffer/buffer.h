#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include "rpcxx.h"
#include "../../../atomicops.h"
#include "../../../readerwriterqueue.h"
#include "../resampler/resampler.h"
#include <atomic>

namespace Module::Audio {

    /*
     *  NOTE: There can be only one reader and one writer at a given time.
     */
    class Buffer {
    public:
        Buffer(double sampleRate = 0);

        void setSampleRate(double sampleRate);

        double getSampleRate() const;
        int getLength() const;

        // The number of samples that the buffer holds without a new allocation. The audio thread of the system
        // calls push, and it must never allocate memory. So the buffer reserves this space when it starts. It is
        // 21 seconds at 48000 Hz.
        static constexpr int capacity = 1 << 20;

        // The number of samples that push dropped, because the buffer was full. The reader is then more than
        // 21 seconds behind.
        long long droppedSamples() const { return mDropped.load(); }

        void pull(double *pOut, int outLength);
        void push(const float *pIn, int inLength);

        static void cancelPulls();
        static bool pullsCancelled();

    private:
        int mId;
        double mSampleRate;

        moodycamel::BlockingReaderWriterQueue<float> mQueue;
        std::atomic<long long> mDropped {0};

        static std::atomic_bool sCancel;
        static std::atomic_int sId;
    };

}

#endif // AUDIO_BUFFER_H
