#ifndef MAIN_CONTEXT_SESSION_PLAYER_H
#define MAIN_CONTEXT_SESSION_PLAYER_H

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

#include "session.h"
#include "../modules/audio/queue/queue.h"

namespace Main {

    // Plays a span of the stored sound through the playback queue. The synthesis thread calls the
    // callback of the queue, and the interface starts and stops the playback from another thread.
    class SessionPlayer {
    public:
        SessionPlayer(const Session *session, Module::Audio::Queue *queue);

        // Start the playback of the span from t0 to t1 seconds. A playback in progress stops first.
        void start(double t0, double t1);
        void stop();

        bool active() const { return mActive.load(); }

        // The time in the session of the sound that plays now, in seconds.
        double position() const { return mPosition.load(); }

        // The callback of the queue. The user data is the player.
        static void audioCallback(double *out, int count, void *self);

    private:
        void fill(double *out, int count);

        const Session *mSession;
        Module::Audio::Queue *mQueue;

        std::mutex mMutex;
        std::atomic<bool> mActive;
        std::atomic<double> mPosition;

        double mStart;
        long long mNextSample;
        long long mEndSample;
        long long mPlayedOutput;
        int mOutputRate;
        std::unique_ptr<Module::Audio::Resampler> mResampler;
        std::deque<double> mPending;
    };

}

#endif // MAIN_CONTEXT_SESSION_PLAYER_H
