#include "sessionplayer.h"

#include <algorithm>
#include <cmath>

using namespace Main;

SessionPlayer::SessionPlayer(const Session *session, Module::Audio::Queue *queue)
    : mSession(session),
      mQueue(queue),
      mActive(false),
      mPosition(0.0),
      mStart(0.0),
      mNextSample(0),
      mEndSample(0),
      mPlayedOutput(0),
      mOutputRate(1)
{
}

void SessionPlayer::start(double t0, double t1)
{
    std::lock_guard<std::mutex> lock(mMutex);

    const double begin = std::max(0.0, t0);
    const double end = std::min(t1, mSession->duration());
    if (!(end > begin)) {
        mActive = false;
        return;
    }

    mStart = begin;
    mNextSample = std::llround(begin * Session::sampleRate);
    mEndSample = std::llround(end * Session::sampleRate);
    mPlayedOutput = 0;
    mOutputRate = std::max(1, mQueue->getInSampleRate());
    mResampler = std::make_unique<Module::Audio::Resampler>(Session::sampleRate, mOutputRate);
    mPending.clear();
    mPosition = begin;
    mActive = true;
}

void SessionPlayer::stop()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mActive = false;
    mPending.clear();
}

void SessionPlayer::audioCallback(double *out, int count, void *self)
{
    static_cast<SessionPlayer *>(self)->fill(out, count);
}

void SessionPlayer::fill(double *out, int count)
{
    std::fill(out, out + count, 0.0);

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mActive) return;

    // Convert the stored sound to the rate of the queue, one short block at a time.
    constexpr long long block = Session::sampleRate / 20;
    while ((int) mPending.size() < count && mNextSample < mEndSample) {
        const long long n = std::min(block, mEndSample - mNextSample);
        rpm::vector<float> raw;
        mSession->read((double) mNextSample / Session::sampleRate, (double) (mNextSample + n) / Session::sampleRate, raw);
        mNextSample += n;

        rpm::vector<double> in(raw.begin(), raw.end());
        for (double y : mResampler->process(in)) mPending.push_back(y);
    }

    const int given = std::min(count, (int) mPending.size());
    for (int i = 0; i < given; ++i) {
        out[i] = mPending.front();
        mPending.pop_front();
    }

    mPlayedOutput += given;
    mPosition = mStart + (double) mPlayedOutput / mOutputRate;

    if (mPending.empty() && mNextSample >= mEndSample) {
        mActive = false;
    }
}
