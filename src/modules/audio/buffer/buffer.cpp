#include "buffer.h"
#include <atomic>
#include <memory>
#include <iostream>
#include <thread>

using namespace Module::Audio;
using namespace std::chrono_literals;

std::atomic_bool Buffer::sCancel(false);
std::atomic_int Buffer::sId(0);

Buffer::Buffer(double sampleRate)
    : mId(sId++),
      mSampleRate(sampleRate),
      mQueue(capacity)
{
}

void Buffer::setSampleRate(double newSampleRate)
{
    if (mSampleRate == newSampleRate) {
        return;
    }
    else {
        if (mSampleRate > 0) {
            std::cout << "Audio::Buffer#" << mId << "] Sample rate changed, might cause jank for the next few frames." << std::endl;
        }
        mSampleRate = newSampleRate;
    }
}

double Buffer::getSampleRate() const
{
    return mSampleRate;
}

int Buffer::getLength() const
{
    return (int) mQueue.size_approx();
}

void Buffer::pull(double *pOut, int outLength)
{
    for (int i = 0; i < outLength; ++i) {
        // The timeout is in microseconds. A timeout of 50 woke this thread 20000 times each second
        // while it waited for the next block of audio. The pull stops within 5 ms after a cancel.
        while (!mQueue.wait_dequeue_timed(pOut[i], 5000) && !sCancel);
    }
}

void Buffer::push(const float *pIn, int inLength)
{
    // The audio thread of the system runs this function. try_enqueue never allocates memory, and it never
    // blocks. A full buffer drops the sample, and the counter records it.
    for (int i = 0; i < inLength; ++i) {
        if (!mQueue.try_enqueue(pIn[i])) {
            mDropped.fetch_add(inLength - i, std::memory_order_relaxed);
            return;
        }
    }
}

void Buffer::cancelPulls()
{
    sCancel = true;
}

bool Buffer::pullsCancelled()
{
    return sCancel;
}
