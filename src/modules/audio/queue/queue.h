#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#include "rpcxx.h"
#include "../../../atomicops.h"
#include "../../../readerwriterqueue.h"
#include "../resampler/resampler.h"
#include <functional>
#include <atomic>

namespace Module::Audio {

    using QueueCallback = std::function<void(double*, int, void*)>;

    class Queue {
    public:
        Queue(int blockDurationInMs, int inSampleRate, QueueCallback callback);
        ~Queue();

        void setOutSampleRate(int outSampleRate);
        
        void setCallback(QueueCallback callback);

        int getInSampleRate() const;
        int getOutSampleRate() const;

        void pushIfNeeded(void *userdata);
        void pushSilenceIfNeeded();
        void pull(float *pOut, int outLength);

    private: 
        void reportUnderrun();

        std::atomic<float> mBlockDuration;
        std::atomic<bool> mUnderrun {false};

        Resampler mResampler;
        QueueCallback mCallback;
        
        moodycamel::BlockingReaderWriterQueue<float> mQueue;
    };

}

#endif // AUDIO_QUEUE_H
