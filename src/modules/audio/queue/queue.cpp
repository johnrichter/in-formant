#include "queue.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <cmath>

using namespace Module::Audio;

Queue::Queue(int blockDurationInMs, int inSampleRate, QueueCallback callback)
    : mBlockDuration((float) blockDurationInMs),
      mResampler(inSampleRate),
      mCallback(callback),
      mQueue((int) std::round((48000 * 1.5 * mBlockDuration) / 1000))
{
}

Queue::~Queue()
{
}

void Queue::setCallback(QueueCallback callback)
{
    mCallback = callback;
}

void Queue::setOutSampleRate(int newOutSampleRate)
{
    mResampler.setOutputRate(newOutSampleRate);
}

int Queue::getInSampleRate() const
{
    return mResampler.getInputRate();
}

int Queue::getOutSampleRate() const
{
    return mResampler.getOutputRate();
}

void Queue::reportUnderrun()
{
    // The audio thread sets the flag. Write the message here, because writing to the log can block,
    // and the audio thread must never block. Write at most one message for each second.
    static auto lastMessage = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    if (mUnderrun.exchange(false)) {
        const auto now = std::chrono::steady_clock::now();
        if (now - lastMessage >= std::chrono::seconds(1)) {
            lastMessage = now;
            std::cout << "Audio::Queue] Not enough data to pull. Block duration is " << mBlockDuration.load() << " ms" << std::endl;
        }
    }
}

void Queue::pushSilenceIfNeeded()
{
    reportUnderrun();

    const int outputRate = mResampler.getOutputRate();
    const int blockSizeDst = (int) std::round((mBlockDuration.load() * outputRate) / 1000);

    if ((int) mQueue.size_approx() > blockSizeDst) {
        return;
    }

    for (int i = 0; i < blockSizeDst; ++i) {
        mQueue.emplace(0.0f);
    }
}

void Queue::pushIfNeeded(void *userdata)
{
    reportUnderrun();

    int inputRate = mResampler.getInputRate();
    int outputRate = mResampler.getOutputRate();
    int queueSize = (int) mQueue.size_approx();

    float blockDuration = mBlockDuration.load();
    int blockSizeSrc = (int) std::round((blockDuration * inputRate) / 1000);
    int blockSizeDst = (int) std::round((blockDuration * outputRate) / 1000);

    if (queueSize > blockSizeDst) {
        // There's already enough data in the queue.
        return;
    }

    rpm::vector<double> blockSrc(blockSizeSrc);
    mCallback(blockSrc.data(), blockSizeSrc, userdata);
   
    rpm::vector<double> blockDst = mResampler.process(blockSrc);
    for (const double& y : blockDst) {
        mQueue.emplace((float) y);
    }
}

void Queue::pull(float *pOut, int outLength)
{
    bool ranOutOfData = false;

    int i = 0;
    for (; i < outLength; ++i) {
        if (!mQueue.try_dequeue(pOut[i])) {
            ranOutOfData = true;
            break;
        }
    }

    if (ranOutOfData) {
        // Pad the rest with zeroes.
        for (; i < outLength; ++i) {
            pOut[i] = 0.0;
        }

        // An idle queue has no feed on purpose, so this is no underrun.
        if (mIdle.load()) return;

        // Make the blocks longer, but not without a limit.
        constexpr float maxBlockDuration = 200.0f;
        const float newBlockDuration = mBlockDuration + 0.25f;
        if (newBlockDuration <= maxBlockDuration) {
            mBlockDuration = newBlockDuration;
        }
        mUnderrun = true;
    }
}
