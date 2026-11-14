#ifndef MAIN_CONTEXT_VIEWRANGE_H
#define MAIN_CONTEXT_VIEWRANGE_H

#include <algorithm>

// The time range of the main graph. The functions have no dependency on the display, so a test can run them.

namespace Main {

    // The clock of the live view. It follows the stored sound, which the program counts in samples. Between two
    // steps of the stored sound, the clock goes on with the wall time, so the picture moves with no jerk. It never
    // goes more than maxLead seconds beyond the stored sound. A pause, a restart of the input, a stall, and the
    // small difference between the clock of the device and the wall clock then leave no lag at the end of the
    // picture. A clock that only follows the wall time keeps each of these errors for ever.
    class LiveClock {
    public:
        // The seconds of sound that the program stored, and the wall time in seconds on a steady clock.
        double advance(double stored, double wall, double maxLead = 0.1)
        {
            if (stored < mStored) {
                // The sound went away, for example at the reset of the graph. Start again.
                mEnd = -1e300;
            }
            if (stored != mStored) {
                mStored = stored;
                mWall = wall;
            }
            const double estimate = mStored + std::clamp(wall - mWall, 0.0, maxLead);
            // The clock never goes back, so the picture never jumps back.
            mEnd = std::max(mEnd, estimate);
            return mEnd;
        }

    private:
        double mStored = -1.0;
        double mWall = 0.0;
        double mEnd = -1e300;
    };

    // The end of the live view, in seconds of the session. The newest slice is older than the newest sound by
    // timeDelay. With the flip, the view follows the newest sound from the first moment, and the start
    // is below 0 for a young capture. Without the flip, the graph fills from the left, and the view ends
    // at the length of the view at least, so the start is never below 0.
    inline double liveViewEnd(double realTimeEnd, double timeDelay, double viewDuration, bool flipped)
    {
        return flipped ? realTimeEnd - timeDelay : std::max(realTimeEnd - timeDelay, viewDuration);
    }

    // The end of a view of a past time. A loaded file has a view of at least the length of the view.
    // The view of a capture ends at a time that has sound.
    inline double pastViewEnd(bool fileMode, double requestedEnd, double viewDuration, double sessionLength)
    {
        return fileMode ? std::clamp(requestedEnd, viewDuration, std::max(sessionLength, viewDuration))
                        : std::clamp(requestedEnd, std::min(viewDuration, sessionLength), sessionLength);
    }

}

#endif // MAIN_CONTEXT_VIEWRANGE_H
