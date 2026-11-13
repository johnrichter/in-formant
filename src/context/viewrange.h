#ifndef MAIN_CONTEXT_VIEWRANGE_H
#define MAIN_CONTEXT_VIEWRANGE_H

#include <algorithm>

// The time range of the main graph. The functions have no dependency on the display, so a test can run them.

namespace Main {

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
