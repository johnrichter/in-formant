#ifndef GUI_TICKS_H
#define GUI_TICKS_H

#include <functional>
#include <string>
#include <vector>

namespace Gui {

    struct Tick {
        double value;

        // 0 is a tick with a label. 1 is a medium tick. 2 is a small tick.
        int level;

        // The text of the label. The text is empty for a tick with no label.
        std::string label;
    };

    enum class FrequencyTickMode {
        Automatic = 0,   // Round values. The number of ticks follows the height of the axis.
        Step = 1,        // A multiple of a step that the user sets, in Hz.
        Notes = 2,       // Musical notes, with the label of each C and some other notes.
        Octaves = 3,     // The C notes only.
    };

    enum class TimeTickMode {
        Automatic = 0,
        Step = 1,        // A multiple of a step that the user sets, in seconds.
    };

    // How many labels the automatic frequency axis shows. A denser axis uses more round values for its labels.
    enum class TickDensity {
        Sparse = 0,      // 1, 2, 5 times a power of ten: 100, 200, 500, 1000
        Normal = 1,      // 1, 2, 3, 5: 100, 200, 300, 500, 1000
        Dense = 2,       // 1, 1.5, 2, 3, 4, 5: 100, 150, 200, 300, 400, 500, 1000
    };

    // Convert a value to the position on the axis in pixels.
    using ToPixel = std::function<double(double)>;

    // Make the ticks of the frequency axis.
    //   labelSize: the size of one label along the axis in pixels. The labels never overlap.
    //   labelGap:  the free space between two labels in pixels.
    //   linearScale: true when equal steps in Hz have equal size on the axis. The automatic mode then uses
    //                equal steps (100, 200, 300) and not powers of ten.
    //   density:   how many labels the automatic mode shows.
    std::vector<Tick> makeFrequencyTicks(FrequencyTickMode mode, double step,
                                         double minFrequency, double maxFrequency,
                                         const ToPixel &toPixel,
                                         double labelSize, double labelGap,
                                         bool linearScale = false,
                                         TickDensity density = TickDensity::Normal);

    // The step between two labels of the time axis in seconds. The step is never below 1 second.
    // A long span has larger steps: about 15 labels at most (15 s: 1 s, 60 s: 5 s, 120 s: 10 s).
    double timeLabelStep(double span, double pixelsPerLabel, double secondsPerPixel);

    // Make the ticks of the time axis.
    //   labelWidth: the width of one label in pixels. This is the width of a label with the most digits.
    std::vector<Tick> makeTimeTicks(TimeTickMode mode, double step,
                                    double minTime, double maxTime,
                                    const ToPixel &toPixel,
                                    double labelWidth, double labelGap);

    // The text of a value with no extra zeros: 1000, 2.5, 0.125.
    std::string formatNumber(double value);

    // The text of a time in seconds. A long time has the form m:ss or h:mm:ss.
    std::string formatTime(double seconds, double step);

    // The name of the note that is nearest to a frequency, for example "A4", and the offset in cents.
    std::string noteName(double frequency, double *cents = nullptr);

}

#endif // GUI_TICKS_H
