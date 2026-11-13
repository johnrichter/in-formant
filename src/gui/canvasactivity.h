#ifndef GUI_CANVAS_ACTIVITY_H
#define GUI_CANVAS_ACTIVITY_H

#include <atomic>
#include <chrono>

namespace Gui {

    // The state that decides whether the canvas draws the next frame at once. The GUI thread writes it, and the render
    // thread reads it. A canvas that draws a frame for each screen refresh uses the CPU and the GPU also when nothing
    // moves. So it draws at the rate of the screen only while the capture or a playback runs, or for a short time
    // after an input of the user. Else it draws a few frames in a second.

    // The time after an input of the user in which the canvas keeps the rate of the screen.
    constexpr long long canvasInputHoldMs = 1000;

    // The time between two frames of an idle canvas, in milliseconds.
    constexpr int canvasIdleFrameMs = 200;

    // The time between two reads of the running state, in milliseconds.
    constexpr int canvasTickMs = 50;

    struct CanvasActivity {
        std::atomic<bool> running {false};
        std::atomic<long long> lastInputMs {-1000000};

        // True when the next frame must follow at once.
        bool busy(long long nowMs) const
        {
            return running.load() || nowMs - lastInputMs.load() < canvasInputHoldMs;
        }
    };

    inline long long canvasNowMs()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

}

#endif // GUI_CANVAS_ACTIVITY_H
